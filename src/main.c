#include "../include/dlms_config.h"
#include "../include/general_config.h"
#include "../include/plcc_rel.h"

#define GET_OLD_DATA 1
// #define MAX_EVENTS 7 //rithika 31oct2025
#define MAX_STRING_LENGTH 50
#define INST_20_SEC_CHK 20
#define DBGCFG_FILE_NAME "/usr/cms/config/debuglog.cfg"
#define LS_BLOCK_TIME "now"
// rithika 13nov2025
#define POWER_EVENTS 3

#define POWER_EVENT_IDX 2
time_t dbgcfgtime = 0;
int dbgloglevel = 0;

// Function pointer types
typedef int (*nameplate_func)(int);
typedef int (*inst_func)(int);
typedef int (*ls_day_func)(int, const char *, int);
typedef int (*ls_block_func)(int, const char *, int);
typedef int (*event_func)(int, char *, int);
typedef int (*midnight_func)(int, int);
typedef int (*billing_func)(int, int);
// rithika 13oct2025
typedef int (*inst_all_func)();

// Union to hold different function pointer types
typedef union
{
    nameplate_func get_name_plate;
    inst_func inst;
    ls_day_func ls_day;
    ls_block_func ls_block;
    event_func event;
    midnight_func midnight;
    billing_func billing;
    inst_all_func inst_all_data;
} FunctionPointer;
// hello
//  Structure to hold function information
typedef struct
{
    char type[20];
    FunctionPointer func;
    char date[11];
    char event_type[16];
    char param[8];
    int midx;
} PollFunction;

// Function prototypes
int get_name_plate(int midx);
int get_inst_data(int midx);
int get_ls_data_day(int midx, char *date, char *num_days);
int get_ls_data_block(int midx, const char *date, char *num_blocks);
int get_billing_data(int midx, char *num_months);
int get_midnight_data(int midx, char *num_days);
int get_event_data(int midx, char *event_type, char *num_events);
// rithika 13oct2025
int get_all_meter_inst_data();

// Global array to store poll functions
PollFunction *poll_functions = NULL;
extern poll_cfg_t poll_cfg;

int num_poll_functions = 0;

extern redisContext *redis;

int curr_nc_idx = 0;
int no_of_meter = 0;
uint8_t g_pidx;

feature_cfg_t feature_cfg;
strategy_cfg_t strategy_cfg;
ser_port_channel_t ser_port_data[MAX_NO_OF_SERIAL_PORT];

char dcu_ser_num[16];
char p_comm_port_det[16];

time_t last_rs_check_time;

uint8_t midnight_polled[MAX_NO_OF_METER_PER_PORT] = {0};

int enbl_data_clean = 1;  // rithika 23sep
int data_clean_hour = 23; // 23 value
int data_clean_min = 2;
// int reset_datacln_hr = 16; //0

uint8_t inst_poll_flag[MAX_NO_OF_METER_PER_PORT];
uint8_t ls_poll_flag[MAX_NO_OF_METER_PER_PORT];
uint8_t mn_poll_flag[MAX_NO_OF_METER_PER_PORT];
uint8_t event_poll_flag[MAX_NO_OF_METER_PER_PORT];
uint8_t bill_poll_flag[MAX_NO_OF_METER_PER_PORT];

uint8_t g_met_status_flag[MAX_NO_OF_METER_PER_PORT];
uint8_t np_polled_comp[MAX_NO_OF_METER_PER_PORT];

// rithika 09Dec2025
// uint8_t init_ls_polled_comp[MAX_NO_OF_METER_PER_PORT] = {0};
uint8_t init_mn_polled_comp[MAX_NO_OF_METER_PER_PORT] = {0};
uint8_t init_billing_polled_comp[MAX_NO_OF_METER_PER_PORT] = {0};
uint8_t init_event_polled_comp[MAX_NO_OF_METER_PER_PORT][MAX_EVENTS] = {0};

// rithika 20nov2025
int last_inst_poll_min[MAX_NO_OF_METER_PER_PORT];

typedef struct
{
    bool get_nameplate;
    bool inst_data;
    bool ls_data;
    bool mn_data;
    bool bill_data;
    bool event_data[MAX_EVENTS];
    char ls_date[MAX_STRING_LENGTH];
    int num_blocks;
    char mn_date[MAX_STRING_LENGTH];
    int num_days;
    char bill_month[MAX_STRING_LENGTH];
    float current;
    int event_counts[MAX_EVENTS];
} strategy_poll_flags_t;

strategy_poll_flags_t periodic_poll_flags;
strategy_poll_flags_t storage_poll_flags;

Date g_last_met_on_time_info[MAX_NO_OF_METER_PER_PORT];
extern Date meter_date_time[MAX_NO_OF_METER_PER_PORT];

time_t last_inst_ret[MAX_NO_OF_METER_PER_PORT] = {0};
time_t last_ls_ret[MAX_NO_OF_METER_PER_PORT] = {0};
time_t last_event_ret[MAX_NO_OF_METER_PER_PORT] = {0};
time_t last_mn_ret[MAX_NO_OF_METER_PER_PORT] = {0};
time_t last_bill_ret[MAX_NO_OF_METER_PER_PORT] = {0};

char g_old_ls_date_time[11];

bool old_ls_flag = 0;

extern meter_metrics_t inst_meter_metrics;
extern meter_metrics_t bill_meter_metrics;
extern meter_metrics_t ls_meter_metrics;
extern meter_metrics_t mn_meter_metrics;
extern meter_metrics_t event_meter_metrics;
extern meter_metrics_t np_meter_metrics;

extern time_t date_to_time_t(Date date);
extern int days_in_month(int month, int year);

// rithika 06oct
int inst_data_read[MAX_NO_OF_METER_PER_PORT];
int ls_data_read_day[MAX_NO_OF_METER_PER_PORT];
int ls_data_read_block[MAX_NO_OF_METER_PER_PORT];
int mn_data_read[MAX_NO_OF_METER_PER_PORT];
int bill_data_read[MAX_NO_OF_METER_PER_PORT];
int event_data_read[MAX_NO_OF_METER_PER_PORT];

int inst_obis_read_enb[MAX_NO_OF_METER_PER_PORT]; // rithika16oct
// int ls_day_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
int ls_block_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
int mn_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
int bill_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
int event_obis_read_enb[MAX_NO_OF_METER_PER_PORT][MAX_EVENTS];

// rithika 09Dec2025
int present_ls_flag = 0;

// rithika 10Dec2025
extern double meter_drift_sec[MAX_NO_OF_METER_PER_PORT];
//================================================================================

#define MAX_DAYS 10
#define MAX_TIME_ZONES 10
#define MAX_SEASONS 5
#define MAX_WEEKS 5

typedef struct
{
    char day_start_time[32];
    char script_table[64];
    int script_selector;
} time_zones;

typedef struct
{
    int day_id;
    int max_time_zones;
    time_zones timezones[MAX_TIME_ZONES];
} day;

typedef struct
{
    char week_name[32];
    unsigned char weekday[7];
} week;

typedef struct
{
    char season_name[32];
    char start_time[64];
    char week_name[32];
} season;

typedef struct
{
    char calendar_name[32];
    int num_seasons;
    season season_profile[MAX_SEASONS];
    int num_weeks;
    week week_profile[MAX_WEEKS];
    int num_days;
    day day_profile[MAX_DAYS];
} activity_calendar;

// rithika 13nov2025
extern char dev_model[64];
//==================================================================================

/**
 * @brief Validates a JSON object against expected structures and values.
 *
 * This function checks if the root JSON object contains valid keys and values,
 * ensuring that certain keys meet specific criteria.
 *
 * @param root Pointer to the cJSON object representing the JSON data.
 * @return Returns 1 if the JSON is valid, 0 otherwise.
 */
int validate_json(const cJSON *root)
{

    static char fun_name[] = "validate_json()";

    if (!cJSON_IsObject(root))
    {
        dbg_log(FATAL, "[%-25s] :Root is not an object\n", fun_name);
        return -1;
    }

    cJSON *item = NULL;
    cJSON_ArrayForEach(item, root)
    {
        if (!cJSON_IsString(item))
        {
            dbg_log(SEVERE, "[%-25s] :Value for key %s is not a string\n", fun_name, item->string);
            return -1;
        }

        const char *value = item->valuestring;
        char *str_value = strdup(value);
        char *token = strtok(str_value, ":");

        if (strcmp(token, "inst_data") == 0)
        {
            // inst doesn't require additional parameters
        }
        else if (strcmp(token, "get_nameplate") == 0)
        {
            // get_nameplate doesn't require additional parameters
        }
        else if (strcmp(token, "ls_day") == 0 || strcmp(token, "ls_block") == 0)
        {
            token = strtok(NULL, ":");
            if (!token)
            {
                dbg_log(SEVERE, "[%-25s] :Missing date for %s\n", fun_name, value);
                free(str_value);
                return -1;
            }
            token = strtok(NULL, ":");

            if (!token)
            {
                dbg_log(SEVERE, "[%-25s] :Invalid or missing number for %s\n", fun_name, value);
                free(str_value);
                return -1;
            }
        }
        else if (strncmp(token, "event", 5) == 0)
        {
            int event_type = atoi(&token[5]);
            if (event_type < 0 || event_type > 6)
            {
                dbg_log(SEVERE, "[%-25s] :Invalid event type: %d\n", fun_name, event_type);
                free(str_value);
                return -1;
            }
            token = strtok(NULL, ":");

            if (!token)
            {
                dbg_log(SEVERE, "[%-25s] :Invalid or missing number of events\n", fun_name);
                free(str_value);
                return -1;
            }
        }
        else if (strcmp(token, "mn_data") == 0 || strcmp(token, "bill_data") == 0)
        {
            token = strtok(NULL, ":");

            if (!token)
            {
                dbg_log(SEVERE, "[%-25s] :Invalid or missing number for %s\n", fun_name, value);
                free(str_value);
                return -1;
            }
        }
        else if (strcmp(token, "install_data") == 0)
        {
            // inst doesn't require additional parameters
        }

        else
        {
            dbg_log(SEVERE, "[%-25s] :Unknown function type: %s\n", fun_name, token);
            free(str_value);
            return -1;
        }

        free(str_value);
    }

    return 1;
}

/**
 * @brief Parses a JSON string and creates an array of polling functions based on its contents.
 *
 * This function processes a JSON string to set up a list of polling functions
 * according to the specified commands within the JSON. It validates the JSON structure
 * and initializes function pointers accordingly.
 *
 * @param json_string The input JSON string containing commands for polling functions.
 * @return Returns 0 on success, or -1 on failure.
 */
int create_poll_functions(const char *json_string)
{

    static char fun_name[] = "create_poll_functions()";

    cJSON *root = cJSON_Parse(json_string);
    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            dbg_log(FATAL, "[%-25s] :JSON parsing error before: %s\n", fun_name, error_ptr);
        }
        return -1;
    }

    if (!validate_json(root))
    {
        dbg_log(FATAL, "[%-25s] :JSON validation failed.\n", fun_name);
        cJSON_Delete(root);
        return -1;
    }

    num_poll_functions = cJSON_GetArraySize(root);
    dbg_log(INFORM, "[%-25s] :Number of polling function : %d\n", fun_name, num_poll_functions);

    poll_functions = calloc(num_poll_functions, sizeof(PollFunction));

    cJSON *item = NULL;
    int index = 0;

    cJSON_ArrayForEach(item, root)
    {
        char *str_value = strdup(item->valuestring);

        char *token = strtok(str_value, ":");

        if (strcmp(token, "inst_data") == 0)
        {
            strcpy(poll_functions[index].type, "inst_data");
            poll_functions[index].func.inst = get_inst_data;
        }
        else if (strcmp(token, "get_nameplate") == 0)
        {
            strcpy(poll_functions[index].type, "get_nameplate");
            poll_functions[index].func.get_name_plate = get_name_plate;
        }
        else if (strncmp(token, "ls_day", 6) == 0)
        {
            strcpy(poll_functions[index].type, "ls_day");
            poll_functions[index].func.ls_day = get_ls_data_day;
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].date, token);
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strncmp(token, "ls_block", 8) == 0)
        {
            strcpy(poll_functions[index].type, "ls_block");
            poll_functions[index].func.ls_block = get_ls_data_block;
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].date, token);
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strncmp(token, "event", 5) == 0)
        {
            strcpy(poll_functions[index].type, "event");
            poll_functions[index].func.event = get_event_data;
            strcpy(poll_functions[index].event_type, token);
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strncmp(token, "mn_data", 7) == 0)
        {
            strcpy(poll_functions[index].type, "mn_data");
            poll_functions[index].func.midnight = get_midnight_data;
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strncmp(token, "bill_data", 9) == 0)
        {
            strcpy(poll_functions[index].type, "bill_data");
            poll_functions[index].func.billing = get_billing_data;
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }

        free(str_value);
        index++;
    }

    cJSON_Delete(root);
    return 0;
}

/**
 * @brief Parses a JSON string to set periodic polling flags and parameters.
 *
 * This function processes a JSON string to enable specific periodic polling options
 * based on the commands found in the JSON. It updates a structure that holds flags
 * and parameters for each type of data to be polled.
 *
 * @param json_string The input JSON string containing periodic polling commands.
 * @return Returns 0 on success.
 */
int create_periodic_poll_functions(const char *json_string)
{
    static char fun_name[] = "create_periodic_poll_functions()";

    char *json_copy = strdup(json_string);
    char *token = strtok(json_copy, "{}:,\n");

    dbg_log(INFORM, "[%-25s] :creating periodic_poll_functions\n", fun_name);

    while (token != NULL)
    {
        if (strcmp(token, "get_nameplate") == 0)
        {
            periodic_poll_flags.get_nameplate = true;
        }
        else if (strcmp(token, "inst_data") == 0)
        {
            periodic_poll_flags.inst_data = true;
        }
        else if (strcmp(token, "ls_data") == 0)
        {
            periodic_poll_flags.ls_data = true;
            token = strtok(NULL, "{}:,\n");
            if (token)
                strncpy(periodic_poll_flags.ls_date, token, MAX_STRING_LENGTH - 1);
            token = strtok(NULL, "{}:,\n");
            if (token)
                periodic_poll_flags.num_blocks = atoi(token);
        }
        else if (strcmp(token, "mn_data") == 0)
        {
            periodic_poll_flags.mn_data = true;
            token = strtok(NULL, "{}:,\n");
            if (token)
                strncpy(periodic_poll_flags.mn_date, token, MAX_STRING_LENGTH - 1);
            token = strtok(NULL, "{}:,\n");
            if (token)
                periodic_poll_flags.num_days = atoi(token);
        }
        else if (strcmp(token, "bill_data") == 0)
        {
            periodic_poll_flags.bill_data = true;
            token = strtok(NULL, "{}:,\n");
            if (token)
                strncpy(periodic_poll_flags.bill_month, token, MAX_STRING_LENGTH - 1);
            token = strtok(NULL, "{}:,\n");
            if (token)
                periodic_poll_flags.current = atof(token);
        }
        else if (strncmp(token, "event_data", 10) == 0)
        {
            token = strtok(NULL, "{}:,\n");
            if (token)
            {
                int category = token[3] - '1';
                if (category >= 0 && category < MAX_EVENTS)
                {
                    periodic_poll_flags.event_data[category] = true;
                    token = strtok(NULL, "{}:,\n");
                    if (token)
                        periodic_poll_flags.event_counts[category] = atoi(token);
                }
            }
        }
        token = strtok(NULL, "{}:,\n");
    }

    free(json_copy);
    return 0;
}

/**
 * @brief Parses a JSON string to create polling functions for old load survey data.
 *
 * This function processes a JSON string to populate a structure of polling functions,
 * specifically for historical load survey data. It also handles other types of polling
 * requests like instantaneous data, nameplate information, and billing data.
 *
 * @param json_string The input JSON string containing polling commands.
 * @return Returns 1 on success, or 0 if there was an error.
 */
