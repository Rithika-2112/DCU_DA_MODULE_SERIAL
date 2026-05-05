
#include "../include/general_config.h"
#include "../include/dlms_config.h"

#define SD_DB_LOC "/run/media/mmcblk0p1/dcu_dlms.db"

#define DB_LOC "/usr/cms/data/dcu_dlms.db"

extern uint8_t g_pidx;

extern event_obis_t event_obis; // rithika 9Sep
extern int obis_count;

sqlite3 *db;
// char sql[16384];
char sql[182024]; // rithika 8Sep

char placeholders[2048] = "";
// rithika 09oct20205
//  char columns[4062] = "";
char columns[8124] = "";
char event_msg_str[128];
char ls_date[36];

// rithika 30oct2025
int latest_row_exist = 0;

extern bool old_ls_flag;

extern nameplate_info_t nameplate_info;
extern inst_val_info_t inst_val_info;
extern billing_data_value_t billing_data_value;
extern daily_profile_data_value_t daily_profile_data_value;
extern load_survey_data_value_t load_survey_data_value;
extern events_data_value_t events_data_value;
// r ithika 31oct2025
extern event_val_obis_t event_val_obis[MAX_NO_OF_METER_PER_PORT][MAX_EVENTS];

// rithika 03oct2025
extern inst_val_obis_t inst_val_obis[MAX_NO_OF_METER_PER_PORT];
extern ls_val_obis_t ls_val_obis[MAX_NO_OF_METER_PER_PORT];
extern daily_profile_val_obis_t daily_profile_val_obis[MAX_NO_OF_METER_PER_PORT];
extern billing_val_obis_t billing_val_obis[MAX_NO_OF_METER_PER_PORT];

extern OD_CMD_RECEIVED;

feature_cfg_t feature_cfg;
events_data_t db_event_data;

// rithika 13nov2025
extern Date meter_date_time[MAX_NO_OF_METER_PER_PORT];

// rithika 09Dec2025
extern int present_ls_flag;

const char *obis_codes[] = {
    "0_0_1_0_0_255",
    "0_0_96_11_0_255",
    "1_0_31_7_0_255",
    "1_0_51_7_0_255",
    "1_0_71_7_0_255",
    "1_0_32_7_0_255",
    "1_0_52_7_0_255",
    "1_0_72_7_0_255",
    "1_0_33_7_0_255",
    "1_0_53_7_0_255",
    "1_0_73_7_0_255",
    "1_0_1_8_0_255",
    "1_0_91_7_0_255",
    "0_0_96_11_1_255",
    "0_0_96_11_2_255",
    "0_0_96_11_3_255",
    "0_0_96_11_5_255",
    "0_0_96_11_4_255",
    "0_0_96_11_6_255",
    "1_0_94_91_14_255",
    "1_0_12_7_0_255",
    "1_0_13_7_0_255",
    "0_0_94_91_0_255",
    "0_0_96_15_0_255",
    "1_0_9_8_0_255",
    "1_0_2_8_0_255",
    "1_0_10_8_0_255",
    "0_0_96_15_1_255",
    "0_0_96_15_2_255",
    "0_0_96_15_3_255",
    "0_0_96_15_4_255",
    "0_0_96_15_5_255",
    "0_0_96_15_6_255",
    "0_0_96_1_0_255",
    "1_0_11_7_0_255",
    "0_0_96_9_0_255",
    "0_0_96_9_128_255"};

int num_obis_codes = sizeof(obis_codes) / sizeof(obis_codes[0]);

extern int extract_block_int(const char *);

int db_open()
{
    static char fun_name[] = "db_open()";
    int rc;
    char *err_msg = 0;

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        rc = sqlite3_open(SD_DB_LOC, &db);
    }
    else
    {
        rc = sqlite3_open(DB_LOC, &db);
    }
    if (rc)
    {
        dbg_log(REPORT, "[%-25s] :Can't open database: %s\n", fun_name, sqlite3_errmsg(db));
        return -1;
    }
    else
    {
        // add_process_diag("DB connected successfully");
        // dbg_log(INFORM, "[%-25s] :DB connected successfully\n", fun_name);
    }

    sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg);

    if (err_msg)
    {
        sqlite3_free(err_msg);
        err_msg = NULL;
    }

    sqlite3_busy_timeout(db, 10000); // 10 seconds timeout

    sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA temp_store=FILE;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA cache_size=-2000;", NULL, NULL, NULL);
    sqlite3_exec(db, "PRAGMA read_uncommitted=1;", NULL, NULL, NULL); // this will allow the reader while write is happening

    return 0;
}

int open_db_readonly()
{

    int rc;
    static char fun_name[] = "open_db_readonly()";
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        rc = sqlite3_open_v2(SD_DB_LOC, &db,
                             SQLITE_OPEN_READONLY,
                             NULL);
    }
    else
    {
        rc = sqlite3_open_v2(DB_LOC, &db,
                             SQLITE_OPEN_READONLY,
                             NULL);
    }
    if (rc != SQLITE_OK)
    {
        dbg_log(REPORT, "[%-25s] :Cannot open DB in read-only mode: %s\n", fun_name, sqlite3_errmsg(db));

        if (db)
            sqlite3_close(db);
        return -1;
    }

    sqlite3_busy_timeout(db, 10000); // 10 seconds timeout

    return 0;
}

int drop_table(const char *table_name)
{
    char *err_msg = 0;
    char sql[256];

    db_open();

    // Prepare SQL query to drop the table
    snprintf(sql, sizeof(sql), "DROP TABLE IF EXISTS %s;", table_name);

    // Execute the SQL query
    int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        printf("SQL error: %s\n", err_msg);
        sqlite3_free(err_msg);
        sqlite3_close(db); // rithika 9Sep
        return rc;
    }

    printf("Table %s dropped successfully\n", table_name);
    sqlite3_close(db); // rithika 9Sep
    return SQLITE_OK;
}

static double safe_atof(const char *s)
{
    if (s == NULL || s[0] == '\0')
        return 0.0;

    char *end = NULL;
    double d = strtod(s, &end);

    if (end == s) // not a number
        return 0.0;

    return d;
}

int checkIfRowExists(const char *table_name, events_data_value_t *events_data_value)
{
    sqlite3_stmt *stmt;
    int rc;
    char sql[2048]; // Adjust size based on your number of columns
    char where_clause[1024] = "";
    int param_index = 1; // The starting index for binding

    // Start building the SQL query
    snprintf(sql, sizeof(sql), "SELECT COUNT(*) FROM %s WHERE ", table_name);

    // For each value in events_data_value.values[]

    for (int i = 0; i < events_data_value->num_event; i++)
    {
        // Add dynamic checks for each obis_code and corresponding value
        char column_condition[256];
        snprintf(column_condition, sizeof(column_condition), " \"%s\" = ? AND", events_data_value->values[i].obis_code);
        strcat(where_clause, column_condition);
        param_index++;
    }

    // Remove the last "AND" from where_clause
    if (strlen(where_clause) > 0)
    {
        where_clause[strlen(where_clause) - 3] = '\0'; // Remove the last " AND"
    }

    // Combine SQL query with the dynamic WHERE clause
    snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), "%s", where_clause);

    // Print the final SQL query
    printf("SQL Query: %s\n", sql);

    // Prepare the SQL statement
    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare SELECT statement: %s\n", sqlite3_errmsg(db));
        if (stmt)
            sqlite3_finalize(stmt);
        return -1;
    }

    param_index = 0;
    // Bind values for each obis_code dynamically
    for (int i = 0; i < events_data_value->num_event; i++)
    {
        printf("value : %s\n", events_data_value->values[i].val);
        // rithika 11nov2025
        if (i <= 4)
        {
            sqlite3_bind_text(stmt, i + 1, events_data_value->values[i].val, -1, SQLITE_TRANSIENT);
        }
        else
        {
            // sqlite3_bind_double(stmt, i + 1, atof(events_data_value->values[i].val));
            // rithika 12Dec2025
            sqlite3_bind_double(stmt, i + 1, safe_atof(events_data_value->values[i].val));
        }
    }

    // Execute the query
    rc = sqlite3_step(stmt);
    int exists = 0;

    // printf("7777777777777777 %s \n", sqlite3_expanded_sql(stmt));

    // Check the result of sqlite3_step
    if (rc == SQLITE_ROW)
    {
        exists = sqlite3_column_int(stmt, 0); // Get the count result
        printf("Row exists: %d\n", exists);

        // Optionally, you can print the actual column values returned from the query
        for (int i = 0; i < sqlite3_column_count(stmt); i++)
        {
            const char *column_text = (const char *)sqlite3_column_text(stmt, i);
            if (column_text)
            {
                printf("Column %d: %s\n", i, column_text); // Print column value
            }
        }
    }
    else if (rc == SQLITE_DONE)
    {
        printf("No rows found.\n");
    }
    else
    {
        fprintf(stderr, "Failed to execute SELECT statement: %s\n", sqlite3_errmsg(db));
    }

    sqlite3_finalize(stmt);

    printf("exists: %d\n", exists);

    return exists; // 1 if exists, 0 if not
}

