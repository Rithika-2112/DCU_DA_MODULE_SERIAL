#include "../include/dlms_config.h"
#include "../include/general_config.h"
#include "../include/plcc_rel.h"

extern feature_cfg_t feature_cfg;
// rithika 24sep
extern int checkDbgStatus(void);
extern int dbgloglevel;
extern uint8_t g_pidx;

// FUNC DEC
Date calculate_end_date(Date ls_date, int num_days);
Date calculate_hours_minutes(int num_blocks, int blk_int);

time_t date_to_time_t(Date date);

double string_to_double(const char *str);

int days_in_month(int month, int year);

char *replace_dot_with_underscore(char *str);
char *removeSpace(char *hexString, char *outputBuffer, size_t bufferSize, int type_of_data, char *manufacturer);

void convertDate(const char *input, char *output, int, int);
void calculate_start_and_end_date(const char *date_str, struct tm *start_date, struct tm *end_date, int);

extern char *time_format_conversion(time_t);
extern Date meter_date_time[MAX_NO_OF_METER_PER_PORT];
////////////////////////////////////////////////////////////////
#define TT_FILE_SIZE_EXCEEDS 1 * 1024 * 1024
#define LOGS_FILE_SIZE_EXCEEDS 100 * 1024 // 100kb
#define LOG_DIR "/usr/cms/log/"

char od_time_buf[246];

int snrm_aarq_failed_cnt(const char *data, ...)
{
    checkDbgStatus();

    if (dbgloglevel == 0)
    {
        return 0;
    }

    FILE *fpt;
    printf("======================================= > >  > %s", data);
    fpt = fopen("/usr/cms/log/dcu_da_cmd.log", "a");

    if (fpt == NULL)
    {
        printf("open call failed for file : %s , Error : %s\n", "/usr/cms/log/dcu_da_cmd.log", strerror(errno));
        return -1;
    }

    va_list args;
    va_start(args, data);

    // Print to console
    vprintf(data, args);

    // Write formatted data to file
    vfprintf(fpt, data, args);

    fflush(fpt); // Flush the data to the file
    va_end(args);

    long pos = ftell(fpt);

    if (pos >= LOGS_FILE_SIZE_EXCEEDS)
    {
        fclose(fpt);

        char bkup_file[64];
        char tmp_buf[128];

        memset(bkup_file, 0, sizeof(bkup_file));
        sprintf(bkup_file, "%s.log", "/usr/cms/log/dcu_da_cmd_bck");

        memset(tmp_buf, 0, sizeof(tmp_buf));
        sprintf(tmp_buf, "mv %s %s", "/usr/cms/log/dcu_da_cmd.log", bkup_file);
        system(tmp_buf);

        fpt = fopen("/usr/cms/log/dcu_da_cmd.log", "w");
        // perror("fopen:");
    }

    fclose(fpt);

    return 0;
}

int create_sd_log_dir()
{
    struct stat st;
    char fun_name[] = "create_sd_log_dir()";
    if (stat(SD_LOC, &st) == 0 && S_ISDIR(st.st_mode))
    {
        // Path exists and is a directory
        printf("Storage directory exists.\n");
        dbg_log(INFORM, "[%-25s] path %s exits creating log dir in it", fun_name, SD_LOC);
        char path[128];

        sprintf(path, "%slog", SD_LOC);
        mkdir(path, 0777);
    }
    else
    {
        // Path doesn't exist or is not a directory
        dbg_log(INFORM, "[%-25s] path %s doesn't exists", fun_name, SD_LOC);
        perror("Storage path invalid");
    }
}

