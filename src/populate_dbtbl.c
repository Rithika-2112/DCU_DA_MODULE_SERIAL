#include "../include/sqlite3.h" // Required for SQLite3 functions
#include <stdio.h>              // Required for standard input/output (e.g., fprintf, printf)
#include <stdlib.h>             // Required for general utilities (e.g., exit)
#include <string.h>             // Required for string manipulation functions (e.g., snprintf, strncat)
#include <time.h>               // Required for time-related functions (e.g., time, localtime, strftime)

// Define constants for the number of columns and total rows per table
#define NUM_OTHER_COLUMNS 30
#define NUM_OTHER_COLUMNS_BILL 121
#define NUM_OTHER_COLUMNS_EVENT 40
#define NUM_OTHER_COLUMNS_MN 15

#define ROWS_PER_TABLE (288 * 30) // Calculate total rows per table: 8640
#define ROWS_PER_TABLE_BILL 12
#define ROWS_PER_TABLE_EVENT 100
#define ROWS_PER_TABLE_MN 30

#define NUM_TABLES 30 // Number of tables to create

/**
 * @brief Gets the current local time formatted as an ISO 8601 string (YYYY-MM-DD HH:MM:SS).
 *
 * @param buffer A character array to store the formatted time string.
 * @param buffer_size The size of the buffer.
 */
void get_current_iso_time(char *buffer, size_t buffer_size)
{
    time_t rawtime;
    struct tm *info;
    // Get current time
    time(&rawtime);
    // Convert to local time structure
    info = localtime(&rawtime);
    // Format the time into the buffer
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", info);
}

/**
 * @brief Populates a specific SQLite database table with default values.
 *
 * The table will have 'id' (PRIMARY KEY AUTOINCREMENT), 'updatetime',
 * and 30 other columns named '1_0_0_2_0_0' through '1_0_0_2_0_29'.
 * It inserts a total of ROWS_PER_TABLE rows.
 *
 * @param db_name The name of the SQLite database file (e.g., "my_database.db").
 * @param table_name The specific name of the table to populate (e.g., "ls_1").
 * @return 0 on success, 1 on failure.
 */
int populate_ls_table(const char *db_name, const char *table_name, int type)
{
    printf("type %d\n",type);
    int i, j;
    sqlite3 *db;       // SQLite database connection object
    char *err_msg = 0; // Buffer for SQLite error messages
    int rc;            // Return code for SQLite API calls

    // --- 1. Open the SQLite database connection ---
    // sqlite3_open opens a database file. If the file does not exist, it is created.
    rc = sqlite3_open(db_name, &db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db); // Close the connection if opening fails
        return 1;
    }
    fprintf(stdout, "Database '%s' opened successfully for table '%s'.\n", db_name, table_name);

    // --- 2. Construct the CREATE TABLE SQL statement ---
    // This buffer will hold the dynamically generated SQL for creating the table.
    // It needs to be large enough to accommodate the base statement + 30 column definitions.
    char create_table_sql[2048 * 6];
    // Start with the fixed part of the CREATE TABLE statement
    memset(create_table_sql, 0, sizeof(create_table_sql));
    snprintf(create_table_sql, sizeof(create_table_sql),
             "CREATE TABLE IF NOT EXISTS %s (id INTEGER PRIMARY KEY AUTOINCREMENT, update_time DATETIME DEFAULT NULL",
             table_name);

    // Append the 30 other column definitions dynamically
    int num_col, num_rows;
    if (type == 1)
    {
        printf("inside type 1\n");
        num_col = NUM_OTHER_COLUMNS;
        num_rows = ROWS_PER_TABLE;
    }
    else if (type == 2)
    {
        printf("inside type 2\n");
        num_col = NUM_OTHER_COLUMNS_MN;
        num_rows = ROWS_PER_TABLE_MN;
    }
    else if (type == 3)
    {
        printf("inside type 3\n");
        num_col = NUM_OTHER_COLUMNS_BILL;
        num_rows = ROWS_PER_TABLE_BILL;
    }
    else if (type == 4)
    {
        printf("inside type 4\n");
        num_col = NUM_OTHER_COLUMNS_EVENT;
        num_rows = ROWS_PER_TABLE_EVENT;
    }
    printf("strlen of 1 %d\n",strlen(create_table_sql));
    for (i = 0; i < num_col; i++)
    {
        char col_name[32]; // Buffer for a single column name (e.g., "1_0_0_2_0_0")
        // Format the column name as specified: "1_0_0_2_0_X" where X is the index
        snprintf(col_name, sizeof(col_name), "1_0_0_2_0_%d", i);
        // Append ", column_name TEXT" to the SQL string
        strncat(create_table_sql, ", ", sizeof(create_table_sql) - strlen(create_table_sql) - 1);
        strcat(create_table_sql, "\"");
        strncat(create_table_sql, col_name, sizeof(create_table_sql) - strlen(create_table_sql) - 1);
        strncat(create_table_sql, "\" Real NOT NULL DEFAULT '0.0'", sizeof(create_table_sql) - strlen(create_table_sql) - 1);
        printf("strlen of %d %d\n",i, strlen(create_table_sql));
    }
    printf("strlen of 3 %d\n",strlen(create_table_sql));
    // Close the CREATE TABLE statement
    strncat(create_table_sql, ");", sizeof(create_table_sql) - strlen(create_table_sql) - 1);

    fprintf(stdout, "Creating table with SQL: %s\n", create_table_sql);