#if 1
/**
 * Inserts values into a specified SQLite database table based on the given data type.
 *
 * @param table_name The name of the table where data will be inserted.
 * @param data_type The type of data being inserted (e.g., "inst_data", "ls_data", etc.).
 *
 * This function constructs an SQL INSERT statement dynamically, binds the current time
 * and other relevant data values, and executes the statement to store the data.
 *
 * The function handles different data types by determining the appropriate columns
 * and values based on the provided data type. It supports the following data types:
 * - inst_data
 * - ls_data
 * - daily_profile_data
 * - event_data
 * - bill_data
 *
 * On successful execution, it prints a confirmation message. In case of errors,
 * it logs them to stderr and does not proceed with the insertion.
 */

void insertValues(const char *table_name, char *data_type, int event_type, int midx)
{
    static char fun_name[] = "insertValues()";

    char *err_msg = 0;
    char current_time[20];
    char bill_date[64];
    char column[256];
    char event_type_str[20];
    char combined_string[256];

    time_t now;
    struct tm *tm_info;

    int i, j;

    send_hanging_check();

    memset(columns, 0, sizeof(columns));
    memset(placeholders, 0, sizeof(placeholders));

    // Build the columns and placeholders

    // rithika 08oct2025
    // strcat(columns, "update_time");
    // strcat(placeholders, "?");
    snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), "update_time");
    snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), "?");

    if (strcmp(data_type, "inst_data") == 0)
    {
        // rithika 03nov2025
        inst_val_info.num_vals = inst_val_obis[midx].num_val_params;

        for (i = 0; i < inst_val_info.num_vals; i++)
        {
            // rithika 03nov2025
            strcpy(inst_val_info.values[i].obis_code, inst_val_obis[midx].obis_code[i]);

            snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), ", \"%s\"", inst_val_info.values[i].obis_code);
            snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");
        }
    }
    else if (strcmp(data_type, "ls_data") == 0)
    {
        // rithika 03nov2025
        load_survey_data_value.num_vals = ls_val_obis[midx].num_val_params;

        for (i = 0; i < load_survey_data_value.num_vals; i++)
        {

            // rithika 03nov2025
            strcpy(load_survey_data_value.values[i].obis_code, ls_val_obis[midx].obis_code[i]);
            // rithika 06oct
            snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), ", \"%s\"", load_survey_data_value.values[i].obis_code);
            snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");
        }
    }
    else if (strcmp(data_type, "daily_profile_data") == 0)
    {
        // rithika 03nov2025
        daily_profile_data_value.num_vals = daily_profile_val_obis[midx].num_val_params;

        for (i = 0; i < daily_profile_data_value.num_vals; i++)
        {
            memset(column, 0, sizeof(column));

            strcpy(daily_profile_data_value.values[i].obis_code, daily_profile_val_obis[midx].obis_code[i]);
            snprintf(column, sizeof(column), ", \"%s\"", daily_profile_data_value.values[i].obis_code);

            // rithika 08oct
            // strcat(columns, column);
            // strcat(placeholders, ", ?");

            snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), "%s", column);
            snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");
        }
    }
    else if (strcmp(data_type, "event_data") == 0)
    {

        // rithika 30oct2025
        events_data_value.num_event = event_val_obis[midx][event_type - 1].num_event_params;

        snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), ",event_type");
        snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");

        snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), ",unique_id");
        snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");

        for (i = 0; i < events_data_value.num_event; i++)
        {
            char column[256];
            // rithika 30oct2025
            strcpy(events_data_value.values[i].obis_code, event_val_obis[midx][event_type - 1].obis_code[i]);

            snprintf(column, sizeof(column), ", \"%s\"", events_data_value.values[i].obis_code);

            // rithika 08oct
            // strcat(columns, column);
            // strcat(placeholders, ", ?");

            snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), "%s", column);
            snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");
        }
    }
    else if (strcmp(data_type, "bill_data") == 0)
    {
        // rithika 08oct
        // strcat(columns, ",bill_date");
        // strcat(placeholders, ", ?");

        // rithika 03nov2025
        billing_data_value.num_vals = billing_val_obis[midx].num_val_params;

        snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), ",bill_date");
        snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");

        for (i = 0; i < billing_data_value.num_vals; i++)
        {
            // rithika 03nov2025
            strcpy(billing_data_value.values[i].obis_code, billing_val_obis[midx].obis_code[i]);

            snprintf(column, sizeof(column), ", \"%s\"", billing_data_value.values[i].obis_code);

            // rithika 08oct
            // strcat(columns, column);
            // strcat(placeholders, ", ?");

            snprintf(columns + strlen(columns), sizeof(columns) - strlen(columns), "%s", column);
            snprintf(placeholders + strlen(placeholders), sizeof(placeholders) - strlen(placeholders), ", ?");
        }
    }

    // db_open(); //rithika 6oct

    if (strcmp(data_type, "event_data") == 0)
    {
        if (checkIfRowExists(table_name, &events_data_value) == 0)
        {
            snprintf(sql, sizeof(sql), "INSERT  INTO %s (%s) VALUES (%s)", table_name, columns, placeholders);
            printf("SQL: %s\n", sql);
        }
        else
        { // rithika 22/08/2025
            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Skipping the insertion of row in table %s, because it is already present in the table.\n", midx, fun_name, table_name);
            // rithika 30oct2025
            latest_row_exist = 1;
            // sqlite3_close(db); //rithika 6oct
            return;
        }
    }
    else
    {
        snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO %s (%s) VALUES (%s)", table_name, columns, placeholders);
        printf("SQL: %s\n", sql);
    }

    sqlite3_stmt *stmt;

    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "%s : Failed to prepare statement: %s\n", fun_name, sqlite3_errmsg(db));
        drop_table(table_name);
        if (stmt)
            sqlite3_finalize(stmt);
        // sqlite3_close(db); //rithika 6oct
        return;
    }

    // Get current time
    time(&now);
    tm_info = localtime(&now);
    strftime(current_time, sizeof(current_time), "%Y-%m-%d %H:%M:%S", tm_info);

    // Begin a transaction to reduce lock contention
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to start transaction: %s\n", err_msg);
        sqlite3_finalize(stmt);
        // sqlite3_close(db); //rithika 6oct
        if (err_msg)
            sqlite3_free(err_msg);
        return;
    }

    // Bind the update_time
    sqlite3_bind_text(stmt, 1, current_time, -1, SQLITE_TRANSIENT);

    // Bind other parameters based on the data type
    int param_index = 2; // Start at 2 because index 1 is for update_time

    if (strcmp(data_type, "ls_data") == 0)
    {
        for (int i = 0; i < load_survey_data_value.num_vals; i++)
        {
            // sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg); rithika 02Jan2025
            // rithika 11nov2025
            if (param_index == 2)
            {
                sqlite3_bind_text(stmt, param_index++, load_survey_data_value.values[i].val, -1, SQLITE_STATIC);
            }
            else
            {
                if (!(load_survey_data_value.values[i].val))
                {
                    sqlite3_bind_double(stmt, param_index++, 0.0);
                }
                else
                {
                    // sqlite3_bind_double(stmt, param_index++, atof(load_survey_data_value.values[i].val));
                    sqlite3_bind_double(stmt, param_index++, safe_atof(load_survey_data_value.values[i].val));
                }
            }
        }
    }
    else if (strcmp(data_type, "daily_profile_data") == 0)
    {
        for (i = 0; i < daily_profile_data_value.num_vals; i++)
        {
            // sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg); rithika 02Jan2025
            // rithika 11nov2025
            if (i == 0)
            {
                sqlite3_bind_text(stmt, i + 2, daily_profile_data_value.values[i].val, -1, SQLITE_STATIC);
            }
            else
            {
                // sqlite3_bind_double(stmt, i + 2, atof(daily_profile_data_value.values[i].val));
                sqlite3_bind_double(stmt, i + 2, safe_atof(daily_profile_data_value.values[i].val));
            }
        }
    }
    else if (strcmp(data_type, "event_data") == 0)
    {

        memset(event_type_str, 0, sizeof(event_type_str));
        sprintf(event_type_str, "%d", event_type);

        sqlite3_bind_text(stmt, 2, event_type_str, -1, SQLITE_TRANSIENT);

        for (i = 0; i < events_data_value.num_event; i++)
        {
            if (strcmp(events_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
            {
                memset(bill_date, 0, sizeof(bill_date));
                strcpy(bill_date, events_data_value.values[i].val);
                printf("bill_date : %s\n", bill_date);
            }
            for (j = 0; j < events_data_value.num_event; j++)
            {
                if ((strcmp(events_data_value.values[j].obis_code, "0_0_96_11_0_255") == 0) ||
                    (strcmp(events_data_value.values[j].obis_code, "0_0_96_11_1_255") == 0) ||
                    (strcmp(events_data_value.values[j].obis_code, "0_0_96_11_2_255") == 0) ||
                    (strcmp(events_data_value.values[j].obis_code, "0_0_96_11_3_255") == 0) ||
                    (strcmp(events_data_value.values[j].obis_code, "0_0_96_11_4_255") == 0) ||
                    (strcmp(events_data_value.values[j].obis_code, "0_0_96_11_5_255") == 0) ||
                    (strcmp(events_data_value.values[j].obis_code, "0_0_96_11_6_255") == 0))
                {

                    memset(combined_string, 0, sizeof(combined_string));
                    sprintf(combined_string, "%s_%s", bill_date, events_data_value.values[j].val);

                    sqlite3_bind_text(stmt, 3, combined_string, -1, SQLITE_TRANSIENT);
                    printf("combined_string : %s\n", combined_string);
                }
            }
            // sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg); rithika 02Jan2025
            // rithika 11nov2025
            if (i == 0)
            {
                sqlite3_bind_text(stmt, i + 4, events_data_value.values[i].val, -1, SQLITE_STATIC);
            }
            else
            {

                // sqlite3_bind_double(stmt, i + 4, atof(events_data_value.values[i].val));
                sqlite3_bind_double(stmt, i + 4, safe_atof(events_data_value.values[i].val));
            }
        }
    }
    else if (strcmp(data_type, "bill_data") == 0)
    {
        for (i = 0; i < billing_data_value.num_vals; i++)
        {
            if (strcmp(billing_data_value.values[i].obis_code, "0_0_0_1_2_255") == 0)
            {
                int day, month, year, hour, minute;

                const char *month_names[] = {
                    "Jan", "Feb", "Mar", "Apr", "May", "June",
                    "July", "Aug", "Sep", "Oct", "Nov", "Dec", "curr mon"};

                if (sscanf(billing_data_value.values[i].val, "%d-%d-%d %d:%d", &day, &month, &year, &hour, &minute) == 5)
                {
                    // rithika 13nov2025
                    //  if (day != 1)
                    if (day == meter_date_time[midx].day && month == meter_date_time[midx].month && year == meter_date_time[midx].year)
                    {
                        snprintf(bill_date, sizeof(bill_date), "%s %d", month_names[12], year);
                        printf("Formatted String: %s\n", bill_date);
                    }
                    else
                    {
                        snprintf(bill_date, sizeof(bill_date), "%s %d", month_names[month - 1], year);
                        printf("Formatted String: %s\n", bill_date);
                    }
                }
                else
                {
                    printf("Failed to parse the date and time string.\n");
                }

                // sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg); rithika 02Jan2025

                sqlite3_bind_text(stmt, 2, bill_date, -1, SQLITE_TRANSIENT);
            }
            // rithika 11nov2025
            if (i == 0 || strstr(billing_data_value.values[i].obis_code, "DT"))
            {
                sqlite3_bind_text(stmt, i + 3, billing_data_value.values[i].val, -1, SQLITE_STATIC);
            }
            else
            {
                // sqlite3_bind_double(stmt, i + 3, atof(billing_data_value.values[i].val));

                sqlite3_bind_double(stmt, i + 3, safe_atof(billing_data_value.values[i].val));
            }
        }
    }

    // Execute the statement with retries
    int retries = 5;
    while (retries > 0)
    {
        rc = sqlite3_step(stmt);
        // printf("Actual query:\n%s \n", sqlite3_expanded_sql(stmt));

        if (rc == SQLITE_DONE)
        {
            // Success
            break;
        }
        else if (rc == SQLITE_BUSY)
        {
            // Database is locked, retry after a short delay
            retries--;
            printf("Database is locked, retrying... (%d retries left)\n", retries);
            usleep(500000); // Wait for 500 milliseconds
        }
        else
        {
            // Other error occurred
            fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, &err_msg); // Rollback if error occurs
            sqlite3_finalize(stmt);
            if (err_msg)
                sqlite3_free(err_msg);
            // sqlite3_close(db); //rithika 6oct
            return;
        }
    }

    if (retries == 0)
    {
        fprintf(stderr, "Failed after multiple retries\n");
        sqlite3_finalize(stmt);
        // sqlite3_close(db); //rithika 6oct
        return;
    }

    // Commit the transaction
    rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to commit transaction: %s\n", err_msg);
        if (err_msg)
            sqlite3_free(err_msg);
    }
    else
    {
        printf("Values inserted successfully into table '%s'.\n", table_name);
        // dbg_log(INFORM, "[%-25s] : Values inserted successfully into table '%s'.\n", fun_name, table_name);
    }

    // Finalize the statement
    sqlite3_finalize(stmt);
    // sqlite3_close(db); //rithika 6oct
    if (err_msg)
        sqlite3_free(err_msg);
}