int poll_status_log(const char *data, ...)
{
    checkDbgStatus();

    if (dbgloglevel == 0)
    {
        return 0;
    }

    FILE *fpt;
    printf("======================================= > >  > %s", data);
    char path[256];
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        sprintf(path, "%slog/polling_status.log", SD_LOC);
    }
    else
    {
        sprintf(path, "%sserial_polling_status.log", LOG_DIR);
    }

    fpt = fopen(path, "a");

    if (fpt == NULL)
    {
        printf("open call failed for file : %s , Error : %s\n", path, strerror(errno));
        return -1;
    }

    va_list args;
    va_start(args, data);

    // Print to console
    vprintf(data, args);

    // Write formatted data to file
    vfprintf(fpt, data, args);

    fflush(fpt); // Flush the data to the file
    va_end(args);

    long pos = ftell(fpt);

    printf("%d\n", pos);

    if (pos >= LOGS_FILE_SIZE_EXCEEDS)
    {
        fclose(fpt);

        char bkup_file[64];
        char tmp_buf[128];

        memset(bkup_file, 0, sizeof(bkup_file));
        memset(path, 0, sizeof(path));
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(path, "%slog/polling_status_bck", SD_LOC);
        }
        else
        {
            sprintf(path, "%sserial_polling_status_bck", LOG_DIR);
        }

        sprintf(bkup_file, "%s.log", path);

        memset(tmp_buf, 0, sizeof(tmp_buf));
        memset(path, 0, sizeof(path));

        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(path, "%slog/polling_status.log", SD_LOC);
        }
        else
        {
            sprintf(path, "%sserial_polling_status.log", LOG_DIR);
        }
        sprintf(tmp_buf, "mv %s %s", path, bkup_file);
        system(tmp_buf);

        memset(path, 0, sizeof(path));
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(path, "%slog/polling_status.log", SD_LOC);
        }
        else
        {
            sprintf(path, "%sserial_polling_status.log", LOG_DIR);
        }
        fpt = fopen(path, "w");
        // perror("fopen:");
    }

    fclose(fpt);

    return 0;
}

int data_status_log(const char *data, ...)
{
    checkDbgStatus();

    if (dbgloglevel == 0)
    {
        return 0;
    }

    FILE *fpt;
    long pos;

    // Print separator to console
    printf("======================================= > >  > %s", data);
    char path[256];
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        sprintf(path, "%slog/data_timing_calc.log", SD_LOC);
    }
    else
    {
        sprintf(path, "%sserial_data_timing_calc.log", LOG_DIR);
    }

    fpt = fopen(path, "a");

    if (fpt == NULL)
    {
        printf("open call failed for file : %s , Error : %s\n", path, strerror(errno));
        return -1;
    }

    va_list args1, args2;
    va_start(args1, data);
    va_copy(args2, args1);

    // Print to console
    vprintf(data, args1);

    // Write to file
    vfprintf(fpt, data, args2);

    fflush(fpt); // Flush to file

    va_end(args1);
    va_end(args2);

    pos = ftell(fpt);

    if (pos >= LOGS_FILE_SIZE_EXCEEDS)
    {
        fclose(fpt);

        char bkup_file[64];
        char tmp_buf[128];

        memset(path, 0, sizeof(path));
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(path, "%slog/data_timing_calc_bck", SD_LOC);
        }
        else
        {
            sprintf(path, "%sserial_data_timing_calc_bck", LOG_DIR);
        }
        snprintf(bkup_file, sizeof(bkup_file), "%s.log", path);

        memset(path, 0, sizeof(path));
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(path, "%slog/data_timing_calc.log", SD_LOC);
        }
        else
        {
            sprintf(path, "%sserial_data_timing_calc.log", LOG_DIR);
        }
        snprintf(tmp_buf, sizeof(tmp_buf), "mv %s %s", path, bkup_file);
        system(tmp_buf);

        fpt = fopen(path, "w");
    }

    fclose(fpt);

    return 0;
}