int create_old_ls_poll_functions(const char *json_string)
{

    static char fun_name[] = "create_old_ls_poll_functions()";

    cJSON *root = cJSON_Parse(json_string);
    if (!root)
    {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL)
        {
            dbg_log(FATAL, "[%-25s] :JSON parsing error before: %s\n", fun_name, error_ptr);
        }
        return -1;
    }

    if (!validate_json(root))
    {
        dbg_log(FATAL, "[%-25s] :JSON validation failed.\n", fun_name);
        cJSON_Delete(root);
        return -1;
    }

    num_poll_functions = cJSON_GetArraySize(root);
    dbg_log(INFORM, "[%-25s] :Number of polling function : %d\n", fun_name, num_poll_functions);

    poll_functions = calloc(num_poll_functions, sizeof(PollFunction));

    cJSON *item = NULL;
    int index = 0;

    cJSON_ArrayForEach(item, root)
    {

        char *str_value = strdup(item->valuestring);
        char *token = strtok(str_value, ":");

        if (strcmp(token, "inst_data") == 0)
        {
            strcpy(poll_functions[index].type, "inst_data");
            poll_functions[index].func.inst = get_inst_data;
        }
        else if (strcmp(token, "get_nameplate") == 0)
        {
            strcpy(poll_functions[index].type, "get_nameplate");
            poll_functions[index].func.get_name_plate = get_name_plate;
        }
        else if (strcmp(token, "ls_day") == 0)
        {
            strcpy(poll_functions[index].type, "ls_day");
            poll_functions[index].func.ls_day = get_ls_data_day;
            strcpy(poll_functions[index].date, g_old_ls_date_time);
            strcpy(poll_functions[index].param, "1");
        }
        else if (strcmp(token, "ls_block") == 0)
        {
            strcpy(poll_functions[index].type, "ls_block");
            poll_functions[index].func.ls_block = get_ls_data_block;
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].date, token);
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strncmp(token, "event", 5) == 0)
        {
            strcpy(poll_functions[index].type, "event");
            poll_functions[index].func.event = get_event_data;
            strcpy(poll_functions[index].event_type, token);
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strcmp(token, "mn_data") == 0)
        {
            strcpy(poll_functions[index].type, "mn_data");
            poll_functions[index].func.midnight = get_midnight_data;
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strcmp(token, "bill_data") == 0)
        {
            strcpy(poll_functions[index].type, "bill_data");
            poll_functions[index].func.billing = get_billing_data;
            token = strtok(NULL, ":");
            strcpy(poll_functions[index].param, token);
        }
        else if (strcmp(token, "install_data") == 0)
        {
            strcpy(poll_functions[index].type, "install_data");
            poll_functions[index].func.inst_all_data = get_all_meter_inst_data;
        }

        free(str_value);
        index++;
    }

    cJSON_Delete(root);
    return 0;
}

/**
 * @brief Executes polling functions based on the specified type.
 *
 * This function determines the appropriate polling function to call
 * based on the provided polling function type and executes it.
 * It also manages the state of the polling process, indicating whether
 * the execution succeeded or failed.
 *
 * @param midx Index or identifier for the meter or device being polled.
 * @param polling_func_type The type of polling function to execute (e.g., "inst_data").
 * @param polling_type The general type of polling operation being performed.
 */
void execute_poll_functions(int midx, char *polling_func_type, char *polling_type, int poll_idx)
{

    static char fun_name[] = "execute_poll_functions()";

    int ret = 0;
    char *temp_buf[255];

    memset(temp_buf, 0, sizeof(temp_buf));
    sprintf(temp_buf, "%s_%s", polling_type, "STARTED");
    add_process_state(midx, temp_buf, polling_func_type);

    dbg_log(INFORM, "[Meter_%d]:[%-25s]: %s going to execute for polling type %s\n", midx, fun_name, polling_func_type, polling_type);

    if (strcmp(polling_func_type, "inst_data") == 0)
    {
        ret = poll_functions[poll_idx].func.inst(midx);
    }
    else if (strcmp(polling_func_type, "get_nameplate") == 0)
    {
        ret = poll_functions[poll_idx].func.get_name_plate(midx);
    }
    else if (strcmp(polling_func_type, "ls_day") == 0)
    {
        ret = poll_functions[poll_idx].func.ls_day(midx, poll_functions[poll_idx].date, poll_functions[poll_idx].param);
    }
    else if (strcmp(polling_func_type, "ls_block") == 0)
    {
        ret = poll_functions[poll_idx].func.ls_block(midx, poll_functions[poll_idx].date, poll_functions[poll_idx].param);
    }
    else if (strncmp(polling_func_type, "event", 5) == 0)
    {
        // print_colored_message(YELLOW, 1, "!!!!!!!!!!!!!!!!!!!!!!!!!!event_type %s \n", poll_functions[poll_idx].event_type);
        ret = poll_functions[poll_idx].func.event(midx, poll_functions[poll_idx].event_type, poll_functions[poll_idx].param);

        // rithika 09Dec2025
        if (ret != 0 && strcmp(polling_type, "INIT_POLL") == 0)
        {

            if (strcmp(poll_functions[poll_idx].event_type, "event_data_cat1") == 0)
            {
                init_event_polled_comp[midx][0] = 1;
            }
            else if (strcmp(poll_functions[poll_idx].event_type, "event_data_cat2") == 0)
            {
                init_event_polled_comp[midx][1] = 1;
            }
            else if (strcmp(poll_functions[poll_idx].event_type, "event_data_cat3") == 0)
            {
                init_event_polled_comp[midx][POWER_EVENT_IDX] = 1;
            }
            else if (strcmp(poll_functions[poll_idx].event_type, "event_data_cat4") == 0)
            {
                init_event_polled_comp[midx][3] = 1;
            }
            else if (strcmp(poll_functions[poll_idx].event_type, "event_data_cat5") == 0)
            {
                init_event_polled_comp[midx][4] = 1;
            }
            else if (strcmp(poll_functions[poll_idx].event_type, "event_data_cat6") == 0)
            {
                init_event_polled_comp[midx][5] = 1;
            }
        }
    }
    else if (strcmp(polling_func_type, "mn_data") == 0)
    {
        ret = poll_functions[poll_idx].func.midnight(midx, poll_functions[poll_idx].param);
        // rithika 09Dec2025
        if (ret != 0 && strcmp(polling_type, "INIT_POLL") == 0)
        {
            init_mn_polled_comp[midx] = 1;
        }
    }
    else if (strcmp(polling_func_type, "bill_data") == 0)
    {
        ret = poll_functions[poll_idx].func.billing(midx, poll_functions[poll_idx].param);
        // rithika 09Dec2025
        if (ret != 0 && strcmp(polling_type, "INIT_POLL") == 0)
        {
            init_billing_polled_comp[midx] = 1;
        }
    }
    // rithika 13oct2025
    else if (strcmp(polling_func_type, "install_data") == 0)
    {
        ret = poll_functions[poll_idx].func.inst_all_data();
    }

    if (ret != 0)
    {
        memset(temp_buf, 0, sizeof(temp_buf));
        sprintf(temp_buf, "%s_%s", polling_type, "FAILED");
        add_process_state(midx, temp_buf, polling_func_type);

        snrm_aarq_failed_cnt("[%s] MANF : %s.polling failed.ret : %d\n", time_format_conversion(time(NULL)), plcc_modem_info.plcc_meter_map[midx].meter_manf_name, ret);
        dbg_log(INFORM, "[Meter_%d]:[%-25s]: %s polling failed.\n", midx, fun_name, polling_func_type);
    }
    else
    {
        memset(temp_buf, 0, sizeof(temp_buf));
        sprintf(temp_buf, "%s_%s", polling_type, "SUCCEEDED");
        add_process_state(midx, temp_buf, polling_func_type);

        dbg_log(INFORM, " [Meter_%d]:[%-25s]: %s polling succeeded.\n", midx, fun_name, polling_func_type);

        // rithika 26Nov2025
        if (strcmp(polling_func_type, "get_nameplate") == 0)
        {
            np_polled_comp[midx] = 1;
        }
    }
}

/**
 * @brief Cleans up the resources allocated for polling functions.
 *
 * This function frees the memory allocated for the polling function array
 * and resets relevant variables to their default states. This is crucial
 * for preventing memory leaks in the application.
 */
void cleanup_poll_functions()
{
    static char fun_name[] = "cleanup_poll_functions()";

    free(poll_functions);
    poll_functions = NULL;
    num_poll_functions = 0;

    dbg_log(INFORM, "[%-25s]: poll function cleanup completed.\n", fun_name);
}

int get_all_meter_inst_data()
{
    static char fun_name[] = "get_all_meter_inst_data()";
    int meter_idx;

    // Set the number of meters based on the meter type
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        if (plcc_modem_info.no_of_nodes <= MAX_NO_OF_METER_PER_PORT)
        {
            no_of_meter = plcc_modem_info.no_of_nodes;
        }
        else
        {
            no_of_meter = MAX_NO_OF_METER_PER_PORT;
        }
    }
    else
    {
        no_of_meter = ser_port_data[g_pidx].num_dev;
    }

    for (meter_idx = 0; meter_idx < no_of_meter; meter_idx++)
    {

        if ((ser_port_data[g_pidx].meter_cfg[meter_idx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
        {
            // rithika added 25/07
            curr_nc_idx = meter_idx;
            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
            {
                if (plcc_modem_info.plcc_meter_map[meter_idx].rs_status == 0)
                {
                    dbg_log(WARNING, "[Meter_%d]:[%-25s]: RS %d NOT COMMN\n", meter_idx, fun_name, meter_idx);
                    if (strcmp(plcc_modem_info.plcc_meter_map[meter_idx].meter_ser_num, "NC") == 0)
                    {
                        strcpy(plcc_modem_info.plcc_meter_map[meter_idx].meter_ser_num, "NC");
                    }
                    meter_commn_status(curr_nc_idx, 0);
                    continue;
                }
            }
            process_od_cmd(); // rithika
            check_od_command_resp();

            // rithika 14Apr2026
            if (RECONNECT)
            {
                iec104_log_sink_poll_network();
            }

            int ret = init_dlms(meter_idx);
            if (ret != DLMS_ERROR_CODE_OK) // 03/09
            {
                dbg_log(WARNING, "[Meter_%d]:[%-25s]: SNRM-AARQ FAILED : ret %d - %s\n", meter_idx, fun_name, ret, (hlp_getErrorMessage(ret)));
                continue;
            }

            dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_inst ...\n", meter_idx, fun_name);
            add_process_state(meter_idx, "PERIODIC", "inst");
            // get_name_plate(midx); // need to check with mam
            get_inst_data(meter_idx);
            // rithika 17oct2025
            dis_connect();
        }
    }

    return 0;
}

/**
 * @brief Initializes data collection from meters.
 *
 * This function sets up the necessary configurations for collecting data
 * from the meters based on the type of communication protocol being used.
 * It establishes connections, initializes the polling functions, and executes
 * the polling process for each meter.
 *
 * @return Returns 0 upon successful initialization.
 */
int init_data_collection()
{

    static char fun_name[] = "init_data_collection()";

    int midx;
    int ret, i;

    // // rithika 15nov2025
    // memset(np_polled_comp, 0, sizeof(np_polled_comp));

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        if (plcc_modem_info.no_of_nodes <= MAX_NO_OF_METER_PER_PORT)
        {
            no_of_meter = plcc_modem_info.no_of_nodes;
        }
        else
        {
            no_of_meter = MAX_NO_OF_METER_PER_PORT;
        }
    }
    else
    {
        no_of_meter = ser_port_data[g_pidx].num_dev;
    }

    dbg_log(INFORM, "[%-25s] : Number of meters : %d\n", fun_name, no_of_meter);

#if 1
    for (midx = 0; midx < no_of_meter; midx++)
    {
        if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
        {
            curr_nc_idx = midx;

            send_hanging_check();

            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
            {
                if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                {
                    dbg_log(WARNING, "[Meter_%d]:[%-25s]: RS %d NOT COMMN\n", midx, fun_name, midx);

                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                    {
                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                    }
                    meter_commn_status(curr_nc_idx, 0);

                    continue;
                }
            }
            if (np_polled_comp[midx] == 0)
            {
                ret = init_dlms(midx);
                if (ret != DLMS_ERROR_CODE_OK) // 03/09
                {
                    dbg_log(WARNING, "[Meter_%d]:[%-25s]: SNRM-AARQ FAILED : ret - %s\n", midx, fun_name, (hlp_getErrorMessage(ret)));
                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                    {
                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                    }
                    continue;
                }

                if (get_name_plate(midx) != 0) // 03/09
                {
                    dbg_log(WARNING, "[Meter_%d]:[%-25s] :  Get nameplate failed\n", midx, fun_name);
                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                    {
                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                    }

                    continue;
                }
                // rithika 15nov2025
                else
                {
                    np_polled_comp[midx] = 1;
                }

                // rithika 17oct2025
                dis_connect();
            }
        }
    }

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        update_serial_status_in_redis("1");
    }

    // rithika 15nov2025
    //  np_polled_comp = 1;
    for (midx = 0; midx < no_of_meter; midx++)
    {
        dbg_log(INFORM, "[%-25s]: Name plate polling completion status for meter id %d : %d\n", fun_name, midx, np_polled_comp[midx]);
    }
#endif

#if 1
    if (create_poll_functions(strategy_cfg.initial_poll_starts) != 0)
    {
        dbg_log(FATAL, "[%-25s]: Failed to create poll functions due to invalid JSON\n", fun_name);
    }
    int num_poll_func = num_poll_functions;

    for (i = 0; i < num_poll_func; i++)
    {

        for (midx = 0; midx < no_of_meter; midx++)
        {

            if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
            {
                send_hanging_check();

                process_od_cmd();
                check_od_command_resp();

                // rithika 14Apr2026
                if (RECONNECT)
                {
                    iec104_log_sink_poll_network();
                }
                curr_nc_idx = midx;

                if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                {
                    if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                    {
                        dbg_log(WARNING, "[Meter_%d]:[%-25s]:  RS %d NOT COMMN\n", midx, fun_name, midx);
                        if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                        {
                            strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                        }
                        meter_commn_status(curr_nc_idx, 0);
                        continue;
                    }
                }

                // if (strcmp(poll_functions[i].type, "get_nameplate") == 0)
                // {
                // rithika 15nov2025
                if (strcmp(poll_functions[i].type, "get_nameplate") == 0 && np_polled_comp[midx] == 1)
                {

                    dbg_log(INFORM, "[%-25s]: skipping nameplate because it's ret previously.\n", fun_name);

                    continue;
                }

                ret = init_dlms(midx);         // rithika
                if (ret != DLMS_ERROR_CODE_OK) // 03/09
                {
                    dbg_log(WARNING, "[Meter_%d]:[%-25s]: SNRM-AARQ FAILED : ret - %s\n", midx, fun_name, (hlp_getErrorMessage(ret)));
                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                    {
                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                    }

                    // rithika 3feb2026
                    //  dbg_log(INFORM, "[%-25s]: ********poll_functions[i].type %s\n", fun_name, poll_functions[i].type);

                    if (strcmp(poll_functions[i].type, "mn_data") == 0)
                    {
                        init_mn_polled_comp[midx] = 1;
                    }

                    if (strcmp(poll_functions[i].type, "bill_data") == 0)
                    {
                        init_billing_polled_comp[midx] = 1;
                    }

                    if (strcmp(poll_functions[i].event_type, "event_data_cat1") == 0)
                    {
                        init_event_polled_comp[midx][0] = 1;
                    }
                    else if (strcmp(poll_functions[i].event_type, "event_data_cat2") == 0)
                    {
                        init_event_polled_comp[midx][1] = 1;
                    }
                    else if (strcmp(poll_functions[i].event_type, "event_data_cat3") == 0)
                    {
                        init_event_polled_comp[midx][POWER_EVENT_IDX] = 1;
                    }
                    else if (strcmp(poll_functions[i].event_type, "event_data_cat4") == 0)
                    {
                        init_event_polled_comp[midx][3] = 1;
                    }
                    else if (strcmp(poll_functions[i].event_type, "event_data_cat5") == 0)
                    {
                        init_event_polled_comp[midx][4] = 1;
                    }
                    else if (strcmp(poll_functions[i].event_type, "event_data_cat6") == 0)
                    {
                        init_event_polled_comp[midx][5] = 1;
                    }

                    dbg_log(INFORM, "[%-25s]: ********init_mn_polled_comp[midx] %d init_billing_polled_comp[midx] %d init_event_polled_comp[midx][2] %d init_event_polled_comp[midx][5] %d\n", fun_name, init_mn_polled_comp[midx], init_billing_polled_comp[midx], init_event_polled_comp[midx][POWER_EVENT_IDX], init_event_polled_comp[midx][5]);

                    continue;
                }

                execute_poll_functions(midx, poll_functions[i].type, "INIT_POLL", i);

                // rithika 06oct
                //  if (strcmp(poll_functions[i].type, "inst_data") == 0)
                //  {
                //  inst_data_read[midx] = 1;
                //  }

                if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0) && strcmp(poll_functions[i].type, "inst_data") != 0)
                {
                    get_all_meter_inst_data(); // rithika 04/09
                }
                // rithika 17oct2025
                dis_connect();
            }
        }
    }

    cleanup_poll_functions();