/**
 * Creates a new SQLite data table based on the specified data type, port, serial number,
 * and meter index. If the table already exists, it is not recreated.
 *
 * @param data_type The type of data being stored (e.g., "ls_data", "event_data", etc.).
 * @param port The port number associated with the meter.
 * @param serial_num The serial number of the meter.
 * @param midx The meter index used for generating table names.
 *
 * This function constructs a table name dynamically based on the provided parameters
 * and then builds and executes a SQL CREATE TABLE statement to create the table.
 * The columns in the table are defined based on the data type, with appropriate data types
 * and constraints applied.
 */
void createDataTable(char *data_type, int port, char *serial_num, int midx, int event_idx)
{

    static char fun_name[] = "createDataTable()";
    char *err_msg = 0;
    int rc;
    int i;
    char table_name[100];
    // char temp_met_sn[64];
    char temp_manufacturer[64] = {0};

    // rithika 06oct
    struct timeval start, end;

    gettimeofday(&start, NULL);

    char temp_met_sn[64] = {0};
    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
    }

    if (get_manufacturer(midx, temp_met_sn, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter manufacturer.\n", midx, fun_name);
    }

    // rithika 28Nov2025
    for (i = 0; i < strlen(temp_met_sn); i++)
    {

        if (temp_met_sn[i] == '-')
        {
            temp_met_sn[i] = '_';
        }
    }

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {

        // memset(temp_met_sn, 0, sizeof(temp_met_sn));

        // char *serial_number = get_meter_2_ser_num(midx);

        // if (serial_number == NULL)
        // {
        //     dbg_log(INFORM, "[%-25s] : [Meter - %d] : Failed to get meter serial number.\n", fun_name, midx);
        // }
        // else
        // {
        // snprintf(temp_met_sn, sizeof(temp_met_sn), "%s", serial_number);
        // }

        if (strcmp(temp_met_sn, "NC") == 0)
        {
            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Returning from creating data table because ser num is not available for this meter.\n", midx, fun_name);
            return 0;
        }

        // char manufacturer[64] = {0};
        // char *manufacturer = get_manufacturer(midx, temp_met_sn);

        // // strcpy(manufacturer, get_manufacturer(midx, temp_met_sn));
        // // print_colored_message(GREEN, 1, "manuf %s temp_met_sn %s\n",manufacturer, temp_met_sn);
        // if (manufacturer == NULL)
        // {
        //     dbg_log(INFORM, "[%-25s] : [Meter - %d] : Failed to get meter manufacturer.\n", fun_name, midx);
        // }
        // else
        // {
        //     snprintf(temp_manufacturer, sizeof(temp_manufacturer), "%s", manufacturer);
        // }

        if (OD_CMD_RECEIVED == 1)
        {
            memset(table_name, 0, sizeof(table_name));
            snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%s", data_type, temp_manufacturer, serial_num, port, temp_met_sn);
        }
        else
        {
            memset(table_name, 0, sizeof(table_name));
            snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", data_type, temp_manufacturer, serial_num, port, temp_met_sn);
        }
    }
    else
    {
        // char *serial_number = get_meter_2_ser_num(midx);

        // if (serial_number == NULL)
        // {
        //     dbg_log(INFORM, "[%-25s] : [Meter - %d] : Failed to get meter serial number.\n", fun_name, midx);
        // }
        // else
        // {
        //     snprintf(temp_met_sn, sizeof(temp_met_sn), "%s", serial_number);
        // }

        if (strcmp(temp_met_sn, "NC") == 0)
        {
            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Returning from creating data table because ser num is not available for this meter.\n", midx, fun_name);
            return 0;
        }

        // char manufacturer[64] = {0};
        // char *manufacturer = get_manufacturer(midx, temp_met_sn);

        // // strcpy(manufacturer, get_manufacturer(midx, temp_met_sn));
        // // print_colored_message(GREEN, 1, "manuf %s temp_met_sn %s\n",manufacturer, temp_met_sn);
        // if (manufacturer == NULL)
        // {
        //     dbg_log(INFORM, "[%-25s] : [Meter - %d] : Failed to get meter manufacturer.\n", fun_name, midx);
        // }
        // else
        // {
        //     snprintf(temp_manufacturer, sizeof(temp_manufacturer), "%s", manufacturer);
        // }
        ///////////

        if (OD_CMD_RECEIVED == 1)
        {
            memset(table_name, 0, sizeof(table_name));
            snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%s", data_type, temp_manufacturer, serial_num, port, temp_met_sn);
            printf("table_name : %s\n", table_name);
        }
        else
        {
            memset(table_name, 0, sizeof(table_name));
            snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", data_type, temp_manufacturer, serial_num, port, temp_met_sn);
            printf("table_name : %s\n", table_name);
        }
    }

    // db_open(); rithika 03Jan2026 just pasting it below to reduce database lock error

    memset(sql, 0, sizeof(sql));
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s ("
             "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
             "update_time DATETIME DEFAULT NULL",
             table_name);

    if (strcmp(data_type, "ls_data") == 0)
    {
        // rithika 26nov2025
        load_survey_data_value.num_vals = ls_val_obis[midx].num_val_params;

        for (i = 0; i < load_survey_data_value.num_vals; i++)
        {
            // strcat(sql, ", ");
            // strcat(sql, "\"");
            // if (strcmp(load_survey_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
            // {
            //     strcat(sql, load_survey_data_value.values[i].obis_code);
            //     strcat(sql, "\" DATETIME NOT NULL UNIQUE");

            // }
            // else
            // {
            //     strcat(sql, load_survey_data_value.values[i].obis_code);
            //     strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
            // }

            // rithika 26nov2025
            strcpy(load_survey_data_value.values[i].obis_code, ls_val_obis[midx].obis_code[i]);

            ////// rithika 06oct25
            if (strcmp(load_survey_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
            {
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" DATETIME NOT NULL UNIQUE", load_survey_data_value.values[i].obis_code);
            }
            else
            {
                // rithika 11nov2025
                //  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" varchar(46) NOT NULL DEFAULT '0.0'", load_survey_data_value.values[i].obis_code);

                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" REAL NOT NULL DEFAULT 0.0", load_survey_data_value.values[i].obis_code);
            }
        }
    }
    else if (strcmp(data_type, "daily_profile_data") == 0)
    {
        // rithika 26nov2025
        daily_profile_data_value.num_vals = daily_profile_val_obis[midx].num_val_params;

        for (i = 0; i < daily_profile_data_value.num_vals; i++)
        {
            // strcat(sql, ", ");
            // strcat(sql, "\"");
            // if (strcmp(daily_profile_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
            // {
            //     strcat(sql, daily_profile_data_value.values[i].obis_code);
            //     strcat(sql, "\" DATETIME NOT NULL UNIQUE");
            // }
            // else
            // {
            //     strcat(sql, daily_profile_data_value.values[i].obis_code);
            //     strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
            // }
            // rithika 26nov2025
            strcpy(daily_profile_data_value.values[i].obis_code, daily_profile_val_obis[midx].obis_code[i]);

            printf("daily_profile_data_value.values[%d].obis_code %s\n", i, daily_profile_data_value.values[i].obis_code);
            ////// rithika 08oct25
            if (strcmp(daily_profile_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
            {
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" DATETIME NOT NULL UNIQUE", daily_profile_data_value.values[i].obis_code);
            }
            else
            {
                // rithika 11nov2025
                //  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" varchar(46) NOT NULL DEFAULT '0.0'", daily_profile_data_value.values[i].obis_code);

                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" REAL NOT NULL DEFAULT 0.0", daily_profile_data_value.values[i].obis_code);
            }
        }
    }
    // rithika 8Sep
    else if (strcmp(data_type, "event_data") == 0)
    {
        // rithika 9sep
        get_event_obis();

        // rithika 08oct2025
        // strcat(sql, ", ");
        // strcat(sql, "\"event_type\" varchar(46) NOT NULL DEFAULT 'NA'");

        // strcat(sql, ", ");
        // strcat(sql, "\"unique_id\" varchar(46) NOT NULL DEFAULT 'NA'");

        snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"event_type\" varchar(46) NOT NULL DEFAULT 'NA', \"unique_id\" varchar(46) NOT NULL DEFAULT 'NA'");

        for (i = 0; i < obis_count; i++)
        {
            // strcat(sql, ", ");
            // strcat(sql, "\"");
            // if (strcmp(event_obis.event_obis_codes[i], "0_0_1_0_0_255") == 0)
            // {
            //     strcat(sql, event_obis.event_obis_codes[i]);
            //     strcat(sql, "\" varchar(46) NOT NULL DEFAULT 'NA'");
            // }
            // else
            // {
            //     strcat(sql, event_obis.event_obis_codes[i]);
            //     strcat(sql, "\" varchar(46) NOT NULL DEFAULT 'NA'");
            // }

            ////// rithika 08oct25
            if (strcmp(event_obis.event_obis_codes[i], "0_0_1_0_0_255") == 0)
            {
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" DATETIME NOT NULL", event_obis.event_obis_codes[i]);
            }
            else
            {
                // rithika 11nov2025
                //  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" varchar(46) NOT NULL DEFAULT 'NA'", event_obis.event_obis_codes[i]);
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" REAL DEFAULT FFFFFFFF", event_obis.event_obis_codes[i]);
            }
        }
    }

    //  else if (strcmp(data_type, "event_data") == 0)
    // {
    //     strcat(sql, ", ");
    //     strcat(sql, "\"event_type\" varchar(46) NOT NULL DEFAULT 'NA'");

    //     strcat(sql, ", ");
    //     strcat(sql, "\"unique_id\" varchar(46) NOT NULL DEFAULT 'NA'");

    //     for (i = 0; i < events_data_value.num_event; i++)
    //     {
    //         strcat(sql, ", ");
    //         strcat(sql, "\"");
    //         if (strcmp(events_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
    //         {
    //             strcat(sql, events_data_value.values[i].obis_code);
    //             strcat(sql, "\" varchar(46) NOT NULL DEFAULT 'NA'");
    //         }
    //         else
    //         {
    //             strcat(sql, events_data_value.values[i].obis_code);
    //             strcat(sql, "\" varchar(46) NOT NULL DEFAULT 'NA'");
    //         }
    //     }
    // }
    else if (strcmp(data_type, "bill_data") == 0)
    {
        ////// rithika 08oct25
        // strcat(sql, ", ");
        // strcat(sql, "\"bill_date\" DATETIME NOT NULL UNIQUE");

        snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"bill_date\" DATETIME NOT NULL UNIQUE");

        // rithika 26nov2025
        billing_data_value.num_vals = billing_val_obis[midx].num_val_params;

        for (i = 0; i < billing_data_value.num_vals; i++)
        {

            // rithika 26nov2025
            strcpy(billing_data_value.values[i].obis_code, billing_val_obis[midx].obis_code[i]);

            // rithika 11nov2025
            //  snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" varchar(46) NOT NULL DEFAULT '0.0'", billing_data_value.values[i].obis_code);

            if (strcmp(billing_data_value.values[i].obis_code, "0_0_0_1_2_255") == 0)
            {
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" DATETIME NOT NULL UNIQUE", billing_data_value.values[i].obis_code);
            }
            else if (strstr(billing_data_value.values[i].obis_code, "DT"))
            {
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" DATETIME NOT NULL", billing_data_value.values[i].obis_code);
            }
            else
            {
                snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ", \"%s\" REAL NOT NULL DEFAULT 0.0", billing_data_value.values[i].obis_code);
            }
        }
    }

    ////// rithika 08oct25
    // strcat(sql, ")");
    snprintf(sql + strlen(sql), sizeof(sql) - strlen(sql), ")");

    printf("sql : %s\n", sql);

    db_open();

    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        dbg_log(INFORM, "[%-25s] : SQL error: %s\tTable_name : %s\n", fun_name, err_msg, table_name);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(db_event_data.eventmsg, "%s : SQL error: %s", fun_name, err_msg);

        strcpy(db_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&db_event_data);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }
    else if (rc == SQLITE_OK)
    {
        printf("Table '%s' created successfully.\n", table_name);
        // dbg_log(INFORM, "[%-25s] : Table '%s' created successfully.\n", fun_name, table_name);
    }
    else
    {
        // printf("Table '%s' created successfully.\n", table_name);
        dbg_log(INFORM, "[%-25s] : Table '%s' already exits.\n", fun_name, table_name);
    }

    // sqlite3_close(db); //rithika 6oct

    gettimeofday(&end, NULL);

    dbg_log(INFORM, "[%-25s] : took %.3f ms\n", fun_name, elapsed_ms(start, end));

    printf("--------------- createtable function() took %.3f ms\n", elapsed_ms(start, end));

    //    rithika 02Jan2025
    // sqlite3_exec(db, "PRAGMA synchronous=OFF;", NULL, NULL, NULL);
    //  sqlite3_exec(db, "PRAGMA journal_mode=MEMORY;", NULL, NULL, NULL);
    // sqlite3_exec(db, "PRAGMA temp_store=MEMORY;", NULL, NULL, NULL);
    // sqlite3_exec(db, "PRAGMA cache_size=-10000;", NULL, NULL, NULL); // ~10 MBsqlite3_exec(db, "PRAGMA locking_mode=EXCLUSIVE;", NULL, NULL, NULL);

    //***************** */ rithika 03Jan2026 to avoid the database lock it is placed in the db open funtion
    // sqlite3_exec(db, "PRAGMA synchronous=NORMAL;", NULL, NULL, NULL);
    // sqlite3_exec(db, "PRAGMA journal_mode=WAL;", NULL, NULL, NULL);
    // sqlite3_exec(db, "PRAGMA temp_store=FILE;", NULL, NULL, NULL);
    // sqlite3_exec(db, "PRAGMA cache_size=-2000;", NULL, NULL, NULL);

    insertValues(table_name, data_type, event_idx, midx);
    sqlite3_close(db); // rithika 6oct
}
#endif