int hc_status_log(const char *data, ...)
{
    checkDbgStatus();

    if (dbgloglevel == 0)
    {
        return 0;
    }

    FILE *fpt;
    // printf("======================================= > >  > %s", data);
    char path[256];
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        snprintf(path, sizeof(path), "%slog/hc_update_status.log", SD_LOC);
    }
    else
    {
        snprintf(path, sizeof(path), "%sserial_hc_update_status.log", LOG_DIR);
    }
    fpt = fopen(path, "a");

    if (fpt == NULL)
    {
        printf("open call failed for file : %s , Error : %s\n", path, strerror(errno));
        return -1;
    }

    va_list args;
    va_start(args, data);

    // Print to console
    // vprintf(data, args); // after uncomment rithika

    // Write formatted data to file
    vfprintf(fpt, data, args);

    fflush(fpt); // Flush the data to the file
    va_end(args);

    long pos = ftell(fpt);

    if (pos >= LOGS_FILE_SIZE_EXCEEDS)
    {

        fclose(fpt);

        char bkup_file[64];
        char tmp_buf[128];

        memset(path, 0, sizeof(path));
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(path, "%slog/hc_update_status_bck", SD_LOC);
        }
        else
        {
            sprintf(path, "%sserial_hc_update_status_bck", LOG_DIR);
        }
        memset(bkup_file, 0, sizeof(bkup_file));
        sprintf(bkup_file, "%s.log", path);

        memset(path, 0, sizeof(path));
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            sprintf(path, "%slog/hc_update_status.log", SD_LOC);
        }
        else
        {
            sprintf(path, "%sserial_hc_update_status.log", LOG_DIR);
        }
        memset(tmp_buf, 0, sizeof(tmp_buf));
        sprintf(tmp_buf, "mv %s %s", path, bkup_file);
        system(tmp_buf);

        fpt = fopen(path, "w");
        // perror("fopen:");
    }

    fclose(fpt);

    return 0;
}

////////////////////////////////////////////////////////////////

void print_colored_message(const char *color, int TESTING, const char *message, ...)
{
    if (TESTING == 1)
    {
        va_list args;
        va_start(args, message);

        printf("%s", color);
        vprintf(message, args);
        printf("%s", RESET);

        va_end(args);
    }
}

time_t date_to_time_t(Date date)
{
    struct tm tm_time = {0};
    tm_time.tm_year = date.year - 1900; // tm_year is years since 1900
    tm_time.tm_mon = date.month - 1;    // tm_mon is 0-based (0 = January)
    tm_time.tm_mday = date.day;
    tm_time.tm_hour = date.hour;
    tm_time.tm_min = date.minute;
    tm_time.tm_sec = date.second;

    return mktime(&tm_time);
}

void print_tm(const struct tm *time_info)
{
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", time_info);
    printf("%s\n", buffer);
}

void calculate_start_and_end_date(const char *date_str, struct tm *start_date, struct tm *end_date, int midx)
{
    int month, year;
    sscanf(date_str, "%d_%d", &month, &year);

    start_date->tm_year = year - 1900;
    start_date->tm_mon = month - 1;
    start_date->tm_mday = 1;
    start_date->tm_hour = 0;
    start_date->tm_min = 0;
    start_date->tm_sec = 0;
    start_date->tm_isdst = -1;

    mktime(start_date);

    printf("========================== month : %d meter_date_time[midx].month : %d= ======\n", month, meter_date_time[midx].month);

    if (month == meter_date_time[midx].month)
    {
        end_date->tm_year = year - 1900;
        end_date->tm_mon = (meter_date_time[midx].month - 1);
        end_date->tm_mday = (meter_date_time[midx].day);
        end_date->tm_hour = 23;
        end_date->tm_min = 59;
        end_date->tm_sec = 59;
        // end_date->tm_isdst = -1;
    }
    else
    {
        end_date->tm_year = year - 1900;
        end_date->tm_mon = month;
        end_date->tm_mday = 0;
        end_date->tm_hour = 23;
        end_date->tm_min = 59;
        end_date->tm_sec = 59;
        end_date->tm_isdst = -1;
    }

    mktime(end_date);

    printf("Start Date: ");
    print_tm(start_date);

    printf("End Date: ");
    print_tm(end_date);
}

/**
 * Replaces all occurrences of '.' with '_' in the given string.
 *
 * @param str The input string to be modified. It is assumed that this string is mutable.
 * @return A pointer to the modified string (the same string that was passed in).
 *         Returns NULL if the input string is NULL.
 */
char *replace_dot_with_underscore(char *str)
{

    if (str != NULL)
    {

        for (int i = 0; str[i] != '\0'; i++)
        {
            if (str[i] == '.')
            {
                str[i] = '_';
            }
        }
    }

    return str;
}