#endif

    return 0;
}

/**
 * @brief Retrieves old data from meters over a specified number of days.
 *
 * This function iterates through a specified number of previous days and
 * retrieves old data for each meter connected, using previously defined
 * polling functions. It adjusts the date for each iteration to collect data
 * for the desired historical periods.
 *
 * @return Returns 0 upon successful completion of the data retrieval process.
 */

#if 0
int get_old_data()
{
    static char fun_name[] = "get_old_data()";
    int midx, idx, ret = 0, i;

    time_t time_of_day = 0, local_tm = 0, next_time_day = 0, curr_time = 0, st_time_in_sec = 0;
    time_t now;
    struct tm *local_t;
    int temp_day = 0;
    int date_cnt = 0;

    time(&now);
    local_t = localtime(&now);

    local_t->tm_mday--;
    mktime(local_t);
    temp_day = local_t->tm_mday;

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        if (plcc_modem_info.no_of_nodes <= MAX_NO_OF_METER_PER_PORT)
        {
            no_of_meter = plcc_modem_info.no_of_nodes;
        }
        else
        {
            no_of_meter = MAX_NO_OF_METER_PER_PORT;
        }
    }
    else
    {
        no_of_meter = ser_port_data[g_pidx].num_dev;
    }
    
    for (i = 0; i < num_poll_functions; i++)
    {
        PollFunction *pf = &poll_functions[i];
        for (idx = 0; idx < poll_cfg.num_old_days; idx++)
        {
            memset(g_old_ls_date_time, 0, sizeof(g_old_ls_date_time));
            strftime(g_old_ls_date_time, sizeof(g_old_ls_date_time), "%d-%m-%Y", local_t);

            print_colored_message(YELLOW,1,"------------------ old data : %s -----------\n",g_old_ls_date_time);

            for (midx = 0; midx < no_of_meter; midx++)
            {

                process_od_cmd();
                curr_nc_idx = midx;

                ret = init_dlms(midx);
                if (ret < 0)
                {
                    dbg_log(WARNING, "[%-25s] :MIDX : %d SNRM-AARQ FAILED : ret - %s\n", fun_name, midx, (hlp_getErrorMessage(ret)));
                    continue;
                }

                if (create_old_ls_poll_functions(strategy_cfg.store_poll_starts))
                {
                    // execute_poll_functions(midx, pf->type, "OLD_LS_DATA");
                    cleanup_poll_functions();
                }
                else
                {
                    fprintf(stderr, "Failed to create poll functions due to invalid JSON\n");
                }
            }
            temp_day--;
            local_t->tm_mday = temp_day;
            mktime(local_t); // Normalize date (handle month/year changes)
        }
    }

    return 0;
}
#endif

#if 0
int get_old_data()
{
    static char fun_name[] = "get_old_data()";
    int midx, idx, ret = 0, i;

    time_t time_of_day = 0, local_tm = 0, next_time_day = 0, curr_time = 0, st_time_in_sec = 0;
    time_t now;
    struct tm *local_t;
    int temp_day = 0;
    int date_cnt = 0;

    time(&now);
    local_t = localtime(&now);

    local_t->tm_mday--;
    mktime(local_t);
    temp_day = local_t->tm_mday;

    print_colored_message(MAGENTA, 1, "------------------ temp_day : %d -----------\n", temp_day);

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        if (plcc_modem_info.no_of_nodes <= MAX_NO_OF_METER_PER_PORT)
        {
            no_of_meter = plcc_modem_info.no_of_nodes;
        }
        else
        {
            no_of_meter = MAX_NO_OF_METER_PER_PORT;
        }
    }
    else
    {
        no_of_meter = ser_port_data[g_pidx].num_dev;
    }

    dbg_log(INFORM, "[%-25s] : Number of meters : %d\n", fun_name, no_of_meter);

    dbg_log(INFORM, "[%-25s] : Number of old_days : %d\n", fun_name, poll_cfg.num_old_days);

    for (idx = 0; idx < poll_cfg.num_old_days; idx++)
    {
        memset(g_old_ls_date_time, 0, sizeof(g_old_ls_date_time));
        strftime(g_old_ls_date_time, sizeof(g_old_ls_date_time), "%d-%m-%Y", local_t);

        print_colored_message(YELLOW, 1, "------------------ old data : %s -----------\n", g_old_ls_date_time);

#if 1

        if (create_old_ls_poll_functions(strategy_cfg.store_poll_starts) != 0)
        {
            dbg_log(FATAL, "[%-25s] : Failed to create poll functions due to invalid JSON\n", fun_name);
        }

        int num_poll_func = num_poll_functions;

        for (midx = 0; midx < no_of_meter; midx++)
        {
            send_hanging_check();

            process_od_cmd();

            curr_nc_idx = midx;

            for (i = 0; i < num_poll_func; i++)
            {
                process_od_cmd();
                old_ls_flag = 1;
                dbg_log(INFORM, "[%-25s] :  [Meter_%d]  : LS date : %s\n", fun_name, midx, g_old_ls_date_time);

                if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                {
                    if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                    {
                        dbg_log(WARNING, "[%-25s] : [Meter_%d]  : RS %d NOT COMMN\n", fun_name, midx, midx);
                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                        meter_commn_status(curr_nc_idx, 0);
                        continue;
                    }
                }

                ret = init_dlms(midx);
                if (ret < 0)
                {
                    dbg_log(WARNING, "[%-25s] : [Meter_%d]  : SNRM-AARQ FAILED : ret - %s\n", fun_name, midx, (hlp_getErrorMessage(ret)));
                    continue;
                }

                execute_poll_functions(midx, poll_functions[i].type, "OLD_LS_DATA", i);
            }
        }

        cleanup_poll_functions();
#endif

        printf("===============================================\n");
        local_t->tm_mday--;

        print_colored_message(YELLOW, 1, "------------------ local_t : %d -----------\n", local_t->tm_mday);

        if (local_t->tm_mday <= 0)
        {
            local_t->tm_mon--;
            print_colored_message(YELLOW, 1, "------------------ local_t mon: %d -----------\n", local_t->tm_mon);

            if (local_t->tm_mon < 0)
            {
                local_t->tm_mon = 11;
                local_t->tm_year--;
            }
            int days_in_previous_month = days_in_month(local_t->tm_mon, local_t->tm_year);
            local_t->tm_mday = days_in_previous_month;
        }
    }

    return 0;
}
#endif

int get_old_data()
{
    static char fun_name[] = "get_old_data()";
    int midx, idx, ret = 0, i;

    time_t time_of_day = 0, local_tm = 0, next_time_day = 0, curr_time = 0, st_time_in_sec = 0;
    time_t now;
    struct tm *local_t;
    struct tm local_copy; // Local copy to preserve date while iterating
    int temp_day = 0;
    int date_cnt = 0;

    time(&now);
    local_t = localtime(&now);

    // Create a copy of local_t to avoid modifying the original during the process
    local_copy = *local_t;

    // Reduce the day by 1 (working on the copy)
    local_copy.tm_mday--;
    mktime(&local_copy);
    temp_day = local_copy.tm_mday;

    print_colored_message(MAGENTA, 1, "------------------ temp_day : %d -----------\n", temp_day);

    // Set the number of meters based on the meter type
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        if (plcc_modem_info.no_of_nodes <= MAX_NO_OF_METER_PER_PORT)
        {
            no_of_meter = plcc_modem_info.no_of_nodes;
        }
        else
        {
            no_of_meter = MAX_NO_OF_METER_PER_PORT;
        }
    }
    else
    {
        no_of_meter = ser_port_data[g_pidx].num_dev;
    }

    dbg_log(INFORM, "[%-25s] : Number of meters : %d\n", fun_name, no_of_meter);

    dbg_log(INFORM, "[%-25s] : Number of old_days : %d\n", fun_name, poll_cfg.num_old_days);

    // Loop through old days and poll the data for each
    for (idx = 0; idx < poll_cfg.num_old_days; idx++)
    {

        memset(g_old_ls_date_time, 0, sizeof(g_old_ls_date_time));

        // Format the date string for the current day (using local_copy)
        strftime(g_old_ls_date_time, sizeof(g_old_ls_date_time), "%d-%m-%Y", &local_copy);

        print_colored_message(YELLOW, 1, "------------------ old data : %s -----------\n", g_old_ls_date_time);

#if 1
        if (create_old_ls_poll_functions(strategy_cfg.store_poll_starts) != 0)
        {
            dbg_log(FATAL, "[%-25s] : Failed to create poll functions due to invalid JSON\n", fun_name);
        }

        int num_poll_func = num_poll_functions;

        // Loop through meters
        for (midx = 0; midx < no_of_meter; midx++)
        {
            dbg_log(INFORM, "[Meter_%d]:[%-25s]: LS date : %s\n", midx, fun_name, g_old_ls_date_time);

            if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
            {
                send_hanging_check();
                process_od_cmd();
                check_od_command_resp();

                // rithika 14Apr2026
                if (RECONNECT)
                {
                    iec104_log_sink_poll_network();
                }
                // Loop through poll functions
                int event_group_active = 0;

                for (i = 0; i < num_poll_func; i++)
                {
                    process_od_cmd();

                    // rithika 14Apr2026
                    if (RECONNECT)
                    {
                        iec104_log_sink_poll_network();
                    }
                    curr_nc_idx = midx;

                    check_od_command_resp();

                    if (strcmp(poll_functions[i].type, "ls_day") == 0)
                    {
                        old_ls_flag = 1;
                    }

                    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                    {
                        if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                        {
                            dbg_log(WARNING, "[Meter_%d]:[%-25s]:  RS %d NOT COMMN\n", midx, fun_name, midx);
                            if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                            {
                                strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                            }
                            meter_commn_status(curr_nc_idx, 0);
                            continue;
                        }
                    }

                    // ret = init_dlms(midx);
                    // if (ret != DLMS_ERROR_CODE_OK) // 03/09
                    // {
                    //     dbg_log(WARNING, "[%-25s] : [Meter_%d]  : SNRM-AARQ FAILED : ret - %s\n", fun_name, midx, (hlp_getErrorMessage(ret)));
                    //     if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0))
                    //     {
                    //         get_all_meter_inst_data(); // rithika 04/09
                    //     }

                    //     continue;
                    // }

                    int skip_init_dlms = 0;

                    // Check the current poll function type
                    char *curr_type = poll_functions[i].type;

                    // ---- LS_DAY → LS_BLOCK logic ----
                    // rithika 17oct
                    // if (strncmp(curr_type, "ls_block", 8) == 0 && i > 0)
                    // {
                    //     if (strncmp(poll_functions[i - 1].type, "ls_day", 6) == 0)
                    //     {
                    //         skip_init_dlms = 1; // skip init_dlms for ls_block
                    //     }
                    // } // need to uncomment

                    if (strstr(curr_type, "install_data") != NULL)
                    {

                        skip_init_dlms = 1; // skip init_dlms for ls_block
                    }

                    // ---- EVENT_DATA_CAT logic ----
                    //  int event_group_active = 0; // persists across i iterations

                    // printf(" %s : %s _____________\n", curr_type, poll_functions[i].type);

                    // if (strncmp(curr_type, "event_data_cat", 14) == 0)

                    if (strstr(curr_type, "event") != NULL)

                    {

                        if (event_group_active == 0)
                        {
                            // first event category → allow init_dlms

                            event_group_active = 1;
                        }
                        else
                        {

                            // already in event group → skip
                            skip_init_dlms = 1;
                        }
                    }
                    else
                    {
                        // reset if we move out of event_data_cat types
                        event_group_active = 0;
                    }

                    // ---- Do init_dlms only if not skipped ----
                    if (!skip_init_dlms)
                    {
                        ret = init_dlms(midx);
                        if (ret != DLMS_ERROR_CODE_OK)
                        {
                            // rithika 23oct
                            event_group_active = 0; // to handle when snrm failed for event cat 1, then do init dlms for event cat 2

                            dbg_log(WARNING, "[Meter_%d]:[%-25s]:  SNRM-AARQ FAILED : ret - %s\n",
                                    midx, fun_name, (hlp_getErrorMessage(ret)));

                            if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0))
                            {
                                get_all_meter_inst_data();
                            }

                            continue;
                        }
                    }
                    else
                    {
                        dbg_log(INFORM, "[Meter_%d]:[%-25s]: Skipping init_dlms for %s\n",
                                midx, fun_name, curr_type);
                    }

                    // rithika 15nov2025
                    int chk_nameplate = check_name_plate(midx);

                    if (chk_nameplate != 0 && !skip_init_dlms)
                    {
                        dbg_log(WARNING, "[Meter_%d]:[%-25s] : Nameplate is not available in redis, Reading name plate details for %d \n", midx, fun_name, midx);
                        int np_ret = get_name_plate(midx);

                        // rithika 27nov2025
                        if (np_ret == 0)
                        {
                            np_polled_comp[midx] = 1;
                        }
                    }

                    execute_poll_functions(midx, poll_functions[i].type, "OLD_LS_DATA", i);
                    // rithika 29sep
                    old_ls_flag = 0;

                    // rithika 13oct2205
                    //  if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0))
                    //  {
                    //      get_all_meter_inst_data(); // rithika 24/09
                    //  }

                    // rithika 23oct
                    // if ((!skip_init_dlms && (strncmp(curr_type, "ls_day", 6) != 0 ) && (strncmp(curr_type, "event_data_cat1", 15) != 0 )) || (strncmp(curr_type, "ls_block", 8) == 0 ) || (strncmp(curr_type, "event_data_cat6", 15) == 0 ))
                    // {
                    int next_is_event = 0;

                    if (i + 1 < num_poll_func && strstr(poll_functions[i + 1].type, "event") != NULL)
                    {
                        next_is_event = 1;
                    }

                    // Now disconnect only when:
                    // - current type is NOT event-related
                    // - OR this is the LAST event in a series
                    if (!skip_init_dlms && strncmp(curr_type, "ls_day", 6) == 0 && i > 0)
                    {
                        if (strncmp(poll_functions[i + 1].type, "ls_block", 8) != 0)
                        {
                            dis_connect();
                        }
                    }

                    else if ((!skip_init_dlms && strncmp(curr_type, "ls_day", 6) != 0 && strstr(curr_type, "event") == NULL) || strncmp(curr_type, "ls_block", 8) == 0 || (!next_is_event && strstr(curr_type, "event") != NULL))
                    {

                        dis_connect();
                    }

                    // rithika testing 23/10
                    // sleep(20);
                }
            }
        }

        cleanup_poll_functions();