#if 0
/**
 * Inserts values into a specified SQLite database table based on the given data type.
 *
 * @param table_name The name of the table where data will be inserted.
 * @param data_type The type of data being inserted (e.g., "inst_data", "ls_data", etc.).
 *
 * This function constructs an SQL INSERT statement dynamically, binds the current time
 * and other relevant data values, and executes the statement to store the data.
 *
 * The function handles different data types by determining the appropriate columns
 * and values based on the provided data type. It supports the following data types:
 * - inst_data
 * - ls_data
 * - daily_profile_data
 * - event_data
 * - bill_data
 *
 * On successful execution, it prints a confirmation message. In case of errors,
 * it logs them to stderr and does not proceed with the insertion.
 */

void insertValues(const char *table_name, char *data_type, int event_type)
{
    static char fun_name[] = "insertValues()";
    char *err_msg = 0;
    char current_time[20];
    char bill_date[20];

    time_t now;
    struct tm *tm_info;

    int i;

    memset(columns, 0, sizeof(columns));
    memset(placeholders, 0, sizeof(placeholders));

    // Build the columns and placeholders
    strcat(columns, "update_time");
    strcat(placeholders, "?");

    if (strcmp(data_type, "inst_data") == 0)
    {
        for (i = 0; i < inst_val_info.num_vals; i++)
        {
            strcat(columns, ", ");
            strcat(columns, "\"");
            strcat(columns, inst_val_info.values[i].obis_code);
            strcat(columns, "\"");
            strcat(placeholders, ", ?");
        }
    }
    else if (strcmp(data_type, "ls_data") == 0)
    {
        for (i = 0; i < load_survey_data_value.num_vals; i++)
        {
            strcat(columns, ", ");
            strcat(columns, "\"");
            strcat(columns, load_survey_data_value.values[i].obis_code);
            strcat(columns, "\"");
            strcat(placeholders, ", ?");
        }
    }
    else if (strcmp(data_type, "daily_profile_data") == 0)
    {

        for (i = 0; i < daily_profile_data_value.num_vals; i++)
        {
            char column[256];
            memset(column, 0, sizeof(column));
            snprintf(column, sizeof(column), ", \"%s\"", daily_profile_data_value.values[i].obis_code);
            strcat(columns, column);
            strcat(placeholders, ", ?");
        }
    }
    else if (strcmp(data_type, "event_data") == 0)
    {
        strcat(columns, ",event_type");
        strcat(placeholders, ", ?");

        for (i = 0; i < events_data_value.num_event; i++)
        {
            char column[256];
            snprintf(column, sizeof(column), ", \"%s\"", events_data_value.values[i].obis_code);
            strcat(columns, column);
            strcat(placeholders, ", ?");
        }
    }
    else if (strcmp(data_type, "bill_data") == 0)
    {

        strcat(columns, ",bill_date");
        strcat(placeholders, ", ?");

        for (i = 0; i < billing_data_value.num_vals; i++)
        {
            char column[256];
            snprintf(column, sizeof(column), ", \"%s\"", billing_data_value.values[i].obis_code);
            strcat(columns, column);
            strcat(placeholders, ", ?");
        }

        printf("placeholders: %s   columns : %s\n", placeholders, columns);
    }

    db_open();

    // Construct the SQL statement
    snprintf(sql, sizeof(sql), "INSERT OR REPLACE INTO %s (%s) VALUES (%s)", table_name, columns, placeholders);
    printf("SQL: %s\n", sql);

    sqlite3_stmt *stmt;
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    printf("--------3444-----------\n");
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "%s : Failed to prepare statement: %s\n", fun_name, sqlite3_errmsg(db));
        printf("--------34-----------\n");
        sqlite3_close(db);
        return;
    }

    // Get current time
    time(&now);
    tm_info = localtime(&now);
    strftime(current_time, sizeof(current_time), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("--------348-----------\n");
    // Begin a transaction to reduce lock contention
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to start transaction: %s\n", err_msg);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }
    printf("--------341-----------\n");
    // Bind the update_time
    sqlite3_bind_text(stmt, 1, current_time, -1, SQLITE_TRANSIENT);

    // Bind other parameters based on the data type
    int param_index = 2; // Start at 2 because index 1 is for update_time

    if (strcmp(data_type, "ls_data") == 0)
    {
        for (int i = 0; i < load_survey_data_value.num_vals; i++)
        {
            sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg);
            sqlite3_bind_text(stmt, param_index++, load_survey_data_value.values[i].val, -1, SQLITE_STATIC);
        }
    }
    else if (strcmp(data_type, "daily_profile_data") == 0)
    {
        for (i = 0; i < daily_profile_data_value.num_vals; i++)
        {
            sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg);
            sqlite3_bind_text(stmt, i + 2, daily_profile_data_value.values[i].val, -1, SQLITE_STATIC);
        }
    }
    else if (strcmp(data_type, "event_data") == 0)
    {
        printf("======================78============\n");
        char event_type_str[20];
        sprintf(event_type_str, "%d", event_type);

        sqlite3_bind_text(stmt, 2, event_type_str, -1, SQLITE_TRANSIENT);

        printf("======================708============\n");
        for (i = 0; i < events_data_value.num_event; i++)
        {
            sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg);
            sqlite3_bind_text(stmt, i + 3, events_data_value.values[i].val, -1, SQLITE_STATIC);
        }
    }
    else if (strcmp(data_type, "bill_data") == 0)
    {

        for (i = 0; i < billing_data_value.num_vals; i++)
        {
            if (strcmp(billing_data_value.values[i].obis_code, "0_0_0_1_2_255") == 0)
            {
                printf("================== billing_data_value.values[i].val : %s============\n", billing_data_value.values[i].val);

                int day, month, year, hour, minute;

                if (sscanf(billing_data_value.values[i].val, "%d-%d-%d %d:%d", &day, &month, &year, &hour, &minute) == 5)
                {
                    snprintf(bill_date, sizeof(bill_date), "%d-%d-%d", year, month, day);

                    printf("Formatted String: %s\n", bill_date);
                }
                else
                {
                    printf("Failed to parse the date and time string.\n");
                }

                printf("================== bill_date : %s============\n", bill_date);
                sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, &err_msg);
                sqlite3_bind_text(stmt, 2, bill_date, -1, SQLITE_TRANSIENT);
            }
            sqlite3_bind_text(stmt, i + 3, billing_data_value.values[i].val, -1, SQLITE_STATIC);
        }
    }

    // Execute the statement with retries
    int retries = 5;
    while (retries > 0)
    {
        rc = sqlite3_step(stmt);
        if (rc == SQLITE_DONE)
        {
            // Success
            break;
        }
        else if (rc == SQLITE_BUSY)
        {
            // Database is locked, retry after a short delay
            retries--;
            printf("Database is locked, retrying... (%d retries left)\n", retries);
            usleep(500000); // Wait for 500 milliseconds
        }
        else
        {
            // Other error occurred
            fprintf(stderr, "Execution failed: %s\n", sqlite3_errmsg(db));
            sqlite3_exec(db, "ROLLBACK;", NULL, NULL, &err_msg); // Rollback if error occurs
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return;
        }
    }

    if (retries == 0)
    {
        fprintf(stderr, "Failed after multiple retries\n");
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return;
    }

    // Commit the transaction
    rc = sqlite3_exec(db, "COMMIT;", NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to commit transaction: %s\n", err_msg);
    }
    else
    {
        printf("Values inserted successfully into table '%s'.\n", table_name);
    }

    // Finalize the statement
    sqlite3_finalize(stmt);
    sqlite3_close(db);
}