/**
 * @brief Removes spaces from a hexadecimal string and converts it to a formatted date-time string.
 *
 * This function takes a string representing a hexadecimal date-time, removes any spaces,
 * parses the year, month, day, hour, and minute from the cleaned string, and formats
 * it into a more human-readable format (DD-MM-YYYY HH:MM).
 *
 * @param hexString A pointer to a null-terminated string containing the hexadecimal date-time
 *                  representation, potentially containing spaces.
 * @return A pointer to a statically allocated string containing the formatted date-time
 *         (DD-MM-YYYY HH:MM). Note that this buffer will be overwritten on subsequent calls
 *         to this function.
 */
char *removeSpace(char *hexString, char *outputBuffer, size_t bufferSize, int type_of_data, char *manufacturer)
{
    int index = 0;
    unsigned short year;
    unsigned char month;
    unsigned char day;
    unsigned char hour;
    unsigned char minute;

    for (int i = 0; hexString[i] != '\0' && index < bufferSize - 1; i++)
    {
        if (hexString[i] != ' ')
        {
            outputBuffer[index++] = hexString[i];
        }
    }

    outputBuffer[index] = '\0';

    // rithika 28oct2025

    if (type_of_data == 1 || (type_of_data == 5 && (strcmp(manufacturer, "genus") == 0 || strcmp(manufacturer, "secure") == 0)))
    {

        unsigned char sec;
        sscanf(outputBuffer, "%4hx%2hhx%2hhx%*2hhx%2hhx%2hhx%2hhx", &year, &month, &day, &hour, &minute, &sec);

        snprintf(outputBuffer, bufferSize, "%04u-%02u-%02u %02u:%02u:%02u", year, month, day, hour, minute, sec);
    }
    else if (type_of_data == 2 || type_of_data == 4 || (type_of_data == 5 && strcmp(manufacturer, "lnt") == 0))
    {
        sscanf(outputBuffer, "%4hx%2hhx%2hhx%*2hhx%2hhx%2hhx", &year, &month, &day, &hour, &minute);

        snprintf(outputBuffer, bufferSize, "%04u-%02u-%02u %02u:%02u:%s", year, month, day, hour, minute, "00");
    }
    else
    {
        sscanf(outputBuffer, "%4hx%2hhx%2hhx%*2hhx%2hhx%2hhx", &year, &month, &day, &hour, &minute);

        snprintf(outputBuffer, bufferSize, "%02u-%02u-%04u %02u:%02u", day, month, year, hour, minute);
    }
}

void convertDate(const char *input, char *output, int change, int type_of_data)
{
    char clean[256];
    strncpy(clean, input, sizeof(clean));
    clean[sizeof(clean) - 1] = '\0';

    // 1. Strip timezone (cut at first space before UTC)
    char *utc = strstr(clean, "UTC");
    if (utc)
        *utc = '\0';

    // 2. Replace '*' with '0'
    for (int i = 0; clean[i]; i++)
    {
        if (clean[i] == '*')
            clean[i] = '0';
    }

    int month = 0, day = 0, year = 0, hour = 0, minute = 0, second = 0;

    if (sscanf(clean, "%d/%d/%d %d:%d:%d", &month, &day, &year,
               &hour, &minute, &second) == 6)
    {
        snprintf(output, 64, "%04d-%02d-%02d %02d:%02d:%02d",
                 year, month, day, hour, minute, second);
    }
    else
    {
        strcpy(output, "INVALID_DATE");
    }
}

/**
 * @brief Converts a date-time string from one format to another.
 *
 * This function takes an input date-time string in the format "MM/DD/YYYY HH:MM:SS.mmm",
 * parses it, and converts it to the format "YYYY-MM-DD HH:MM:SS.mmm". The output is stored
 * in a provided buffer.
 *
 * @param input A pointer to a null-terminated string containing the input date-time in the
 *              format "MM/DD/YYYY HH:MM:SS.mmm".
 * @param output A pointer to a buffer where the formatted date-time string will be stored.
 *               This buffer must be large enough to hold the resulting string (at least
 *               24 characters).
 */