#endif

        // Reduce the day by 1 (preserving changes in local_copy)
        local_copy.tm_mday--;

        dbg_log(INFORM, "[%-25s] : After reduction - local_copy tm_mday: %d\n", fun_name, local_copy.tm_mday);

        // If day becomes 0 or negative, adjust for previous month
        if (local_copy.tm_mday <= 0)
        {
            local_copy.tm_mon--;
            if (local_copy.tm_mon < 0)
            {
                local_copy.tm_mon = 11;
                local_copy.tm_year--;
            }

            dbg_log(INFORM, "[%-25s] : After reduction - local_copy tm_mon: %d local_copy.tm_year : %d\n", fun_name, local_copy.tm_mon, local_copy.tm_year);

            // Get the number of days in the previous month
            int days_in_previous_month = days_in_month(local_copy.tm_mon, local_copy.tm_year);
            local_copy.tm_mday = days_in_previous_month;
        }
    }

    return 0;
}

uint8_t find_missed_ls_data(Date st_date_time, Date end_date_time)
{
    if ((st_date_time.day != 0) && (st_date_time.day != end_date_time.day))
    {
        return 1;
    }

    return RET_OK;
}

int calculate_number_of_blocks(Date start_date, Date end_date, int block_duration)
{

    // Convert Date structures to time_t
    time_t start_time = date_to_time_t(start_date);
    time_t end_time = date_to_time_t(end_date);

    double time_difference = difftime(end_time, start_time);

    if (time_difference < 0)
    {
        fprintf(stderr, "Error: end_time cannot be earlier than start_time\n");
        return -1;
    }

    int num_blocks = (int)(time_difference / (block_duration * 60));

    return num_blocks;
}

/**
 * @brief Executes the idle polling phase for meter data collection.
 *
 * This function continuously polls data from connected meters in an idle state.
 * It checks various conditions to determine when to collect different types of data,
 * ensuring that polling occurs based on configured intervals.
 *
 * @return Returns 0 (though this function runs indefinitely in practice).
 */