/**
 * Creates a new SQLite data table based on the specified data type, port, serial number,
 * and meter index. If the table already exists, it is not recreated.
 *
 * @param data_type The type of data being stored (e.g., "ls_data", "event_data", etc.).
 * @param port The port number associated with the meter.
 * @param serial_num The serial number of the meter.
 * @param midx The meter index used for generating table names.
 *
 * This function constructs a table name dynamically based on the provided parameters
 * and then builds and executes a SQL CREATE TABLE statement to create the table.
 * The columns in the table are defined based on the data type, with appropriate data types
 * and constraints applied.
 */
void createDataTable(char *data_type, int port, char *serial_num, int midx, int event_idx)
{
    static char fun_name[] = "createDataTable()";
    char *err_msg = 0;
    int rc;
    int i;
    char table_name[100];

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        char temp_met_sn[64];
        memset(temp_met_sn, 0, sizeof(temp_met_sn));
        strcpy(temp_met_sn, get_meter_2_ser_num(midx));

        if (strcmp(temp_met_sn, "NC") == 0)
        {
            dbg_log(INFORM, "[%-25s] : [Meter - %d] : Returning from creating data table because ser num is not available for this meter.\n", fun_name, midx);
            return 0;
        }

        // if (strcmp(data_type, "event_data") == 0)
        // {
        //     if (OD_CMD_RECEIVED == 1)
        //     {
        //         memset(table_name, 0, sizeof(table_name));
        //         snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%d_%s", data_type, get_manufacturer(midx), serial_num, port, event_idx, temp_met_sn);
        //     }
        //     else
        //     {
        //         memset(table_name, 0, sizeof(table_name));
        //         snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%d_%s", data_type, get_manufacturer(midx), serial_num, port, event_idx, temp_met_sn);
        //     }
        // }
        // else
        // {
        if (OD_CMD_RECEIVED == 1)
        {
            memset(table_name, 0, sizeof(table_name));
            snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%s", data_type, get_manufacturer(midx), serial_num, port, temp_met_sn);
        }
        else
        {
            memset(table_name, 0, sizeof(table_name));
            snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", data_type, get_manufacturer(midx), serial_num, port, temp_met_sn);
        }
        // }
    }
    else
    {
        // if (strcmp(data_type, "event_data") == 0)
        // {
        //     if (OD_CMD_RECEIVED == 1)
        //     {
        //         memset(table_name, 0, sizeof(table_name));
        //         snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%d_%d", data_type, get_manufacturer(midx), serial_num, port, midx, event_idx);
        //         printf("table_name : %s\n", table_name);
        //     }
        //     else
        //     {
        //         memset(table_name, 0, sizeof(table_name));
        //         snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%d_%d", data_type, get_manufacturer(midx), serial_num, port, midx, event_idx);
        //         printf("table_name : %s\n", table_name);
        //     }
        // }
        // else
        // {
            if (OD_CMD_RECEIVED == 1)
            {
                memset(table_name, 0, sizeof(table_name));
                snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%d", data_type, get_manufacturer(midx), serial_num, port, midx);
                printf("table_name : %s\n", table_name);
            }
            else
            {
                memset(table_name, 0, sizeof(table_name));
                snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%d", data_type, get_manufacturer(midx), serial_num, port, midx);
                printf("table_name : %s\n", table_name);
            }
        // }
    }

    db_open();

    // if (strcmp(data_type, "ls_data") == 0)
    // {
    //     memset(sql, 0, sizeof(sql));
    //     snprintf(sql, sizeof(sql),
    //              "CREATE TABLE IF NOT EXISTS %s ("
    //              "id INTEGER AUTOINCREMENT NOT NULL,"
    //              "met_time DATETIME DEFAULT NULL,"
    //              "update_time DATETIME DEFAULT NULL);",
    //              table_name);
    // }
    // else
    // {
    memset(sql, 0, sizeof(sql));
    snprintf(sql, sizeof(sql),
             "CREATE TABLE IF NOT EXISTS %s ("
             "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
             "update_time DATETIME DEFAULT NULL",
             table_name);
    // }

    // if (strcmp(data_type, "inst_data") == 0)
    // {

    //     for (i = 0; i < inst_val_info.num_vals; i++)
    //     {
    //         strcat(sql, ", ");
    //         strcat(sql, "\"");
    //         strcat(sql, inst_val_info.values[i].obis_code);
    //         strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
    //     }
    // }
    if (strcmp(data_type, "ls_data") == 0)
    {

        for (i = 0; i < load_survey_data_value.num_vals; i++)
        {
            strcat(sql, ", ");
            strcat(sql, "\"");
            if (strcmp(load_survey_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
            {
                strcat(sql, load_survey_data_value.values[i].obis_code);
                strcat(sql, "\" DATETIME NOT NULL UNIQUE");
            }
            else
            {
                strcat(sql, load_survey_data_value.values[i].obis_code);
                strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
            }
        }
    }
    else if (strcmp(data_type, "daily_profile_data") == 0)
    {
        for (i = 0; i < daily_profile_data_value.num_vals; i++)
        {
            strcat(sql, ", ");
            strcat(sql, "\"");
            if (strcmp(daily_profile_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
            {
                strcat(sql, daily_profile_data_value.values[i].obis_code);
                strcat(sql, "\" DATETIME NOT NULL UNIQUE");
            }
            else
            {
                strcat(sql, daily_profile_data_value.values[i].obis_code);
                strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
            }
        }
    }
    else if (strcmp(data_type, "event_data") == 0)
    {
        // char temp_sql[255];
        // memset(temp_sql, 0, sizeof(temp_sql));
        // snprintf(temp_sql, sizeof(temp_sql), "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", table_name);

        // printf("++++++++++++++++++++++++++++++++++ %s +++++++++++++++++++++++++++++\n", temp_sql);

        // sqlite3_stmt *stmt;
        // rc = sqlite3_prepare_v2(db, temp_sql, -1, &stmt, NULL);
        // if (rc != SQLITE_OK)
        // {
        //     fprintf(stderr, "%s : Failed to prepare statement: %s\n", fun_name, sqlite3_errmsg(db));
        //     printf("&&&&&&&&&&&&&&&&&&&&&24&&&&&&&&&&&&&&&&&&&\n");
        //     sqlite3_close(db);
        //     return;
        // }

        // printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&\n");
        // rc = sqlite3_step(stmt);
        // if (rc == SQLITE_ROW)
        // {
        //     printf("Table '%s' already exists. Checking for required columns...\n", table_name);

        //     // Iterate through each event column and check if it exists
        //     for (i = 0; i < events_data_value.num_event; i++)
        //     {
        //         char column_sql[512];
        //         snprintf(column_sql, sizeof(column_sql), "PRAGMA table_info(%s);", table_name);
        //         sqlite3_stmt *col_stmt;
        //         rc = sqlite3_prepare_v2(db, column_sql, -1, &col_stmt, NULL);
        //         if (rc != SQLITE_OK)
        //         {
        //             fprintf(stderr, "%s : Failed to prepare column info query: %s\n", fun_name, sqlite3_errmsg(db));
        //             sqlite3_finalize(stmt);
        //             sqlite3_close(db);
        //             return;
        //         }

        //         int column_exists = 0;
        //         while (sqlite3_step(col_stmt) == SQLITE_ROW)
        //         {
        //             const char *column_name = (const char *)sqlite3_column_text(col_stmt, 1); // Column name is at index 1
        //             if (strcmp(column_name, events_data_value.values[i].obis_code) == 0)
        //             {
        //                 column_exists = 1; // Column already exists
        //                 break;
        //             }
        //         }

        //         sqlite3_finalize(col_stmt);

        //         if (!column_exists)
        //         {
        //             // Column does not exist, so alter the table
        //             snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD COLUMN \"%s\" varchar(46) NOT NULL DEFAULT '0.0';", table_name, events_data_value.values[i].obis_code);
        //             rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
        //             if (rc != SQLITE_OK)
        //             {
        //                 fprintf(stderr, "%s : Failed to add column: %s\n", fun_name, err_msg);
        //                 sqlite3_free(err_msg);
        //                 sqlite3_finalize(stmt);
        //                 sqlite3_close(db);
        //                 return;
        //             }
        //             printf("Added column '%s' to table '%s'.\n", events_data_value.values[i].obis_code, table_name);
        //         }
        //     }
        // }
        // else
        // {
        // printf("Table '%s' already exists. 09Checking for required columns...\n", table_name);
        strcat(sql, ", ");
        strcat(sql, "\"event_type\" varchar(46) NOT NULL DEFAULT '0.0'");

        for (i = 0; i < num_obis_codes; i++)
        {
            strcat(sql, ", ");
            strcat(sql, "\"");
            if (strcmp(obis_codes[i], "0_0_1_0_0_255") == 0)
            {
                strcat(sql, obis_codes[i]);
                strcat(sql, "\" DEFAULT NULL UNIQUE");
            }
            else
            {
                strcat(sql, obis_codes[i]);
                strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
            }
        }
    }
    else if (strcmp(data_type, "bill_data") == 0) // 0_0_0_1_2_255
    {
        strcat(sql, ", ");
        strcat(sql, "\"bill_date\" DATETIME NOT NULL UNIQUE");

        for (i = 0; i < billing_data_value.num_vals; i++)
        {
            strcat(sql, ", ");
            strcat(sql, "\"");
            // if (strcmp(billing_data_value.values[i].obis_code, "0_0_0_1_2_255") == 0)
            // {
            //     strcat(sql, billing_data_value.values[i].obis_code);
            //     strcat(sql, "\" DATETIME NOT NULL UNIQUE");
            // }
            // else
            // {
            strcat(sql, billing_data_value.values[i].obis_code);
            strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
            // }
        }
    }

    strcat(sql, ")");
    printf("sql : %s\n", sql);

    rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err_msg);
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(db_event_data.eventmsg, "%s : SQL error: %s", fun_name, err_msg);
        strcpy(db_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&db_event_data);
        sqlite3_free(err_msg);
        sqlite3_close(db);
        return;
    }
    else
    {
        printf("Table '%s' created successfully.\n", table_name);
    }

    sqlite3_close(db);

    insertValues(table_name, data_type, event_idx);
}
#endif
/**
 * Converts a date string from the format "day-month-year" to "year-month-day".
 * The function expects the input in the format "DD-MM-YYYY" and returns
 * the reformatted string.
 *
 * @param input The input date string in "DD-MM-YYYY" format.
 * @return A pointer to the converted date string, or NULL if parsing fails
 *         or if the date is invalid.
 */
char *convert_date_format(char *input)
{
    printf("-------------------------------------ls_date : %s-------------------------\n", input);
    int year, month, day;
    if (sscanf(input, "%d-%d-%d", &day, &month, &year) != 3)
    {
        printf("-------------------------------------failed : %s-------------------------\n", input);
        return 0; // Parsing failed
    }
    if (year < 0 || month < 1 || month > 12 || day < 1 || day > 31)
    {
        return 0; // Invalid date
    }
    memset(ls_date, 0, sizeof(ls_date));
    sprintf(ls_date, "%04d-%02d-%02d", year, month, day);
    printf("-------------------------------------ls_date : %s-------------------------\n", ls_date);
    return ls_date; // Success
}

/**
 * Checks if a specified date exists in a given SQLite table.
 *
 * @param check_date The date to check, expected in "DD-MM-YYYY" format.
 * @param table_name The name of the table to query.
 * @return The count of rows with the specified date if found, or -1 if an error occurs.
 */
int old_data_check(char *check_date, const char *table_name, int midx)
{
    static char fun_name[] = "old_data_check()";
    sqlite3_stmt *stmt;
    int rc;
    int count, block_int;
    char stored_date_format[36]; // MM/DD/YYYY\0
    char event_buff[64];

    memset(stored_date_format, 0, sizeof(stored_date_format));

    printf("-------------------------------------%s-------------------------\n", convert_date_format(check_date));
    strcpy(stored_date_format, convert_date_format(check_date));

    // Prepare the SQL statement
    char sql[256];
    // snprintf(sql, sizeof(sql), "SELECT COUNT(`0_0_1_0_0_255`) FROM %s WHERE `0_0_1_0_0_255` >= ?;", table_name);

    snprintf(sql, sizeof(sql), "SELECT COUNT(`0_0_1_0_0_255`) FROM %s WHERE DATE(`0_0_1_0_0_255`) = ?;", table_name);

    printf("_____________________ sql : %s ____________________________\n", sql);

    // rithika 25Nov2025
    //  db_open();
    open_db_readonly();

    rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(db_event_data.eventmsg, "%s : Failed to prepare statement: %s", fun_name, sqlite3_errmsg(db));
        strcpy(db_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&db_event_data);

        if (stmt)
            sqlite3_finalize(stmt);
        sqlite3_close(db);
        return -1;
    }

    // Bind the converted date parameter
    sqlite3_bind_text(stmt, 1, stored_date_format, -1, SQLITE_STATIC);

    // Execute the query
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        count = sqlite3_column_int(stmt, 0);

        // rithika 12Dec2025
        sqlite3_finalize(stmt);
        sqlite3_close(db);

        print_colored_message(MAGENTA, 1, "****************** actual entry : %d *************\n", count);

        memset(event_buff, 0, sizeof(event_buff));

        // char temp_met_sn[64] = {0};
        // char *serial_number = get_meter_2_ser_num(midx);

        // if (serial_number == NULL)
        // {
        //     dbg_log(INFORM, "[%-25s] : [Meter - %d] : Failed to get meter serial number.\n", fun_name, midx);
        // }
        // else
        // {
        //     snprintf(temp_met_sn, sizeof(temp_met_sn), "%s", serial_number);
        // }

        char temp_met_sn[64] = {0};
        if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
        {
            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
        }
        // rithika 29/08/2025
        // if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        // {
        //     sprintf(event_buff, "meter%d_details", (midx + 1));
        // }
        // else
        // {
        //     sprintf(event_buff, "meter_%d_%d_details", g_pidx, (midx + 1), temp_met_sn);
        // }
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(event_buff, "meter%d_%s_details", (midx + 1), temp_met_sn);
        }
        else
        {
            sprintf(event_buff, "meter_%d_%d_%s_details", g_pidx, (midx + 1), temp_met_sn);
        }

        block_int = extract_block_int(event_buff);

        print_colored_message(MAGENTA, 1, "****************** available entry : %d actual entry : %d *************\n", block_int, count);

        if (block_int == count)
        {
            // printf("The date %s (stored as %s) is available in the table.\n", check_date, stored_date_format);
            dbg_log(WARNING, "[Meter_%d]:[%-25s] : The date %s (stored as %s) is available in the table.\n", midx, fun_name, check_date, stored_date_format);
        }
        else
        {
            // rithika 11Dec2025
            if (present_ls_flag == 0)
            {
                // rithika 12Dec2025
                if (block_int != count && old_ls_flag != 1)
                {
                    int ret = is_in_blackout_window(table_name, stored_date_format, block_int, count);

                    if (ret == 1)
                    {
                        return count;
                    }
                }
                // printf("The date %s (stored as %s) is not available in the table.\n", check_date, stored_date_format);
                dbg_log(WARNING, "[Meter_%d]:[%-25s] :The date %s (stored as %s) is not available in the table.\n", midx, fun_name, check_date, stored_date_format);
                // Clean up
                // sqlite3_finalize(stmt);
                // sqlite3_close(db);
                return -1;
            }
            else
            {
                // Clean up
                // sqlite3_finalize(stmt);
                // sqlite3_close(db);
                return count;
            }

            // return -1; //rithika 09/12/2025
        }
    }
    else
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(db_event_data.eventmsg, "%s : Execution failed: %s", fun_name, sqlite3_errmsg(db));
        strcpy(db_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&db_event_data);
        // Clean up
        sqlite3_finalize(stmt);
        sqlite3_close(db);
    }

    return 0;
}