printf("strlen of 4 %d\n",strlen(create_table_sql));
    // Execute the CREATE TABLE statement
    rc = sqlite3_exec(db, create_table_sql, 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error creating table '%s': %s\n", table_name, err_msg);
        sqlite3_free(err_msg); // Free the error message allocated by SQLite
        sqlite3_close(db);
        return 1;
    }
    printf("strlen of 5 %d\n",strlen(create_table_sql));
    fprintf(stdout, "Table '%s' created or already exists.\n", table_name);

    // --- 3. Construct the INSERT SQL statement with placeholders ---
    // This buffer will hold the dynamically generated SQL for inserting data.
   
    char insert_sql[20480];
    memset(insert_sql, 0 ,sizeof(insert_sql));
    // Start with the fixed part of the INSERT statement, listing the columns to insert into
    snprintf(insert_sql, sizeof(insert_sql), "INSERT INTO %s (update_time", table_name);
    printf("seg 1\n",strlen(insert_sql));
    // Append the 30 other column names to the INSERT statement
    for (i = 0; i < num_col; i++)
    {
        char col_name[32];
        snprintf(col_name, sizeof(col_name), "1_0_0_2_0_%d", i);

        strncat(insert_sql, ", ", sizeof(insert_sql) - strlen(insert_sql) - 1);
        strcat(insert_sql, "\"");
        strncat(insert_sql, col_name, sizeof(insert_sql) - strlen(insert_sql) - 1);
        strcat(insert_sql, "\"");
    }
      printf("seg 2\n", strlen(insert_sql));
    // Start the VALUES clause with a placeholder for 'updatetime'
    strncat(insert_sql, ") VALUES (?, ", sizeof(insert_sql) - strlen(insert_sql) - 1);

    // Append placeholders for the 30 other columns
    for (i = 0; i < num_col; i++)
    {
        strncat(insert_sql, "?", sizeof(insert_sql) - strlen(insert_sql) - 1);
        if (i < num_col - 1)
        { // Add comma if not the last placeholder
            strncat(insert_sql, ", ", sizeof(insert_sql) - strlen(insert_sql) - 1);
        }
        printf("strlen of insert_sql %d\n", strlen(insert_sql));
    }
     printf("seg 3\n", strlen(insert_sql));
    // Close the VALUES clause
    strncat(insert_sql, ");", sizeof(insert_sql) - strlen(insert_sql) - 1);

    fprintf(stdout, "Insert statement SQL for '%s': %s\n", table_name, insert_sql);
 printf("seg 4\n", strlen(insert_sql));
    sqlite3_stmt *stmt; // Prepared statement object
    // Prepare the INSERT statement. This compiles the SQL into a reusable form.
    rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to prepare statement for table '%s': %s\n", table_name, sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
 printf("seg 5\n");
    // --- 4. Populate Data using a Transaction for performance ---
    fprintf(stdout, "Populating %d rows into table '%s'...\n", num_rows, table_name);

    // Begin a transaction. This groups multiple inserts into a single atomic operation,
    // which is significantly faster for bulk insertions.
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to begin transaction for table '%s': %s\n", table_name, err_msg);
        sqlite3_free(err_msg);
        sqlite3_finalize(stmt); // Finalize the prepared statement
        sqlite3_close(db);
        return 1;
    }
 printf("seg 6\n");
    char updatetime_buffer[20];     // Buffer for the formatted updatetime string
    char default_value_buffer[128]; // Buffer for default column values

    for (i = 0; i < num_rows; i++)
    { 
        // printf("seg 7\n");
        // Get the current time for the 'updatetime' column
        get_current_iso_time(updatetime_buffer, sizeof(updatetime_buffer));

        // Bind the updatetime string to the first placeholder (index 1)
        // SQLITE_TRANSIENT tells SQLite to make a private copy of the string.
        sqlite3_bind_text(stmt, 1, updatetime_buffer, -1, SQLITE_TRANSIENT);

        float float_value = 100.002;
        // Bind default values to the other 30 columns
        for (j = 0; j < num_col; j++)
        {
        //  printf("seg 8\n");
            // Create a unique default value string for each column and row
            // Include table name in the default value for better distinction
            // snprintf(default_value_buffer, sizeof(default_value_buffer),
            //          "default_value_%s_1_0_0_2_0_%d_row_%d", table_name, j, i);
            // // Bind the default value string to the appropriate placeholder
            // // (index 2 for the first custom column, 3 for the second, and so on)
            // sqlite3_bind_text(stmt, j + 2, default_value_buffer, -1, SQLITE_TRANSIENT);

            // Example float value
            sqlite3_bind_double(stmt, j + 2, float_value);
        }

        // Execute the prepared statement. SQLITE_DONE indicates successful execution.
        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE)
        {
            fprintf(stderr, "Execution failed for table '%s' at row %d: %s\n", table_name, i, sqlite3_errmsg(db));
            // Rollback the transaction on error to undo any partial insertions
            sqlite3_exec(db, "ROLLBACK;", 0, 0, 0);
            sqlite3_finalize(stmt);
            sqlite3_close(db);
            return 1;
        }

        // Reset the statement to its initial state, ready for the next set of bindings
        sqlite3_reset(stmt);
        // Clear any previous bindings (optional, but good practice)
        sqlite3_clear_bindings(stmt);
    }
 printf("seg 9\n");
    // Commit the transaction. This writes all the inserted rows to the database file.
    rc = sqlite3_exec(db, "COMMIT;", 0, 0, &err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Failed to commit transaction for table '%s': %s\n", table_name, err_msg);
        sqlite3_free(err_msg);
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return 1;
    }

    fprintf(stdout, "Successfully populated %d rows into table '%s'.\n", ROWS_PER_TABLE, table_name);

    // --- 5. Clean up ---
    sqlite3_finalize(stmt); // Finalize the prepared statement
    sqlite3_close(db);      // Close the database connection

    return 0; // Indicate success
}