int idle_poll_phase()
{
    static char fun_name[] = "idle_poll_phase()";
    int midx, ret, i;
    time_t curr_time;
    struct tm *local_time;
    int event_idx = 0;
    char event_buff[64];

    memset(&periodic_poll_flags, 0, sizeof(strategy_poll_flags_t));

    memset(inst_poll_flag, 1, sizeof(inst_poll_flag));
    memset(ls_poll_flag, 1, sizeof(ls_poll_flag));
    memset(mn_poll_flag, 1, sizeof(mn_poll_flag));
    memset(event_poll_flag, 1, sizeof(event_poll_flag));
    memset(bill_poll_flag, 1, sizeof(bill_poll_flag));
    memset(midnight_polled, 0, sizeof(midnight_polled));

    // rithika 14oct2025
    //  memset(last_inst_ret, 0, sizeof(last_inst_ret));
    //  memset(last_event_ret, 0, sizeof(last_event_ret));
    //  memset(last_ls_ret, 0, sizeof(last_ls_ret));
    //  memset(last_mn_ret, 0, sizeof(last_mn_ret));
    //  memset(last_bill_ret, 0, sizeof(last_bill_ret));

    time_t now = time(NULL);
    for (int i = 0; i < MAX_NO_OF_METER_PER_PORT; i++)
    {
        last_inst_poll_min[i] = -1; // rithika 20nov2025
        last_inst_ret[i] = 0;
        last_event_ret[i] = now;
        last_ls_ret[i] = now;
        last_mn_ret[i] = now;
        last_bill_ret[i] = now;
    }

    int missing_old_data_midx = 0;
    int old_poll_day = 0;
    int first_day = 1;

    // rithika 09Dec2025
    int chk_curr_day_ls_midx = 0;
    // rithika 12Dec2025
    struct tm local_copy; // Local copy to preserve date while iterating

    while (1)
    {

        print_colored_message(YELLOW, 1, "------------------------------------------------------------------------------------------------------------------\n\n\n*************************\n");
        curr_time = time(NULL);

        local_time = localtime(&curr_time);
        if (local_time->tm_hour == data_clean_hour && enbl_data_clean)
        {
            // 3 * port_idx to 3* port_idx + 2

            // rithika 05nov2025
            if (local_time->tm_min >= g_pidx * data_clean_min && local_time->tm_min <= (g_pidx * data_clean_min) + 2)
            {

                dbg_log(INFORM, "[%-25s] : Current hour is %d, calling the data cleanup function...\n", fun_name, data_clean_hour);
                data_cleanup();
                enbl_data_clean = 0; // prevent multiple calls in the same hour
            }
        }

        if (local_time->tm_hour != data_clean_hour)
        {
            enbl_data_clean = 1;
        }

        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            if (plcc_modem_info.no_of_nodes <= MAX_NO_OF_METER_PER_PORT)
            {
                no_of_meter = plcc_modem_info.no_of_nodes;
            }
            else
            {
                no_of_meter = MAX_NO_OF_METER_PER_PORT;
            }
        }
        else
        {
            no_of_meter = ser_port_data[g_pidx].num_dev;
        }

        int num_poll_func = 0;
        // rithika 20/08/2025
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            check_nc_nw(1);
        }
        //////////

        /// not applicable for tcs need to change logic inside it for plcc /////////
        if (strategy_cfg.idle_poll_starts[0] != '\0')
        {
            if (create_poll_functions(strategy_cfg.idle_poll_starts) != 0)
            {
                fprintf(stderr, "Failed to create poll functions due to invalid JSON\n");
            }

            num_poll_func = num_poll_functions;
            print_colored_message(MAGENTA, 1, "------------------ num_poll_functions : %d -----------\n", num_poll_functions);
            send_hanging_check();

            for (i = 0; i < num_poll_func; i++)
            {
                for (midx = 0; midx < no_of_meter; midx++)
                {
                    if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                    {
                        curr_time = time(NULL);

                        local_time = localtime(&curr_time);

                        struct tm midnight = *local_time;
                        struct tm one_am = *local_time;

                        midnight.tm_hour = 0;
                        midnight.tm_min = 0;
                        midnight.tm_sec = 0;

                        one_am.tm_hour = 1;
                        one_am.tm_min = 0;
                        one_am.tm_sec = 0;

                        time_t midnight_time = mktime(&midnight);
                        time_t one_am_time = mktime(&one_am);

                        if (midnight_time == -1 || one_am_time == -1)
                        {
                            printf("Error converting time to time_t\n");
                        }

                        process_od_cmd();
                        check_od_command_resp();

                        // rithika 14Apr2026
                        if (RECONNECT)
                        {
                            iec104_log_sink_poll_network();
                        }
                        curr_nc_idx = midx;

                        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                        {
                            if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : RS %d NOT COMMN\n", midx, fun_name, midx);
                                if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                                {
                                    strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                                }
                                meter_commn_status(curr_nc_idx, 0);
                                continue;
                            }
                        }

                        ret = init_dlms(midx);
                        if (ret != DLMS_ERROR_CODE_OK) // 03/09
                        {
                            dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                            continue;
                        }

                        if (curr_time >= midnight_time && curr_time < one_am_time)
                        {
                            if (!midnight_polled[midx])
                            {
                                dbg_log(INFORM, "[Meter_%d]:[%-25s] :  Mid night polling ...\n", fun_name, midx);

                                add_process_state(midx, "PERIODIC", "midnight");
                                get_midnight_data(midx, "today");

                                midnight_polled[midx] = 1;
                            }
                        }

                        // if (find_missed_ls_data(g_last_met_on_time_info[midx], meter_date_time[midx]))
                        // {
                        //     memset(event_buff, 0, sizeof(event_buff));
                        //     sprintf(event_buff, "meter%d_details", midx);

                        //     block_int = extract_block_int(event_buff);
                        //     dbg_log(INFORM, "[%-25s] : [Meter : %d] block_int : %d\n", fun_name, midx,block_int);

                        //     if (block_int > 0)
                        //     {
                        //         int num_blocks = calculate_number_of_blocks(g_last_met_on_time_info[midx], meter_date_time[midx], block_int);

                        //         if (num_blocks > poll_cfg.num_blocks)
                        //         {
                        //             get_ls_data_block(midx, "all", 0);
                        //         }
                        //     }
                        // }
                        print_colored_message(MAGENTA, 1, "\n\n------------------ before idle execute poll -----------\n");
                        execute_poll_functions(midx, poll_functions[i].type, "IDLE_POLL", i);
                        print_colored_message(MAGENTA, 1, "\n\n------------------ after indle execute poll -----------\n");

                        //  periodic data, here that metercommm status needs to update, if added then add after this curr_nc_idx = midx; line
                        if (((curr_time - last_inst_ret[midx]) > poll_cfg.inst_poll_int))
                        {
                            for (midx = 0; midx < no_of_meter; midx++)
                            {
                                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                                {
                                    // rithika added 25/07
                                    ret = init_dlms(midx);
                                    curr_nc_idx = midx;
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_inst ...\n", midx, fun_name);
                                    add_process_state(midx, "PERIODIC", "inst");

                                    get_inst_data(midx);
                                }
                            }

                            last_inst_ret[midx] = time(NULL);
                        }

                        if (((curr_time - last_ls_ret[midx]) > poll_cfg.ls_poll_int))
                        {
                            for (midx = 0; midx < no_of_meter; midx++)
                            {
                                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                                {
                                    // rithika added 25/07
                                    ret = init_dlms(midx);
                                    curr_nc_idx = midx;
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_ls ...\n", midx, fun_name);
                                    add_process_state(midx, "PERIODIC", "ls_block");
                                    get_ls_data_block(midx, "all", "0");
                                    get_inst_data(midx);
                                }
                            }
                            last_ls_ret[midx] = time(NULL);
                        }

                        if (((curr_time - last_mn_ret[midx]) > poll_cfg.mn_poll_int))
                        {
                            for (midx = 0; midx < no_of_meter; midx++)
                            {
                                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                                {
                                    // rithika added 25/07
                                    ret = init_dlms(midx);
                                    curr_nc_idx = midx;
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_mn ...\n", midx, fun_name);
                                    add_process_state(midx, "PERIODIC", "midnight");
                                    get_midnight_data(midx, "today");
                                    get_inst_data(midx);
                                }
                            }

                            last_mn_ret[midx] = time(NULL);
                        }

                        if (((curr_time - last_bill_ret[midx]) > poll_cfg.bill_poll_int))
                        {
                            for (midx = 0; midx < no_of_meter; midx++)
                            {
                                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                                {
                                    // rithika added 25/07
                                    ret = init_dlms(midx);
                                    curr_nc_idx = midx;
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_billing ...\n", midx, fun_name);
                                    add_process_state(midx, "PERIODIC", "billing");
                                    get_billing_data(midx, "current");
                                    get_inst_data(midx);
                                }
                            }
                            last_bill_ret[midx] = time(NULL);
                        }

                        if ((curr_time - last_event_ret[midx]) > poll_cfg.event_poll_int)
                        {
                            for (midx = 0; midx < no_of_meter; midx++)
                            {
                                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                                {
                                    // rithika added 25july2025
                                    ret = init_dlms(midx);
                                    curr_nc_idx = midx;
                                    if (strcmp(dev_model, "TCS_DCU") == 0)
                                    {
                                        dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_event ...\n", midx, fun_name);
                                        memset(event_buff, 0, sizeof(event_buff));

                                        sprintf(event_buff, "event_data:cat%d", POWER_EVENTS);
                                        add_process_state(midx, "PERIODIC", event_buff);
                                        get_event_data(midx, event_buff, poll_cfg.num_of_events);
                                        get_inst_data(midx);
                                    }
                                    else
                                    {
                                        for (event_idx = 0; event_idx < MAX_EVENTS; event_idx++)
                                        {

                                            dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_event ...\n", midx, fun_name);
                                            memset(event_buff, 0, sizeof(event_buff));

                                            sprintf(event_buff, "event_data:cat%d", event_idx + 1);
                                            add_process_state(midx, "PERIODIC", event_buff);
                                            get_event_data(midx, event_buff, poll_cfg.num_of_events);
                                            get_inst_data(midx);
                                        }
                                    }
                                }
                            }
                            last_event_ret[midx] = time(NULL);
                        }
                    }
                }
            }
            cleanup_poll_functions();
            sleep(5);
        }
        ////////////////////////////////////////////////////////////////

        // rithika 25/07/2025
        if (num_poll_func == 0)
        {

            // rithika 09Dec2025
            if (chk_curr_day_ls_midx < no_of_meter)
            {
                if ((ser_port_data[g_pidx].meter_cfg[chk_curr_day_ls_midx].enable_meter == 1)) // rithika 03dec
                {
                    curr_nc_idx = chk_curr_day_ls_midx;
                    print_colored_message(YELLOW, 1, "starting of present day check \n");
                    ret = init_dlms(chk_curr_day_ls_midx);
                    if (ret != DLMS_ERROR_CODE_OK)
                    {
                        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", chk_curr_day_ls_midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                        chk_curr_day_ls_midx++;
                    }
                    else
                    {

                        if (get_curr_date_time(chk_curr_day_ls_midx) < 0)
                        {
                            dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", chk_curr_day_ls_midx, fun_name);
                        }
                        else
                        {
                            int blk_interval = get_blk_period(); // will be in minutes
                            if (blk_interval <= 0)
                            {
                                // ignore
                            }
                            else
                            {
                                int meter_hour = meter_date_time[chk_curr_day_ls_midx].hour;
                                int one_hr_in_sec = 3600;

                                int num_blks = one_hr_in_sec / (blk_interval * 60);
                                int total_hr_blks = (meter_hour * num_blks) + 1; // 1 is added to take the curr hour also;

                                int meter_min = meter_date_time[chk_curr_day_ls_midx].minute;

                                int left_over_minutes = meter_min * 60 / (blk_interval * 60);

                                int total_blks = total_hr_blks + left_over_minutes;

                                dbg_log(INFORM, "[Meter_%d]:[%-25s] : Total number of blocks to be present for current day upto currtime is %d \n", chk_curr_day_ls_midx, fun_name, total_blks);

                                char serial_number[64] = {0};
                                if (get_meter_2_ser_num(chk_curr_day_ls_midx, serial_number, sizeof(serial_number)) != 0)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", chk_curr_day_ls_midx, fun_name);
                                }

                                char temp_manufacturer[64] = {0};
                                if (get_manufacturer(chk_curr_day_ls_midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s]: Failed to get meter manufacturer.\n", chk_curr_day_ls_midx, fun_name);
                                }

                                char table_name[100];
                                char temp_met_serial_no[64] = {0};

                                strcpy(temp_met_serial_no, serial_number);
                                // rithika 28Nov2025
                                for (int i = 0; i < strlen(temp_met_serial_no); i++)
                                {

                                    if (temp_met_serial_no[i] == '-')
                                    {
                                        temp_met_serial_no[i] = '_';
                                    }
                                }

                                if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                                {

                                    memset(table_name, 0, sizeof(table_name));

                                    snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", "ls_data", temp_manufacturer, dcu_ser_num, g_pidx, temp_met_serial_no);
                                }
                                else
                                {

                                    memset(table_name, 0, sizeof(table_name));
                                    snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", "ls_data", temp_manufacturer, dcu_ser_num, g_pidx, temp_met_serial_no);
                                }

                                printf("table_name : %s\n", table_name);
                                memset(g_old_ls_date_time, 0, sizeof(g_old_ls_date_time));

                                time_t current_time = time(NULL);
                                struct tm *tm_info = localtime(&current_time);

                                strftime(g_old_ls_date_time, sizeof(g_old_ls_date_time), "%d-%m-%Y", tm_info);

                                present_ls_flag = 1;
                                int entry_cnt = old_data_check(g_old_ls_date_time, table_name, chk_curr_day_ls_midx);

                                if (entry_cnt != -1)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Number of blocks present in table for current day upto currtime is %d \n", chk_curr_day_ls_midx, fun_name, entry_cnt);

                                    if (entry_cnt < total_blks)
                                    {
                                        print_colored_message(RED, 1, "drift sec %f\n", meter_drift_sec[chk_curr_day_ls_midx]);
                                        if (meter_drift_sec[chk_curr_day_ls_midx] > 0)
                                        {
                                            int drift_blks_cnt = (meter_drift_sec[chk_curr_day_ls_midx] / poll_cfg.ls_poll_int) + 1;
                                            int cnt = total_blks - entry_cnt;
                                            if (cnt <= drift_blks_cnt)
                                            {
                                                // ignore
                                            }
                                            else
                                            {
                                                dbg_log(INFORM, "[Meter_%d]:[%-25s] : Number of blocks present in table is not matching so polling the present day ls data\n", chk_curr_day_ls_midx, fun_name);

                                                ret = get_ls_data_block(chk_curr_day_ls_midx, "all", "1");
                                                if (ret == 0)
                                                {

                                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Present day ls data polling succeeded\n", chk_curr_day_ls_midx, fun_name);
                                                }
                                                else
                                                {
                                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Present day ls data polling failed ret = %d\n", chk_curr_day_ls_midx, fun_name, ret);
                                                }
                                            }
                                        }
                                        else
                                        {
                                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Number of blocks present in table is not matching so polling the present day ls data\n", chk_curr_day_ls_midx, fun_name);

                                            ret = get_ls_data_block(chk_curr_day_ls_midx, "all", "1");
                                            if (ret == 0)
                                            {

                                                dbg_log(INFORM, "[Meter_%d]:[%-25s] : Present day ls data polling succeeded\n", chk_curr_day_ls_midx, fun_name);
                                            }
                                            else
                                            {
                                                dbg_log(INFORM, "[Meter_%d]:[%-25s] : Present day ls data polling failed ret = %d\n", chk_curr_day_ls_midx, fun_name, ret);
                                            }
                                        }
                                    }
                                }
                                present_ls_flag = 0;
                            }
                            dis_connect();
                        }
                        chk_curr_day_ls_midx++;
                    }
                }
                else
                {
                    chk_curr_day_ls_midx++;
                }
            }
            else
            {
                chk_curr_day_ls_midx = 0;
            }

            if (poll_cfg.enable_stored_data_poll == 1)
            {
                int missing_old_data_chk = get_missing_old_data_check();

                if (first_day == 1 && missing_old_data_chk == 1)
                {
                    dbg_log(INFORM, "[%-25s] : missing_old_data_chk : %d is enabled\n", fun_name, missing_old_data_chk);

                    time_t now;
                    struct tm *local_t;

                    int temp_day = 0;

                    time(&now);
                    local_t = localtime(&now);

                    // Create a copy of local_t to avoid modifying the original during the process
                    local_copy = *local_t;

                    // Reduce the day by 1 (working on the copy)
                    local_copy.tm_mday--;
                    mktime(&local_copy);
                    temp_day = local_copy.tm_mday;
                }

                if (missing_old_data_chk == 1)
                {

                    if (missing_old_data_midx < no_of_meter)
                    {

                        if ((ser_port_data[g_pidx].meter_cfg[missing_old_data_midx].enable_meter == 1)) // rithika 03dec
                        {

                            if (old_poll_day < poll_cfg.num_old_days)
                            {
                                print_colored_message(YELLOW, 1, "inside old day poll first check ");
                                char serial_number[64] = {0};
                                if (get_meter_2_ser_num(missing_old_data_midx, serial_number, sizeof(serial_number)) != 0)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", missing_old_data_midx, fun_name);
                                }

                                char temp_manufacturer[64] = {0};
                                if (get_manufacturer(missing_old_data_midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s]: Failed to get meter manufacturer.\n", missing_old_data_midx, fun_name);
                                }

                                // old_ls_flag = 1;
                                char table_name[100];
                                char temp_met_serial_no[64] = {0};
                                // memset(temp_met_sn, 0, sizeof(temp_met_sn));

                                // if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
                                // {
                                // 	dbg_log(INFORM, "[Meter_%d]:[%-25s] :  Failed to get meter serial number \n", fun_name, midx);
                                // }

                                // strcpy(temp_met_sn, get_meter_2_ser_num(midx));

                                strcpy(temp_met_serial_no, serial_number);
                                // rithika 28Nov2025
                                for (int i = 0; i < strlen(temp_met_serial_no); i++)
                                {

                                    if (temp_met_serial_no[i] == '-')
                                    {
                                        temp_met_serial_no[i] = '_';
                                    }
                                }

                                if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                                {

                                    memset(table_name, 0, sizeof(table_name));

                                    snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", "ls_data", temp_manufacturer, dcu_ser_num, g_pidx, temp_met_serial_no);
                                }
                                else
                                {

                                    memset(table_name, 0, sizeof(table_name));
                                    snprintf(table_name, sizeof(table_name), "%s_%s_%s_%d_%s", "ls_data", temp_manufacturer, dcu_ser_num, g_pidx, temp_met_serial_no);
                                }

#if DEBUG
                                printf("table_name : %s\n", table_name);
#endif
                                memset(g_old_ls_date_time, 0, sizeof(g_old_ls_date_time));

                                // Format the date string for the current day (using local_copy)
                                strftime(g_old_ls_date_time, sizeof(g_old_ls_date_time), "%d-%m-%Y", &local_copy);

                                curr_nc_idx = missing_old_data_midx;

                                ret = old_data_check(g_old_ls_date_time, table_name, missing_old_data_midx);

                                if (ret != 0 && ret != -1)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Power down occured for this date %s so it has only %d entries. \n", missing_old_data_midx, fun_name, g_old_ls_date_time, ret);
                                }
                                else if (ret != 0)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Old day ls data is going to poll for the date %s \n", missing_old_data_midx, fun_name, g_old_ls_date_time);

                                    ret = init_dlms(missing_old_data_midx);
                                    if (ret != DLMS_ERROR_CODE_OK)
                                    {
                                        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", missing_old_data_midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                    }
                                    else
                                    {
                                        ret = get_ls_data_day(missing_old_data_midx, g_old_ls_date_time, "1");
                                        if (ret == 0)
                                        {
                                            update_ls_list(missing_old_data_midx, serial_number, g_old_ls_date_time);
                                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Old day ls data polling succeeded for the date %s\n", missing_old_data_midx, fun_name, g_old_ls_date_time);
                                        }
                                        else
                                        {
                                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Old day ls data polling failed for the date %s ret = %d \n", missing_old_data_midx, fun_name, g_old_ls_date_time, ret);
                                        }

                                        // old_ls_flag = 0;

                                        dis_connect();
                                    }
                                }
                                missing_old_data_midx++;
                            }
                        }
                        else
                        {
                            missing_old_data_midx++;
                        }
                    }
                    else
                    {

                        if (old_poll_day < poll_cfg.num_old_days - 1) // subtracting 1 is to avoid incrementing  the days to exact num of days here for 29 it will enter into this loop and make old_data_day as 30 then it got stuck in the above old_data_day check
                        {
                            first_day = 0;

                            old_poll_day++;
                            missing_old_data_midx = 0;
                            // Reduce the day by 1 (preserving changes in local_copy)
                            local_copy.tm_mday--;

                            dbg_log(INFORM, "[%-25s] : After reduction - local_copy tm_mday: %d\n", fun_name, local_copy.tm_mday);

                            // If day becomes 0 or negative, adjust for previous month
                            if (local_copy.tm_mday <= 0)
                            {
                                local_copy.tm_mon--;
                                if (local_copy.tm_mon < 0)
                                {
                                    local_copy.tm_mon = 11;
                                    local_copy.tm_year--;
                                }

                                dbg_log(INFORM, "[%-25s] : After reduction - local_copy tm_mon: %d local_copy.tm_year : %d\n", fun_name, local_copy.tm_mon, local_copy.tm_year);

                                // Get the number of days in the previous month
                                int days_in_previous_month = days_in_month(local_copy.tm_mon, local_copy.tm_year);
                                local_copy.tm_mday = days_in_previous_month;
                            }
                        }
                        else
                        {
                            dbg_log(INFORM, "[%-25s] : Completed the checking of old ls data for %d days. Restarting the checking of old data from the first meter.\n", fun_name, poll_cfg.num_old_days);

                            missing_old_data_midx = 0;
                            old_poll_day = 0;
                            first_day = 1;
                        }
                    }
                }

                // else
                // {

                //     missing_old_data_midx = 0;
                //     old_poll_day = 0;
                //     first_day = 1;
                // }
            }

            /////////////////////////////////////////////////////////          ////////////////////////////////////////////////////
            send_hanging_check();
            process_od_cmd();
            check_od_command_resp();

            // rithika 14Apr2026
            if (RECONNECT)
            {
                iec104_log_sink_poll_network();
            }
            curr_time = time(NULL);

            local_time = localtime(&curr_time);

            int curr_hr = local_time->tm_hour; // rithika
            int curr_min = local_time->tm_min;

            struct tm midnight = *local_time;
            struct tm one_am = *local_time;

            int poll_inst_interval_min = poll_cfg.inst_poll_int / 60;
            int poll_mn_interval_hrs = poll_cfg.mn_poll_int / 3600;
            int poll_bill_interval_hrs = poll_cfg.bill_poll_int / 3600;
            int poll_ls_interval_min = poll_cfg.ls_poll_int / 60;
            int poll_event_interval_min = poll_cfg.event_poll_int / 60;

            // rithika 18nov2025
            int time_left;

            midnight.tm_hour = 0;
            midnight.tm_min = 0;
            midnight.tm_sec = 0;

            one_am.tm_hour = 1;
            one_am.tm_min = 0;
            one_am.tm_sec = 0;

            time_t midnight_time = mktime(&midnight);
            time_t one_am_time = mktime(&one_am);

            if (midnight_time == -1 || one_am_time == -1)
            {
                printf("Error converting time to time_t\n");
            }

            if (curr_time >= midnight_time && curr_time < one_am_time)
            {
                for (midx = 0; midx < no_of_meter; midx++)
                {
                    if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                    {
                        if (!midnight_polled[midx])
                        {
                            process_od_cmd(); // rithika

                            // rithika 14Apr2026
                            if (RECONNECT)
                            {
                                iec104_log_sink_poll_network();
                            }
                            curr_nc_idx = midx;

                            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                            {
                                if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                                {
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s]: RS %d NOT COMMN\n", midx, fun_name, midx);
                                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                                    {
                                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                                    }
                                    meter_commn_status(curr_nc_idx, 0);
                                    continue;
                                }
                            }

                            ret = init_dlms(midx);
                            if (ret != DLMS_ERROR_CODE_OK) // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s]: SNRM-AARQ FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                continue;
                            }

                            check_od_command_resp();

                            dbg_log(INFORM, "[Meter_%d]:[%-25s]: Mid night polling ...\n", midx, fun_name);

                            add_process_state(midx, "PERIODIC", "midnight");
                            get_midnight_data(midx, "today");

                            midnight_polled[midx] = 1;

                            // rithika 17oct2025
                            dis_connect();
                        }
                    }
                }
            }

            for (midx = 0; midx < no_of_meter; midx++)
            {
                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                {

                    // curr_time = time(NULL);

                    // local_time = localtime(&curr_time);

                    // curr_hr = local_time->tm_hour; // rithika
                    // curr_min = local_time->tm_min;

                    // dbg_log(INFORM, "*************** Polling periodic inst data based on periodicity ***********");
                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_time - last_inst_ret[midx] %d \n", fun_name, midx, curr_time - last_inst_ret[midx]);
                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_min %d curr_min / poll_inst_interval_min %d inst_poll_flag[%d] %d\n", fun_name, midx, curr_min, curr_min % poll_inst_interval_min, midx, inst_poll_flag[midx]);

                    // if (poll_cfg.poll_inst_data != 0 && ((poll_inst_interval_min > 1 && curr_min % poll_inst_interval_min == 0) || ((curr_time - last_inst_ret[midx]) >= poll_cfg.inst_poll_int)))
                    if (poll_cfg.poll_inst_data != 0 && ((poll_inst_interval_min >= 1 && curr_min % poll_inst_interval_min == 0) || ((curr_time - last_inst_ret[midx]) >= poll_cfg.inst_poll_int)))

                    {

                        if (poll_inst_interval_min <= 60)
                        {
                            time_left = poll_inst_interval_min - (curr_min % poll_inst_interval_min);
                        }
                        else if (poll_inst_interval_min > 60)
                        {
                            int total_minutes = curr_hr * 60 + curr_min;
                            time_left = poll_inst_interval_min - (total_minutes % poll_inst_interval_min);
                        }

                        long nxt_poll_time = (time_left * 60) + time(NULL);

                        //  time_t current_time = time(NULL);
                        struct tm *tm_info = localtime(&nxt_poll_time);
                        char datetime_str[32];

                        strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

                        update_nxt_poll_time("inst_nxt_poll_time", datetime_str);
                        printf("nxt_poll_time -----------> %s\n", datetime_str);

                        // dbg_log(INFORM, "[%-25s] : [Meter : %d] inst_poll_flag[midx] %d \n", fun_name, midx, inst_poll_flag[midx]);

                        // to set flag  (even though it is time for polling but the flag didn't set)
                        // if ((curr_time - last_inst_ret[midx]) >= poll_cfg.inst_poll_int)
                        // rithika 20nov2025
                        if ((last_inst_poll_min[midx] != curr_min) && ((poll_inst_interval_min >= 1 && curr_min % poll_inst_interval_min == 0) || ((curr_time - last_inst_ret[midx]) >= poll_cfg.inst_poll_int)))
                        {
                            last_inst_poll_min[midx] = curr_min;
                            inst_poll_flag[midx] = 1;
                        }

                        // if ((poll_inst_interval_min >= 1 && curr_min % poll_inst_interval_min == 0) || ((curr_time - last_inst_ret[midx]) >= poll_cfg.inst_poll_int))
                        // {

                        //     inst_poll_flag[midx] = 1;
                        // }
                        if (inst_poll_flag[midx] == 1)
                        {
                            process_od_cmd(); // rithika

                            // rithika 14Apr2026
                            if (RECONNECT)
                            {
                                iec104_log_sink_poll_network();
                            }
                            // rithika added 25/07
                            curr_nc_idx = midx;
                            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                            {
                                if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                                {
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s]: RS %d NOT COMMN\n", midx, fun_name, midx);
                                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                                    {
                                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                                    }
                                    meter_commn_status(curr_nc_idx, 0);
                                    continue;
                                }
                            }

                            check_od_command_resp();

                            ret = init_dlms(midx);

                            if (ret != DLMS_ERROR_CODE_OK) // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s]: SNRM-AARQ FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                continue;
                            }
                            dbg_log(INFORM, "[Meter_%d]:[%-25s]: Periodic_inst ...\n", midx, fun_name);
                            add_process_state(midx, "PERIODIC", "inst");
                            // get_name_plate(midx); // need to check with mam
                            int ret1 = get_inst_data(midx);

                            if (ret1 == 0)
                            {
                                inst_poll_flag[midx] = 0;
                                dbg_log(WARNING, "[Meter_%d]:[%-25s]: Inst polled success \n", midx, fun_name);
                            }

                            else // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s]: INST_POLLING_FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                continue;
                            }

                            // rithika 14oct2025
                            last_inst_ret[midx] = time(NULL);
                            // rithika 17oct2025
                            dis_connect();
                        }
                        // else
                        // {
                        //     last_inst_ret[midx] = time(NULL);
                        // }
                    }
                    else
                    {
                        inst_poll_flag[midx] = 1;
                    }
                }
            }

            for (midx = 0; midx < no_of_meter; midx++)
            {
                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                {

                    // curr_time = time(NULL);

                    // local_time = localtime(&curr_time);

                    // curr_hr = local_time->tm_hour; // rithika
                    // curr_min = local_time->tm_min;
                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_time - last_ls_ret[midx] %d \n", fun_name, midx, curr_time - last_ls_ret[midx]);
                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_min %d curr_min / poll_ls_interval_min %d \n", fun_name, midx, curr_min, curr_min % poll_ls_interval_min);

                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] ls inst_poll_flag[%d] %d \n", fun_name, midx, midx, inst_poll_flag[midx]);

                    if (poll_cfg.poll_ls_data != 0 && (((curr_time - last_ls_ret[midx]) >= poll_cfg.ls_poll_int) || (curr_min % poll_ls_interval_min == 0)))
                    {
                        if (poll_ls_interval_min <= 60)
                        {
                            time_left = poll_ls_interval_min - (curr_min % poll_ls_interval_min);
                        }
                        else if (poll_ls_interval_min > 60)
                        {
                            int total_minutes = curr_hr * 60 + curr_min;
                            time_left = poll_ls_interval_min - (total_minutes % poll_ls_interval_min);
                        }

                        long nxt_poll_time = (time_left * 60) + time(NULL);

                        //  time_t current_time = time(NULL);
                        struct tm *tm_info = localtime(&nxt_poll_time);
                        char datetime_str[32];

                        strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

                        update_nxt_poll_time("ls_nxt_poll_time", datetime_str);
                        printf("nxt_poll_time -----------> %s\n", datetime_str);

                        // dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_min / poll_ls_interval_min %d ls_poll_flag[midx] %d\n", fun_name, midx, curr_min % poll_ls_interval_min, ls_poll_flag[midx]);
                        if (ls_poll_flag[midx] == 1)
                        {
                            process_od_cmd(); // rithika
                            // rithika added 25/07
                            curr_nc_idx = midx;
                            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                            {
                                if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                                {
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s] : RS %d NOT COMMN\n", midx, fun_name, midx);
                                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                                    {
                                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                                    }
                                    meter_commn_status(curr_nc_idx, 0);
                                    continue;
                                }
                            }

                            check_od_command_resp();

                            ret = init_dlms(midx);

                            if (ret != DLMS_ERROR_CODE_OK) // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                continue;
                            }

                            // rithika 07nov2025
                            int chk_nameplate = check_name_plate(midx);

                            if (chk_nameplate != 0)
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : Nameplate is not available in redis, Reading name plate details for %d \n", midx, fun_name, midx);
                                int np_ret = get_name_plate(midx);
                                // rithika 27nov2025
                                if (np_ret == 0)
                                {
                                    np_polled_comp[midx] = 1;
                                }
                            }

                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Periodic_ls ...\n", midx, fun_name);
                            add_process_state(midx, "PERIODIC", "ls_block");

                            char num_blocks_str[20];
                            snprintf(num_blocks_str, sizeof(num_blocks_str), "%u", poll_cfg.num_blocks);
                            int ret1 = get_ls_data_block(midx, LS_BLOCK_TIME, num_blocks_str);

                            if (ret1 == 0)
                            {
                                ls_poll_flag[midx] = 0;
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : LS polled success \n", midx, fun_name);
                            }

                            else // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : LS_POLLING_FAILED : ret %d - %s\n", midx, fun_name, ret1, (hlp_getErrorMessage(ret1)));
                                continue;
                            }

                            if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0))
                            {
                                // dbg_log(WARNING, "[%-25s] : [Meter_%d]  : ++++++++++++ curr_time - last_inst_ret[midx]- %d\n", fun_name, midx, curr_time - last_inst_ret[midx]);
                                if (time(NULL) - last_inst_ret[midx] >= poll_cfg.inst_poll_int)
                                {
                                    get_inst_data(midx);
                                    // rithika 14oct2025

                                    last_inst_ret[midx] = time(NULL);
                                }
                                // get_all_meter_inst_data(); // rithika 04/09 inst for specific
                            }

                            last_ls_ret[midx] = time(NULL);

                            // rithika 17oct2025
                            dis_connect();
                        }
                    }
                    else
                    {
                        // printf("else 11111\n");
                        ls_poll_flag[midx] = 1;
                        // dbg_log(INFORM, "[%-25s] : [Meter : %d] else ls_poll_flag[midx] %d \n", fun_name, midx, ls_poll_flag[midx]);
                    }
                }
            }

            for (midx = 0; midx < no_of_meter; midx++)
            {
                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                {

                    // curr_time = time(NULL);

                    // local_time = localtime(&curr_time);

                    // curr_hr = local_time->tm_hour; // rithika
                    // curr_min = local_time->tm_min;

                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] midnight inst_poll_flag[%d] %d \n", fun_name, midx, midx, inst_poll_flag[midx]);

                    dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_time - last_mn_ret[midx] %d \n", fun_name, midx, curr_time - last_mn_ret[midx]);
                    printf("before mn call \n");
                    if (poll_cfg.poll_mn_data != 0 && (((curr_time - last_mn_ret[midx]) >= poll_cfg.mn_poll_int) || (curr_hr % poll_mn_interval_hrs == 0)))
                    {
                        // rithika 18nov2025
                        int poll_mn_interval_min = poll_cfg.mn_poll_int / 60;
                        int total_minutes = curr_hr * 60 + curr_min;
                        time_left = poll_mn_interval_min - (total_minutes % poll_mn_interval_min);

                        long nxt_poll_time = (time_left * 60) + time(NULL);

                        //  time_t current_time = time(NULL);
                        struct tm *tm_info = localtime(&nxt_poll_time);
                        char datetime_str[32];

                        strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

                        update_nxt_poll_time("mn_nxt_poll_time", datetime_str);
                        printf("nxt_poll_time -----------> %s\n", datetime_str);

                        if (mn_poll_flag[midx] == 1)
                        {
                            process_od_cmd(); // rithika
                            // rithika added 25/07
                            curr_nc_idx = midx;
                            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                            {
                                if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                                {
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s] : RS %d NOT COMMN\n", midx, fun_name, midx);
                                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                                    {
                                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                                    }
                                    meter_commn_status(curr_nc_idx, 0);
                                    continue;
                                }
                            }

                            check_od_command_resp();

                            ret = init_dlms(midx);

                            if (ret != DLMS_ERROR_CODE_OK) // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                continue;
                            }

                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Periodic_mn ...\n", midx, fun_name);
                            add_process_state(midx, "PERIODIC", "midnight");

                            int ret1 = get_midnight_data(midx, "today");

                            if (ret1 == 0)
                            {
                                mn_poll_flag[midx] = 0;
                                // dbg_log(WARNING, "[%-25s] : [Meter_%d]  : MIDNIGHT polled success mn_poll_flag- %d\n", fun_name, midx, mn_poll_flag[midx]);
                            }
                            else
                            {
                                mn_poll_flag[midx] = 0;
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : MIDNIGHT polling Failed mn_poll_flag- %d\n", midx, fun_name, mn_poll_flag[midx]);
                            }
                            if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0))
                            {
                                // dbg_log(WARNING, "[%-25s] : [Meter_%d]  : midnight curr_time - last_inst_ret[midx]- %d\n", fun_name, midx, time(NULL) - last_inst_ret[midx]);

                                if (time(NULL) - last_inst_ret[midx] >= poll_cfg.inst_poll_int)
                                {
                                    get_inst_data(midx);
                                    // rithika 14oct2025
                                    last_inst_ret[midx] = time(NULL);
                                }

                                // if ()                          // curr time - last int read > 20
                                // get_all_meter_inst_data(); // rithika 04/09 for particular meter
                            }

                            last_mn_ret[midx] = time(NULL);

                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : init_mn_polled_comp[%d] %d\n", midx, fun_name, midx, init_mn_polled_comp[midx]);

                            // rithika 09dec2025
                            if (init_mn_polled_comp[midx] == 1)
                            {
                                dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for midnight is missing Going to poll midnight data for init_poll \n", midx, fun_name);

                                ret = get_midnight_data(midx, "all");
                                if (ret == 0)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for midnight data succeeded\n", midx, fun_name);

                                    init_mn_polled_comp[midx] = 0;
                                }
                                else
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for midnight data failed\n", midx, fun_name);
                                }
                            }
                            // rithika 17oct2025
                            dis_connect();
                        }
                    }
                    else
                    {

                        // printf("else 222222\n");
                        mn_poll_flag[midx] = 1;
                        // dbg_log(INFORM, "[%-25s] : [Meter : %d] else mn_poll_flag[midx] %d \n", fun_name, midx, mn_poll_flag[midx]);
                    }
                }
            }

            for (midx = 0; midx < no_of_meter; midx++)
            {
                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                {
                    // curr_time = time(NULL);

                    // local_time = localtime(&curr_time);

                    // curr_hr = local_time->tm_hour; // rithika
                    // curr_min = local_time->tm_min;

                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] bill inst_poll_flag[%d] %d \n", fun_name, midx, midx, inst_poll_flag[midx]);

                    // rithika 15nov2025
                    if (poll_cfg.poll_bill_data != 0 && (((curr_time - last_bill_ret[midx]) >= poll_cfg.bill_poll_int) || (curr_hr % poll_bill_interval_hrs == 0)))
                    {

                        // rithika 18nov2025
                        int poll_bill_interval_min = poll_cfg.bill_poll_int / 60;
                        int total_minutes = curr_hr * 60 + curr_min;
                        time_left = poll_bill_interval_min - (total_minutes % poll_bill_interval_min);

                        long nxt_poll_time = (time_left * 60) + time(NULL);

                        //  time_t current_time = time(NULL);
                        struct tm *tm_info = localtime(&nxt_poll_time);
                        char datetime_str[32];

                        strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

                        update_nxt_poll_time("billing_nxt_poll_time", datetime_str);
                        printf("nxt_poll_time -----------> %s\n", datetime_str);

                        if (bill_poll_flag[midx] == 1)
                        {
                            process_od_cmd(); // rithika
                            // rithika added 25/07
                            curr_nc_idx = midx;
                            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                            {
                                if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                                {
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s] : RS %d NOT COMMN\n", midx, fun_name, midx);
                                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                                    {
                                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                                    }
                                    meter_commn_status(curr_nc_idx, 0);
                                    continue;
                                }
                            }

                            check_od_command_resp();

                            ret = init_dlms(midx);
                            if (ret != DLMS_ERROR_CODE_OK) // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                continue;
                            }

                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : Periodic_billing ...\n", midx, fun_name);
                            add_process_state(midx, "PERIODIC", "billing");
                            int ret1 = get_billing_data(midx, "current");
                            // get_inst_data(midx);
                            if (ret1 == 0)
                            {

                                bill_poll_flag[midx] = 0;
                                // dbg_log(WARNING, "[%-25s] : [Meter_%d]  : BILLING polled success bill_poll_flag- %d\n", fun_name, midx, bill_poll_flag[midx]);
                            }
                            else
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : BILLING polling failed bill_poll_flag- %d\n", midx, fun_name, bill_poll_flag[midx]);
                            }

                            if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0))
                            {
                                // dbg_log(WARNING, "[%-25s] : [Meter_%d]  : billing curr_time - last_inst_ret[midx]- %d\n", fun_name, midx, time(NULL) - last_inst_ret[midx]);

                                if (time(NULL) - last_inst_ret[midx] >= poll_cfg.inst_poll_int)
                                {
                                    get_inst_data(midx);
                                    // rithika 14oct2025
                                    last_inst_ret[midx] = time(NULL);
                                }
                                // get_all_meter_inst_data(); // rithika 04/09
                            }

                            last_bill_ret[midx] = time(NULL);
                            // rithika 17oct2025

                            // rithika 09dec2025
                            if (init_billing_polled_comp[midx] == 1)
                            {
                                dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for billing is missing Going to poll billing data for init_poll \n", midx, fun_name);

                                ret = get_billing_data(midx, "all");
                                if (ret == 0)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for billing data succeeded\n", midx, fun_name);
                                    init_billing_polled_comp[midx] = 0;
                                }
                                else
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for billing data failed\n", midx, fun_name);
                                }
                            }

                            dis_connect();
                        }
                    }
                    else
                    {
                        // printf("else 333333\n");
                        bill_poll_flag[midx] = 1;
                    }
                }
            }
            for (midx = 0; midx < no_of_meter; midx++)
            {
                if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1) || (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)) // rithika 05Sep
                {
                    // curr_time = time(NULL);

                    // local_time = localtime(&curr_time);

                    // curr_hr = local_time->tm_hour; // rithika
                    // curr_min = local_time->tm_min;

                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_time - last_event_ret[midx] %d \n", fun_name, midx, time(NULL) - last_event_ret[midx]);
                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_min %d curr_min / poll_event_interval_min %d \n", fun_name, midx, curr_min, curr_min % poll_event_interval_min);

                    // dbg_log(INFORM, "[%-25s] : [Meter : %d] event inst_poll_flag[%d] %d \n", fun_name, midx, midx, inst_poll_flag[midx]);

                    // rithika 15nov2025
                    if (poll_cfg.poll_event_data != 0 && (((curr_time - last_event_ret[midx]) >= poll_cfg.event_poll_int) || (curr_min % poll_event_interval_min == 0)))
                    {
                        dbg_log(INFORM, "[%-25s] : [Meter : %d] curr_time - last_event_ret[%d] %d event_poll_flag[%d] = %d\n", fun_name, midx, midx, curr_time - last_event_ret[midx], midx, event_poll_flag[midx]);

                        if (poll_event_interval_min <= 60)
                        {
                            time_left = poll_event_interval_min - (curr_min % poll_event_interval_min);
                        }
                        else if (poll_event_interval_min > 60)
                        {
                            int total_minutes = curr_hr * 60 + curr_min;
                            time_left = poll_event_interval_min - (total_minutes % poll_event_interval_min);
                        }

                        long nxt_poll_time = (time_left * 60) + time(NULL);

                        //  time_t current_time = time(NULL);
                        struct tm *tm_info = localtime(&nxt_poll_time);
                        char datetime_str[32];

                        strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);

                        update_nxt_poll_time("event_nxt_poll_time", datetime_str);
                        printf("nxt_poll_time -----------> %s\n", datetime_str);

                        if (event_poll_flag[midx] == 1)
                        {
                            process_od_cmd(); // rithika

                            // rithika added 25/07
                            curr_nc_idx = midx;
                            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                            {
                                if (plcc_modem_info.plcc_meter_map[midx].rs_status == 0)
                                {
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s] : RS %d NOT COMMN\n", midx, fun_name, midx);
                                    if (strcmp(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC") == 0)
                                    {
                                        strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, "NC");
                                    }
                                    meter_commn_status(curr_nc_idx, 0);
                                    continue;
                                }
                            }

                            ret = init_dlms(midx);

                            if (ret != DLMS_ERROR_CODE_OK) // 03/09
                            {
                                dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", midx, fun_name, ret, (hlp_getErrorMessage(ret)));
                                continue;
                            }

                            check_od_command_resp();

                            // rithika 13nov2025
                            if (strcmp(dev_model, "TCS_DCU") == 0)
                            {
                                dbg_log(INFORM, "[Meter_%d]:[%-25s] : Periodic_event ...\n", midx, fun_name);
                                memset(event_buff, 0, sizeof(event_buff));

                                sprintf(event_buff, "event_data_cat%d", 3);
                                add_process_state(midx, "PERIODIC", event_buff);
                                int ret1 = get_event_data(midx, event_buff, poll_cfg.num_of_events);
                                if (ret1 == 0)
                                {
                                    event_poll_flag[midx] = 0;
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s] : EVENT polled success\n", midx, fun_name);
                                }
                                else
                                {
                                    dbg_log(WARNING, "[Meter_%d]:[%-25s] : EVENT polling failed event_poll_flag - %d\n", midx, fun_name, event_poll_flag[midx]);
                                }
                            }
                            else
                            {
                                for (event_idx = 0; event_idx < MAX_EVENTS; event_idx++)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : Periodic_event ...\n", midx, fun_name);
                                    memset(event_buff, 0, sizeof(event_buff));

                                    sprintf(event_buff, "event_data_cat%d", event_idx + 1);
                                    add_process_state(midx, "PERIODIC", event_buff);
                                    int ret1 = get_event_data(midx, event_buff, poll_cfg.num_of_events);
                                    if (ret1 == 0)
                                    {
                                        event_poll_flag[midx] = 0;
                                        dbg_log(WARNING, "[Meter_%d]:[%-25s] : EVENT polled success\n", midx, fun_name);
                                    }
                                    else
                                    {
                                        dbg_log(WARNING, "[Meter_%d]:[%-25s] : EVENT polling failed event_poll_flag - %d\n", midx, fun_name, event_poll_flag[midx]);
                                    }
                                    // get_inst_data(midx);
                                }
                            }

                            if ((strcmp(feature_cfg.meter_inf_type, "PLCC") != 0))
                            {
                                // dbg_log(WARNING, "[%-25s] : [Meter_%d]  : event curr_time - last_inst_ret[midx]- %d\n", fun_name, midx, time(NULL) - last_inst_ret[midx]);

                                if (time(NULL) - last_inst_ret[midx] >= poll_cfg.inst_poll_int)
                                {
                                    get_inst_data(midx);
                                    // rithika 14oct2025
                                    last_inst_ret[midx] = time(NULL);
                                }
                                // get_all_meter_inst_data(); // rithika 04/09
                            }

                            last_event_ret[midx] = time(NULL);

                            // rithika 09dec2025
                            if (strcmp(dev_model, "TCS_DCU") == 0)
                            {
                                // rithika 10Dec2025
                                if (init_event_polled_comp[midx][POWER_EVENT_IDX] == 1)
                                {
                                    dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for event is missing Going to poll event data for init_poll for event category %d\n", midx, fun_name, 3);

                                    ret = get_event_data(midx, event_buff, "all");
                                    if (ret == 0)
                                    {
                                        dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for event category %d succeeded\n", midx, fun_name, 3);
                                        init_event_polled_comp[midx][POWER_EVENT_IDX] = 0;
                                    }
                                    else
                                    {
                                        dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for event category %d failed\n", midx, fun_name, 3);
                                    }
                                }
                            }
                            else
                            {
                                for (event_idx = 0; event_idx < MAX_EVENTS; event_idx++)
                                {
                                    memset(event_buff, 0, sizeof(event_buff));

                                    sprintf(event_buff, "event_data_cat%d", event_idx + 1);
                                    if (init_event_polled_comp[midx][event_idx] == 1)
                                    {
                                        dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for event is missing Going to poll event data for init_poll for event category %d\n", midx, fun_name, event_idx + 1);

                                        ret = get_event_data(midx, event_buff, "all");
                                        if (ret == 0)
                                        {
                                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for event category %d succeeded\n", midx, fun_name, event_idx + 1);
                                            init_event_polled_comp[midx][event_idx] = 0;
                                        }
                                        else
                                        {
                                            dbg_log(INFORM, "[Meter_%d]:[%-25s] : INIT data polling for event category %d failed\n", midx, fun_name, event_idx + 1);
                                        }
                                    }
                                }
                            }

                            dis_connect();
                        }
                    }
                    else
                    {

                        // printf("else 44444\n");
                        event_poll_flag[midx] = 1;
                        dbg_log(INFORM, "[%-25s] : [Meter : %d] inside else block event_poll_flag[%d] = %d\n", fun_name, midx, midx, event_poll_flag[midx]);
                    }
                }
            }
        }
    }
    return 0;
}