// void createDataTable(char *data_type, int port, char *serial_num, int midx, int event_idx)
// {
//     static char fun_name[] = "createDataTable()";
//     char *err_msg = 0;
//     int rc;
//     int i;
//     char table_name[100];
//     char sql[2048]; // Make sure to allocate sufficient memory for SQL queries

//     // Handle "PLCC" meter types with specific logic for serial number
//     if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
//     {
//         char temp_met_sn[64];
//         memset(temp_met_sn, 0, sizeof(temp_met_sn));
//         strcpy(temp_met_sn, get_meter_2_ser_num(midx));

//         if (strcmp(temp_met_sn, "NC") == 0)
//         {
//             dbg_log(INFORM, "[%-25s] : [Meter - %d] : Returning from creating data table because serial number is not available for this meter.\n", fun_name, midx);
//             return; // Exit if no serial number is available
//         }

//         if (OD_CMD_RECEIVED == 1)
//         {
//             snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%s", data_type, get_manufacturer(midx), serial_num, port, temp_met_sn);
//         }
//         else
//         {
//             snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", data_type, get_manufacturer(midx), serial_num, port, temp_met_sn);
//         }
//     }
//     else
//     {
//         if (OD_CMD_RECEIVED == 1)
//         {
//             snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%d", data_type, get_manufacturer(midx), serial_num, port, midx);
//         }
//         else
//         {
//             snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%d", data_type, get_manufacturer(midx), serial_num, port, midx);
//         }
//     }