// void convertDate(const char *input, char *output, int change, int type_of_data)
// {
//     int month, day, year;
//     int hour, minute, second;

//     sscanf(input, "%d/%d/%d %d:%d:%d", &month, &day, &year, &hour, &minute, &second);

//     sprintf(output, "%04d-%02d-%02d %02d:%02d:%02d",
//             year, month, day, hour, minute, second);

//     // if (type_of_data != LSDATA)
//     // {
//     //     sprintf(output, "%04d-%02d-%02d %02d:%02d:%02d",
//     //             year, month, day, hour, minute, second);
//     // }
//     // else
//     // {
//     //     // if (change == 0)
//     //     // {
//     //     //     sprintf(output, "%04d-%02d-%02d %02d:%02d:%02d",
//     //     //             year, month, day, hour, minute, second);
//     //     // }
//     //     // else
//     //     // {
//     //         hour += 5;
//     //         minute += 30;

//     //         if (minute >= 60)
//     //         {
//     //             minute -= 60;
//     //             hour += 1;
//     //         }

//     //         if (hour >= 24)
//     //         {
//     //             hour -= 24;
//     //             day += 1;
//     //         }
//     //         sprintf(output, "%04d-%02d-%02d %02d:%02d:%02d",
//     //                 year, month, day, hour, minute, second);
//     //     // }
//     // }
// }

/**
 * @brief Determines if a given year is a leap year.
 *
 * A leap year is defined as a year that is divisible by 4,
 * except for end-of-century years, which must be divisible by 400
 * to be considered leap years. This function checks these conditions
 * to return true if the year is a leap year, and false otherwise.
 *
 * @param year The year to be checked.
 * @return true if the year is a leap year; false otherwise.
 */
bool is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

/**
 * @brief Returns the number of days in a specified month of a given year.
 *
 * This function accounts for leap years when determining the number of days
 * in February. For all other months, it returns the standard number of days.
 *
 * @param month The month (1 for January, 2 for February, ..., 12 for December).
 * @param year The year used to determine if February has 28 or 29 days.
 * @return The number of days in the specified month of the given year.
 */
int days_in_month(int month, int year)
{
    int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2 && is_leap_year(year))
        return 29;
    // return days[month - 1];
    // rithika 06nov2025
    return days[month];
}

/**
 * @brief Calculates the end date by adding a specified number of days
 * to a given starting date.
 *
 * This function takes a starting date and adds a number of days to it,
 * properly handling month transitions and leap years. The resulting
 * end date is returned.
 *
 * @param ls_date The starting date from which to calculate the end date.
 * @param num_days The number of days to add to the starting date.
 * @return The calculated end date after adding the specified number of days.
 */
// Date calculate_end_date(Date ls_date, int num_days)
// {
//     Date end_date = ls_date;

//     while (num_days > 0)
//     {
//         int days_this_month = days_in_month(end_date.month, end_date.year);
//         printf("days_this_month : %d end_date.day : %d num_days : %d\n",days_this_month,end_date.day,num_days);
//         if (end_date.day + num_days <= days_this_month)
//         {
//             end_date.day += num_days;
//             break;
//         }
//         else
//         {
//             num_days -= (days_this_month - end_date.day + 1);
//             end_date.day = 1;
//             end_date.month++;
//             if (end_date.month > 12)
//             {
//                 end_date.month = 1;
//                 end_date.year++;
//             }
//         }
//     }
//     end_date.day = end_date.day - 1;
//     return end_date;
// }