/**
 * @brief Initializes the DLMS process for data collection.
 *
 * This function sets up the data collection process by initializing
 * the necessary configurations and potentially fetching historical data
 * based on the polling configuration. It serves as the entry point
 * for the DLMS data collection process.
 *
 * @return Returns 0 upon successful initialization.
 */
int init_dlms_proc()
{

    int midx;

    for (midx = 0; midx < MAX_NO_OF_METER_PER_PORT; midx++)
    {
        g_met_status_flag[midx] = 1;
        memset(&g_last_met_on_time_info[midx], 0, sizeof(Date));
    }

#if 0

#endif
    printf("main =++++++++++++++++++ num_dev         :  %d\n", ser_port_data[g_pidx].num_dev);

    // rithika 07oct2025
    memset(inst_data_read, 0, sizeof(inst_data_read));
    memset(ls_data_read_day, 0, sizeof(ls_data_read_day));
    memset(ls_data_read_block, 0, sizeof(ls_data_read_block));
    memset(mn_data_read, 0, sizeof(mn_data_read));
    memset(bill_data_read, 0, sizeof(bill_data_read));
    memset(event_data_read, 0, sizeof(event_data_read));

    // rithika 15nov2025
    memset(np_polled_comp, 0, sizeof(np_polled_comp));

    // rithika 17oct2025
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        if (plcc_modem_info.no_of_nodes <= MAX_NO_OF_METER_PER_PORT)
        {
            no_of_meter = plcc_modem_info.no_of_nodes;
        }
        else
        {
            no_of_meter = MAX_NO_OF_METER_PER_PORT;
        }
    }
    else
    {
        no_of_meter = ser_port_data[g_pidx].num_dev;
    }

    for (midx = 0; midx < no_of_meter; midx++)
    {

        inst_obis_read_enb[midx] = 1;
        // ls_day_obis_read_enb[midx] = 1; // rithika16oct
        ls_block_obis_read_enb[midx] = 1;
        mn_obis_read_enb[midx] = 1;   // rithika16oct
        bill_obis_read_enb[midx] = 1; // rithika16oct

        for (int evnt_indx = 0; evnt_indx < MAX_EVENTS; evnt_indx++)
        {
            event_obis_read_enb[midx][evnt_indx] = 1; // rithika16oct
        }
    }

    // rithika 19nov2025
    char *next_poll_time_buff = "initial polling in progress";
    update_nxt_poll_time("inst_nxt_poll_time", next_poll_time_buff);
    update_nxt_poll_time("ls_nxt_poll_time", next_poll_time_buff);
    update_nxt_poll_time("mn_nxt_poll_time", next_poll_time_buff);
    update_nxt_poll_time("billing_nxt_poll_time", next_poll_time_buff);
    update_nxt_poll_time("event_nxt_poll_time", next_poll_time_buff);

    init_data_collection();

    // rithika 31Dec2025
    system("/usr/cms/bin/save_redis_config_bgsave.sh");

    printf("poll_cfg.enable_stored_data_poll  : %d\n", poll_cfg.enable_stored_data_poll);

    // rithika 09Dec2025
    for (midx = 0; midx < no_of_meter; midx++)
    {
        if ((ser_port_data[g_pidx].meter_cfg[midx].enable_meter == 1)) // rithika 03dec
        {
            curr_nc_idx = midx;

            int ret = init_dlms(midx);
            if (ret != DLMS_ERROR_CODE_OK)
            {
                dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret %d - %s\n", midx, "init_dlms_proc", ret, (hlp_getErrorMessage(ret)));
            }
            else
            {
                get_inst_data(midx);
                dbg_log(INFORM, "[%-25s] : Present day ls data is going to poll upto current time for midx %d\n", "init_dlms_proc", midx);
                ret = get_ls_data_block(midx, "all", "1");
                if (ret == 0)
                {
                    dbg_log(INFORM, "[%-25s] : Present day ls data polling upto current time for midx %d succeeded\n", "init_dlms_proc", midx);
                }
                else
                {
                    dbg_log(INFORM, "[%-25s] : Present day ls data polling upto current time for midx %d failed\n", "init_dlms_proc", midx);
                }
                dis_connect();
            }
        }
    }

    if (poll_cfg.enable_stored_data_poll == 1)
    {
        // rithika 19nov2025
        next_poll_time_buff = "old data polling in progress";
        update_nxt_poll_time("inst_nxt_poll_time", next_poll_time_buff);
        update_nxt_poll_time("ls_nxt_poll_time", next_poll_time_buff);
        update_nxt_poll_time("mn_nxt_poll_time", next_poll_time_buff);
        update_nxt_poll_time("billing_nxt_poll_time", next_poll_time_buff);
        update_nxt_poll_time("event_nxt_poll_time", next_poll_time_buff);

        get_old_data();

        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            update_old_data_poll_status_in_redis("1");
        }
    }

    idle_poll_phase();
    return 0;
}