//     // Check if the table already exists
//     memset(sql, 0, sizeof(sql));
//     snprintf(sql, sizeof(sql), "SELECT name FROM %s WHERE type='table' AND name='%s';", table_name);

//     db_open(); // Open the database connection

//     sqlite3_stmt *stmt;
//     rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
//     if (rc != SQLITE_OK)
//     {
//         fprintf(stderr, "%s : Failed to prepare statement: %s\n", fun_name, sqlite3_errmsg(db));
//         sqlite3_close(db);
//         return;
//     }

//     rc = sqlite3_step(stmt);
//     if (rc == SQLITE_ROW)
//     {
//         // Table exists, now check if we need to add columns
//         printf("Table '%s' already exists. Checking for required columns...\n", table_name);

//         // Iterate through each event column and check if it exists
//         for (i = 0; i < events_data_value.num_event; i++)
//         {
//             char column_sql[512];
//             snprintf(column_sql, sizeof(column_sql), "PRAGMA table_info(%s);", table_name);
//             sqlite3_stmt *col_stmt;
//             rc = sqlite3_prepare_v2(db, column_sql, -1, &col_stmt, NULL);
//             if (rc != SQLITE_OK)
//             {
//                 fprintf(stderr, "%s : Failed to prepare column info query: %s\n", fun_name, sqlite3_errmsg(db));
//                 sqlite3_finalize(stmt);
//                 sqlite3_close(db);
//                 return;
//             }

