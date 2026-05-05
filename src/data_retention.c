#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
// #include <dirent.h>
#include "../include/sqlite3.h"
#include "../include/hiredis.h"

// ---------- CONFIGURATION ----------
#define DB_PATH "/usr/cms/data/dcu_dlms.db" // Path to your sqlite database
#define LOG_FILE "/usr/cms/log/data_retention.log"

// ---------- LOGGING ----------
void log_message(const char *message)
{
    FILE *f = fopen(LOG_FILE, "a");
    if (f)
    {
        time_t now = time(NULL);
        char ts[64];
        strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
        fprintf(f, "[%s] %s\n", ts, message);
        fclose(f);
    }
    printf("%s\n", message);
}

// ---------- SQL EXECUTION ----------
int execute_sql(sqlite3 *db, const char *sql)
{
    char *err_msg = NULL;
    int rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        char buf[512];
        snprintf(buf, sizeof(buf), "SQL error: %s | Query: %s", err_msg, sql);
        log_message(buf);
        sqlite3_free(err_msg);
        return rc;
    }
    return SQLITE_OK;
}

// ---------- REDIS HELPER ----------
int get_int_from_redis(redisContext *c, const char *hash, const char *field, int default_val)
{
    redisReply *reply = redisCommand(c, "HGET %s %s", hash, field);
    int value = default_val;
    if (reply)
    {
        if (reply->type == REDIS_REPLY_STRING)
            value = atoi(reply->str);
        freeReplyObject(reply);
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "Redis: %s = %d", field, value);
    log_message(buf);
    return value;
}

// ---------- TABLE PROCESSING ----------
void process_table(sqlite3 *db, const char *pattern, const char *date_col, int days)
{
    sqlite3_stmt *stmt;
    char sql[512];

    // Get all tables matching pattern
    snprintf(sql, sizeof(sql), "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '%s';", pattern);
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        log_message("Failed to query table names");
        return;
    }

    time_t now = time(NULL);
    time_t cutoff = now - (days * 24 * 60 * 60);

    char cutoff_str[32];
    strftime(cutoff_str, sizeof(cutoff_str), "%Y-%m-%d %H:%M:%S", localtime(&cutoff));

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *table = (const char *)sqlite3_column_text(stmt, 0);

        char del_sql[512];
        snprintf(del_sql, sizeof(del_sql),
                 "DELETE FROM %s WHERE \"%s\" < '%s';",
                 table, date_col, cutoff_str);

        printf("------------ %s ---\n", del_sql);

        char buf[512];
        snprintf(buf, sizeof(buf), "--------del query\n%s \n", del_sql);

        log_message(buf);
        
        memset(buf, 0 , sizeof(buf));
        snprintf(buf, sizeof(buf), "Cleaning table %s older than %s", table, cutoff_str);
        log_message(buf);

        execute_sql(db, del_sql);
    }
    sqlite3_finalize(stmt);
}

void process_bill_table(sqlite3 *db, const char *pattern, const char *bill_col, int months)
{
    sqlite3_stmt *stmt;
    char sql[512];

    snprintf(sql, sizeof(sql), "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '%s';", pattern);
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK)
    {
        log_message("Failed to query bill tables");
        return;
    }

    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    lt->tm_mon -= months;
    mktime(lt);

    char cutoff_str[16];
    strftime(cutoff_str, sizeof(cutoff_str), "%Y-%m-%d", lt);

    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const char *table = (const char *)sqlite3_column_text(stmt, 0);

        char del_sql[512];
        // snprintf(del_sql, sizeof(del_sql),
        //          "DELETE FROM %s WHERE strftime('%%m-%%Y', \"%s\") < '%s';",
        //          table, bill_col, cutoff_str);

        //  snprintf(del_sql, sizeof(del_sql), " DELETE FROM bill_data_lnt_DCU_CMS_0_31900677 WHERE (substr('0_0_0_1_2_255', 4, 2) || '-' ||   -- MM   substr('0_0_0_1_2_255', 7, 4) -- YYYY) < '03-2025';" );

        // snprintf(del_sql, sizeof(del_sql),
        //     "DELETE FROM bill_data_lnt_DCU_CMS_0_31900677 "
        //     "WHERE (substr(\"0_0_0_1_2_255\", 4, 2) || '-' || "
        //     "substr(\"0_0_0_1_2_255\", 7, 4)) < '03-2025';");

        // snprintf(del_sql, sizeof(del_sql),
        //     "DELETE FROM bill_data_lnt_DCU_CMS_0_31900677 "
        //     "WHERE (substr(\"0_0_0_1_2_255\", 7, 4) || '-' || "
        //            "substr(\"0_0_0_1_2_255\", 4, 2) || '-' || "
        //            "substr(\"0_0_0_1_2_255\", 1, 2)) < '2025-03-01';"
        // );

        snprintf(del_sql, sizeof(del_sql),
                 "DELETE FROM %s "
                 "WHERE (substr(\"%s\", 7, 4) || '-' || "
                 "substr(\"%s\", 4, 2) || '-' || "
                 "substr(\"%s\", 1, 2)) < '%s';",
                 table, bill_col, bill_col, bill_col, cutoff_str);

        printf("-----bill %s ------\n", del_sql);
       
        char buf[512];
        snprintf(buf, sizeof(buf), "-----bill query ------ \n %s \n", del_sql);

        log_message(buf);
        
        memset(buf, 0 , sizeof(buf));
        snprintf(buf, sizeof(buf), "Cleaning billing table %s older than month %s", table, cutoff_str);
        log_message(buf);

        execute_sql(db, del_sql);
    }
    sqlite3_finalize(stmt);
}

// ---------- MAIN ----------
int data_cleanup()
{
    log_message("=== Data Retention Job Started ===");

    // Connect to Redis
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err)
    {
        log_message("Failed to connect to Redis");
        return 1;
    }

    int ls_days = get_int_from_redis(c, "data_retain_det", "ls_days", 30);

    int mn_days = get_int_from_redis(c, "data_retain_det", "mn_days", 30);
    int bill_months = get_int_from_redis(c, "data_retain_det", "bill_months", 6);
    int event_days = get_int_from_redis(c, "data_retain_det", "event_days", 30);

    redisFree(c);

    // Connect to SQLite
    sqlite3 *db;
    if (sqlite3_open(DB_PATH, &db) != SQLITE_OK)
    {
        log_message("Failed to open SQLite database");
        return 1;
    }

    process_table(db, "ls_data_%", "0_0_1_0_0_255", ls_days);
    process_table(db, "daily_profile_data%", "0_0_1_0_0_255", mn_days);

    // process_bill_table(db, "bill_data_%", "bill_date", bill_months);
    process_bill_table(db, "bill_data_%", "0_0_0_1_2_255", bill_months);

    process_table(db, "event_data%", "0_0_1_0_0_255", event_days);

    sqlite3_close(db);
    log_message("=== Data Retention Job Finished ===");
    return 0;
}