/**
 * @brief Fills and initializes the configuration settings for data collection.
 *
 * This function resets various cycle counts for meter metrics and updates
 * the configurations related to features, serial ports, polling, and
 * strategies. It serves as a setup function to ensure all necessary
 * configurations are in place before data collection begins.
 *
 * @return Returns 0 upon successful completion.
 */
int fill_config()
{
    last_rs_check_time = time(NULL);
    inst_meter_metrics.cycleCount = 0;
    ls_meter_metrics.cycleCount = 0;
    mn_meter_metrics.cycleCount = 0;
    bill_meter_metrics.cycleCount = 0;
    event_meter_metrics.cycleCount = 0;

    memset(&plcc_modem_info, 0, sizeof(plcc_modem_info_t));

    printf("============================= Extract configuration from redis ===========================\n");
    update_feature_configuration();

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        update_ser_port_config_plcc();
    }
    else
    {
        update_ser_port_config_serial();
    }

    get_dev_model();
    update_polling_cfg();
    get_polling_strategy();
    printf("==========================================================================================\n");
}

/**
 * @brief Reads the DCU serial number from a configuration file.
 *
 * This function attempts to open a specified file containing the DCU serial number,
 * reads it into a global variable, and logs relevant information.
 *
 * @return int Returns 0 on success, -1 on failure.
 */
int readDcuSerNum(void)
{
    static const char fun_name[] = "readDcuSerNum()"; // Function name for logging

    FILE *p_file_ptr = fopen(DCU_SERIAL_NUM, "r"); // Open file for reading
    if (p_file_ptr == NULL)
    {
        dbg_log(INFORM, "[%-25s] : Unable to open cfg file, Error: %s\n", fun_name, strerror(errno));
        return -1; // Return error code
    }

    // Attempt to read the serial number from the file
    // rithika 01Dec2025
    // if (fscanf(p_file_ptr, "%15[^\n]", dcu_ser_num) != 1)
    if (fscanf(p_file_ptr, "%17[^\n]", dcu_ser_num) != 1)
    {
        fclose(p_file_ptr);
        dbg_log(INFORM, "[%-25s] : Failed to read serial number\n", fun_name);
        return -1; // Return error code on failure to read
    }

    fclose(p_file_ptr); // Close the file after reading

    // rithika 01Dec2025
    if (strcmp(dcu_ser_num, "DCU-DEFAULT-SERNO") == 0)
    {
        strcpy(dcu_ser_num, "DEFAULT");
    }

    dbg_log(INFORM, "[%-25s] : serial_num : %s\n", fun_name, dcu_ser_num);
    return 0; // Success
}

int test(int midx)
{

    const char *json_text = "{\
  \"msgType\": \"OD_ACT_CAL_MESSAGE\",\
  \"seq_no\": \"231\",\
  \"init_source\": \"web\",\
  \"data\": {\
    \"meter\": \"serial_num\",\
    \"val\": {\
      \"cal_name\": \"Passive\",\
      \"num_days\": \"1\",\
      \"day_prof\": [\
        {\
          \"id\": \"1\",\
          \"num_tz\": \"3\",\
          \"time_prof\": [\
            {\
              \"start_time\": \"2025-06-01 10:30:00\",\
              \"table\": \"0.0.10.0.100.255\",\
              \"selc\": \"1\"\
            },\
            {\
              \"start_time\": \"2025-06-01 11:30:00\",\
              \"table\": \"0.0.10.0.100.255\",\
              \"selc\": \"3\"\
            },\
            {\
              \"start_time\": \"2025-06-01 12:30:00\",\
              \"table\": \"0.0.10.0.100.255\",\
              \"selc\": \"2\"\
            }\
          ]\
        }\
      ],\
      \"num_season\": \"1\",\
      \"season\": [\
        {\
          \"season_prof\": {\
            \"season_name\": \"Summer\",\
            \"start_time\": \"2025-06-01 09:30:00\",\
            \"week_prof\": {\
              \"week_name\": \"Week1\",\
              \"week_days\": [1, 1, 1, 1, 1, 1, 1]\
            }\
          }\
        }\
      ]\
    },\
    \"timestamp\": \"dd-mm-yyyy hh:mm:ss\"\
  }\
}";
    cJSON *root = cJSON_Parse(json_text);
    if (!root)
    {
        printf("JSON parsing failed\n");
        return 1;
    }

    activity_calendar cal = {0};

    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *val = cJSON_GetObjectItem(data, "val");

    // Calendar name
    cJSON *cal_name = cJSON_GetObjectItem(val, "cal_name");
    if (cal_name)
        strncpy(cal.calendar_name, cal_name->valuestring, sizeof(cal.calendar_name));

    // ---------- Day Profile ----------
    cJSON *day_prof = cJSON_GetObjectItem(val, "day_prof");
    if (day_prof && cJSON_IsArray(day_prof))
    {
        cal.num_days = cJSON_GetArraySize(day_prof);
        for (int i = 0; i < cal.num_days && i < MAX_DAYS; i++)
        {
            cJSON *day_obj = cJSON_GetArrayItem(day_prof, i);
            cal.day_profile[i].day_id = atoi(cJSON_GetObjectItem(day_obj, "id")->valuestring);
            cal.day_profile[i].max_time_zones = atoi(cJSON_GetObjectItem(day_obj, "num_tz")->valuestring);

            cJSON *time_prof = cJSON_GetObjectItem(day_obj, "time_prof");
            if (time_prof && cJSON_IsArray(time_prof))
            {
                for (int j = 0; j < cJSON_GetArraySize(time_prof) && j < MAX_TIME_ZONES; j++)
                {
                    cJSON *tp = cJSON_GetArrayItem(time_prof, j);
                    strncpy(cal.day_profile[i].timezones[j].day_start_time,
                            cJSON_GetObjectItem(tp, "start_time")->valuestring, 32);
                    strncpy(cal.day_profile[i].timezones[j].script_table,
                            cJSON_GetObjectItem(tp, "table")->valuestring, 64);
                    cal.day_profile[i].timezones[j].script_selector =
                        atoi(cJSON_GetObjectItem(tp, "selc")->valuestring);
                }
            }
        }
    }

    // ---------- Season Profile ----------
    cJSON *season_arr = cJSON_GetObjectItem(val, "season");
    if (season_arr && cJSON_IsArray(season_arr))
    {
        cal.num_seasons = cJSON_GetArraySize(season_arr);
        for (int i = 0; i < cal.num_seasons && i < MAX_SEASONS; i++)
        {
            cJSON *season_obj = cJSON_GetArrayItem(season_arr, i);
            cJSON *season_prof = cJSON_GetObjectItem(season_obj, "season_prof");

            strncpy(cal.season_profile[i].season_name,
                    cJSON_GetObjectItem(season_prof, "season_name")->valuestring, 32);
            strncpy(cal.season_profile[i].start_time,
                    cJSON_GetObjectItem(season_prof, "start_time")->valuestring, 64);
            strncpy(cal.season_profile[i].week_name,
                    cJSON_GetObjectItem(season_prof, "week_prof")->child->valuestring, 32);

            // Save week profile
            cJSON *week_prof = cJSON_GetObjectItem(season_prof, "week_prof");
            cJSON *week_name = cJSON_GetObjectItem(week_prof, "week_name");
            cJSON *week_days = cJSON_GetObjectItem(week_prof, "week_days");

            if (week_prof && cJSON_IsArray(week_days))
            {
                cal.num_weeks = 1;
                strncpy(cal.week_profile[0].week_name, week_name->valuestring, 32);
                for (int j = 0; j < cJSON_GetArraySize(week_days) && j < 7; j++)
                {
                    cal.week_profile[0].weekday[j] = cJSON_GetArrayItem(week_days, j)->valueint;
                }
            }
        }
    }

    // ------- PRINT ALL VALUES ---------
    printf("Calendar Name: %s\n", cal.calendar_name);
    printf("\n-- Day Profiles (%d) --\n", cal.num_days);
    for (int i = 0; i < cal.num_days; i++)
    {
        printf("Day ID: %d\n", cal.day_profile[i].day_id);
        for (int j = 0; j < cal.day_profile[i].max_time_zones; j++)
        {
            printf("  Time Zone %d:\n", j + 1);
            printf("    Start Time : %s\n", cal.day_profile[i].timezones[j].day_start_time);
            printf("    Script Table: %s\n", cal.day_profile[i].timezones[j].script_table);
            printf("    Selector   : %d\n", cal.day_profile[i].timezones[j].script_selector);
        }
    }

    printf("\n-- Season Profiles (%d) --\n", cal.num_seasons);
    for (int i = 0; i < cal.num_seasons; i++)
    {
        printf("Season Name: %s\n", cal.season_profile[i].season_name);
        printf("Start Time : %s\n", cal.season_profile[i].start_time);
        printf("Week Name  : %s\n", cal.season_profile[i].week_name);
    }

    printf("\n-- Week Profiles (%d) --\n", cal.num_weeks);
    for (int i = 0; i < cal.num_weeks; i++)
    {
        printf("Week Name: %s\n", cal.week_profile[i].week_name);
        printf("Week Days: ");
        for (int j = 0; j < 7; j++)
        {
            printf("%d ", cal.week_profile[i].weekday[j]);
        }
        printf("\n");
    }

    cJSON_Delete(root);

    // memset(temp_json,0,sizeof(temp_json));

    // printf("**********************************************************************************************\n");

    // printf("temp json : %d\n",sizeof(temp_json));

    // activity_calnd(midx, cal);

    // printf("after func temp json : %s\n",temp_json);

    return 0;
}