//             int column_exists = 0;
//             while (sqlite3_step(col_stmt) == SQLITE_ROW)
//             {
//                 const char *column_name = (const char *)sqlite3_column_text(col_stmt, 1); // Column name is at index 1
//                 if (strcmp(column_name, events_data_value.values[i].obis_code) == 0)
//                 {
//                     column_exists = 1; // Column already exists
//                     break;
//                 }
//             }

//             sqlite3_finalize(col_stmt);

//             if (!column_exists)
//             {
//                 // Column does not exist, so alter the table
//                 snprintf(sql, sizeof(sql), "ALTER TABLE %s ADD COLUMN \"%s\" varchar(46) NOT NULL DEFAULT '0.0';", table_name, events_data_value.values[i].obis_code);
//                 rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
//                 if (rc != SQLITE_OK)
//                 {
//                     fprintf(stderr, "%s : Failed to add column: %s\n", fun_name, err_msg);
//                     sqlite3_free(err_msg);
//                     sqlite3_finalize(stmt);
//                     sqlite3_close(db);
//                     return;
//                 }
//                 printf("Added column '%s' to table '%s'.\n", events_data_value.values[i].obis_code, table_name);
//             }
//         }
//     }
//     else
//     {
//         // Table does not exist, create a new one
//         printf("Table '%s' does not exist. Creating a new table...\n", table_name);
//         memset(sql, 0, sizeof(sql));
//         snprintf(sql, sizeof(sql),
//                  "CREATE TABLE IF NOT EXISTS %s ("
//                  "id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,"
//                  "update_time DATETIME DEFAULT NULL",
//                  table_name); // Create basic columns

//         if (strcmp(data_type, "event_data") == 0)
//         {
//             // Add event_type column
//             strcat(sql, ", ");
//             strcat(sql, "\"event_type\" varchar(46) NOT NULL DEFAULT '0.0'");

//             // Add dynamic columns for each event
//             for (i = 0; i < events_data_value.num_event; i++)
//             {
//                 strcat(sql, ", ");
//                 strcat(sql, "\"");
//                 if (strcmp(events_data_value.values[i].obis_code, "0_0_1_0_0_255") == 0)
//                 {
//                     strcat(sql, events_data_value.values[i].obis_code);
//                     strcat(sql, "\" DEFAULT NULL UNIQUE");
//                 }
//                 else
//                 {
//                     strcat(sql, events_data_value.values[i].obis_code);
//                     strcat(sql, "\" varchar(46) NOT NULL DEFAULT '0.0'");
//                 }
//             }
//         }

//         strcat(sql, ")"); // Closing the table creation query
//         rc = sqlite3_exec(db, sql, NULL, NULL, &err_msg);
//         if (rc != SQLITE_OK)
//         {
//             fprintf(stderr, "%s : SQL error: %s\n", fun_name, err_msg);
//             memset(event_msg_str, 0, sizeof(event_msg_str));
//             sprintf(db_event_data.eventmsg, "%s : SQL error: %s", fun_name, err_msg);
//             strcpy(db_event_data.timestamp, time_format_conversion(time(NULL)));
//             add_event_data(&db_event_data);
//             sqlite3_free(err_msg);
//             sqlite3_close(db);
//         }
//         else
//         {
//             printf("Table '%s' created successfully.\n", table_name);
//         }
//     }

//     sqlite3_finalize(stmt);
//     sqlite3_close(db); // Close the database connection

//     // Insert values into the table (this function is likely responsible for inserting data into the created table)
//     insertValues(table_name, data_type, event_idx);
// }

int is_in_blackout_window(const char *table_name, const char *date_str, int expected_cnt, int table_cnt)
{
    char event_table[256];
    // ls_data_genus_DEFAULT_2_3022193
    if (strncmp(table_name, "ls_data_", 8) == 0)
    {

        snprintf(event_table, sizeof(event_table), "event_data_%s", table_name + 8);
    }

    else
    {

        snprintf(event_table, sizeof(event_table), "%s", table_name);
    }

    dbg_log(INFORM, "[BLACKOUT] Checking date = %s table name %s\n", date_str, event_table);

    //---------------------------------------------------------
    // STEP 1: Get latest OFF event
    //---------------------------------------------------------

    char sql_off[512];
    snprintf(sql_off, sizeof(sql_off),
             "SELECT MAX(\"0_0_1_0_0_255\") FROM %s "
             "WHERE \"0_0_96_11_2_255\" IN ('101.0','105.0') "
             "AND datetime(\"0_0_1_0_0_255\") <= datetime('%s');",
             event_table, date_str);

    sqlite3_stmt *stmt_off = NULL;

    open_db_readonly();

    sqlite3_prepare_v2(db, sql_off, -1, &stmt_off, NULL);

    char last_off[32] = {0};
    if (sqlite3_step(stmt_off) == SQLITE_ROW)
    {
        const unsigned char *tmp = sqlite3_column_text(stmt_off, 0);
        if (tmp)
            snprintf(last_off, sizeof(last_off), "%s", tmp);
    }
    sqlite3_finalize(stmt_off);

    if (!last_off[0])
    {
        sqlite3_close(db);
        return 0;
    }

    //---------------------------------------------------------
    // STEP 2: Get earliest ON event > last_off
    //---------------------------------------------------------
    char sql_on[512];
    snprintf(sql_on, sizeof(sql_on),
             "SELECT MIN(\"0_0_1_0_0_255\") FROM %s "
             "WHERE \"0_0_96_11_2_255\" IN ('102.0','106.0') "
             "AND datetime(\"0_0_1_0_0_255\") > datetime('%s');",
             event_table, last_off);

    sqlite3_stmt *stmt_on = NULL;
    sqlite3_prepare_v2(db, sql_on, -1, &stmt_on, NULL);

    char next_on[32] = {0};
    if (sqlite3_step(stmt_on) == SQLITE_ROW)
    {
        const unsigned char *tmp = sqlite3_column_text(stmt_on, 0);
        if (tmp)
            snprintf(next_on, sizeof(next_on), "%s", tmp);
    }
    sqlite3_finalize(stmt_on);

    if (!next_on[0])
    {
        sqlite3_close(db);
        return 0;
    }

    //---------------------------------------------------------
    // STEP 3: blackout duration in FULL days
    //---------------------------------------------------------
    char sql_span[256];
    snprintf(sql_span, sizeof(sql_span),
             "SELECT julianday(date('%s')) - julianday(date('%s'));",
             next_on, last_off);

    sqlite3_stmt *stmt_span = NULL;
    sqlite3_prepare_v2(db, sql_span, -1, &stmt_span, NULL);

    double day_diff = 0.0;
    if (sqlite3_step(stmt_span) == SQLITE_ROW)
        day_diff = sqlite3_column_double(stmt_span, 0);
    sqlite3_finalize(stmt_span);

    if (day_diff < 1.0)
    {
        sqlite3_close(db);
        return 0;
    } // same-day → ignore
    if (day_diff == 1.0)
    {
        sqlite3_close(db);
        return 0;
    } // exactly 1 day → skip

    //---------------------------------------------------------
    // STEP 4: check if date is within blackout window
    //---------------------------------------------------------
    char sql_between[512];
    snprintf(sql_between, sizeof(sql_between),
             "SELECT (date('%s') >= date('%s')) AND (date('%s') <= date('%s'));",
             date_str, last_off, date_str, next_on);

    sqlite3_stmt *stmt_between = NULL;
    sqlite3_prepare_v2(db, sql_between, -1, &stmt_between, NULL);

    int in_window = 0;
    if (sqlite3_step(stmt_between) == SQLITE_ROW)
        in_window = sqlite3_column_int(stmt_between, 0);
    sqlite3_finalize(stmt_between);

    sqlite3_close(db);

    if (!in_window)
    {
        // sqlite3_close(db);
        return 0;
    }

    char off_day[11], cur_day[11];
    strncpy(off_day, last_off, 10);
    off_day[10] = '\0';
    strncpy(cur_day, date_str, 10);
    cur_day[10] = '\0';

    if (strcmp(cur_day, off_day) == 0)
    {
        // sqlite3_close(db);
        return 0; // skip first day
    }

    //---------------------------------------------------------
    // STEP 6: If LAST blackout day → check LS expected count
    //---------------------------------------------------------
    char next_on_day[11];
    strncpy(next_on_day, next_on, 10);
    next_on_day[10] = '\0';

    // LAST blackout day condition
    if (strcmp(cur_day, next_on_day) == 0)
    {

        // Expected count must MATCH exactly to return 1
        if ((expected_cnt - 1) == table_cnt)
        {
            // sqlite3_close(db);
            dbg_log(INFORM, "[BLACKOUT] Last blackout day %s has EXACT count -----", date_str);

            return 1;
        }
        else
        {
            // sqlite3_close(db);
            dbg_log(INFORM, "[BLACKOUT] Last blackout day %s count NOT exact ", date_str);

            return 0;
        }
    }
    else
    {
        dbg_log(INFORM, "Powered down occured at  %s \n", date_str);
    }

    //---------------------------------------------------------
    // All middle blackout days ALWAYS generate
    //---------------------------------------------------------

    return 1;
}