int create_db_vacuum(char *db_name)
{
    sqlite3 *db;
    sqlite3_open(db_name, &db);

    // Disable auto-vacuum (optional)
    sqlite3_exec(db, "PRAGMA auto_vacuum = NONE;", NULL, NULL, NULL);

    // Set page size (e.g., 512 bytes)
    sqlite3_exec(db, "PRAGMA page_size = 512;", NULL, NULL, NULL);

    // Run VACUUM
    sqlite3_exec(db, "VACUUM;", NULL, NULL, NULL);

    sqlite3_close(db);
}

// --- Main function for demonstration ---
int main_populate(int argc, char *argv[])
{
    printf("Starting database population process for %d tables...\n", NUM_TABLES);
    const char *db_file_name = "dcu_dlms.db"; // The database file name

    int i, j;
    create_db_vacuum(db_file_name);
    printf("argv[1] %d\n",argv[1]);

    int type = atoi(argv[1]);
    printf("type %d\n",type);

    // for (j = 1; j < 5; j++)
    // {
        for (i = 1; i <= NUM_TABLES; i++)
        {
            char current_table_name[32]; // Buffer for table name (e.g., "ls_1", "ls_2", ...)

            if(type == 1){
                 snprintf(current_table_name, sizeof(current_table_name), "LS_%d", i);

            }
            else if(type == 2){
                 snprintf(current_table_name, sizeof(current_table_name), "MN_%d", i);

            }
             else if(type == 3){
                 snprintf(current_table_name, sizeof(current_table_name), "BILL_%d", i);

            }
             else if(type == 4){
                 snprintf(current_table_name, sizeof(current_table_name), "EVENT_%d", i);

            }
           
            printf("\n--- Processing table: %s ---\n", current_table_name);
            // Call the function to populate the current table
            if (populate_ls_table(db_file_name, current_table_name, type) != 0)
            {
                fprintf(stderr, "Database population failed for table '%s'. Aborting.\n", current_table_name);
                return 1; // Exit if any table population fails
            }
        }
    // }

    printf("\nAll %d tables populated successfully in '%s'.\n", NUM_TABLES, db_file_name);
    return 0;
}