Date calculate_end_date(Date ls_date, int num_days)
{
    Date end_date = ls_date;

    while (num_days > 0)
    {
        int days_this_month = days_in_month(end_date.month, end_date.year);
        // printf("days_this_month: %d, end_date.day: %d, num_days: %d\n", days_this_month, end_date.day, num_days);

        // If adding num_days doesn't overflow the month, simply add it
        if (end_date.day + num_days <= days_this_month)
        {
            end_date.day += num_days;
            break;
        }
        else
        {
            // If adding days overflows the month, move to the next month
            num_days -= (days_this_month - end_date.day + 1);
            end_date.day = 1;
            end_date.month++;
            if (end_date.month > 12)
            {
                end_date.month = 1;
                end_date.year++;
            }
        }
    }

    return end_date;
}
/**
 * @brief Calculates the start date and time based on the number of
 * blocks and block interval.
 *
 * This function computes the start date and time by subtracting a
 * specified number of blocks, each of a given interval (in minutes),
 * from the current time. The result is returned as a Date structure
 * containing the year, month, day, hour, and minute of the calculated
 * start time.
 *
 * @param num_blocks The number of blocks to subtract from the current time.
 * @param blk_int The duration of each block in minutes.
 * @return A Date structure representing the calculated start date and time.
 */

Date calculate_hours_minutes(int num_blocks, int blk_int)
{
    time_t t = time(NULL);
    time_t start_time_t = t - (blk_int * num_blocks * 60); // Convert minutes to seconds

    struct tm *tm_start = localtime(&start_time_t);

    Date start_time;
    start_time.year = tm_start->tm_year + 1900; // tm_year is years since 1900
    start_time.month = tm_start->tm_mon + 1;    // tm_mon is 0-indexed
    start_time.day = tm_start->tm_mday;
    start_time.hour = tm_start->tm_hour;
    start_time.minute = tm_start->tm_min;

    return start_time;
}

/**
 * @brief Converts a string to a floating-point number.
 *
 * This function takes a string representation of a number and converts
 * it to a `float` using the `strtof` function. If the conversion is
 * successful, the resulting floating-point value is returned.
 *
 * @param str The input string to be converted.
 * @return The converted floating-point number.
 */
double string_to_double(const char *str)
{
    // rithika 08oct2025
    // return strtof(str, NULL);
    return strtod(str, NULL);
}

int lsdata_not_avalb_log(const char *data, ...)
{
    checkDbgStatus();

    if (dbgloglevel == 0)
    {
        return 0;
    }

    FILE *fpt;
    long pos;

    char path[128];
    sprintf(path, "%sser_port_%d_ls_data_miss.log", LOG_DIR, g_pidx);

    // Build the final string from varargs
    char final_msg[512];
    va_list args;
    va_start(args, data);
    vsnprintf(final_msg, sizeof(final_msg), data, args);
    va_end(args);

    // Trim trailing newline
    final_msg[strcspn(final_msg, "\r\n")] = '\0';

    printf("======================================= > >  > %s\n", final_msg);

    // --- Duplicate check ---
    int already_exists = 0;
    {
        FILE *fr = fopen(path, "r");
        if (fr != NULL)
        {
            char line[512];
            while (fgets(line, sizeof(line), fr) != NULL)
            {
                line[strcspn(line, "\r\n")] = '\0'; // remove newline
                if (strcmp(line, final_msg) == 0)
                {
                    already_exists = 1;
                    break;
                }
            }
            fclose(fr);
        }
    }

    if (already_exists)
    {
        return 0; // Skip duplicate
    }

    // --- Append to log ---
    fpt = fopen(path, "a");
    if (fpt != NULL)
    {
        fprintf(fpt, "%s\n", final_msg); // write clean message with newline
        fflush(fpt);

        pos = ftell(fpt);

        if (pos >= LOGS_FILE_SIZE_EXCEEDS)
        {
            fclose(fpt);

            char bkup_file[64];
            char tmp_buf[128];

            sprintf(path, "%sser_port_%d_ls_data_miss_bkp", LOG_DIR, g_pidx);
            snprintf(bkup_file, sizeof(bkup_file), "%s.log", path);
            sprintf(path, "%sser_port_%d_ls_data_miss.log", LOG_DIR, g_pidx);
            snprintf(tmp_buf, sizeof(tmp_buf), "mv %s %s", path, bkup_file);
            system(tmp_buf);

            fpt = fopen(path, "w");
            if (fpt == NULL)
            {
                printf("open call failed for file: %s , Error: %s\n",
                       path, strerror(errno));
                return -1;
            }

            fclose(fpt);
        }
        else
        {
            fclose(fpt);
        }
    }

    return 0;
}