int checkDbgStatus(void)
{

    static char fun_name[] = "checkDbgStatus()";
    struct stat sb;
    FILE *fp = NULL;

    memset(&sb, 0, sizeof(sb));

    if (stat(DBGCFG_FILE_NAME, &sb) == -1)
    {
        perror("stat");
        dbgloglevel = 1;
    }

    if ((dbgcfgtime == 0) || (dbgcfgtime != sb.st_mtime))
    {
        dbgcfgtime = sb.st_mtime;

        fp = fopen(DBGCFG_FILE_NAME, "r");
        if (fp != NULL)
        {
            fscanf(fp, "%d", &dbgloglevel);
            fclose(fp);
            printf("debug level %d\n", dbgloglevel);
            dbg_log(REPORT, "%s:debug log level changed %d\n", fun_name, dbgloglevel);
        }
    }

    return 0;
}

/**
 * @brief Main entry point for the DCU DLMS Meter Processing application.
 *
 * This function initializes the application by setting up necessary configurations,
 * establishing database and Redis connections, and starting the data collection process.
 *
 * @param argc Argument count
 * @param argv Argument vector containing command-line parameters
 * @return Returns 0 on success, or a non-zero error code on failure.
 */
int main(int argc, char *argv[])
{

    static char fun_name[] = "main()";

    snrm_aarq_failed_cnt("START TIME  : %s.\n", time_format_conversion(time(NULL)));

    if (argc < 2)
    {
        dbg_log(REPORT, "[%-25s] :Usase Msg !!! , please Use ./dcuDaModule PortNo ..\n", fun_name);
        printf("[%-25s] :Usase Msg !!! , please Use ./dcuDaModule PortNo ..\n", fun_name);
        return -1;
    }
printf("main");
    g_pidx = atoi(argv[1]); // rithika

    
    initialize_log_filenames();

    redis = redisConnect(REDIS_HOST_NAME, REDIS_PORT);
    if (redis == NULL || redis->err)
    {
        if (redis)
        {
            dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
            redisFree(redis);
        }
        else
        {
            dbg_log(REPORT, "[%-25s] :Redis connection error: can't allocate redis context\n", fun_name);
        }
        return -1;
    }
    else
    {
        dbg_log(INFORM, "[%-25s] :Redis connected successfully\n", fun_name);
    }

    if (RECONNECT)
    {
        char redis_key[64];
         snprintf(redis_key, sizeof(redis_key), "DA_SER:%d", g_pidx);
        dcu_netlog_init(redis_key);
    }
    
    // rithika 15nov2025
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        update_serial_status_in_redis("0");
    }
    add_process_diag("************************************ DA_MODULE STARTED ************************************");

    dbg_log(INFORM, "[%-25s] :***************** DA_MODULE STARTED :%d *****************\n", fun_name, OD_CHANGES);

    // g_pidx = atoi(argv[1]); rithika 23sep

    switch (g_pidx)
    {
    case 0:
        strcpy(p_comm_port_det, COMM_PORT0_DET);
        break;

    case 1:
        strcpy(p_comm_port_det, COMM_PORT1_DET);
        break;

    case 2:
        strcpy(p_comm_port_det, COMM_PORT2_DET);
        break;

    case 3:
        strcpy(p_comm_port_det, COMM_PORT3_DET);
        break;

    default:
        dbg_log(REPORT, "[%-25s] :Invalid Port Recvd!!\n", fun_name);
        return -1;
    }

    readDcuSerNum();

    // np_polled_comp = 1;
    // meter_commn_status(0,0);
    // exit(0);

    fill_config();

    // rithika 22sep
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        g_pidx += 1;
    }

    check_ftp_enable(); // rithika 28/08/2025

    printf("main -------------------> num_dev         :  %d\n", ser_port_data[g_pidx].num_dev);

    // get_old_data();
    // process_od_cmd();
    // exit(0);

    dbg_log(INFORM, "[%-25s] :Config extract completed.\n", fun_name);

    dbg_log(INFORM, "[%-25s] :Meter inf type : %s\n", fun_name, feature_cfg.meter_inf_type);

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        add_process_diag("Interface type PLCC");
        init_plcc_modem(0);
        init_serial_port(p_comm_port_det);
        discover_meter_manf();
    }
    else
    {
        add_process_diag("Interface type SERIAL");

        init_serial_port(p_comm_port_det);
        // discover_meter_manf();
    }

    printf("===================================================\n");
    create_sd_log_dir();

    init_dlms_proc();

    // test_inst_data();
    // get_inst_data(0);

    // idle_poll_phase();

    // process_od_cmd();

    // init_dlms(curr_nc_idx);

    //  while (1)
    // {
    //     init_dlms(curr_nc_idx);
    //     get_name_plate(curr_nc_idx);
    //     process_od_cmd();
    //     sleep(1);
    // }

#if 0

    // process_od_cmd();
    // clock_t start, end;
    curr_nc_idx = 0;

    // while (1)
    // {
        // init_dlms(curr_nc_idx);
        // get_name_plate(curr_nc_idx);
        test_inst_data();
        sleep(5);
    // }

    // test(curr_nc_idx);

    // process_od_cmd();

    // char in_buf[256];

    // instant_push_setup(curr_nc_idx, "[2402:3a80:1700:92:88fc:1051:7b72:4d3b]:4059", in_buf);

    // alert_push_setup(curr_nc_idx, "[2402:3a80:1700:92:88fc:1051:7b72:4d3b]:4059",in_buf);

    // set_apn(curr_nc_idx,"airtelgprs.com",6,in_buf);

    // single_action_schl_billing(curr_nc_idx, "2025-05-07 12:00:00", in_buf);

    // init_dlms(curr_nc_idx);

    // get_ls_data_block(curr_nc_idx, "all", "3 ");

    init_dlms(curr_nc_idx);

    get_name_plate(curr_nc_idx);

    init_dlms(curr_nc_idx);

    get_inst_data(curr_nc_idx);

    init_dlms(curr_nc_idx);

    get_billing_data(curr_nc_idx,"current");

    init_dlms(curr_nc_idx);

    get_billing_data(curr_nc_idx,"all");

    init_dlms(curr_nc_idx);

    get_midnight_data(curr_nc_idx, "2");

    init_dlms(curr_nc_idx);

    get_midnight_data(curr_nc_idx, "30");

    init_dlms(curr_nc_idx);

    get_event_data(curr_nc_idx, "event_data_cat1", "100");

    get_event_data(curr_nc_idx, "event_data_cat2", "100");

    get_event_data(curr_nc_idx, "event_data_cat3", "100");

    get_event_data(curr_nc_idx, "event_data_cat4", "100");

    get_event_data(curr_nc_idx, "event_data_cat5", "100");

    get_event_data(curr_nc_idx, "event_data_cat6", "100");

    get_event_data(curr_nc_idx, "event_data_cat7", "100");

    init_dlms(curr_nc_idx);

    get_event_data(curr_nc_idx, "event_data_cat1", "50");

    get_event_data(curr_nc_idx, "event_data_cat2", "50");

    get_event_data(curr_nc_idx, "event_data_cat3", "50");

    get_event_data(curr_nc_idx, "event_data_cat4", "50");

    get_event_data(curr_nc_idx, "event_data_cat5", "50");

    get_event_data(curr_nc_idx, "event_data_cat6", "50");

    get_event_data(curr_nc_idx, "event_data_cat7", "50");
    
    init_dlms(curr_nc_idx);
   
    get_event_data(curr_nc_idx, "event_data_cat1", "10");

    get_event_data(curr_nc_idx, "event_data_cat2", "10");

    get_event_data(curr_nc_idx, "event_data_cat3", "10");

    
    get_event_data(curr_nc_idx, "event_data_cat4", "10");

    init_dlms(curr_nc_idx);
    get_event_data(curr_nc_idx, "event_data_cat5", "10");

    init_dlms(curr_nc_idx);
   
    get_event_data(curr_nc_idx, "event_data_cat6", "10");

    init_dlms(curr_nc_idx);

    get_event_data(curr_nc_idx, "event_data_cat7", "10");

    
    // get_billing_data(curr_nc_idx,"current");

    // get_midnight_data(curr_nc_idx, "1");

    // init_dlms(curr_nc_idx);

    // init_dlms(curr_nc_idx);

   

    // get_inst_data(curr_nc_idx);


    
    // get_inst_data(curr_nc_idx);
    // get_ls_data_day(curr_nc_idx, "01-02-2025", "1");
    

    // set_demand_period(curr_nc_idx,&in_buf);

    // set_clock_time(curr_nc_idx,2025,4,30,13,10,30,in_buf);

    // set_profile_capture_period(curr_nc_idx,1800,&in_buf);

    // set_metering_mode(curr_nc_idx,0,&in_buf);

    // set_payment_mode(curr_nc_idx,1,&in_buf);

    // set_lst_token_recharge_amt(curr_nc_idx,1258,&in_buf);

    // set_tot_amt_lst_recharge(curr_nc_idx,456,&in_buf);

    // set_curr_bal_amt(curr_nc_idx,500,&in_buf);

    // set_relay_op_overload_overcurr(curr_nc_idx,0,&in_buf);

    // enbl_disbl_load_limit_fn(curr_nc_idx,4,&in_buf);

    // set_md_reset(curr_nc_idx, 0, &in_buf);

    // set_eswf(curr_nc_idx, "0", &in_buf);

    // set_load_limit(curr_nc_idx,4,&in_buf);

    // get_old_data();

    // get_curr_date_time(curr_nc_idx);
    // exit(0);

    // unsigned char loadStatus;

    // int result = get_load_status(curr_nc_idx, &loadStatus);
    // if (result == 0)
    // {
    //     if (loadStatus)
    //     {
    //         dbg_log(REPORT, "%s: [Meter_%d] :  Load status -  Connected.\n", fun_name, curr_nc_idx);
    //     }
    //     else
    //     {
    //         dbg_log(REPORT, "%s: [Meter_%d] :  Load status -  Disonnected.\n", fun_name, curr_nc_idx);
    //     }
    // }
    // exit(0);
    // read_scalar_values(0);
    // config_param();

    // fill_config();
    //  config_parameter();

    // for (int i = 0; i < 3; i++)
    // {
    // get_inst_data(curr_nc_idx);
    // }
    // get_name_plate(curr_nc_idx);
    // get_midnight_data(curr_nc_idx, "30");
    // sleep(10);
    // get_midnight_data(curr_nc_idx, "30");
    // get_event_data(curr_nc_idx, "event_data_cat3", "all");

    // curr_nc_idx = 1;
    // init_dlms(curr_nc_idx);

    // get_name_plate(curr_nc_idx);
    // // get_billing_data(curr_nc_idx,"all");
    // get_event_data(curr_nc_idx, "event_data_cat1", "all");

    // get_midnight_data(curr_nc_idx, "30");
    // init_dlms(curr_nc_idx);
    // get_ls_data_day(curr_nc_idx, "28-03-2025", "1");

    // sleep(5);

    // init_dlms(curr_nc_idx);
    // get_ls_data_day(curr_nc_idx, "12-03-2025", "1");
    // init_dlms(curr_nc_idx);
    // get_ls_data_day(curr_nc_idx, "31-03-2025", "1");
    // init_dlms(curr_nc_idx);
    // get_ls_data_day(curr_nc_idx, "26-03-2025", "1");
    // init_dlms(curr_nc_idx);
    // get_ls_data_day(curr_nc_idx, "27-03-2025", "1");
    // get_ls_data_block(curr_nc_idx, "all", "3 ");
    // get_event_data(curr_nc_idx, "event_data_cat1", "10");
    // sleep(20);
    // get_event_data(curr_nc_idx, "event_data_cat1", "100");
    // sleep(10);
    // get_event_data(curr_nc_idx, "event_data_cat2", "10");
    // sleep(20);
    // get_event_data(curr_nc_idx, "event_data_cat2", "100");
    // sleep(10);
    
   
//     // get_event_data(curr_nc_idx, "event_data_cat3", "100");
//     // sleep(10);
//     get_event_data(curr_nc_idx, "event_data_cat4", "10");
//    sleep(20);
//     // get_event_data(curr_nc_idx, "event_data_cat4", "100");
//     // sleep(10);
//     get_event_data(curr_nc_idx, "event_data_cat5", "10");
//     sleep(20);
//     // get_event_data(curr_nc_idx, "event_data_cat5", "100");
//     // sleep(10);
//     get_event_data(curr_nc_idx, "event_data_cat6", "10");
//     sleep(20);
//     // get_event_data(curr_nc_idx, "event_data_cat6", "100");
//     // sleep(10);
//     get_event_data(curr_nc_idx, "event_data_cat7", "10");
//     sleep(20);
    // get_event_data(curr_nc_idx, "event_data_cat7", "100");
    // sleep(10);
    // int cnt;
    // char temp_buf[256];

    // for (cnt = 0; cnt < 6; cnt++)
    // {
    //     memset(temp_buf, 0, sizeof(temp_buf));
    //     sprintf(temp_buf, "event_data_cat%d", cnt + 1);
    //     get_event_data(curr_nc_idx, temp_buf, poll_cfg.num_of_events);
    // }

    // get_name_plate(curr_nc_idx);
    // get_manufacturer(0);
#endif

    return 0;
}
