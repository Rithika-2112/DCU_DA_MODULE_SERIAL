#include "../include/general_config.h"
#include "../include/dlms_config.h"
#include "../include/plcc_rel.h"
#include "../include/meter_params.h"

#define DBL_QUOTES '"'
#define HASH_KEY "plcc_modem_info"
#define MAX_DIAG_MESSAGE_LIMIT 100
#define DIAG_ENBL_HASH "diag_enable"

redisContext *redis;

extern feature_cfg_t feature_cfg;
extern ser_port_channel_t ser_port_data[MAX_NO_OF_SERIAL_PORT];
extern strategy_cfg_t strategy_cfg;

extern char meter_time_str[MAX_NO_OF_METER_PER_PORT][20]; // rithika 03/09

events_data_t redis_event_data;
poll_cfg_t poll_cfg;
od_cmd_t od_cmd;

event_obis_t event_obis; // rithika 9Sep
int obis_count;

int ftp_enable = 0;

static tcflag_t baudrate[16] = {0, B50, B75, B110, B200, B300, B600, B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200};
char parity[3][8] = {"none", "odd", "even"};
char json_buff[4096];
char json_send_buff[4096];
char time_str[32];
char event_msg_str[128];

int OD_CMD_RECEIVED = 0;

extern uint8_t name_plate_obis[MAX_NAMEPLATE_VALS][6]; // rithika 17Feb2026
extern uint8_t g_pidx;
extern int curr_nc_idx;
extern char dcu_ser_num[16];

extern char *time_format_conversion(time_t);
extern int get_all_meter_inst_data();

//  uint8_t np_polled_comp;
extern uint8_t np_polled_comp[MAX_NO_OF_METER_PER_PORT];

// rithika 26sep
int midx_od_ftp;

// rithika 13nov2025
char dev_model[64];

int add_event_data(const events_data_t *);
// char *get_manufacturer(int, char *);
int get_manufacturer(int, char *, char *, int);
// char *get_meter_2_ser_num(int);
int get_meter_2_ser_num(int, char *, int);
int extract_block_int(const char *);

// rithika 01/12/2025
char one_day_str[32];

typedef struct
{
    int code;
    const char *message;
} OdError;

#if RECONNECT
OdError od_errors[] = {
    {0, "SUCCESS"},
    {-1, "Data Read Initialization Failed"},
    {-2, "Data Obis read Failed"},
    {-3, "Data read Failed"},
    {-4, "Failed to get date value"},
    {-5, "Failed - CURLE_RTSP_SESSION_ERROR"},
    {-6, "Failed - CURLE_FTP_BAD_FILE_LIST"},
    {-7, "SNRM-AARQ Failed"},
    {3, "Failed - Device reports Read-Write denied"},
    {8, "Failed - wrong date format"},
    {9, "Failed -Remote directory or file not found"},
    {-4, "Failed - Select error(At port)"},
    {-10, "PLCC - Init Serial Port Failed"},
    {-11, "Failed - Invaild Resp len"},
    {-12, "Failed - Soft reset - Invaild resp"},
    {-13, "Failed - PLCC - Invalid start byte"},
    {-14, "Failed - PLCC - Invalid Packet Type"},
    {-15, "Failed - PLCC - Invalid RS SN"},
    {-16, "Failed - No Data"},
    {-17, "Failed to write command"}};

#else
// Define the array of curl error messages
OdError od_errors[] = {
    {0, "SUCCESS"},
    {-1, "Data Read Initialization Failed"},
    {-2, "Data Obis read Failed"},
    {-3, "Data read Failed"},
    {-4, "Failed to get date value"},
    {-5, "CURLE_RTSP_SESSION_ERROR"},
    {-6, "CURLE_FTP_BAD_FILE_LIST"},
    {-7, "SNRM-AARQ Failed"},
    {3, "Device reports Read-Write denied"},
    {8, "Failed - wrong date format"},
    {9, "Remote directory or file not found"},
    {-4, "Select error(At port)"},
    {-10, "PLCC - Init Serial Port Failed"},
    {-11, "Failed - Invaild Resp len"},
    {-12, "Failed - Soft reset - Invaild resp"},
    {-13, "Failed - PLCC - Invalid start byte"},
    {-14, "Failed - PLCC - Invalid Packet Type"},
    {-15, "Failed - PLCC - Invalid RS SN"},
    {-16, "Failed - No Data"},
    {-17, "Failed to write command"}};
#endif


#define NUM_ERRORS (sizeof(od_errors) / sizeof(OdError))

char temp_json[2048];
// rithika 10Dec2025
double meter_drift_sec[MAX_NO_OF_METER_PER_PORT] = {0};

/**
 * Retrieves the error message corresponding to a given CURL error code.
 *
 * This function iterates through a predefined array of error codes and
 * their corresponding messages. If a match is found, it returns the
 * associated error message. If no match is found, it returns a default
 * message indicating an unknown error.
 *
 * @param code The CURL error code for which to retrieve the message.
 * @return A pointer to a string containing the corresponding error message,
 *         or "Unknown CURL error" if the code is not recognized.
 */
const char *get_curl_error_message(int code)
{
    int i;

    for (i = 0; i < NUM_ERRORS; i++)
    {
#if DEBUG
        printf("%d %d %s\n", od_errors[i].code, code, od_errors[i].message);
#endif
        if (od_errors[i].code == code)
        {
            return od_errors[i].message;
        }
    }
    return "Unknown error";
}

/**
 * Converts a time_t value to a formatted string representation.
 *
 * This function takes a time_t value (representing a time in seconds since the
 * Unix epoch), converts it to a struct tm representation, and then formats it
 * into a string of the form "YYYY-MM-DD HH:MM:SS".
 *
 * @param sample_time The time_t value to be converted.
 * @return A pointer to a string containing the formatted date and time.
 */
char *time_format_conversion(time_t sample_time)
{
    struct tm info_time;
    memset(time_str, 0, sizeof(time_str));                                           // Clear the time_str buffer
    strptime(strtok(ctime(&sample_time), "\n"), "%a %b %d %H:%M:%S %Y", &info_time); // Parse the time
    strftime(time_str, 32, "%Y-%m-%d %H:%M:%S", &info_time);                         // Format the time into the desired format
#if 0
    printf("Current date and time %s\n", time_str); // Debug output
#endif
    return time_str; // Return the formatted time string
}

int check_ftp_enable()
{
    static char fun_name[] = "check_ftp_enable()";

    redisReply *reply;
    reply = redisCommand(redis, "HGET ftp_cfg ftp_enable");

    if (reply == NULL)
    {
        fprintf(stderr, "Redis command failed: %s\n", redis->errstr);
        return -1;
    }
    if (reply->type == REDIS_REPLY_STRING && reply->str != NULL)
    {
        ftp_enable = atoi(reply->str);
    }
    freeReplyObject(reply);
}

/**
 * Updates the feature configuration by retrieving values from a Redis hash.
 *
 * This function connects to a Redis database, retrieves all key-value pairs
 * from a specified hash, and updates the feature configuration structure
 * accordingly. It also handles errors and logs the relevant information.
 *
 * @return Returns 0 on successful completion.
 */
int update_feature_configuration()
{
    static char fun_name[] = "update_feature_configuration()";

    redisReply *reply;
    char key[64];
    int i;

    sprintf(key, "features_cfg");

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    reply = redisCommand(redis, "EXISTS %s", key);
    if (reply->integer == 0)
    {
        printf("Key '%s' does not exist in Redis.\n", key);
        freeReplyObject(reply);
        return -1;
    }

    reply = redisCommand(redis, "HGETALL %s", key);

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        for (i = 0; i < reply->elements; i += 2)
        {
            const char *field = reply->element[i]->str;
            const char *value = reply->element[i + 1]->str;
#if 0
            printf("Field: %s\n", field);
            printf("Value: %s\n", value);
#endif
            if (strcmp(field, "num_ser_ports") == 0)
            {
                feature_cfg.num_ser_ports = atoi(value);
            }
            else if (strcmp(field, "num_eth_ports") == 0)
            {
                feature_cfg.num_eth_ports = atoi(value);
            }
            else if (strcmp(field, "num_ipsec_tunnels") == 0)
            {
                feature_cfg.num_ipsec_tunnels = atoi(value);
            }
            else if (strcmp(field, "num_ftp_instances") == 0)
            {
                feature_cfg.num_ftp_instances = atoi(value);
            }
            else if (strcmp(field, "meter_inf_type") == 0)
            {
                strcpy(feature_cfg.meter_inf_type, value);
                feature_cfg.meter_inf_type[sizeof(feature_cfg.meter_inf_type) - 1] = '\0';
            }
        }
    }
    else
    {
        printf("Error retrieving hash\n");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Error retrieving hash", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
    }

    freeReplyObject(reply);

#if DEBUG
    printf("num_ser_ports       : %d\n", feature_cfg.num_ser_ports);
    printf("num_eth_ports       : %d\n", feature_cfg.num_eth_ports);
    printf("num_ipsec_tunnels   : %d\n", feature_cfg.num_ipsec_tunnels);
    printf("num_ftp_instances   : %d\n", feature_cfg.num_ftp_instances);
    printf("meter_inf_type      : %s\n", feature_cfg.meter_inf_type);
#endif

    return 0;
}

/**
 * Updates the serial port configuration by retrieving values from a Redis hash.
 *
 * This function retrieves configuration data related to serial ports from Redis,
 * updates the corresponding structures, and logs any errors encountered during the
 * process.
 *
 * @return Returns 0 on successful completion.
 */
// int update_ser_port_config()
// {
//     static char fun_name[] = "update_ser_port_config()";
//     redisReply *reply;
//     char key[64];
//     int i, j;

//     if (redis == NULL || redis->err)
//     {
//         dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", redis->errstr);
//         return -1;
//     }

//     printf("num_ser_ports       : %d\n", feature_cfg.num_ser_ports);
//     for (j = 1; j <= feature_cfg.num_ser_ports; j++)
//     {
//         memset(key, 0, sizeof(key));
//         sprintf(key, "serial_port_%d_cfg", j);

//         reply = redisCommand(redis, "EXISTS %s", key);
//         if (reply->integer == 0)
//         {
//             printf("Key '%s' does not exist in Redis.\n", key);
//             freeReplyObject(reply);
//             return -1;
//         }

//         // reply = redisCommand(redis, "HGET SERIAL_PORT_id_CFG num_dev");
//         // if (reply == NULL)
//         // {
//         //     memset(event_msg_str, 0, sizeof(event_msg_str));
//         //     sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
//         //     strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
//         //     add_event_data(&redis_event_data);
//         //     return NULL;
//         // }

//         printf("key %s reply->str : %d\n", key, reply->type);

//         // char *result = NULL;
//         // if (reply->type == REDIS_REPLY_STRING)
//         // {
//         //     result = strdup(reply->str);
//         //     ser_port_data[j].num_dev = atoi(result);
//         // }
//         // freeReplyObject(reply);

//         reply = redisCommand(redis, "HGETALL %s", key);

//         if (reply->type == REDIS_REPLY_ARRAY)
//         {
//             for (i = 0; i < reply->elements; i += 2)
//             {
//                 const char *field = reply->element[i]->str;
//                 const char *value = reply->element[i + 1]->str;
// #if 1
//                 printf("Field: %s\n", field);
//                 printf("Value: %s\n", value);
// #endif
//                 if (strcmp(field, "enable_port") == 0)
//                 {
//                     ser_port_data[j].ser_port_cfg.enable_port = (unsigned char)atoi(value);
//                 }
//                 else if (strcmp(field, "port_name") == 0)
//                 {
//                     strcpy(ser_port_data[j].ser_port_cfg.port_name, value);
//                     ser_port_data[j].ser_port_cfg.port_name[sizeof(ser_port_data[j].ser_port_cfg.port_name) - 1] = '\0';
//                 }
//                 else if (strcmp(field, "serial_port_id") == 0)
//                 {
//                     ser_port_data[j].ser_port_cfg.ser_portid = (unsigned char)atoi(value);
//                 }
//                 else if (strcmp(field, "baudrate") == 0)
//                 {
//                     ser_port_data[j].ser_port_cfg.baud_rate = atoi(value);

//                     // strcpy(ser_port_data[j].ser_port_cfg.baud_rate, value);
//                     // ser_port_data[j].ser_port_cfg.baud_rate[sizeof(ser_port_data[j].ser_port_cfg.baud_rate) - 1] = '\0';
//                 }
//                 else if (strcmp(field, "databits") == 0)
//                 {
//                     ser_port_data[j].ser_port_cfg.data_bits = (unsigned char)atoi(value);
//                 }
//                 else if (strcmp(field, "stopbits") == 0)
//                 {
//                     ser_port_data[j].ser_port_cfg.stop_bits = (unsigned char)atoi(value);
//                 }
//                 else if (strcmp(field, "parity") == 0)
//                 {
//                     ser_port_data[j].ser_port_cfg.parity = (unsigned char)atoi(value);
//                 }
//                 else if (strcmp(field, "handshake") == 0)
//                 {
//                     // ser_port_data[j].ser_port_cfg.handshake = (unsigned char)atoi(value);
//                     strcpy(ser_port_data[j].ser_port_cfg.handshake, value);
//                     ser_port_data[j].ser_port_cfg.handshake[sizeof(ser_port_data[j].ser_port_cfg.handshake) - 1] = '\0';
//                 }
//                 else if (strcmp(field, "infmode") == 0)
//                 {
//                     // ser_port_data[j].ser_port_cfg.inf_mode = (unsigned char)atoi(value);
//                     strcpy(ser_port_data[j].ser_port_cfg.inf_mode, value);
//                     ser_port_data[j].ser_port_cfg.inf_mode[sizeof(ser_port_data[j].ser_port_cfg.inf_mode) - 1] = '\0';
//                 }
//                 else if (strcmp(field, "num_dev") == 0)
//                 {
//                     ser_port_data[j].num_dev = atoi(value);
//                 }
//                 else if (strcmp(field, "meter_make") == 0)
//                 {
//                     strcpy(ser_port_data[j].meter_make, value); // doubt
//                 }

//                 if (strncmp(field, "enable_meter[", 13) == 0)
//                 {
//                     int index = atoi(field + 13);
//                     printf("index : %d : %d\n", index, ser_port_data[j].num_dev);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         ser_port_data[j].meter_cfg[index].enable_device = (unsigned char)atoi(value);
//                         printf("-------------------------- Meter[%d] Enable_device: %u\n", i, ser_port_data[j].meter_cfg[index].enable_device);
//                     }
//                 }
//                 else if (strncmp(field, "retries[", 8) == 0)
//                 {
//                     int index = atoi(field + 8);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         char *endptr;
//                         ser_port_data[j].meter_cfg[index].num_retries = (unsigned char)atoi(value);
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].num_retries: %u\n", i, ser_port_data[j].meter_cfg[index].num_retries);
//                     }
//                 }
//                 else if (strncmp(field, "resp_timeout[", 13) == 0)
//                 {
//                     int index = atoi(field + 13);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         ser_port_data[j].meter_cfg[index].resp_timeouts = atoi(value);
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].resp_timeouts: %u\n", i, ser_port_data[j].meter_cfg[index].resp_timeouts);
//                     }
//                 }
//                 else if (strncmp(field, "apply_ctpt_ratio[", 17) == 0)
//                 {
//                     int index = atoi(field + 17);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         ser_port_data[j].meter_cfg[index].apply_ctpt_ratio = (unsigned char)atoi(value);
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].apply_ctpt_ratio: %u\n", i, ser_port_data[j].meter_cfg[index].apply_ctpt_ratio);
//                     }
//                 }
//                 else if (strncmp(field, "ct_ratio[", 9) == 0)
//                 {
//                     int index = atoi(field + 9);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         ser_port_data[j].meter_cfg[index].ct_ratio = (unsigned char)atoi(value);
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].ct_ratio: %u\n", i, ser_port_data[j].meter_cfg[index].ct_ratio);
//                     }
//                 }
//                 else if (strncmp(field, "pt_ratio[", 9) == 0)
//                 {
//                     int index = atoi(field + 9);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         ser_port_data[j].meter_cfg[index].pt_ratio = (unsigned char)atoi(value);
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].pt_ratio: %u\n", i, ser_port_data[j].meter_cfg[index].pt_ratio);
//                     }
//                 }
//                 else if (strncmp(field, "meter_addr[", 11) == 0)
//                 {
//                     int index = atoi(field + 11);
//                     printf("-----index : %d j = %d\n", index, j);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         ser_port_data[j].meter_cfg[index].meter_addr = atoi(value);
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].meter_addr: %u\n", i, ser_port_data[j].meter_cfg[index].meter_addr);
//                     }
//                 }
//                 else if (strncmp(field, "meter_addr_size[", 16) == 0)
//                 {
//                     int index = atoi(field + 16);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         ser_port_data[j].meter_cfg[index].meter_addr_size = (unsigned char)atoi(value);
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].meter_addr_size: %u\n", i, ser_port_data[j].meter_cfg[index].meter_addr_size);
//                     }
//                 }
//                 else if (strncmp(field, "auth_type[", 10) == 0)
//                 {
//                     int index = atoi(field + 10);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         // ser_port_data[j].meter_cfg[index].auth_type = (unsigned char)atoi(value);
//                         strcpy(ser_port_data[j].meter_cfg[index].auth_type, value);
//                         ser_port_data[j].meter_cfg[index].auth_type[sizeof(ser_port_data[j].meter_cfg[index].auth_type) - 1] = '\0';

//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].auth_type %s\n", i, ser_port_data[j].meter_cfg[index].auth_type);
//                     }
//                 }
//                 else if (strncmp(field, "password[", 9) == 0)
//                 {
//                     int index = atoi(field + 9);
//                     printf("||||||||||||||||||||  index : %d value %s |||||||||||||||\n\n\n\n", index, value);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         memset(ser_port_data[j].meter_cfg[index].met_pw, 0, sizeof(ser_port_data[j].meter_cfg[index].met_pw));
//                         strncpy(ser_port_data[j].meter_cfg[index].met_pw, value, sizeof(ser_port_data[j].meter_cfg[index].met_pw) - 1);
//                         ser_port_data[j].meter_cfg[index].met_pw[sizeof(ser_port_data[j].meter_cfg[index].met_pw) - 1] = '\0';

//                         // strcpy(ser_port_data[j].meter_cfg[index].met_pw, value);
//                         ser_port_data[j].meter_cfg[index].met_pw[sizeof(ser_port_data[j].meter_cfg[index].met_pw) - 1] = '\0';
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].met_pw: %s\nindex %d", i, ser_port_data[j].meter_cfg[index].met_pw, index);
//                     }
//                 }
//                 else if (strncmp(field, "sys_title[", 10) == 0)
//                 {
//                     int index = atoi(field + 10);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         strcpy(ser_port_data[j].meter_cfg[index].sys_title, value);
//                         ser_port_data[j].meter_cfg[index].sys_title[sizeof(ser_port_data[j].meter_cfg[index].auth_type) - 1] = '\0';
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].sys_title: %s\n", i, ser_port_data[j].meter_cfg[index].sys_title);
//                     }
//                 }
//                 else if (strncmp(field, "cipher_key[", 11) == 0)
//                 {
//                     int index = atoi(field + 11);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         strcpy(ser_port_data[j].meter_cfg[index].cipher_key, value);
//                         ser_port_data[j].meter_cfg[index].cipher_key[sizeof(ser_port_data[j].meter_cfg[index].auth_type) - 1] = '\0';
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].cipher_key: %s\n", i, ser_port_data[j].meter_cfg[index].cipher_key);
//                     }
//                 }
//                 else if (strncmp(field, "auth_key[", 9) == 0)
//                 {
//                     int index = atoi(field + 9);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         strcpy(ser_port_data[j].meter_cfg[index].auth_key, value);
//                         ser_port_data[j].meter_cfg[index].auth_key[sizeof(ser_port_data[j].meter_cfg[index].auth_type) - 1] = '\0';
//                         printf("-------------------------- Meter[%d] ser_port_data[j].meter_cfg[index].auth_key: %s\n", i, ser_port_data[j].meter_cfg[index].auth_key);
//                     }
//                 }
//                 else if (strncmp(field, "meter_loc[", 10) == 0)
//                 {
//                     int index = atoi(field + 10);
//                     printf("index : %d\n", index);
//                     if (index >= 0 && index <= ser_port_data[j].num_dev)
//                     {
//                         strcpy(ser_port_data[j].meter_cfg[index].meter_loc, value);
//                         printf("Meter[%d] meter_loc: %s\n", i, ser_port_data[j].meter_cfg[index].meter_loc);
//                         // ser_port_data[j].meter_cfg[index].meter_loc[sizeof(ser_port_data[j].meter_cfg[index].auth_type) - 1] = '\0';
//                     }
//                 }
//             }
//         }
//         else
//         {
//             printf("Error retrieving hash\n");

//             memset(event_msg_str, 0, sizeof(event_msg_str));
//             sprintf(redis_event_data.eventmsg, "%s : Error retrieving hash", fun_name);
//             strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
//             add_event_data(&redis_event_data);
//         }
//         freeReplyObject(reply);
// #if DEBUG
// int k;
//         for (k = 1; k <= ser_port_data[j].num_dev; k++)
//         {
//             printf("enable_port     : %u j = %d\n", ser_port_data[k].ser_port_cfg.enable_port, k);
//             printf("port_name       : %s j = %d\n", ser_port_data[k].ser_port_cfg.port_name, k);
//             printf("ser_portid      : %u\n", ser_port_data[k].ser_port_cfg.ser_portid);
//             printf("baud_rate       : %d\n", ser_port_data[k].ser_port_cfg.baud_rate);
//             printf("data_bits       : %u\n", ser_port_data[k].ser_port_cfg.data_bits);
//             printf("stop_bits       : %u\n", ser_port_data[k].ser_port_cfg.stop_bits);
//             printf("parity          : %u\n", ser_port_data[k].ser_port_cfg.parity);
//             printf("handshake       : %s\n", ser_port_data[k].ser_port_cfg.handshake);
//             printf("inf_mode        : %s\n", ser_port_data[k].ser_port_cfg.inf_mode);
//             printf("num_dev         :  %d\n", ser_port_data[k].num_dev);
//             printf("meter_make      :  %s\n", ser_port_data[k].meter_make);
//             for (i = 1; i <= ser_port_data[j].num_dev; i++)
//             {
//                 printf("Meter[%d] Enable_device     : %u i = %d j = %d\n", i, ser_port_data[k].meter_cfg[i].enable_device, i, j);
//                 printf("Meter[%d] num_retries       : %u i = %d j = %d\n", i, ser_port_data[k].meter_cfg[i].num_retries, i, j);
//                 printf("Meter[%d] resp_timeouts     : %u i = %d j = %d\n", i, ser_port_data[k].meter_cfg[i].resp_timeouts, i, j);
//                 printf("Meter[%d] apply_ctpt_ratio  : %u\n", i, ser_port_data[k].meter_cfg[i].apply_ctpt_ratio);
//                 printf("Meter[%d] ct_ratio          : %u\n", i, ser_port_data[k].meter_cfg[i].ct_ratio);
//                 printf("Meter[%d] pt_ratio          : %u\n", i, ser_port_data[k].meter_cfg[i].pt_ratio);
//                 printf("Meter[%d] meter_addr        : %u\n", i, ser_port_data[k].meter_cfg[i].meter_addr);
//                 printf("Meter[%d] meter_addr_size   : %u\n", i, ser_port_data[k].meter_cfg[i].meter_addr_size);
//                 printf("Meter[%d] auth_type         : %s\n", i, ser_port_data[k].meter_cfg[i].auth_type);
//                 printf("Meter[%d] met_pw            : %s\n", i, ser_port_data[k].meter_cfg[i].met_pw);
//                 printf("Meter[%d] sys_title         : %s\n", i, ser_port_data[k].meter_cfg[i].sys_title);
//                 printf("Meter[%d] cipher_key        : %s\n", i, ser_port_data[k].meter_cfg[i].cipher_key);
//                 printf("Meter[%d] auth_key          : %s\n", i, ser_port_data[k].meter_cfg[i].auth_key);
//                 printf("Meter[%d] meter_loc         : %s\n", i, ser_port_data[k].meter_cfg[i].meter_loc);
//             }
//         }
// #endif
//     }

//     return 0;
// }

int update_ser_port_config_serial()
{
    static char fun_name[] = "update_ser_port_config()";
    redisReply *reply;
    char key[64];
    int i, j;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] : Redis connection error: %s\n", "update_ser_port_config_serial()", redis->errstr);
        return -1;
    }

    printf("num_ser_ports       : %d\n", feature_cfg.num_ser_ports);
    for (j = 0; j < feature_cfg.num_ser_ports; j++)
    {
        memset(key, 0, sizeof(key));
        sprintf(key, "serial_port_%d_cfg", j);

        // Initialize the structure before filling it
        memset(&ser_port_data[j], 0, sizeof(ser_port_data[j]));

        reply = redisCommand(redis, "EXISTS %s", key);
        if (reply->integer == 0)
        {
            printf("Key '%s' does not exist in Redis.\n", key);
            freeReplyObject(reply);
            return -1;
        }
        freeReplyObject(reply); // FREE the EXISTS reply early

        reply = redisCommand(redis, "HGETALL %s", key);
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (i = 0; i < reply->elements; i += 2)
            {
                const char *field = reply->element[i]->str;
                const char *value = reply->element[i + 1]->str;

                if (strcmp(field, "enable_port") == 0)
                    ser_port_data[j].ser_port_cfg.enable_port = (unsigned char)atoi(value);
                else if (strcmp(field, "port_name") == 0)
                    strncpy(ser_port_data[j].ser_port_cfg.port_name, value, sizeof(ser_port_data[j].ser_port_cfg.port_name) - 1);
                else if (strcmp(field, "serial_port_id") == 0)
                    ser_port_data[j].ser_port_cfg.ser_portid = (unsigned char)atoi(value);
                else if (strcmp(field, "baudrate") == 0)
                    ser_port_data[j].ser_port_cfg.baud_rate = atoi(value);
                else if (strcmp(field, "databits") == 0)
                    ser_port_data[j].ser_port_cfg.data_bits = (unsigned char)atoi(value);
                else if (strcmp(field, "stopbits") == 0)
                    ser_port_data[j].ser_port_cfg.stop_bits = (unsigned char)atoi(value);
                else if (strcmp(field, "parity") == 0)
                    ser_port_data[j].ser_port_cfg.parity = (unsigned char)atoi(value);
                else if (strcmp(field, "handshake") == 0)
                {
                    strncpy(ser_port_data[j].ser_port_cfg.handshake, value, sizeof(ser_port_data[j].ser_port_cfg.handshake) - 1);
                    ser_port_data[j].ser_port_cfg.handshake[sizeof(ser_port_data[j].ser_port_cfg.handshake) - 1] = '\0';
                }

                else if (strcmp(field, "infmode") == 0)
                {
                    strncpy(ser_port_data[j].ser_port_cfg.inf_mode, value, sizeof(ser_port_data[j].ser_port_cfg.inf_mode) - 1);
                    ser_port_data[j].ser_port_cfg.inf_mode[sizeof(ser_port_data[j].ser_port_cfg.inf_mode) - 1] = '\0';
                }

                else if (strcmp(field, "num_dev") == 0)
                    ser_port_data[j].num_dev = atoi(value);
                else if (strcmp(field, "meter_make") == 0)
                {
                    strncpy(ser_port_data[j].meter_make, value, sizeof(ser_port_data[j].meter_make) - 1);
                    ser_port_data[j].meter_make[sizeof(ser_port_data[j].meter_make) - 1] = '\0';
                }

                else if (strncmp(field, "enable_meter[", 13) == 0)
                {
                    int index = atoi(field + 13);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].enable_meter = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "retries[", 8) == 0)
                {
                    int index = atoi(field + 8);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].num_retries = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "resp_timeout[", 13) == 0)
                {
                    int index = atoi(field + 13);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].resp_timeouts = atoi(value);
                }
                else if (strncmp(field, "apply_ctpt_ratio[", 17) == 0)
                {
                    int index = atoi(field + 17);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].apply_ctpt_ratio = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "ct_ratio[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].ct_ratio = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "pt_ratio[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].pt_ratio = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "meter_addr[", 11) == 0)
                {
                    int index = atoi(field + 11);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].meter_addr = atoi(value);
                }
                else if (strncmp(field, "meter_addr_size[", 16) == 0)
                {
                    int index = atoi(field + 16);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].meter_addr_size = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "auth_type[", 10) == 0)
                {
                    int index = atoi(field + 10);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        strncpy(ser_port_data[j].meter_cfg[index - 1].auth_type, value, sizeof(ser_port_data[j].meter_cfg[index - 1].auth_type) - 1);
                }
                else if (strncmp(field, "password[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    int idx = index - 1;
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        // strncpy(ser_port_data[j].meter_cfg[idx].met_pw, value, sizeof(ser_port_data[j].meter_cfg[idx].met_pw) - 1);
                        // ser_port_data[j].meter_cfg[idx].met_pw[sizeof(ser_port_data[j].meter_cfg[idx].met_pw) - 1] = '\0';
                        snprintf(ser_port_data[j].meter_cfg[idx].met_pw, sizeof(ser_port_data[j].meter_cfg[idx].met_pw), "%s", value);
                    }
                }
                else if (strncmp(field, "sys_title[", 10) == 0)
                {
                    int index = atoi(field + 10);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].sys_title, value, sizeof(ser_port_data[j].meter_cfg[index - 1].sys_title) - 1);
                        ser_port_data[j].meter_cfg[index - 1].sys_title[sizeof(ser_port_data[j].meter_cfg[index - 1].sys_title) - 1] = '\0';
                    }
                }
                else if (strncmp(field, "cipher_key[", 11) == 0)
                {
                    int index = atoi(field + 11);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].cipher_key, value, sizeof(ser_port_data[j].meter_cfg[index - 1].cipher_key) - 1);
                        ser_port_data[j].meter_cfg[index - 1].cipher_key[sizeof(ser_port_data[j].meter_cfg[index - 1].cipher_key) - 1] = '\0';
                    }
                }
                else if (strncmp(field, "auth_key[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].auth_key, value, sizeof(ser_port_data[j].meter_cfg[index - 1].auth_key) - 1);
                        ser_port_data[j].meter_cfg[index - 1].auth_key[sizeof(ser_port_data[j].meter_cfg[index - 1].auth_key) - 1] = '\0';
                    }
                }
                else if (strncmp(field, "meter_loc[", 10) == 0)
                {
                    int index = atoi(field + 10);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].meter_loc, value, sizeof(ser_port_data[j].meter_cfg[index - 1].meter_loc) - 1);
                        ser_port_data[j].meter_cfg[index - 1].meter_loc[sizeof(ser_port_data[j].meter_cfg[index - 1].meter_loc) - 1] = '\0';
                    }
                }
            }
        }
        else
        {
            printf("Error retrieving hash for key: %s\n", key);
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Error retrieving hash", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
        }
        freeReplyObject(reply);

#if DEBUG
        printf("------- DEBUG DUMP for PORT %d -------\n", j);
        printf("enable_port     : %u\n", ser_port_data[j].ser_port_cfg.enable_port);
        printf("port_name       : %s\n", ser_port_data[j].ser_port_cfg.port_name);
        printf("ser_portid      : %u\n", ser_port_data[j].ser_port_cfg.ser_portid);
        printf("baud_rate       : %d\n", ser_port_data[j].ser_port_cfg.baud_rate);
        printf("data_bits       : %u\n", ser_port_data[j].ser_port_cfg.data_bits);
        printf("stop_bits       : %u\n", ser_port_data[j].ser_port_cfg.stop_bits);
        printf("parity          : %u\n", ser_port_data[j].ser_port_cfg.parity);
        printf("handshake       : %s\n", ser_port_data[j].ser_port_cfg.handshake);
        printf("inf_mode        : %s\n", ser_port_data[j].ser_port_cfg.inf_mode);
        printf("num_dev         : %d\n", ser_port_data[j].num_dev);
        printf("meter_make      : %s\n", ser_port_data[j].meter_make);

        for (i = 0; i < ser_port_data[j].num_dev; i++)
        {
            printf("Meter[%d] enable_meter     : %u\n", i, ser_port_data[j].meter_cfg[i].enable_meter);
            printf("Meter[%d] num_retries       : %u\n", i, ser_port_data[j].meter_cfg[i].num_retries);
            printf("Meter[%d] resp_timeouts     : %u\n", i, ser_port_data[j].meter_cfg[i].resp_timeouts);
            printf("Meter[%d] apply_ctpt_ratio  : %u\n", i, ser_port_data[j].meter_cfg[i].apply_ctpt_ratio);
            printf("Meter[%d] ct_ratio          : %u\n", i, ser_port_data[j].meter_cfg[i].ct_ratio);
            printf("Meter[%d] pt_ratio          : %u\n", i, ser_port_data[j].meter_cfg[i].pt_ratio);
            printf("Meter[%d] meter_addr        : %u\n", i, ser_port_data[j].meter_cfg[i].meter_addr);
            printf("Meter[%d] meter_addr_size   : %u\n", i, ser_port_data[j].meter_cfg[i].meter_addr_size);
            printf("Meter[%d] auth_type         : %s\n", i, ser_port_data[j].meter_cfg[i].auth_type);
            printf("Meter[%d] met_pw            : %s\n", i, ser_port_data[j].meter_cfg[i].met_pw);
            printf("Meter[%d] sys_title         : %s\n", i, ser_port_data[j].meter_cfg[i].sys_title);
            printf("Meter[%d] cipher_key        : %s\n", i, ser_port_data[j].meter_cfg[i].cipher_key);
            printf("Meter[%d] auth_key          : %s\n", i, ser_port_data[j].meter_cfg[i].auth_key);
            printf("Meter[%d] meter_loc         : %s\n", i, ser_port_data[j].meter_cfg[i].meter_loc);
        }
#endif
    }

    return 0;
}

int update_ser_port_config_plcc()
{
    static char fun_name[] = "update_ser_port_config()";
    redisReply *reply;
    char key[64];
    int i, j;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] : Redis connection error: %s\n", "update_ser_port_config_plcc()", redis->errstr);
        return -1;
    }

    printf("num_ser_ports       : %d\n", feature_cfg.num_ser_ports);
    for (j = 1; j <= feature_cfg.num_ser_ports; j++)
    {
        memset(key, 0, sizeof(key));
        sprintf(key, "serial_port_%d_cfg", j);

        // Initialize the structure before filling it
        memset(&ser_port_data[j], 0, sizeof(ser_port_data[j]));

        reply = redisCommand(redis, "EXISTS %s", key);
        if (reply->integer == 0)
        {
            printf("Key '%s' does not exist in Redis.\n", key);
            freeReplyObject(reply);
            return -1;
        }
        freeReplyObject(reply); // FREE the EXISTS reply early

        reply = redisCommand(redis, "HGETALL %s", key);
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            for (i = 0; i < reply->elements; i += 2)
            {
                const char *field = reply->element[i]->str;
                const char *value = reply->element[i + 1]->str;

                if (strcmp(field, "enable_port") == 0)
                    ser_port_data[j].ser_port_cfg.enable_port = (unsigned char)atoi(value);
                else if (strcmp(field, "port_name") == 0)
                    strncpy(ser_port_data[j].ser_port_cfg.port_name, value, sizeof(ser_port_data[j].ser_port_cfg.port_name) - 1);
                else if (strcmp(field, "serial_port_id") == 0)
                    ser_port_data[j].ser_port_cfg.ser_portid = (unsigned char)atoi(value);
                else if (strcmp(field, "baudrate") == 0)
                    ser_port_data[j].ser_port_cfg.baud_rate = atoi(value);
                else if (strcmp(field, "databits") == 0)
                    ser_port_data[j].ser_port_cfg.data_bits = (unsigned char)atoi(value);
                else if (strcmp(field, "stopbits") == 0)
                    ser_port_data[j].ser_port_cfg.stop_bits = (unsigned char)atoi(value);
                else if (strcmp(field, "parity") == 0)
                    ser_port_data[j].ser_port_cfg.parity = (unsigned char)atoi(value);
                else if (strcmp(field, "handshake") == 0)
                {
                    strncpy(ser_port_data[j].ser_port_cfg.handshake, value, sizeof(ser_port_data[j].ser_port_cfg.handshake) - 1);
                    ser_port_data[j].ser_port_cfg.handshake[sizeof(ser_port_data[j].ser_port_cfg.handshake) - 1] = '\0';
                }

                else if (strcmp(field, "infmode") == 0)
                {
                    strncpy(ser_port_data[j].ser_port_cfg.inf_mode, value, sizeof(ser_port_data[j].ser_port_cfg.inf_mode) - 1);
                    ser_port_data[j].ser_port_cfg.inf_mode[sizeof(ser_port_data[j].ser_port_cfg.inf_mode) - 1] = '\0';
                }

                else if (strcmp(field, "num_dev") == 0)
                    ser_port_data[j].num_dev = atoi(value);
                else if (strcmp(field, "meter_make") == 0)
                {
                    strncpy(ser_port_data[j].meter_make, value, sizeof(ser_port_data[j].meter_make) - 1);
                    ser_port_data[j].meter_make[sizeof(ser_port_data[j].meter_make) - 1] = '\0';
                }

                else if (strncmp(field, "enable_meter[", 13) == 0)
                {
                    int index = atoi(field + 13);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].enable_meter = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "retries[", 8) == 0)
                {
                    int index = atoi(field + 8);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].num_retries = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "resp_timeout[", 13) == 0)
                {
                    int index = atoi(field + 13);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].resp_timeouts = atoi(value);
                }
                else if (strncmp(field, "apply_ctpt_ratio[", 17) == 0)
                {
                    int index = atoi(field + 17);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].apply_ctpt_ratio = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "ct_ratio[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].ct_ratio = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "pt_ratio[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].pt_ratio = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "meter_addr[", 11) == 0)
                {
                    int index = atoi(field + 11);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].meter_addr = atoi(value);
                }
                else if (strncmp(field, "meter_addr_size[", 16) == 0)
                {
                    int index = atoi(field + 16);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        ser_port_data[j].meter_cfg[index - 1].meter_addr_size = (unsigned char)atoi(value);
                }
                else if (strncmp(field, "auth_type[", 10) == 0)
                {
                    int index = atoi(field + 10);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                        strncpy(ser_port_data[j].meter_cfg[index - 1].auth_type, value, sizeof(ser_port_data[j].meter_cfg[index - 1].auth_type) - 1);
                }
                else if (strncmp(field, "password[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    int idx = index - 1;
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        // strncpy(ser_port_data[j].meter_cfg[idx].met_pw, value, sizeof(ser_port_data[j].meter_cfg[idx].met_pw) - 1);
                        // ser_port_data[j].meter_cfg[idx].met_pw[sizeof(ser_port_data[j].meter_cfg[idx].met_pw) - 1] = '\0';
                        snprintf(ser_port_data[j].meter_cfg[idx].met_pw, sizeof(ser_port_data[j].meter_cfg[idx].met_pw), "%s", value);
                    }
                }
                else if (strncmp(field, "sys_title[", 10) == 0)
                {
                    int index = atoi(field + 10);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].sys_title, value, sizeof(ser_port_data[j].meter_cfg[index - 1].sys_title) - 1);
                        ser_port_data[j].meter_cfg[index - 1].sys_title[sizeof(ser_port_data[j].meter_cfg[index - 1].sys_title) - 1] = '\0';
                    }
                }
                else if (strncmp(field, "cipher_key[", 11) == 0)
                {
                    int index = atoi(field + 11);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].cipher_key, value, sizeof(ser_port_data[j].meter_cfg[index - 1].cipher_key) - 1);
                        ser_port_data[j].meter_cfg[index - 1].cipher_key[sizeof(ser_port_data[j].meter_cfg[index - 1].cipher_key) - 1] = '\0';
                    }
                }
                else if (strncmp(field, "auth_key[", 9) == 0)
                {
                    int index = atoi(field + 9);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].auth_key, value, sizeof(ser_port_data[j].meter_cfg[index - 1].auth_key) - 1);
                        ser_port_data[j].meter_cfg[index - 1].auth_key[sizeof(ser_port_data[j].meter_cfg[index - 1].auth_key) - 1] = '\0';
                    }
                }
                else if (strncmp(field, "meter_loc[", 10) == 0)
                {
                    int index = atoi(field + 10);
                    if (index > 0 && index <= ser_port_data[j].num_dev)
                    {
                        strncpy(ser_port_data[j].meter_cfg[index - 1].meter_loc, value, sizeof(ser_port_data[j].meter_cfg[index - 1].meter_loc) - 1);
                        ser_port_data[j].meter_cfg[index - 1].meter_loc[sizeof(ser_port_data[j].meter_cfg[index - 1].meter_loc) - 1] = '\0';
                    }
                }
            }
        }
        else
        {
            printf("Error retrieving hash for key: %s\n", key);
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Error retrieving hash", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
        }
        freeReplyObject(reply);

#if DEBUG
        printf("------- DEBUG DUMP for PORT %d -------\n", j);
        printf("enable_port     : %u\n", ser_port_data[j].ser_port_cfg.enable_port);
        printf("port_name       : %s\n", ser_port_data[j].ser_port_cfg.port_name);
        printf("ser_portid      : %u\n", ser_port_data[j].ser_port_cfg.ser_portid);
        printf("baud_rate       : %d\n", ser_port_data[j].ser_port_cfg.baud_rate);
        printf("data_bits       : %u\n", ser_port_data[j].ser_port_cfg.data_bits);
        printf("stop_bits       : %u\n", ser_port_data[j].ser_port_cfg.stop_bits);
        printf("parity          : %u\n", ser_port_data[j].ser_port_cfg.parity);
        printf("handshake       : %s\n", ser_port_data[j].ser_port_cfg.handshake);
        printf("inf_mode        : %s\n", ser_port_data[j].ser_port_cfg.inf_mode);
        printf("num_dev         : %d\n", ser_port_data[j].num_dev);
        printf("meter_make      : %s\n", ser_port_data[j].meter_make);

        for (i = 0; i < ser_port_data[j].num_dev; i++)
        {
            printf("Meter[%d] enable_meter     : %u\n", i, ser_port_data[j].meter_cfg[i].enable_meter);
            printf("Meter[%d] num_retries       : %u\n", i, ser_port_data[j].meter_cfg[i].num_retries);
            printf("Meter[%d] resp_timeouts     : %u\n", i, ser_port_data[j].meter_cfg[i].resp_timeouts);
            printf("Meter[%d] apply_ctpt_ratio  : %u\n", i, ser_port_data[j].meter_cfg[i].apply_ctpt_ratio);
            printf("Meter[%d] ct_ratio          : %u\n", i, ser_port_data[j].meter_cfg[i].ct_ratio);
            printf("Meter[%d] pt_ratio          : %u\n", i, ser_port_data[j].meter_cfg[i].pt_ratio);
            printf("Meter[%d] meter_addr        : %u\n", i, ser_port_data[j].meter_cfg[i].meter_addr);
            printf("Meter[%d] meter_addr_size   : %u\n", i, ser_port_data[j].meter_cfg[i].meter_addr_size);
            printf("Meter[%d] auth_type         : %s\n", i, ser_port_data[j].meter_cfg[i].auth_type);
            printf("Meter[%d] met_pw            : %s\n", i, ser_port_data[j].meter_cfg[i].met_pw);
            printf("Meter[%d] sys_title         : %s\n", i, ser_port_data[j].meter_cfg[i].sys_title);
            printf("Meter[%d] cipher_key        : %s\n", i, ser_port_data[j].meter_cfg[i].cipher_key);
            printf("Meter[%d] auth_key          : %s\n", i, ser_port_data[j].meter_cfg[i].auth_key);
            printf("Meter[%d] meter_loc         : %s\n", i, ser_port_data[j].meter_cfg[i].meter_loc);
        }
#endif
    }

    return 0;
}

/**
 * Updates the polling configuration by retrieving values from a Redis hash.
 *
 * This function fetches polling configuration data from Redis and updates
 * the relevant structures. If there is an error during retrieval, it logs the
 * event.
 *
 * @return Returns 0 on successful completion.
 */
int update_polling_cfg()
{
    static char fun_name[] = "update_polling_cfg()";
    char key[64];
    redisReply *reply;
    int i;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    sprintf(key, "poll_cfg");

    reply = redisCommand(redis, "EXISTS %s", key);
    if (reply->integer == 0)
    {
        printf("Key '%s' does not exist in Redis.\n", key);
        freeReplyObject(reply);
        return -1;
    }

    reply = redisCommand(redis, "HGETALL %s", key);
    if (reply->type == REDIS_REPLY_ARRAY)
    {

        for (i = 0; i < reply->elements; i += 2)
        {
            const char *field = reply->element[i]->str;
            const char *value = reply->element[i + 1]->str;
#if 0
            printf("Field: %s\n", field);
            printf("Value: %s\n", value);
#endif
            if (strcmp(field, "enable_inst_data_poll") == 0)
            {
                poll_cfg.poll_inst_data = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "enable_ls_data_poll") == 0)
            {
                poll_cfg.poll_ls_data = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "enable_mn_data_poll") == 0)
            {
                poll_cfg.poll_mn_data = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "enable_bill_data_poll") == 0)
            {
                poll_cfg.poll_bill_data = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "enable_event_data_poll") == 0)
            {
                poll_cfg.poll_event_data = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "enable_old_data_poll") == 0)
            {
                poll_cfg.enable_stored_data_poll = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "enabled_proc_diag") == 0)
            {
                poll_cfg.enable_proc_diag = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "no_old_days") == 0)
            {
                poll_cfg.num_old_days = (unsigned char)atoi(value);
            }
            else if (strcmp(field, "inst_poll_int") == 0)
            {
                poll_cfg.inst_poll_int = atoi(value);
            }
            else if (strcmp(field, "ls_poll_int") == 0)
            {
                poll_cfg.ls_poll_int = atoi(value);
            }
            else if (strcmp(field, "mn_poll_int") == 0)
            {
                poll_cfg.mn_poll_int = atoi(value);
            }
            else if (strcmp(field, "bill_poll_int") == 0)
            {
                poll_cfg.bill_poll_int = atoi(value);
            }
            else if (strcmp(field, "event_poll_int") == 0)
            {
                poll_cfg.event_poll_int = atoi(value);
            }
            else if (strcmp(field, "num_of_bill_month") == 0)
            {
                poll_cfg.num_of_bill_month = atoi(value);
            }
            else if (strcmp(field, "num_of_events") == 0)
            {
                strcpy(poll_cfg.num_of_events, value);
            }
            else if (strcmp(field, "num_of_mn_data") == 0)
            {
                poll_cfg.num_of_mn_data = atoi(value);
            }
            else if (strcmp(field, "num_blocks") == 0) // plcc_nw_refresh_int
            {
                poll_cfg.num_blocks = atoi(value);
            }
            else if (strcmp(field, "plcc_nw_refresh_int") == 0)
            {
                poll_cfg.plcc_nw_refresh_int = atoi(value);
            }
        }
    }
    else
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Error retrieving hash", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
    }

    freeReplyObject(reply);

#if DEBUG
    printf("poll_inst_data          : %u\n", poll_cfg.poll_inst_data);
    printf("poll_ls_data            : %u\n", poll_cfg.poll_ls_data);
    printf("poll_mn_data            : %u\n", poll_cfg.poll_mn_data);
    printf("poll_bill_data          : %u\n", poll_cfg.poll_bill_data);
    printf("poll_event_data         : %u\n", poll_cfg.poll_event_data);
    printf("Enabled_stored_data_poll : %u\n", poll_cfg.enable_stored_data_poll);
    printf("Enabled_proc_diag       : %u\n", poll_cfg.enable_proc_diag);
    printf("num_old_days            : %u\n", poll_cfg.num_old_days);
    printf("inst_poll_int           : %d\n", poll_cfg.inst_poll_int);
    printf("ls_poll_int             : %d\n", poll_cfg.ls_poll_int);
    printf("mn_poll_int             : %d\n", poll_cfg.mn_poll_int);
    printf("bill_poll_int           : %d\n", poll_cfg.bill_poll_int);
    printf("event_poll_int          : %d\n", poll_cfg.event_poll_int);
    printf("plcc_nw_refresh_int     : %d\n", poll_cfg.plcc_nw_refresh_int);
#endif

    return 0;
}

/**
 * Retrieves the serial number associated with a specific meter from Redis.
 *
 * This function constructs a key based on the meter index and queries
 * a Redis hash to get the corresponding serial number. It handles errors
 * and unexpected reply types by logging events.
 *
 * @param nc_idx The index of the meter for which to retrieve the serial number.
 * @return Returns the serial number as a string, or NULL if an error occurs.
 */
int get_meter_2_ser_num(int nc_idx, char *serial_number_buf, int length)
{
    static char fun_name[] = "get_meter_2_ser_num()";

    // print_colored_message(YELLOW, 1, "meter begin----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, nc_idx, ser_port_data[g_pidx].meter_cfg[nc_idx].met_pw);

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        char key[64];

        redisReply *reply;

        if (redis == NULL || redis->err)
        {
            dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
            return -1;
        }

        sprintf(key, "meter_%d_meter_ser_num", (nc_idx + 1));

        reply = redisCommand(redis, "HGET %s %s", HASH_KEY, key);
        if (reply == NULL)
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            return -1;
        }

        // char *result = NULL;
        if (reply->type == REDIS_REPLY_STRING)
        {
            // result = strdup(reply->str);
            snprintf(serial_number_buf, length, "%s", reply->str);
            freeReplyObject(reply);
        }
        else
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Unexpected reply type: %d", fun_name, reply->type);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            freeReplyObject(reply);
            return -1;
        }

        return 0;
    }
    else
    {

        int midx = nc_idx + 1, i;

        redisReply *reply = redisCommand(redis, "HKEYS meter_status");
        if (reply == NULL || reply->type != REDIS_REPLY_ARRAY)
            return -1;

        for (i = 0; i < reply->elements; ++i)
        {
            redisReply *key_reply = reply->element[i];
            if (key_reply->type != REDIS_REPLY_STRING)
            {
                continue;
            }

            const char *key = key_reply->str;

            // Format expected: meter_1_4_32000043_details
            // We tokenize based on '_' delimiter
            char key_copy[256];
            strncpy(key_copy, key, sizeof(key_copy));
            key_copy[sizeof(key_copy) - 1] = '\0';

            char *tokens[6] = {0};
            char *tok = strtok(key_copy, "_");
            int tok_idx = 0;

            while (tok && tok_idx < 6)
            {
                tokens[tok_idx++] = tok;
                tok = strtok(NULL, "_");
            }

            // if (tok_idx >= 5)
            // {
            if (tok_idx >= 5 && tokens[1] && tokens[2] && tokens[3])
            {
                int key_port_index = atoi(tokens[1]);

                int key_meter_index = atoi(tokens[2]);
                if (key_port_index == g_pidx && key_meter_index == midx)
                {

                    // This is the meter we want
                    // char *serial_number = strdup(tokens[3]); // 32000043
                    snprintf(serial_number_buf, length, "%s", tokens[3]);
                    freeReplyObject(reply);
                    // print_colored_message(YELLOW, 1, "meter end----- ser_port_data[%d].meter_cfg[%d].met_pw %s serial number %s-----------\n", g_pidx, nc_idx, ser_port_data[g_pidx].meter_cfg[nc_idx].met_pw, serial_number);

                    return 0;
                }
            }
        }

        freeReplyObject(reply);
        return -1;
    }
}

// rithika 07nov2025
int check_name_plate(int nc_idx)
{
    static char fun_name[] = "check_name_plate()";

    // print_colored_message(YELLOW, 1, "meter begin----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, nc_idx, ser_port_data[g_pidx].meter_cfg[nc_idx].met_pw);

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        char key[64];

        redisReply *reply;

        if (redis == NULL || redis->err)
        {
            dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
            return -1;
        }

        sprintf(key, "meter_%d_meter_ser_num", (nc_idx + 1));

        reply = redisCommand(redis, "HGET %s %s", HASH_KEY, key);
        if (reply == NULL)
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            return -1;
        }

        // char *result = NULL;
        if (reply->type == REDIS_REPLY_STRING)
        {
            // result = strdup(reply->str);
            // snprintf(serial_number_buf, length, "%s", reply->str);
            freeReplyObject(reply);
        }
        else
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Unexpected reply type: %d", fun_name, reply->type);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            freeReplyObject(reply);
            return -1;
        }

        return 0;
    }
    else
    {

        int midx = nc_idx + 1, i;

        redisReply *reply = redisCommand(redis, "HKEYS meter_status");
        if (reply == NULL || reply->type != REDIS_REPLY_ARRAY)
            return -1;

        for (i = 0; i < reply->elements; ++i)
        {
            redisReply *key_reply = reply->element[i];
            if (key_reply->type != REDIS_REPLY_STRING)
            {
                continue;
            }

            const char *key = key_reply->str;

            // Format expected: meter_1_4_32000043_details
            // We tokenize based on '_' delimiter
            char key_copy[256];
            strncpy(key_copy, key, sizeof(key_copy));
            key_copy[sizeof(key_copy) - 1] = '\0';

            char *tokens[6] = {0};
            char *tok = strtok(key_copy, "_");
            int tok_idx = 0;

            while (tok && tok_idx < 6)
            {
                tokens[tok_idx++] = tok;
                tok = strtok(NULL, "_");
            }

            // if (tok_idx >= 5)
            // {
            if (tok_idx >= 5 && tokens[1] && tokens[2] && tokens[3])
            {
                int key_port_index = atoi(tokens[1]);

                int key_meter_index = atoi(tokens[2]);
                if (key_port_index == g_pidx && key_meter_index == midx)
                {
                    freeReplyObject(reply);
                    return 0;
                }
            }
        }

        freeReplyObject(reply);
        return -1;
    }
}

// char *get_met_ser_num_serialcommn(int midx)
// {
//     static char fun_name[] = "get_met_ser_num_serialcommn";
//     redisReply *reply = redisCommand(redis, "HKEYS meter_status");
//     if (reply == NULL || reply->type != REDIS_REPLY_ARRAY)
//         return NULL;
//     int i;
//     for (i = 0; i < reply->elements; i++)
//     {
//         redisReply *key_reply = reply->element[i];
//         if (key_reply->type != REDIS_REPLY_STRING)
//         {

//             continue;
//         }

//         const char *key = key_reply->str;

//         const char *first_underscore = strchr(key, '_');
//         const char *second_underscore = first_underscore ? strchr(first_underscore + 1, '_') : NULL;

//         if (!first_underscore || !second_underscore)
//             continue;

//         char meter_prefix[32] = {0};
//         size_t prefix_len = first_underscore - key;
//         strncpy(meter_prefix, key, prefix_len);
//         meter_prefix[prefix_len] = '\0';

//         int meter_index = -1;
//         if (sscanf(meter_prefix, "meter%d", &meter_index) != 1)
//             continue;

//         if (meter_index != midx)
//             continue;

//         size_t serial_len = second_underscore - first_underscore - 1;
//         char *serial_num = malloc(serial_len + 1);
//         if (!serial_num)
//         {
//             freeReplyObject(reply);
//             return NULL;
//         }

//         strncpy(serial_num, first_underscore + 1, serial_len);
//         serial_num[serial_len] = '\0';

//         freeReplyObject(reply);
//         return serial_num;
//     }

//     freeReplyObject(reply);
//     return NULL;
// }

char *drift_time(const char *meter_time_str, int midx)
{
    static char out_buf[16]; // Static buffer to return formatted string
    // printf("))))))))))))))))))))))) metertime str %s ))))))))))\n", meter_time_str);
    struct tm meter_tm = {0};
    if (strptime(meter_time_str, "%Y-%m-%d %H:%M:%S", &meter_tm) == NULL)
    {
        snprintf(out_buf, sizeof(out_buf), "ERROR: Invalid meter time format.");
        return out_buf;
    }

    time_t meter_time = mktime(&meter_tm);
    if (meter_time == -1)
    {
        snprintf(out_buf, sizeof(out_buf), "ERROR: Failed to convert meter time.");
        return out_buf;
    }

    time_t system_time = time(NULL);
    double drift = difftime(meter_time, system_time);
    print_colored_message(GREEN, 1, "------------ drift time %f midx %d", drift, midx);
    // rithika 10Dec2025
    meter_drift_sec[midx] = drift;

    int sign = (drift >= 0) ? 1 : -1;
    int abs_seconds = (int)fabs(drift);

    int hours = abs_seconds / 3600;
    int minutes = (abs_seconds % 3600) / 60;
    int seconds = abs_seconds % 60;

    char sign_char = (sign >= 0) ? '+' : '-';

    // Format the result into the buffer
    snprintf(out_buf, sizeof(out_buf), "%c%02d:%02d:%02d", sign_char, hours, minutes, seconds);

    return out_buf;
}

/**
 * Adds instantaneous data for a specified meter to the Redis database.
 *
 * This function constructs a JSON object containing meter information and
 * instantaneous values, and stores it in Redis under a specific hash key.
 *
 * @param meter_no The number of the meter.
 * @param pidx The index of the port.
 * @param midx The index of the meter.
 * @param data Pointer to the structure containing instantaneous value data.
 * @return Returns 0 on success, -1 on failure.
 */
// int add_meter_inst_data(int meter_no, int pidx, int midx, const inst_val_info_t *data)
// {

//     print_colored_message(RED, PRINT, "=================== add_meter_inst_data =======================\n");
//     static char fun_name[] = "add_meter_inst_data";
//     redisReply *reply;
//     char hash_key[64];
//     nx_json *json_obj = NULL;
//     int i;

//     char temp_met_sn[64];
//     char inst_json[4096];

//     dbg_log(INFORM, "[%-25s] : At the beginning the midx is : %d \n", fun_name, midx);

//     memset(temp_met_sn, 0, sizeof(temp_met_sn));

//     // rithika 29/08/2025
//     if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
//     {
//         snprintf(temp_met_sn, sizeof(temp_met_sn), "meter_%d_", (midx + 1));
//     }
//     else
//     {
//         snprintf(temp_met_sn, sizeof(temp_met_sn), "meter_%d_%d_", g_pidx, (midx + 1));
//     }

//     // rithika 26Nov2025
//     //  int deleted_count = delete_old_metercommn_status(temp_met_sn, "meter_inst_info");
//     //  if (deleted_count < 0)
//     //  {
//     //      fprintf(stderr, "Failed to delete keys starting with meter0_\n");
//     //      return -1;
//     //  }
//     //  else if (deleted_count > 0)
//     //  {
//     //      printf("Deleted %d keys with prefix meter0_\n", deleted_count);
//     //  }
//     //  else
//     //  {
//     //      printf("No keys with prefix meter0_ found\n");
//     //  }

//     memset(temp_met_sn, 0, sizeof(temp_met_sn));
//     // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
//     // char *sn = get_meter_2_ser_num(midx);
//     // if (sn)
//     // {
//     //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
//     //     free(sn);
//     // }
//     // else
//     // {
//     //     return -1;
//     // }

//     char serial_number[64] = {0};
//     if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
//     {
//         dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

//         return -1;
//     }
//     strncpy(temp_met_sn, serial_number, sizeof(temp_met_sn) - 1);

//     memset(inst_json, 0, sizeof(inst_json));

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, "{\n");
//     strcat(inst_json, json_buff);

//     memset(json_buff, 0, sizeof(json_buff));

//     char temp_manufacturer[64] = {0};
//     if (get_manufacturer(midx, temp_met_sn, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
//     {
//         // handle error
//     }

//     sprintf(json_buff, " %cmake%c : %c%s%c,", DBL_QUOTES, DBL_QUOTES, DBL_QUOTES, temp_manufacturer, DBL_QUOTES);
//     strcat(inst_json, json_buff);

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, " %cload_status%c : %c%d%c,", DBL_QUOTES, DBL_QUOTES, DBL_QUOTES, data->load_status, DBL_QUOTES);
//     strcat(inst_json, json_buff);

//     // rithika 07oct2025
//     if (get_curr_date_time(midx) < 0)
//     {
//         dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", midx, fun_name);
//         // return -1;
//     }
//     // rithika 03/09
//     char *drift = drift_time(meter_time_str[midx], midx);

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, " %cdrift_time%c : %c%s%c,", DBL_QUOTES, DBL_QUOTES, DBL_QUOTES, drift, DBL_QUOTES);
//     strcat(inst_json, json_buff);

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, " %cname%c : %c%s%c ,", DBL_QUOTES, DBL_QUOTES, DBL_QUOTES, ser_port_data[pidx].meter_cfg[midx].meter_loc, DBL_QUOTES);
//     strcat(inst_json, json_buff);

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, " %cnum_vals%c : %c%d%c, ", DBL_QUOTES, DBL_QUOTES, DBL_QUOTES, data->num_vals, DBL_QUOTES);
//     strcat(inst_json, json_buff);

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, "\"obis_list\" : [\n");
//     strcat(inst_json, json_buff);

//     for (i = 0; i < data->num_vals; i++)
//     {
//         if (i == data->num_vals - 1)
//         {
//             memset(json_buff, 0, sizeof(json_buff));
//             sprintf(json_buff, "%c%s%c\n", DBL_QUOTES, data->values[i].obis_code, DBL_QUOTES);
//             strcat(inst_json, json_buff);
//             break;
//         }

//         memset(json_buff, 0, sizeof(json_buff));
//         sprintf(json_buff, "%c%s%c,\n", DBL_QUOTES, data->values[i].obis_code, DBL_QUOTES);
//         strcat(inst_json, json_buff);
//     }

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, "],\n");
//     strcat(inst_json, json_buff);

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, "\"val_list\" : [\n");
//     strcat(inst_json, json_buff);

//     for (i = 0; i < data->num_vals; i++)
//     {
//         if (i == data->num_vals - 1)
//         {
//             memset(json_buff, 0, sizeof(json_buff));
//             sprintf(json_buff, "%c%s%c\n", DBL_QUOTES, data->values[i].val, DBL_QUOTES);
//             strcat(inst_json, json_buff);
//             break;
//         }

//         memset(json_buff, 0, sizeof(json_buff));
//         sprintf(json_buff, "%c%s%c,\n", DBL_QUOTES, data->values[i].val, DBL_QUOTES);
//         strcat(inst_json, json_buff);
//     }

//     memset(json_buff, 0, sizeof(json_buff));
//     sprintf(json_buff, "      ]\n}");
//     strcat(inst_json, json_buff);

// #if DEBUG
//     print_colored_message(RED, PRINT, "=================================================\n");
//     printf("Inst data JSON :%s\n", inst_json);
//     print_colored_message(RED, PRINT, "=================================================\n");
// #endif

//     snprintf(hash_key, sizeof(hash_key), "meter_inst_info");

//     // rithika 19/08/2025
//     memset(temp_met_sn, 0, sizeof(temp_met_sn));
//     // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
//     // char *sn1 = get_meter_2_ser_num(midx);
//     // if (sn1)
//     // {
//     //     strncpy(temp_met_sn, sn1, sizeof(temp_met_sn) - 1);
//     //     free(sn1);
//     // }
//     // else
//     // {
//     //     return -1;
//     // }

//     strncpy(temp_met_sn, serial_number, sizeof(temp_met_sn) - 1);

//     print_colored_message(RED, PRINT, "===================1 json_buff temp_met_sn %s ==============================\n", temp_met_sn);
//     ///////////

//     memset(json_buff, 0, sizeof(json_buff));
//     // rithika 29/08/2025
//     if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
//     {
//         sprintf(json_buff, "meter_%d_%s", (midx + 1), temp_met_sn);
//     }
//     else
//     {
//         sprintf(json_buff, "meter_%d_%d_%s", g_pidx, (midx + 1), temp_met_sn);
//     }

//     // dbg_log(INFORM, "[%-25s] : key to add in meter_inst_info (temp_met_sn) : %s \n", fun_name, temp_met_sn);

//     reply = redisCommand(redis, "HMSET %s %s %s", hash_key, json_buff, inst_json);
//     print_colored_message(RED, PRINT, "===================1 json_buff field name %s ==============================\n", json_buff);
//     if (reply == NULL)
//     {
//         memset(event_msg_str, 0, sizeof(event_msg_str));
//         sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
//         strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
//         add_event_data(&redis_event_data);
//         print_colored_message(RED, PRINT, "================2=================================\n");
//         return -1;
//     }
//     print_colored_message(RED, PRINT, "========================3=========================\n");
//     if (reply->type == REDIS_REPLY_ERROR)
//     {
//         memset(event_msg_str, 0, sizeof(event_msg_str));
//         sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
//         strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
//         add_event_data(&redis_event_data);
//         print_colored_message(RED, PRINT, "=======================4==========================\n");
//         freeReplyObject(reply);
//         print_colored_message(RED, PRINT, "======================5===========================\n");
//         return -1;
//     }
//     print_colored_message(RED, PRINT, "=============================6====================\n");
//     freeReplyObject(reply);
//     print_colored_message(RED, PRINT, "==========================7=======================\n");

//     return 0;
// }

int add_meter_inst_data(int meter_no, int pidx, int midx, const inst_val_info_t *data)
{

    print_colored_message(RED, PRINT, "=================== add_meter_inst_data =======================\n");
    static char fun_name[] = "add_meter_inst_data";
    redisReply *reply;
    char hash_key[64];

    int i;

    char temp_met_sn[64];
    // char inst_json[4096];

    dbg_log(INFORM, "[%-25s] : At the beginning the midx is : %d \n", fun_name, midx);

    // memset(temp_met_sn, 0, sizeof(temp_met_sn));

    // // rithika 29/08/2025
    // if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    // {
    //     snprintf(temp_met_sn, sizeof(temp_met_sn), "meter_%d_", (midx + 1));
    // }
    // else
    // {
    //     snprintf(temp_met_sn, sizeof(temp_met_sn), "meter_%d_%d_", g_pidx, (midx + 1));
    // }

    // rithika 26Nov2025
    //  int deleted_count = delete_old_metercommn_status(temp_met_sn, "meter_inst_info");
    //  if (deleted_count < 0)
    //  {
    //      fprintf(stderr, "Failed to delete keys starting with meter0_\n");
    //      return -1;
    //  }
    //  else if (deleted_count > 0)
    //  {
    //      printf("Deleted %d keys with prefix meter0_\n", deleted_count);
    //  }
    //  else
    //  {
    //      printf("No keys with prefix meter0_ found\n");
    //  }

    memset(temp_met_sn, 0, sizeof(temp_met_sn));
    // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    // else
    // {
    //     return -1;
    // }

    char serial_number[64] = {0};
    if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }
    strncpy(temp_met_sn, serial_number, sizeof(temp_met_sn) - 1);

    char temp_manufacturer[64] = {0};
    if (get_manufacturer(midx, temp_met_sn, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
    {
        // handle error
    }

    // rithika 07oct2025
    if (get_curr_date_time(midx) < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", midx, fun_name);
        // return -1;
    }
    // rithika 03/09
    char *drift = drift_time(meter_time_str[midx], midx);

    ////////////////////////////////////////////////////////////////////////////////
    cJSON *root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "make", temp_manufacturer);
    cJSON_AddNumberToObject(root, "load_status", data->load_status);
    cJSON_AddStringToObject(root, "drift_time", drift);
    cJSON_AddStringToObject(root, "name",
                            ser_port_data[pidx].meter_cfg[midx].meter_loc);
    cJSON_AddNumberToObject(root, "num_vals", data->num_vals);

    /* OBIS list */
    cJSON *obis_array = cJSON_CreateArray();
    for (i = 0; i < data->num_vals; i++)
    {
        cJSON_AddItemToArray(
            obis_array,
            cJSON_CreateString(data->values[i].obis_code));
    }
    cJSON_AddItemToObject(root, "obis_list", obis_array);

    /* Value list */
    cJSON *val_array = cJSON_CreateArray();
    for (i = 0; i < data->num_vals; i++)
    {
        cJSON_AddItemToArray(
            val_array,
            cJSON_CreateString(data->values[i].val));
    }
    cJSON_AddItemToObject(root, "val_list", val_array);

    /* Convert to string */
    char *inst_json = cJSON_PrintUnformatted(root);

    if (!inst_json)
    {
        dbg_log(REPORT, "[%s] : cJSON_PrintUnformatted failed\n", fun_name);
        cJSON_Delete(root);
        return -1;
    }
    cJSON_Delete(root);
    //////////////////////////////////////////////////////////////////////////////////

    snprintf(hash_key, sizeof(hash_key), "meter_inst_info");

    // rithika 19/08/2025
    memset(temp_met_sn, 0, sizeof(temp_met_sn));
    // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
    // char *sn1 = get_meter_2_ser_num(midx);
    // if (sn1)
    // {
    //     strncpy(temp_met_sn, sn1, sizeof(temp_met_sn) - 1);
    //     free(sn1);
    // }
    // else
    // {
    //     return -1;
    // }

    strncpy(temp_met_sn, serial_number, sizeof(temp_met_sn) - 1);

    print_colored_message(RED, PRINT, "===================1 json_buff temp_met_sn %s ==============================\n", temp_met_sn);
    ///////////

    memset(json_buff, 0, sizeof(json_buff));
    // rithika 29/08/2025
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        sprintf(json_buff, "meter_%d_%s", (midx + 1), temp_met_sn);
    }
    else
    {
        sprintf(json_buff, "meter_%d_%d_%s", g_pidx, (midx + 1), temp_met_sn);
    }

    // dbg_log(INFORM, "[%-25s] : key to add in meter_inst_info (temp_met_sn) : %s \n", fun_name, temp_met_sn);

    reply = redisCommand(redis, "HMSET %s %s %s", hash_key, json_buff, inst_json);
    print_colored_message(RED, PRINT, "===================1 json_buff field name %s ==============================\n", json_buff);
    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
        print_colored_message(RED, PRINT, "================2=================================\n");
        free(inst_json);
        return -1;
    }
    print_colored_message(RED, PRINT, "========================3=========================\n");
    if (reply->type == REDIS_REPLY_ERROR)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
        print_colored_message(RED, PRINT, "=======================4==========================\n");
        freeReplyObject(reply);
        print_colored_message(RED, PRINT, "======================5===========================\n");
        free(inst_json);
        return -1;
    }
    print_colored_message(RED, PRINT, "=============================6====================\n");
    freeReplyObject(reply);
    print_colored_message(RED, PRINT, "==========================7=======================\n");

    free(inst_json);
    return 0;
}

/**
 * Updates the nameplate information in the Redis database for a specified meter index.
 *
 * @param midx The index of the meter whose nameplate is being updated.
 * @param info Pointer to a structure containing the nameplate information.
 * @return 0 on success, -1 on failure.
 */
int update_name_plate(int midx, const nameplate_info_t *info)
{
    static char fun_name[] = "update_name_plate";
    redisReply *reply;
    char key[64];
    nx_json *json_obj = NULL;
    // int i;
    char temp_buff[64];
    // rithika 19/08/2025
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    char datetime_str[32];

    strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);
    ///////

    memset(json_send_buff, 0, sizeof(json_send_buff));

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "{\n");
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"update_time\" : \"%s\",\n", datetime_str);
    strcat(json_send_buff, json_buff);

    // memset(json_buff, 0, sizeof(json_buff));
    // sprintf(json_buff, "\"location\" : \"%s\",\n", info->location);
    // strcat(json_send_buff, json_buff);

    // rithika 31oct2025
    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"location\" : \"%s\",\n", ser_port_data[g_pidx].meter_cfg[midx].meter_loc);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"met_id\" : \"%d\",\n", (midx + 1));
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"meter_name\" : \"M_%d_%d\",\n", midx + 1, g_pidx);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"port\" : \"%d\",\n", g_pidx);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"dcu_serial_number\" : \"%s\",\n", dcu_ser_num);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"serial_number\" : \"%s\",\n", info->serial_number);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"manufacturer\" : \"%s\",\n", info->manufacturer);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"meter_type\" : \"%s\",\n", info->meter_type);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"firmware_version\" : \"%s\",\n", info->firmware_version);
    strcat(json_send_buff, json_buff);

    // rithika 27sep
    if (info->ct_ratio_str[0] != '\0')
    {

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"CT_ratio\" : \"%s\",\n", info->ct_ratio_str);
        strcat(json_send_buff, json_buff);
    }
    else
    {
        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"CT_ratio\" : \"%f\",\n", info->ct_ratio);
        strcat(json_send_buff, json_buff);
    }
    // rithika 27sep
    if (info->pt_ratio_str[0] != '\0')
    {
        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"PT_ratio\" : \"%s\",\n", info->pt_ratio_str);
        strcat(json_send_buff, json_buff);
    }
    else
    {
        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"PT_ratio\" : \"%f\",\n", info->pt_ratio);
        strcat(json_send_buff, json_buff);
    }

    // rithika 18Feb2026
    if (RECONNECT)
    {
        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"meter_category\" : \"%s\",\n", info->meter_category);
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"current_rating\" : \"%s\",\n", info->curr_rating);
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"year_of_manufacture\" : \"%s\",\n", info->year_of_manuf);
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"demand_int_period\" : \"%d\",\n", info->demand_int_period);
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"hdlc_device_address\" : \"%d\",\n", info->hdlc_device_address);
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"ipv4_address\" : \"%s\",\n", info->ipv4_address);
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"transfrmr_volt\" : \"%s\",\n", "");
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"extra_obis_1\" : \"%s\",\n", "");
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"extra_obis_2\" : \"%s\",\n", "");
        strcat(json_send_buff, json_buff);

        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "\"extra_obis_3\" : \"%s\",\n", "");
        strcat(json_send_buff, json_buff);
    }

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"block_int\" : \"%d\"\n", info->block_int);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "}");
    strcat(json_send_buff, json_buff);

#if DEBUG
    print_colored_message(RED, PRINT, "=================================================\n");
    printf("Name plate  data JSON :%s\n", json_send_buff);
    print_colored_message(RED, PRINT, "=================================================\n");
#endif

    if (OD_CHANGES == 0)
    {
        snprintf(key, sizeof(key), "meter%d_details", midx + 1);
    }
    else
    {
        // rithika 29/08/2025
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            snprintf(key, sizeof(key), "meter%d_%s_details", midx + 1, info->serial_number);
        }
        else
        {
            snprintf(key, sizeof(key), "meter_%d_%d_%s_details", g_pidx, midx + 1, info->serial_number);
        }
    }

    memset(temp_buff, 0, sizeof(temp_buff));
    // rithika 29/08/2025
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        snprintf(temp_buff, sizeof(temp_buff), "meter%d_", (midx + 1));
    }
    else
    {
        snprintf(temp_buff, sizeof(temp_buff), "meter_%d_%d_", g_pidx, (midx + 1));
    }
    // rithika 26Nov2025
    //  int deleted_count = delete_old_metercommn_status(temp_buff, "meter_status");

    // if (deleted_count < 0)
    // {
    //     fprintf(stderr, "Failed to delete keys starting with %s\n", temp_buff);
    //     return -1;
    // }
    // else if (deleted_count > 0)
    // {
    //     printf("Deleted %d keys with prefix %s\n", deleted_count, temp_buff);
    // }
    // else
    // {
    //     printf("No keys with prefix %s found\n", temp_buff);
    // }

    reply = redisCommand(redis, "HSET meter_status %s %s", key, json_send_buff);

    if (reply == NULL)
    {
        fprintf(stderr, "Redis command failed: %s\n", redis->errstr);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }
    if (reply->type == REDIS_REPLY_ERROR)
    {
        fprintf(stderr, "Redis error: %s\n", reply->str);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);

    return 0;
}

/**
 * Updates the OBIS (Object Identification System) codes in the Redis database.
 *
 * @return 0 on success, -1 on failure.
 */
// rithika 17Feb2026 commented because there is no use of this redis hash
int update_np_obis()
{
    static char fun_name[] = "update_np_obis";
    char key[64];
    redisReply *reply;
    int i, j;
#if 0
    printf("BEFORE OBIS store_data_polling_strategy : %s\n", strategy_cfg.store_poll_starts);
    printf("periodic_polling_strategy : %s\n", strategy_cfg.periodic_poll_starts);
    printf("init_polling_strategy : %s\n", strategy_cfg.initial_poll_starts);
    printf("idle_polling_strategy : %s\n", strategy_cfg.idle_poll_starts);
#endif
    memset(json_send_buff, 0, sizeof(json_send_buff));

    // memset(json_buff, 0, sizeof(json_buff));
    // sprintf(json_buff, "{");
    // strcat(json_send_buff, json_buff);

    // memset(json_buff, 0, sizeof(json_buff));
    // sprintf(json_buff, "\"obis_codes\" : [");
    // strcat(json_send_buff, json_buff);

    // for (i = 0; i < MAX_NAMEPLATE_VALS; i++)
    // {
    //     sprintf(json_buff, "\"");
    //     strcat(json_send_buff, json_buff);

    //     for (j = 0; j < 6; j++)
    //     {
    //         sprintf(json_buff, "%u", name_plate_obis[i][j]);
    //         strcat(json_send_buff, json_buff);
    //         if (j < 5)
    //         {
    //             strcat(json_send_buff, ", ");
    //         }
    //     }

    //     strcat(json_send_buff, "\"");

    //     if (i < MAX_NAMEPLATE_VALS - 1)
    //     {
    //         strcat(json_send_buff, ",");
    //     }
    //     // else
    //     // {
    //     //     strcat(json_send_buff, "\n");
    //     // }
    // }

    // sprintf(json_buff, "  ]}");
    // strcat(json_send_buff, json_buff);

    int idx = 0;
    idx += sprintf(json_send_buff + idx, "{ \"obis_codes\": [");

    for (i = 0; i < MAX_NAMEPLATE_VALS; i++)
    {
        if (i != 0)
            idx += sprintf(json_send_buff + idx, ",");
        idx += sprintf(json_send_buff + idx,
                       "\"%u, %u, %u, %u, %u, %u\"",
                       name_plate_obis[i][0],
                       name_plate_obis[i][1],
                       name_plate_obis[i][2],
                       name_plate_obis[i][3],
                       name_plate_obis[i][4],
                       name_plate_obis[i][5]);
    }

    sprintf(json_send_buff + idx, "] }");
    sprintf(key, "meter_make_np_obis");
    // Now store in Redis
    reply = redisCommand(redis, "HSET %s num_codes %d obis_codes '%s'", key, MAX_NAMEPLATE_VALS, json_send_buff);

#if DEBUG
    printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Name plate  data OBIS :%s\n", json_send_buff);
#endif

    //

    // memset(json_buff, 0, sizeof(json_buff));
    // sprintf(json_buff, "HSET %s  num_codes %d obis_codes '%s'", key, MAX_NAMEPLATE_VALS, json_send_buff);

    // reply = redisCommand(redis, json_buff);
    if (reply == NULL)
    {
        fprintf(stderr, "Redis command failed: %s\n", redis->errstr);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
        return -1;
    }
    if (reply->type == REDIS_REPLY_ERROR)
    {
        fprintf(stderr, "Redis error: %s\n", reply->str);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);
#if 0
    printf("BEFORE AFTER store_data_polling_strategy : %s\n", strategy_cfg.store_poll_starts);
    printf("periodic_polling_strategy : %s\n", strategy_cfg.periodic_poll_starts);
    printf("init_polling_strategy : %s\n", strategy_cfg.initial_poll_starts);
    printf("idle_polling_strategy : %s\n", strategy_cfg.idle_poll_starts);
#endif
    return 0;
}

/**
 * Adds the process state information for a specified meter index to the Redis list.
 *
 * @param midx The index of the meter.
 * @param process_state The current process state.
 * @param data_type The type of data being processed.
 * @return 0 on success, -1 on failure.
 */
int add_process_state(int midx, char *process_state, char *data_type)
{

    static char fun_name[] = "add_process_state()";

    redisReply *reply;
    char final_string[256];
    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    char datetime_str[32];

    strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);
    // rithika 10oct2025
    sprintf(final_string, "%s:%s:%d:%d:%s:%s", datetime_str, "serial", g_pidx, midx + 1, process_state, data_type);

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    reply = redisCommand(redis, "LPUSH %s %s", "dlms_ser", final_string);
    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        snprintf(redis_event_data.eventmsg, sizeof(redis_event_data.eventmsg), "%s : Error: %s", fun_name, redis->errstr);
        snprintf(redis_event_data.timestamp, sizeof(redis_event_data.timestamp), "%s", time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
        return 1;
    }
    else
    {
        redisReply *reply_trim = redisCommand(redis, "LTRIM %s 0 %d", "dlms_ser", MAX_DIAG_MESSAGE_LIMIT - 1);
        if (reply_trim != NULL)
        {
            freeReplyObject(reply_trim);
        }
    }

    freeReplyObject(reply);

    return 0;
}

/**
 * Checks if a specified Redis list is empty by querying its length.
 *
 * @param redis_ctx A pointer to the Redis context used for the connection.
 * @param list_name The name of the Redis list to check.
 * @return 1 if the list is empty, 0 if it contains elements, or -1 on failure.
 */
int is_list_empty(redisContext *redis_ctx, const char *list_name)
{
    static char fun_name[] = "is_list_empty";

    // printf(RED "--------------- list_name: %s --------------\n" RESET, list_name);

    if (redis_ctx == NULL || redis_ctx->err)
    {
        fprintf(stderr, "Redis connection error: %s\n", redis_ctx ? redis_ctx->errstr : "Unknown error");
        return -1;
    }

    redisReply *reply = redisCommand(redis_ctx, "LLEN %s", list_name);

    if (reply == NULL)
    {
        fprintf(stderr, "Redis command failed\n");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }

    // if (reply->type == REDIS_REPLY_INTEGER)
    // {
    //     printf(RED "--------------- is_empty: %d --------------\n" RESET, reply->integer);
    // }
    // else
    // {
    //     printf("Unexpected reply type: %d\n", reply->type);
    // }

    // if (reply->integer == 0)
    // {
    //     printf("List '%s' is empty.\n", list_name);
    // }
    // else
    // {
    //     printf("List '%s' has %d elements.\n", list_name, reply->integer);
    // }

    int is_empty = reply->integer;

    // printf(RED "List '%s' has %d elements.\n" RESET, list_name, is_empty);

    freeReplyObject(reply);

    return is_empty;
}

int delete_processed_cmd(od_cmd_t od_cmd_delete)
{
    static char fun_name[] = "delete_processed_cmd()";

    const char *list_name = "web_od_command";
    const char *p_gen_ptr = NULL;
    int num_cmd = 0, ret = 0;
    od_cmd_t od_cmd_recv;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    // Check if the list is empty
    int empty_check = is_list_empty(redis, list_name);
    if (empty_check == -1)
    {
        return -1;
    }

    // print_colored_message(RED, 1, "--------------- cmd_list size: %d --------------\n", empty_check);
    snrm_aarq_failed_cnt("[%s] : [%-25s] : checking the  same command \n", time_format_conversion(time(NULL)), fun_name);

    for (num_cmd = 0; num_cmd < empty_check; num_cmd++)
    {
        memset(&od_cmd_recv, 0, sizeof(od_cmd_t));

        printf("------------------- num_cmd : %d ---------------\n", num_cmd);

        redisReply *reply = redisCommand(redis, "LINDEX %s %d", list_name, num_cmd);
        if (reply->type != REDIS_REPLY_STRING)
        {
            printf("Error fetching list item\n");
            freeReplyObject(reply);
            return -1;
        }

        if (reply->type == REDIS_REPLY_STRING)
        {
            const char *json_send_buffing = reply->str;
            const nx_json *json = nx_json_parse(json_send_buffing, 0);

            if (json)
            {
                // Extract seq_no and msg_type from the JSON
                p_gen_ptr = nx_json_get(json, "seq_no")->text_value;
                if (p_gen_ptr != NULL)
                {
                    strncpy(od_cmd_recv.seq_no, p_gen_ptr, sizeof(od_cmd_recv.seq_no) - 1);
                }

                p_gen_ptr = nx_json_get(json, "msgType")->text_value;
                if (p_gen_ptr != NULL)
                {
                    strncpy(od_cmd_recv.msg_type, p_gen_ptr, sizeof(od_cmd_recv.msg_type) - 1);
                }

                // Extract the "data" object and its "meter" value
                const nx_json *data = nx_json_get(json, "data");
                if (data)
                {
                    p_gen_ptr = nx_json_get(data, "meter")->text_value;
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd_recv.meter, p_gen_ptr, sizeof(od_cmd_recv.meter) - 1);
                    }
                }

                // Check if the received data matches the delete criteria
                if (od_cmd_recv.msg_type != NULL)
                {
                    // Debugging output for parsed values
                    printf("Parsed data:\n");
                    printf("msg_type: %s - %s\n", od_cmd_recv.msg_type, od_cmd_delete.msg_type);
                    printf("seq_no: %s - %s\n", od_cmd_recv.seq_no, od_cmd_delete.seq_no);
                    printf("meter serial number : %s - %s\n", od_cmd_recv.meter, od_cmd_delete.meter);

                    // Check if this command matches the one to delete
                    if ((strcmp(od_cmd_recv.seq_no, od_cmd_delete.seq_no) == 0) &&
                        (strcmp(od_cmd_recv.meter, od_cmd_delete.meter) == 0) &&
                        (strcmp(od_cmd_recv.msg_type, od_cmd_delete.msg_type) == 0))
                    {
                        // Command matched, pop it from the list
                        redisReply *reply1 = redisCommand(redis, "LPOP %s", list_name);
                        if (reply1 == NULL)
                        {
                            fprintf(stderr, "Redis command failed to pop item\n");
                            freeReplyObject(reply1);
                            continue;
                        }
                        printf("Deleted the matched command\n");
                        snrm_aarq_failed_cnt("[%s] : [%-25s] : Deleted the matched command \n", time_format_conversion(time(NULL)), fun_name);
                        freeReplyObject(reply1); // Free the reply after use
                        num_cmd--;
                    }
                }

                // Free the JSON object after parsing
                nx_json_free(json);
            }
            else
            {
                fprintf(stderr, "Error parsing JSON\n");
                memset(event_msg_str, 0, sizeof(event_msg_str));
                sprintf(redis_event_data.eventmsg, "%s : JSON parsing failed", fun_name);
                strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
                add_event_data(&redis_event_data);
            }
        }
        else
        {
            fprintf(stderr, "Unexpected Redis reply type\n");
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Unexpected Redis reply type", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
        }

        // Free the Redis reply object after processing
        freeReplyObject(reply);
    }

    return 0;
}

int update_od_sts_ftp(od_cmd_t od_cmd_temp)
{
    print_colored_message(YELLOW, 1, "************************ inside update_od_sts_ftp ****************\n");
    static char fun_name[] = "update_od_sts_ftp";

    cJSON *root = cJSON_CreateObject();

    if (!root)
    {
        printf("Failed to create JSON object\n");
        return -1;
    }

    // Add string fields to the object
    cJSON_AddStringToObject(root, "msg_type", od_cmd_temp.msg_type);
    cJSON_AddStringToObject(root, "meter_ser_num", od_cmd_temp.meter);
    cJSON_AddStringToObject(root, "num_days", od_cmd_temp.num_days);
    cJSON_AddStringToObject(root, "start_date", od_cmd_temp.start_date);

    int ls_num_days = atoi(od_cmd_temp.num_days);

    if (strcmp(od_cmd_temp.msg_type, "OD_LS_DATA") == 0 && ls_num_days > 1)
    {
        cJSON_AddStringToObject(root, "curr_date", one_day_str);
    }
    else
    {
        cJSON_AddStringToObject(root, "curr_date", od_cmd_temp.start_date);
    }

    char portid[4];
    snprintf(portid, sizeof(portid), "%u", g_pidx);
    cJSON_AddStringToObject(root, "port_id", portid);

    // midx_od_ftp += 1;
    char meterid[4];
    snprintf(meterid, sizeof(meterid), "%u", midx_od_ftp);

    cJSON_AddStringToObject(root, "meter_id", meterid);

    // Convert to JSON string
    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        printf("Failed to print JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    redisReply *reply = redisCommand(redis, "LPUSH ftp_od_cmd_resp %s", json_string);

    if (reply == NULL)
    {
        fprintf(stderr, "Redis command failed\n");

        print_colored_message(RED, 1, "Redis command failed\n");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : LTRIM  Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }
    else
    {
        redisReply *reply_trim = redisCommand(redis, "LTRIM ftp_od_cmd_resp 0 %d", MAX_DIAG_MESSAGE_LIMIT - 1);
        print_colored_message(RED, 1, "Else Redis command failed\n");
        if (reply_trim != NULL)
        {
            freeReplyObject(reply_trim);
        }
    }

    freeReplyObject(reply);
    free(json_string);
    cJSON_Delete(root);
}

/**
 * Updates the command status for a specified operational command and pushes the
 * status information to a Redis list if the source is "web".
 *
 * @param od_cmd_temp A structure containing details of the operational command.
 * @param od_status The current status of the operational command.
 * @return 0 on success, -1 on failure (e.g., if Redis command fails).
 */
int update_cmd_status(od_cmd_t od_cmd_temp, char *od_status)
{
    static char fun_name[] = "update_cmd_status";

    // rithika 28/08/2025
    if (strstr(od_status, "SUCCESS") && ftp_enable == 1)
    {
        update_od_sts_ftp(od_cmd_temp);

        dbg_log(WARNING, "[%-25s] : Given ftp success message for date %s\n", fun_name, one_day_str);
    }

    memset(json_send_buff, 0, sizeof(json_send_buff));
    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "{\n");
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cmsg_type%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.msg_type);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cseq_no%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.seq_no);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cinit_source%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.init_source);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cstatus%c:\"%s\",", DBL_QUOTES, DBL_QUOTES, od_status);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cdata%c:\n", DBL_QUOTES, DBL_QUOTES);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "{\n");
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cmeter%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.meter);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));

    if (strcmp(od_cmd_temp.msg_type, "OD_EVENT_DATA") == 0)
        sprintf(json_buff, "%cevent_type%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.event_type);
    else if (strcmp(od_cmd_temp.msg_type, "OD_OVERALL_DB_SIZE") == 0)
        sprintf(json_buff, "%cdb_size%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.db_size);
    else if (strcmp(od_cmd_temp.msg_type, "OD_OVERALL_NET_SIZE") == 0)
        sprintf(json_buff, "%cnw_size%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.nw_size);
    else if (strcmp(od_cmd_temp.msg_type, "OD_TIMESYNC_MESSAGE") == 0)
        sprintf(json_buff, "%cset_time%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_DEMAND_PERIOD_MESSAGE") == 0)
        sprintf(json_buff, "%cperiod%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_PROF_CAP_PERIOD_MESSAGE") == 0)
        sprintf(json_buff, "%cperiod%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_METERING_MODE_MESSAGE") == 0)
        sprintf(json_buff, "%cmode%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_PAYMENT_MODE_MESSAGE") == 0)
        sprintf(json_buff, "%cmode%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_LAST_TKN_RECH_AMT_MESSAGE") == 0)
        sprintf(json_buff, "%camount%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_TOT_AMT_LAST_RECH_MESSAGE") == 0)
        sprintf(json_buff, "%camount%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_CURR_BAL_AMT_MESSAGE") == 0)
        sprintf(json_buff, "%camount%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_LOAD_LIMIT_EN_DIS_MESSAGE") == 0)
        sprintf(json_buff, "%cload_val%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_MD_RESET_MESSAGE") == 0)
        sprintf(json_buff, "%cval%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_SET_LOAD_MESSAGE") == 0)
        sprintf(json_buff, "%climit_val%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_EN_DIS_RLY_OPER_MESSAGE") == 0)
        sprintf(json_buff, "%crly_val%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_APN_MESSAGE") == 0)
    {
        sprintf(json_buff, "%capn_name%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.apn_name);
        // strcat(json_send_buff, json_buff);

        // memset(json_buff, 0, sizeof(json_buff));

        // sprintf(json_buff, "%capn_bearer%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.apn_bearer);
    }
    else if (strcmp(od_cmd_temp.msg_type, "OD_LAST_TKN_RECH_TIME_MESSAGE") == 0)
        sprintf(json_buff, "%ctime%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_CURR_BAL_TIME_MESSAGE") == 0)
        sprintf(json_buff, "%ctime%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_SNGL_ACT_SCHD_MESSAGE") == 0)
        sprintf(json_buff, "%cval%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_PUSH_SETUP_MESSAGE") == 0)
        sprintf(json_buff, "%cval%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_PUSH_ALERT_MESSAGE") == 0)
        sprintf(json_buff, "%cval%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_ESWF_MESSAGE") == 0)
        sprintf(json_buff, "%cval%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_ACT_CAL_MESSAGE") == 0) // OD_LOAD_CONN_DISCONN_MESSAGE
        sprintf(json_buff, "%cval%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);
    else if (strcmp(od_cmd_temp.msg_type, "OD_LOAD_CONN_DISCONN_MESSAGE") == 0) // OD_LOAD_CONN_DISCONN_MESSAGE
        sprintf(json_buff, "%cload_val%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.resp_msg);

    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cport_id%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.port_id);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cnum_days%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.num_days);
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%cstart_date%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.start_date);
    strcat(json_send_buff, json_buff);

    // rithika 02/12/2025
    if (strcmp(od_cmd_temp.msg_type, "OD_LS_DATA") == 0 && od_cmd_temp.num_days > 1)
    {
        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "%ccurr_date%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, one_day_str);
        strcat(json_send_buff, json_buff);
    }
    else
    {
        memset(json_buff, 0, sizeof(json_buff));
        sprintf(json_buff, "%ccurr_date%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.start_date);
        strcat(json_send_buff, json_buff);
    }
    // memset(json_buff, 0, sizeof(json_buff));
    // sprintf(json_buff, "%cnum_days%c:\"%s\",\n", DBL_QUOTES, DBL_QUOTES, od_cmd_temp.num_days);
    // strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "%ctime_stamp%c:\"%s\"\n", DBL_QUOTES, DBL_QUOTES, time_format_conversion(time(NULL)));
    strcat(json_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "}}\n");
    strcat(json_send_buff, json_buff);

    snrm_aarq_failed_cnt("[%s] : [%-25s] : CMD_STATUS : %s \n", time_format_conversion(time(NULL)), fun_name, json_send_buff);

    print_colored_message(RED, 1, "^^^^^^^^^^^^^^^^^^^^^ %s ^^^^^^^^^^^^^^^^^^^^^\n", json_send_buff);

    if (strcmp(od_cmd_temp.init_source, "web") == 0)
    {
        redisReply *reply = redisCommand(redis, "LPUSH web_od_command_resp %s", json_send_buff);

        if (reply == NULL)
        {
            fprintf(stderr, "Redis command failed\n");

            print_colored_message(RED, 1, "Redis command failed\n");

            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : LTRIM  Redis command failed", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);

            return -1;
        }
        else
        {
            print_colored_message(RED, 1, "Els Redis command failed\n");
            freeReplyObject(reply);
        }
    }
     else if (strcmp(od_cmd_temp.init_source, "mqtt") == 0)
    {
        if (strstr(od_status, "SUCCESS") || strstr(od_status, "Failed"))
        {
            redisReply *reply = redisCommand(redis, "LPUSH mqtt_command_resp %s", json_send_buff);

            if (reply == NULL)
            {
                fprintf(stderr, "Redis command failed\n");

                print_colored_message(RED, 1, "Redis command failed\n");

                memset(event_msg_str, 0, sizeof(event_msg_str));
                sprintf(redis_event_data.eventmsg, "%s : LTRIM  Redis command failed", fun_name);
                strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
                add_event_data(&redis_event_data);

                return -1;
            }
            else
            {
                print_colored_message(RED, 1, "Els Redis command failed\n");
                freeReplyObject(reply);
            }
        }
    }

    return 0;
}

int get_ser_num_to_metid(char *ser_num)
{
    static char fun_name[] = "get_ser_num_to_metid()";

    char key[64];

    int i, met_id = -1;

    redisReply *reply;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    for (i = 0; i < plcc_modem_info.no_of_nodes; i++)
    {
        printf("%d\n", i);
        sprintf(key, "meter_%d_meter_ser_num", (i + 1));

        printf("key : %s\n", key);

        reply = redisCommand(redis, "HGET %s %s", HASH_KEY, key);
        if (reply == NULL)
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            return -1;
        }

        char *result = NULL;
        if (reply->type == REDIS_REPLY_STRING)
        {

            result = strdup(reply->str);
            printf("result : %s ser_num : %s\n", result, ser_num);
            if (strcmp(result, ser_num) == 0)
            {
                met_id = i;
                printf("met_id : %d\n", met_id);
                break;
            }
            else
                continue;
            freeReplyObject(reply);
        }
        else
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Unexpected reply type: %d", fun_name, reply->type);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            freeReplyObject(reply);
            return -1;
        }
    }

    return met_id;
}

int get_ser_num_to_metid_serial(char *met_ser_num)
{
    redisReply *meter_reply = redisCommand(redis, "HKEYS meter_status");
    char met_id[32];
    int meter_id = -1, j;

    if (meter_reply && meter_reply->type == REDIS_REPLY_ARRAY)
    {
        for (j = 0; j < meter_reply->elements; j++)
        {
            char *field = meter_reply->element[j]->str;

            if (met_ser_num[0] != '\0' && strstr(field, met_ser_num))
            {
                redisReply *meter_id_reply = redisCommand(redis, "HGET meter_status %s", field);
                if (meter_id_reply && meter_id_reply->type == REDIS_REPLY_STRING)
                {
                    // Parse JSON using cJSON
                    cJSON *meter_det_obj = cJSON_Parse(meter_id_reply->str);
                    if (meter_det_obj)
                    {
                        cJSON *met_id_obj = cJSON_GetObjectItemCaseSensitive(meter_det_obj, "met_id");
                        if (cJSON_IsString(met_id_obj) && (met_id_obj->valuestring != NULL))
                        {
                            strncpy(met_id, met_id_obj->valuestring, sizeof(met_id) - 1);
                            met_id[sizeof(met_id) - 1] = '\0';

                            printf("------------ meter_id %s ------------\n", met_id);
                            meter_id = atoi(met_id);

                            meter_id -= 1;

                            cJSON_Delete(meter_det_obj);
                            freeReplyObject(meter_id_reply);
                            break;
                        }
                        cJSON_Delete(meter_det_obj);
                    }
                }
                if (meter_id_reply)
                    freeReplyObject(meter_id_reply);
            }
        }
    }

    if (meter_reply)
        freeReplyObject(meter_reply);

    return meter_id;
}

activity_calendar parse_act_calender_json(char *val1)
{

    printf("%s\n", val1);
    cJSON *root = cJSON_Parse(val1);
    if (!root)
    {
        printf("JSON parsing failed\n");
    }

    activity_calendar cal = {0};

    // cJSON *data = cJSON_GetObjectItem(root, "data");
    // cJSON *val = cJSON_GetObjectItem(data, "val");

    // Calendar name
    cJSON *cal_name = cJSON_GetObjectItem(root, "cal_name");
    if (cal_name)
        strncpy(cal.calendar_name, cal_name->valuestring, sizeof(cal.calendar_name));

    // ---------- Day Profile ----------
    cJSON *day_prof = cJSON_GetObjectItem(root, "day_prof");
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
    cJSON *season_arr = cJSON_GetObjectItem(root, "season");
    if (season_arr && cJSON_IsArray(season_arr))
    {
        cal.num_seasons = cJSON_GetArraySize(season_arr);
        cal.num_weeks = 0; // Initialize week count before loop

        for (int i = 0; i < cal.num_seasons && i < MAX_SEASONS; i++)
        {
            cJSON *season_obj = cJSON_GetArrayItem(season_arr, i);
            cJSON *season_prof = cJSON_GetObjectItem(season_obj, "season_prof");

            // Extract season name and start time
            strncpy(cal.season_profile[i].season_name,
                    cJSON_GetObjectItem(season_prof, "season_name")->valuestring, 32);
            strncpy(cal.season_profile[i].start_time,
                    cJSON_GetObjectItem(season_prof, "start_time")->valuestring, 64);

            // Extract week profile
            cJSON *week_prof = cJSON_GetObjectItem(season_prof, "week_prof");
            cJSON *week_name = cJSON_GetObjectItem(week_prof, "week_name");
            cJSON *week_days = cJSON_GetObjectItem(week_prof, "week_days");

            if (week_prof && cJSON_IsArray(week_days))
            {
                int widx = cal.num_weeks;

                // Save reference to week profile name in season_profile
                strncpy(cal.season_profile[i].week_name, week_name->valuestring, 32);

                // Save week profile content
                strncpy(cal.week_profile[widx].week_name, week_name->valuestring, 32);
                for (int j = 0; j < cJSON_GetArraySize(week_days) && j < 7; j++)
                {
                    cal.week_profile[widx].weekday[j] = cJSON_GetArrayItem(week_days, j)->valueint;
                }

                cal.num_weeks++; // Increment only if a valid week profile was saved
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

    return cal;
}

int check_meter_port_id(int index)
{
    redisReply *reply = redisCommand(redis, "LINDEX %s %d", "web_od_command", index);
    if (!reply || reply->type != REDIS_REPLY_STRING)
    {
        fprintf(stderr, "Failed to fetch item %d from Redis list.\n", index);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    // Parse JSON using cJSON
    cJSON *root = cJSON_Parse(reply->str);
    if (!root)
    {
        fprintf(stderr, "Failed to parse JSON at index %d.\n", index);
        freeReplyObject(reply);
        return -1;
    }

    // Get the "data" object
    cJSON *data_obj = cJSON_GetObjectItemCaseSensitive(root, "data");
    if (data_obj)
    {
        // Get the "port_id" field (expected to be a string like "123")
        cJSON *port_obj = cJSON_GetObjectItemCaseSensitive(data_obj, "port_id");
        // cJSON *meter_obj = cJSON_GetObjectItemCaseSensitive(data_obj, "meter");

        if ((cJSON_IsString(port_obj) && (port_obj->valuestring != NULL)))
        {
            // Convert port_id string to integer
            int port_id_val = atoi(port_obj->valuestring);

            printf("[Index %d] port_id (JSON): %d, g_pidx: %u\n", index, port_id_val, g_pidx);
            // printf("[Index %d] Meter: %s\n", index, meter_obj->valuestring);
            // Compare with g_pidx
            if ((uint8_t)port_id_val == g_pidx)
            {
                cJSON_Delete(root);
                freeReplyObject(reply);
                print_colored_message(RED, 1, "--------------- return 0 --------------\n");
                return index;
            }
        }
    }
    else
    {
        printf("[Index %d] 'data' object not found.\n", index);
    }

    // Clean up
    cJSON_Delete(root);
    freeReplyObject(reply);
    print_colored_message(RED, 1, "--------------- return -1 --------------\n");
    return -1;
}

Date add_one_day(Date d)
{
    // rithika 01/12/2025
    // int days_this_month = days_in_month(d.month, d.year);
    int days_this_month = days_in_month(d.month - 1, d.year);
    d.day++;
    if (d.day > days_this_month)
    {
        d.day = 1;
        d.month++;
        if (d.month > 12)
        {
            d.month = 1;
            d.year++;
        }
    }
    return d;
}

/**
 * Processes operational data commands from a Redis list, executing actions
 * based on the commands' parameters.
 *
 * This function retrieves commands from the Redis list named "Web_od_command",
 * parses the JSON structure of each command, and performs actions based on
 * the message type specified in the command. It supports two types of commands:
 * "OD_LS_DATA" and "OD_INST_DATA". The function updates the command status
 * and handles any errors encountered during processing.
 *
 * @return 0 on success, -1 on failure (error occurred while checking list status).
 */
int process_od_cmd()
{
    static char fun_name[] = "process_od_cmd()";

    const char *list_name = "web_od_command";
    const char *p_gen_ptr = NULL, *seq_num = NULL, *cmd_type = NULL;
    int midx = 0, ret = 0, num_cmd = 0, cnt;
    char temp_buf[64];
    char temp_buff[256];

    int ret_redis_idx = -1;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    int empty_check = is_list_empty(redis, list_name);

    // print_colored_message(RED, 1, "--------------- cmd_list size: %d --------------\n", empty_check);

    if (empty_check == -1)
    {
        return -1;
    }

    print_colored_message(RED, 1, "--------------- cmd_list size: %d --------------\n", empty_check);

    for (num_cmd = 0; num_cmd < empty_check; num_cmd++)
    {

        if (strcmp(feature_cfg.meter_inf_type, "PLCC") != 0)
        {
            ret_redis_idx = check_meter_port_id(num_cmd);
            // if (ret_redis_idx != 0)
            // {
            //     continue;
            // }
        }

        // rithika 26nov2025 need to check for plcc whether it is working with this change
        if (ret_redis_idx == -1)
        {
            continue;
        }

        memset(&od_cmd, 0, sizeof(od_cmd_t));

        // rithika 26nov2025
        //  redisReply *reply = redisCommand(redis, "LPOP web_od_command");
        redisReply *reply = redisCommand(redis, "LRANGE web_od_command %d %d", ret_redis_idx, ret_redis_idx);

        printf("********************************************************************\n");

        if (reply == NULL)
        {
            fprintf(stderr, "Redis command failed\n");

            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            continue;
        }

        // rithika 26nov2025
        //  if (reply->type == REDIS_REPLY_STRING)
        //  {
        //   const char *json_send_buffing = reply->str;
        if (reply->type == REDIS_REPLY_ARRAY && reply->elements == 1)
        {

            const char *json_send_buffing = reply->element[0]->str;

            redisReply *del = redisCommand(redis, "LREM web_od_command 1 %s", json_send_buffing);

            if (del)
            {
                freeReplyObject(del);
            }

            const nx_json *json = nx_json_parse(json_send_buffing, 0);

            printf("*** %s ***\n", json_send_buffing);

            if (json)
            {

                p_gen_ptr = nx_json_get(json, "seq_no")->text_value;
                if (p_gen_ptr != NULL)
                {
                    strncpy(od_cmd.seq_no, p_gen_ptr, sizeof(od_cmd.seq_no) - 1);
                }

                p_gen_ptr = nx_json_get(json, "init_source")->text_value;
                if (p_gen_ptr != NULL)
                {
                    strncpy(od_cmd.init_source, p_gen_ptr, sizeof(od_cmd.init_source) - 1);
                }

                p_gen_ptr = nx_json_get(json, "msgType")->text_value;
                if (p_gen_ptr != NULL)
                {
                    strncpy(od_cmd.msg_type, p_gen_ptr, sizeof(od_cmd.msg_type) - 1);
                }

                const nx_json *data = nx_json_get(json, "data");
                if (data)
                {

                    p_gen_ptr = nx_json_get(data, "meter")->text_value; // set_time,period,load_val,rly_val
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.meter, p_gen_ptr, sizeof(od_cmd.meter) - 1);
                    }
                    else
                    {
                        strncpy(od_cmd.meter, "na", sizeof(od_cmd.meter) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "set_time")->text_value; // set_time amount
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "period")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "mode")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "amount")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "load_val")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "limit_val")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "rly_val")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "time")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "apn_name")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.apn_name, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "apn_bearer")->text_value; // set_time
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.apn_bearer, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "val")->text_value; // set_time
                    if (p_gen_ptr)
                    {

                        strncpy(od_cmd.commn_msg, p_gen_ptr, sizeof(od_cmd.commn_msg) - 1);
                        printf("*** inside val : %s ***\n", od_cmd.commn_msg);
                    }

                    p_gen_ptr = nx_json_get(data, "startdate")->text_value;
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.start_date, p_gen_ptr, sizeof(od_cmd.start_date) - 1);
                    }
                    else
                    {
                        strncpy(od_cmd.start_date, "na", sizeof(od_cmd.start_date) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "num_days")->text_value;
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.num_days, p_gen_ptr, sizeof(od_cmd.num_days) - 1);
                    }
                    else
                    {
                        strncpy(od_cmd.num_days, "na", sizeof(od_cmd.num_days) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "event_type")->text_value;
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.event_type, p_gen_ptr, sizeof(od_cmd.event_type) - 1);
                    }
                    else
                    {
                        strncpy(od_cmd.event_type, "na", sizeof(od_cmd.event_type) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "time_stamp")->text_value;
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.time_stamp, p_gen_ptr, sizeof(od_cmd.time_stamp) - 1);
                    }
                    else
                    {
                        strncpy(od_cmd.time_stamp, "na", sizeof(od_cmd.time_stamp) - 1);
                    }

                    p_gen_ptr = nx_json_get(data, "port_id")->text_value;
                    if (p_gen_ptr)
                    {
                        strncpy(od_cmd.port_id, p_gen_ptr, sizeof(od_cmd.port_id) - 1);
                    }
                    else
                    {
                        strncpy(od_cmd.port_id, "na", sizeof(od_cmd.port_id) - 1);
                    }
                }

#if 1
                if (od_cmd.msg_type != NULL)
                {
                    // strncpy(od_cmd.meter, p_gen_ptr, sizeof(od_cmd.meter) - 1);
                    if ((strcmp(od_cmd.meter, "-") != 0))
                    {
                        if (OD_CHANGES == 0)
                        {
                            midx = ((atoi(od_cmd.meter)) - 1);
                        }
                        else
                        {
                            if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
                            {
                                midx = (get_ser_num_to_metid(od_cmd.meter));
                            }
                            else
                            {
                                midx = get_ser_num_to_metid_serial(od_cmd.meter);
                            }
                            printf("midx : %d\n", midx);
                            if (midx < 0)
                            {
                                dbg_log(REPORT, "[%-25s] : Given_ser_num not found\n", fun_name);
                                update_cmd_status(od_cmd, "Serial number not found");
                                return -1;
                            }

                            midx_od_ftp = midx;
                            midx_od_ftp++;
                        }
                    }

                    dbg_log(INFORM, "[%-25s] : Given_ser_num %s matched with met_id : %d\n", fun_name, od_cmd.meter, midx);

                    snrm_aarq_failed_cnt("[%s] : [%-25s] : OD_CMD_RECEIVED_TYPE : %s SEQ_NUM : %s Init source : %d Meter_id : %s Start_date : %s Num_days : %s Event_type : %s Port_id : %s\n", time_format_conversion(time(NULL)), fun_name, od_cmd.msg_type, od_cmd.seq_no, od_cmd.init_source, od_cmd.meter, od_cmd.start_date, od_cmd.num_days, od_cmd.event_type, od_cmd.port_id);

                    dbg_log(INFORM, "[%-25s] : OD_CMD_RECEIVED_TYPE : %s SEQ_NUM : %s Init source : %d Meter_id : %s Start_date : %s Num_days : %s Event_type : %s Port_id : %s\n", fun_name, od_cmd.msg_type, od_cmd.seq_no, od_cmd.init_source, od_cmd.meter, od_cmd.start_date, od_cmd.num_days, od_cmd.event_type, od_cmd.port_id);
#if 1
                    printf("Parsed data:\n");
                    printf("msg_type: %s\n", od_cmd.msg_type);
                    printf("seq_no: %s\n", od_cmd.seq_no);
                    printf("init_source: %s\n", od_cmd.init_source);
                    printf("meter: %s\n", od_cmd.meter);
                    printf("start_date: %s\n", od_cmd.start_date);
                    printf("num_days: %s\n", od_cmd.num_days);
                    printf("event_type: %s\n", od_cmd.event_type);
                    printf("time_stamp: %s\n", od_cmd.time_stamp);
                    printf("port_id: %s\n", od_cmd.port_id);
                    printf("commn_msg: %s\n", od_cmd.commn_msg);
#endif
                    // update_cmd_status(od_cmd, "In Progress: Command received by DCU");

                    print_colored_message(RED, 1, "--------------- Received Cmd Type : %s --------------\n", od_cmd.msg_type);

                    if (strcmp(od_cmd.msg_type, "OD_LS_DATA") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;

                        curr_nc_idx = midx;

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            print_colored_message(RED, 1, "--------------- Init failed --------------\n");

                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            print_colored_message(RED, 1, "--------------- Else INit --------------\n");

                            // update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                            // strcpy(od_cmd.num_days, "1"); //rithika

                            // ret = get_ls_data_day(midx, od_cmd.start_date, od_cmd.num_days);
                            // update_cmd_status(od_cmd, get_curl_error_message(ret));

                            // rithika 18sep
                            Date curr_date;
                            if (sscanf(od_cmd.start_date, "%2hhu-%2hhu-%4hu",
                                       &curr_date.day, &curr_date.month, &curr_date.year) != 3)
                            {
                                dbg_log(WARNING, "[%-25s] : Invalid start_date format %s\n", fun_name, od_cmd.start_date);
                            }
                            else
                            {
                                int no_of_meter = 0;
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

                                int ndays = atoi(od_cmd.num_days);

                                for (int i = 0; i < ndays; i++)
                                {

                                    // rithika 01/12/2025
                                    // char one_day_str[32];
                                    memset(one_day_str, 0, sizeof(one_day_str));
                                    char str[64];
                                    snprintf(one_day_str, sizeof(one_day_str), "%02d-%02d-%04d",
                                             curr_date.day, curr_date.month, curr_date.year);

                                    update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                                    print_colored_message(MAGENTA, 1, "Polling LS data for %s (Day %d of %d)\n",
                                                          one_day_str, i + 1, ndays);

                                    ret = get_ls_data_day(midx, one_day_str, "1");

                                    // get_all_meter_inst_data();

                                    memset(str, 0, sizeof(str));

                                    sprintf(str, "In Progress - %s for %s", get_curl_error_message(ret), one_day_str);

                                    update_cmd_status(od_cmd, str);

                                    curr_date = add_one_day(curr_date); // Move to next day

                                    // rithika  07nov2025 adding this at the end, because for the first day init_dlms will happen at the starting
                                    ret = init_dlms(midx);
                                    if (ret < 0)
                                    {
                                        print_colored_message(RED, 1, "--------------- Init failed --------------\n");

                                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                                    }
                                }
                                // update_cmd_status(od_cmd, get_curl_error_message(ret)); //rithika 01Dec2025
                            }
                        }

                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_INST_DATA") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;
                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = get_inst_data(midx);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_EVENT_DATA") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            if (strcmp(od_cmd.event_type, "all") == 0)
                            {
                                for (cnt = 0; cnt < MAX_EVENTS; cnt++)
                                {
                                    memset(temp_buf, 0, sizeof(temp_buf));
                                    sprintf(temp_buf, "event_data_cat%d", cnt + 1); // od_cmd.start_date

                                    memset(temp_buff, 0, sizeof(temp_buff));
                                    sprintf(temp_buff, "In Progress - Polling %s", temp_buf);
                                    update_cmd_status(od_cmd, temp_buff);

                                    // update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                                    if (strcmp(od_cmd.start_date, "all") == 0)
                                    {
                                        ret = get_event_data(midx, temp_buf, "all");
                                        if (ret < 0)
                                        {
                                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                                        }
                                    }
                                    else
                                    {
                                        ret = get_event_data(midx, temp_buf, poll_cfg.num_of_events);
                                        if (ret < 0)
                                        {
                                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                                        }
                                    }

                                    // rithika  07nov2025 adding this at the end, because for the first day init_dlms will happen at the starting
                                    ret = init_dlms(midx);
                                    if (ret < 0)
                                    {
                                        print_colored_message(RED, 1, "--------------- Init failed --------------\n");

                                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                                    }
                                }
                            }
                            else
                            {
                                if (strcmp(od_cmd.start_date, "all") == 0)
                                {
                                    ret = get_event_data(midx, od_cmd.event_type, "all");
                                    if (ret < 0)
                                    {
                                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                                    }
                                }
                                else
                                {
                                    ret = get_event_data(midx, od_cmd.event_type, poll_cfg.num_of_events);
                                    if (ret < 0)
                                    {
                                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                                    }
                                }
                            }
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_BILL_DATA") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = get_billing_data(midx, od_cmd.start_date); // cmd json
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_MN_DATA") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = get_midnight_data(midx, od_cmd.start_date); // cmd json
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_OVERALL_NET_SIZE") == 0)
                    {
                        update_cmd_status(od_cmd, "In Progress - Command sent to NC");

                        ret = get_nw_size(1);

                        memset(od_cmd.nw_size, 0, sizeof(od_cmd.nw_size));
                        sprintf(od_cmd.nw_size, "%d", plcc_modem_info.nw_size);

                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_OVERALL_DB_SIZE") == 0)
                    {
                        update_cmd_status(od_cmd, "In Progress - Command sent to NC");

                        ret = get_db_size(1);

                        memset(od_cmd.db_size, 0, sizeof(od_cmd.db_size));
                        sprintf(od_cmd.db_size, "%d", plcc_modem_info.db_size);

                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_DELETE_DB") == 0)
                    {
                        update_cmd_status(od_cmd, "In Progress - Command sent to NC");

                        // ret = get_db_size(1);

                        // rithika 21/08/2025
                        ret = delete_db();

                        memset(od_cmd.db_size, 0, sizeof(od_cmd.db_size));
                        sprintf(od_cmd.db_size, "%d", plcc_modem_info.db_size);

                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                    }
                    else if (strcmp(od_cmd.msg_type, "NC_RESTART") == 0)
                    {

                        update_cmd_status(od_cmd, "In Progress - Command sent to NC");

                        ret = nc_soft_reset(1);
                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_PLCC_NW_STATUS") == 0)
                    {
                        update_cmd_status(od_cmd, "In Progress - Command sent to NC");

                        ret = init_plcc_modem(1);
                        update_cmd_status(od_cmd, get_curl_error_message(ret));
                    }
                    else if (strcmp(od_cmd.msg_type, "LOAD_CONN") == 0)
                    {

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        ret = load_reconnect(midx);

                        if (ret != 0)
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            update_cmd_status(od_cmd, hlp_getErrorMessage(ret));
                        }
                        else
                            update_cmd_status(od_cmd, "SUCCESS");
                    }
                    else if (strcmp(od_cmd.msg_type, "LOAD_DISCONN") == 0)
                    {

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        ret = load_disconnect(midx);
                        if (ret != 0)
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            update_cmd_status(od_cmd, hlp_getErrorMessage(ret));
                        }
                        else
                            update_cmd_status(od_cmd, "SUCCESS");
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_TIMESYNC_MESSAGE") == 0)
                    {

                        printf("++++++++++++++++++++++++++++++++ OD_TIMESYNC_MESSAGE : %s++++++++++++++++++++++++++++++++++++\n", od_cmd.commn_msg);

                        uint16_t year;
                        unsigned char month, day, hour, minute, second;
                        int d, m, y, h, min, s;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set date is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        if (sscanf(od_cmd.commn_msg, "%d-%d-%d %d:%d:%d", &y, &m, &d, &h, &min, &s) != 6)
                        {
                            update_cmd_status(od_cmd, "Invalid date format");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        day = (unsigned char)d;
                        month = (unsigned char)m;
                        year = (uint16_t)y;
                        hour = (unsigned char)h;
                        minute = (unsigned char)min;
                        second = (unsigned char)s;

                        printf("%u %u %u %u %u %u\n", year, month, day, hour, minute, second);

                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            printf("++++++++++++++++++++++++++++++++ set_clock_time ++++++++++++++++++++++++++++++++++++\n");
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_clock_time(midx, year, month, day, hour, min, second, od_cmd.resp_msg);
                            // ret = com_updateClock(midx, year, month, day, hour, min, second, od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_DEMAND_PERIOD_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set period is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_demand_period(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_PROF_CAP_PERIOD_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set period is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_profile_capture_period(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_METERING_MODE_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set mode is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_metering_mode(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_PAYMENT_MODE_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set amount is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_payment_mode(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_LAST_TKN_RECH_AMT_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set amount is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_lst_token_recharge_amt(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_TOT_AMT_LAST_RECH_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set amount is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_tot_amt_lst_recharge(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_CURR_BAL_AMT_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set amount is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_curr_bal_amt(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_ESWF_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_eswf(midx, od_cmd.commn_msg, od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_LOAD_LIMIT_EN_DIS_MESSAGE") == 0) // OD_LOAD_CONN_DISCONN_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set limit val is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = enbl_disbl_load_limit_fn(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_MD_RESET_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_md_reset(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_LOAD_CONN_DISCONN_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set load value is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        printf("########################################\n\n\natoi(od_cmd.commn_msg) %d =======\n", atoi(od_cmd.commn_msg));
                        if (atoi(od_cmd.commn_msg) == 1)
                            ret = load_reconnect(midx);
                        else
                            ret = load_disconnect(midx);

                        if (ret != 0)
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            update_cmd_status(od_cmd, hlp_getErrorMessage(ret));
                        }
                        else
                        {
                            strcpy(od_cmd.resp_msg, od_cmd.commn_msg);
                            update_cmd_status(od_cmd, "SUCCESS");
                        }

                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_SET_LOAD_MESSAGE") == 0)
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set load limit value is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_load_limit(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_EN_DIS_RLY_OPER_MESSAGE") == 0) // OD_APN_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if (strlen(od_cmd.commn_msg) == 0)
                        {
                            update_cmd_status(od_cmd, "Set mode is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_relay_op_overload_overcurr(midx, atoi(od_cmd.commn_msg), od_cmd.resp_msg);
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_APN_MESSAGE") == 0) // OD_APN_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if ((strlen(od_cmd.apn_name) == 0))
                        {
                            update_cmd_status(od_cmd, "Set apn_name is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");
                            ret = set_apn(midx, od_cmd.apn_name, (unsigned char)atoi(od_cmd.apn_bearer), od_cmd.resp_msg);
                            if (ret == 0)
                            {
                                const char *key = "Index: 2 Value: [";

                                char *position = strstr(od_cmd.resp_msg, key);

                                if (position != NULL)
                                {
                                    position += strlen(key);

                                    sscanf(position, "%[^,]", od_cmd.apn_bearer);

                                    sscanf(position + strlen(od_cmd.apn_bearer) + 1, "%[^]]", od_cmd.apn_name);

                                    printf("apn_bearer1: %s\n", od_cmd.apn_bearer);
                                    printf("apn_name1: %s\n", od_cmd.apn_name);
                                }
                                else
                                {
                                    printf("Key not found\n");
                                }
                            }
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_LAST_TKN_RECH_TIME_MESSAGE") == 0) // OD_APN_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if ((strlen(od_cmd.commn_msg) == 0))
                        {
                            update_cmd_status(od_cmd, "Set time is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                            int year, month, day, hour, minute, second;

                            if (sscanf(od_cmd.commn_msg, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
                            {
                                update_cmd_status(od_cmd, "Invalid date format");
                                printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                                return -1;
                            }

                            unsigned char output[12];

                            output[0] = (year >> 8) & 0xFF; // High byte of year
                            output[1] = year & 0xFF;        // Low byte of year
                            output[2] = (unsigned char)month;
                            output[3] = (unsigned char)day;
                            output[4] = 0xFF; // Fixed/reserved byte
                            output[5] = (unsigned char)hour;
                            output[6] = (unsigned char)minute;
                            output[7] = (unsigned char)second;
                            output[8] = 0xFF;  // Fixed/reserved byte
                            output[9] = 0x01;  // Fixed value (maybe type)
                            output[10] = 0x4A; // Fixed value (e.g., command ID)
                            output[11] = 0x00; // Terminator or reserved

                            // Print the result
                            printf("Final hex output:\n");
                            for (int i = 0; i < 12; i++)
                            {
                                printf("0x%02X, ", output[i]);
                            }
                            printf("\n");
                            ret = set_lst_token_recharge_time(midx, output, 12, od_cmd.resp_msg);

                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_CURR_BAL_TIME_MESSAGE") == 0) // OD_APN_MESSAGE OD_SNGL_ACT_SCHD_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if ((strlen(od_cmd.commn_msg) == 0))
                        {
                            update_cmd_status(od_cmd, "Set time is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                            int year, month, day, hour, minute, second;

                            if (sscanf(od_cmd.commn_msg, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
                            {
                                update_cmd_status(od_cmd, "Invalid date format");
                                printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                                return -1;
                            }

                            unsigned char output[12];

                            output[0] = (year >> 8) & 0xFF; // High byte of year
                            output[1] = year & 0xFF;        // Low byte of year
                            output[2] = (unsigned char)month;
                            output[3] = (unsigned char)day;
                            output[4] = 0xFF; // Fixed/reserved byte
                            output[5] = (unsigned char)hour;
                            output[6] = (unsigned char)minute;
                            output[7] = (unsigned char)second;
                            output[8] = 0xFF;  // Fixed/reserved byte
                            output[9] = 0x01;  // Fixed value (maybe type)
                            output[10] = 0x4A; // Fixed value (e.g., command ID)
                            output[11] = 0x00; // Terminator or reserved

                            // Print the result
                            printf("Final hex output:\n");
                            for (int i = 0; i < 12; i++)
                            {
                                printf("0x%02X, ", output[i]);
                            }
                            printf("\n");
                            ret = set_curr_bal_time(midx, output, 12, od_cmd.resp_msg);

                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }

                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_SNGL_ACT_SCHD_MESSAGE") == 0) // OD_APN_MESSAGE “OD_SNGL_ACT_SCHD_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if ((strlen(od_cmd.commn_msg) == 0))
                        {
                            update_cmd_status(od_cmd, "Set val is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                            ret = single_action_schl_billing(midx, od_cmd.commn_msg, od_cmd.resp_msg);

                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }

                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_ACT_CAL_MESSAGE") == 0) // OD_APN_MESSAGE “OD_SNGL_ACT_SCHD_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        printf("%s\n", od_cmd.commn_msg);

                        if ((strlen(od_cmd.commn_msg) == 0))
                        {
                            update_cmd_status(od_cmd, "Set val is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                            ret = activity_calnd(midx, parse_act_calender_json(od_cmd.commn_msg), temp_json);

                            strcpy(od_cmd.resp_msg, temp_json);

                            printf("+++++++ %s +++++++++\n", od_cmd.resp_msg);

                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }

                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_PUSH_SETUP_MESSAGE") == 0) // OD_APN_MESSAGE “OD_SNGL_ACT_SCHD_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if ((strlen(od_cmd.commn_msg) == 0))
                        {
                            update_cmd_status(od_cmd, "Set val is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                            ret = instant_push_setup(midx, od_cmd.commn_msg, od_cmd.resp_msg);

                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }

                        OD_CMD_RECEIVED = 0;
                    }
                    else if (strcmp(od_cmd.msg_type, "OD_PUSH_ALERT_MESSAGE") == 0) // OD_APN_MESSAGE “OD_SNGL_ACT_SCHD_MESSAGE
                    {
                        OD_CMD_RECEIVED = 1;
                        // rithika
                        // curr_nc_idx = midx - 1;
                        curr_nc_idx = midx;

                        if ((strlen(od_cmd.commn_msg) == 0))
                        {
                            update_cmd_status(od_cmd, "Set val is not available");
                            printf("++++++++++++++++++++++++++++++++ Set date is not available ++++++++++++++++++++++++++++++++++++\n");
                            return -1;
                        }

                        update_cmd_status(od_cmd, "In Progress - Command sent to meter");

                        ret = init_dlms(midx);
                        if (ret < 0)
                        {
                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }
                        else
                        {
                            update_cmd_status(od_cmd, "In Progress - Meter response in progress");

                            ret = alert_push_setup(midx, od_cmd.commn_msg, od_cmd.resp_msg);

                            update_cmd_status(od_cmd, get_curl_error_message(ret));
                        }

                        OD_CMD_RECEIVED = 0;
                    }

                    delete_processed_cmd(od_cmd);
                    memset(&od_cmd, 0, sizeof(od_cmd_t));
                }
                else
                {
                    printf("======================= not available =============\n");
                }
#endif

                nx_json_free(json);
            }
            else
            {
                fprintf(stderr, "JSON parsing failed\n");

                memset(event_msg_str, 0, sizeof(event_msg_str));
                sprintf(redis_event_data.eventmsg, "%s : JSON parsing failed", fun_name);
                strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
                add_event_data(&redis_event_data);
            }
        }
        else
        {
            fprintf(stderr, "Unexpected Redis reply type\n");

            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Unexpected Redis reply type", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
        }

        printf("++++++++++++++++++++++++++++++++++++++++++++++++\n");

        freeReplyObject(reply);
    }
    return 0;
}

/**
 * Stores PLCC modem information in Redis as a hash.
 *
 * This function populates a Redis hash with various properties of PLCC
 * modems, including the number of nodes and details for each meter, such
 * as serial numbers, connection statuses, and IDs. Each property is
 * stored with a specific field name that follows a defined naming
 * convention. The data is taken from the global `plcc_modem_info`
 * structure, which contains information about the modems.
 * The function ensures that Redis commands are executed successfully,
 * logging errors when they occur.
 *
 * @return 0 on success, -1 if any Redis command fails.
 */
int set_plcc_modem_info_to_redis()
{

    static char fun_name[] = "set_plcc_modem_info_to_redis()";

    char field[64];
    char value[256];

    char temp_buf[256];

    int i, j, temp_int;

    // Set no_of_nodeS
    redisReply *reply;

    memset(temp_buf, 0, sizeof(temp_buf));

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    sprintf(temp_buf, "DEL %s", HASH_KEY);
    reply = redisCommand(redis, temp_buf);
    if (reply == NULL)
    {
        printf("Error executing DEL command\n");
        return 1;
    }

    // printf("DEL my_hash: %lld\n", reply->integer); // Returns 1 if the key was deleted, 0 if it didn't exist
    freeReplyObject(reply);

    memset(temp_buf, 0, sizeof(temp_buf));

    // If nc_sn is a 16-byte array:
    for (j = 0; j < 16; j++)
    {
        snprintf(temp_buf + 2 * j, sizeof(temp_buf) - 2 * j, "%02x", plcc_modem_info.nc_sn[j]);
    }

    reply = redisCommand(redis, "HSET %s no_of_nodes %d nc_id %d nc_sn %s db_size %d nw_size %d last_update_time %s", HASH_KEY, plcc_modem_info.no_of_nodes, plcc_modem_info.nc_id, temp_buf, plcc_modem_info.db_size, plcc_modem_info.nw_size, time_format_conversion(time(NULL)));

    if (reply == NULL)
    {
        printf("Redis command failed\n");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }
    freeReplyObject(reply);

    // Set plcc_meter_map entries
    for (i = 0; i < plcc_modem_info.no_of_nodes && i < MAX_NO_OF_METER_PER_PORT; i++)
    {
        const plcc_meter_map_t *meter = &plcc_modem_info.plcc_meter_map[i];

        temp_int = i;

        snprintf(field, sizeof(field), "meter_%d_plcc_ser", (temp_int + 1));

        memset(value, 0, sizeof(value));
        memset(temp_buf, 0, sizeof(temp_buf));
        for (j = 0; j < 8; j++)
        {
            sprintf(temp_buf, "%02x", meter->plcc_ser[j]);
            strcat(value, temp_buf);
        }
        reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, value);
        freeReplyObject(reply);

        snprintf(field, sizeof(field), "meter_%d_rs_status", (temp_int + 1));
        snprintf(value, sizeof(value), "%u", meter->rs_status);
        reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, value);
        freeReplyObject(reply);

        snprintf(field, sizeof(field), "meter_%d_rs_conn_fail_time", (temp_int + 1));
        snprintf(value, sizeof(value), "%ld", meter->rs_conn_fail_time);
        reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, value);
        freeReplyObject(reply);

        printf("\n");

        printf("Meter_ser_num_%d : %s\n", i, meter->meter_ser_num);

        if ((meter->meter_ser_num) == NULL)
        {
            snprintf(field, sizeof(field), "meter_%d_meter_ser_num", (temp_int + 1));
            reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, "NC");
            freeReplyObject(reply);
        }
        else
        {
            snprintf(field, sizeof(field), "meter_%d_meter_ser_num", (temp_int + 1));
            reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, meter->meter_ser_num);
            freeReplyObject(reply);
        }

        snprintf(field, sizeof(field), "meter_%d_node_id", (temp_int + 1));
        snprintf(value, sizeof(value), "%d", meter->node_id);
        reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, value);
        freeReplyObject(reply);

        snprintf(field, sizeof(field), "meter_%d_parent_id", (temp_int + 1));
        snprintf(value, sizeof(value), "%d", meter->parent_id);
        reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, value);
        freeReplyObject(reply);

        snprintf(field, sizeof(field), "meter_%d_signal_quality", (temp_int + 1));
        snprintf(value, sizeof(value), "%d", meter->signal_quality);
        reply = redisCommand(redis, "HSET %s %s %s", HASH_KEY, field, value);
        freeReplyObject(reply);
    }

    return 0;
}

int set_polling_strategy()
{
    static char fun_name[] = "set_polling_strategy()";
    char key[64];
    redisReply *reply;
    const char *init_polling_json;
    const char *store_polling_json;
    const char *idle_polling_json;
    sprintf(key, "default_polling_strategy");

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        init_polling_json =
            "{\"1\": \"get_nameplate\", \"2\": \"inst_data\", \"3\": \"ls_block:all:1\", "
            "\"4\": \"mn_data:all\", \"5\": \"bill_data:all\", \"6\": \"event_data_cat1:all\", "
            "\"7\": \"event_data_cat2:all\", \"8\": \"event_data_cat3:all\", \"9\": \"event_data_cat4:all\", "
            "\"10\": \"event_data_cat5:all\", \"11\": \"event_data_cat6:all\",\"12\": \"event_data_cat7:all\"}";
    }
    else if (strcmp(dev_model, "TCS_DCU") == 0)
    {
        init_polling_json =
            "{\"1\": \"get_nameplate\", \"2\": \"inst_data\", \"3\": \"ls_block:all:1\", "
            "\"4\": \"mn_data:all\", \"5\": \"event_data_cat3:all\"}";
    }
    else
    {
        init_polling_json =
            "{\"1\": \"get_nameplate\", \"2\": \"inst_data\", \"3\": \"ls_block:all:1\", "
            "\"4\": \"mn_data:all\", \"5\": \"bill_data:all\", \"6\": \"event_data_cat1:all\", "
            "\"7\": \"event_data_cat2:all\", \"8\": \"event_data_cat3:all\", \"9\": \"event_data_cat4:all\", "
            "\"10\": \"event_data_cat5:all\", \"11\": \"event_data_cat6:all\"}";
    }

    reply = redisCommand(redis, "HSET %s init_polling_strategy %s", key, init_polling_json);
    if (reply == NULL)
    {
        printf("Error setting init_polling_strategy\n");
        return -1;
    }
    freeReplyObject(reply);

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        store_polling_json =
            "{\"1\": \"get_nameplate\", \"2\": \"inst_data\", \"3\": \"ls_day:date:1\",\"4\": \"ls_block:now:6\", \"5\": \"mn_data:today\", \"6\": \"bill_data:current\", \"7\": \"event_data_cat1:10\", \"8\": \"event_data_cat2:10\", \"9\": \"event_data_cat3:10\", \"10\": \"event_data_cat4:10\", \"11\": \"event_data_cat5:10\", \"12\": \"event_data_cat6:10\", \"13\": \"event_data_cat7:10\"}";
    }
    // rithika 13nov2025
    else if (strcmp(dev_model, "TCS_DCU") == 0)
    {
        store_polling_json = "{ \"1\": \"install_data\", \"2\": \"ls_day:date:1\",\"3\": \"ls_block:now:6\", \"4\": \"install_data\", \"5\": \"event_data_cat3:10\"}";
    }
    // else
    // {
    //     store_polling_json = "{\"1\": \"get_nameplate\", \"2\": \"inst_data\", \"3\": \"ls_day:date:1\",\"4\": \"ls_block:now:6\", \"5\": \"mn_data:today\", \"6\": \"bill_data:current\", \"7\": \"event_data_cat1:10\", \"8\": \"event_data_cat2:10\", \"9\": \"event_data_cat3:10\", \"10\": \"event_data_cat4:10\", \"11\": \"event_data_cat5:10\", \"12\": \"event_data_cat6:10\"}";
    // }
    // rithika 29sep
    else
    {
        // store_polling_json = "{ \"1\": \"inst_data\", \"2\": \"ls_day:date:1\",\"3\": \"ls_block:now:6\", \"4\": \"event_data_cat1:10\", \"5\": \"event_data_cat2:10\", \"6\": \"event_data_cat3:10\", \"7\": \"event_data_cat4:10\", \"8\": \"event_data_cat5:10\", \"9\": \"event_data_cat6:10\"}";
        store_polling_json = "{ \"1\": \"install_data\", \"2\": \"ls_day:date:1\",\"3\": \"ls_block:now:6\", \"4\": \"install_data\", \"5\": \"event_data_cat1:10\", \"6\": \"event_data_cat2:10\", \"7\": \"event_data_cat3:10\", \"8\": \"event_data_cat4:10\", \"9\": \"event_data_cat5:10\", \"10\": \"event_data_cat6:10\"}";
    }

    reply = redisCommand(redis, "HSET %s store_data_polling_strategy %s", key, store_polling_json);
    if (reply == NULL)
    {
        printf("Error setting store_data_polling_strategy\n");
        return -1;
    }
    freeReplyObject(reply);

    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        idle_polling_json =
            "{\"1\": \"get_nameplate\", \"2\": \"inst_data\", \"3\": \"ls_block:now:3\", "
            "\"4\": \"event_data_cat1:10\", \"5\": \"event_data_cat2:10\", \"6\": \"event_data_cat3:10\", "
            "\"7\": \"event_data_cat4:10\", \"8\": \"event_data_cat5:10\", \"9\": \"event_data_cat6:10\",\"10\": \"event_data_cat7:10\"}";
    }
    else
    {
        idle_polling_json = "";
    }

    reply = redisCommand(redis, "HSET %s idle_polling_strategy %s", key, idle_polling_json);
    if (reply == NULL)
    {
        printf("Error setting idle_polling_strategy\n");
        return -1;
    }

    printf("Polling strategies set successfully!\n");

    freeReplyObject(reply);
    return 0;
}

/**
 * Retrieves the polling strategy configuration from Redis.
 *
 * @return 0 on success, or a negative value on error.
 */
int get_polling_strategy()
{
    static char fun_name[] = "get_polling_strategy()";
    char key[32];
    int i;
    redisReply *reply;
    memset(&strategy_cfg, 0, sizeof(strategy_cfg_t));
    sprintf(key, "default_polling_strategy");

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    reply = redisCommand(redis, "EXISTS %s", key);
    if (reply->integer == 0)
    {
        printf("Key '%s' does not exist in Redis.\n", key);
        freeReplyObject(reply);

        set_polling_strategy();
    }
    else
    {
        freeReplyObject(reply);
    }

    reply = redisCommand(redis, "HGETALL %s", key);

    printf("reply->type : %d\n", reply->type);

    if (reply->type == REDIS_REPLY_ARRAY)
    {
        for (i = 0; i < reply->elements; i += 2)
        {
            const char *field = reply->element[i]->str;
            const char *value = reply->element[i + 1]->str;
#if 0
            printf("Field: %s\n", field);
            printf("Value: %s\n", value);
#endif
            if (strcmp(field, "store_data_polling_strategy") == 0)
            {
                strcpy(strategy_cfg.store_poll_starts, value);
            }
            else if (strcmp(field, "periodic_polling_strategy") == 0)
            {
                strcpy(strategy_cfg.periodic_poll_starts, value);
            }
            else if (strcmp(field, "init_polling_strategy") == 0)
            {
                strcpy(strategy_cfg.initial_poll_starts, value);
            }
            else if (strcmp(field, "idle_polling_strategy") == 0)
            {
                strcpy(strategy_cfg.idle_poll_starts, value);
            }
        }
    }
    else
    {
        printf("Error retrieving hash\n");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Error retrieving hash", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
    }
    if (reply)
    {
        freeReplyObject(reply);
    }

#if DEBUG
    printf("store_data_polling_strategy  :  %s\n", strategy_cfg.store_poll_starts);
    printf("periodic_polling_strategy    :  %s\n", strategy_cfg.periodic_poll_starts);
    printf("init_polling_strategy        :  %s\n", strategy_cfg.initial_poll_starts);
    printf("idle_polling_strategy        :  %s\n", strategy_cfg.idle_poll_starts);
#endif
    return 0;
}

/**
 * Retrieves the manufacturer name for a specified meter from Redis.
 *
 * This function sends a command to Redis to fetch the details of the meter
 * identified by its index (midx) from a hash named "Meter_status". It parses
 * the JSON response to extract the "manufacturer" field. The manufacturer name
 * is then standardized to a predefined format.
 *
 * @param midx The index of the meter whose manufacturer is to be retrieved.
 * @return A dynamically allocated string containing the standardized manufacturer
 *         name, or NULL if an error occurs during the process. The caller is
 *         responsible for freeing the returned string.
 *
 * Manufacturer Normalization:
 * - If the manufacturer contains "GENUS POWER INFRASTRUCTURES LTD", it is
 *   standardized to "genus".
 * - If the manufacturer contains "LARSEN AND TOUBRO LIMITED", it is standardized
 *   to "lnt".
 * - If the manufacturer contains "secure", it is standardized to "secure".
 */

int get_manufacturer(int midx, char *met_sn, char *manufacturer, int length)
{

    static char fun_name[] = "get_manufacturer()";
    // print_colored_message(YELLOW, 1, "begin----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);

    redisReply *reply;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    printf("############################################### %s \n", met_sn);

    if (OD_CHANGES == 0)
    {
        reply = redisCommand(redis, "HGET meter_status meter%d_details", midx + 1);
        printf("HGET meter_status meter%d_details\n", midx + 1);
    }
    else
    {
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            reply = redisCommand(redis, "HGET meter_status meter%d_%s_details", midx + 1, met_sn);
        }
        else
        {
            char redis_key[128];
            snprintf(redis_key, sizeof(redis_key), "meter_%d_%d_%s_details", g_pidx, midx + 1, met_sn);
            reply = redisCommand(redis, "HGET meter_status %s", redis_key);

            printf("HGET meter_status %s\n", redis_key);
            printf("Redis reply type: %d\n", reply->type);

            // reply = redisCommand(redis, "HGET meter_status meter_%d_%d_%s_details", g_pidx, midx + 1, met_sn);
            // printf("HGET meter_status meter_%d_%d_%s_details\n", g_pidx, midx + 1, met_sn);
        }
    }

    printf("##########################################12##### %s \n", met_sn);
    if (reply == NULL)
    {
        // print_colored_message(YELLOW, 1, "summa event----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
        fprintf(stderr, "Redis error: %s\n", redis->errstr);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
        // print_colored_message(YELLOW, 1, "end of event summa----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
        return -1;
    }

    if (reply->type == REDIS_REPLY_STRING)
    {

        const nx_json *json = nx_json_parse(reply->str, 0);
        if (json == NULL)
        {

            fprintf(stderr, "JSON parsing error\n");

            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : JSON parsing error", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);

            freeReplyObject(reply);
            return -1;
        }

        const nx_json *json_manufacturer = nx_json_get(json, "manufacturer");
        if (json_manufacturer)
        {
            // manufacturer = strdup(json_manufacturer->text_value);
            snprintf(manufacturer, length, "%s", json_manufacturer->text_value);
        }
        else
        {
            // print_colored_message(YELLOW, 1, "begin event 1----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
            fprintf(stderr, "Manufacturer field not found in JSON or not a string\n");

            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Manufacturer field not found in JSON or not a string", fun_name);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            // print_colored_message(YELLOW, 1, "Before event 1----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
            add_event_data(&redis_event_data);
            // print_colored_message(YELLOW, 1, "end of event 1----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
        }

        nx_json_free(json);
    }
    else
    {
        // print_colored_message(YELLOW, 1, "begin event----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
        fprintf(stderr, "Unexpected Redis reply type\n");

        snprintf(manufacturer, length, "%s", "Unknown Manufacturer");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Unexpected Redis reply type", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        // print_colored_message(YELLOW, 1, "Before event----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
        add_event_data(&redis_event_data);
        // print_colored_message(YELLOW, 1, "end of event----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
    }

    printf("########################+=================== manufact %s +++++++++++++++++++++\n", manufacturer);
    freeReplyObject(reply);

    // strcpy(manufacturer, "genus");

    // if (manufacturer && strstr(manufacturer, "GENUS POWER INFRASTRUCTURES LTD"))
    if (manufacturer && strstr(manufacturer, "GENUS"))
    {
        strcpy(manufacturer, "genus");
    }
    else if (manufacturer && (strstr(manufacturer, "LARSEN AND TOUBRO LIMITED") || strstr(manufacturer, "Schneider Electric India Private Limited")))
    {
        strcpy(manufacturer, "lnt");
    }
    // else if (manufacturer && strstr(manufacturer, "SECURE METERS Ltd"))
    else if (manufacturer && strstr(manufacturer, "SECURE"))
    {
        strcpy(manufacturer, "secure");
    }
    else if (manufacturer && strstr(manufacturer, "HPL Electric & Power Ltd"))
    {
        strcpy(manufacturer, "hpl");
    }
    else if (manufacturer && strstr(manufacturer, "BENTEC INDIA LIMITED"))
    {
        strcpy(manufacturer, "bentec");
    }

    // print_colored_message(YELLOW, 1, "end of manuf----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);
    printf("########################+=================== after copy manufact %s +++++++++++++++++++++\n", manufacturer);
    return 0;
}

/**
 * Stores processing timing metrics for a specified data type in Redis.
 *
 * @param data_type Type of data being processed (e.g., "ls_data_timing").
 * @param metrics Pointer to a meter_metrics_t structure with timing data.
 *
 * Calculates and stores:
 * - Current processing time
 * - Average processing time
 *
 * Logs any Redis command errors.
 */

void store_process_timing(char *data_type, meter_metrics_t *metrics)
{
    static char fun_name[] = "store_process_timing()";
    int op = 0;

    double total_time = 0.0;
    for (op = 0; op < metrics->cycleCount; op++)
    {
        // printf("Operation %d (%s): Total Average Time = %.6f seconds\n", op + 1, data_type, metrics->totalAverages[op]);

        total_time += metrics->totalAverages[op];
    }

    metrics->currentAverages = total_time / metrics->cycleCount;

    // printf("Total Average Time = %.6f seconds\n", metrics->currentAverages);

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    redisReply *reply;

    if (strcmp("ls_data_timing", data_type) == 0)
    {
        reply = redisCommand(redis, "HSET %s meter_%s_1day %.6f", data_type, metrics->make, metrics->currenttime);
        if (reply == NULL)
        {
            printf("Error storing meter status: %s\n", redis->errstr);

            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(redis_event_data.eventmsg, "%s : Error storing meter status: %s", fun_name, redis->errstr);
            strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
        }
        else
        {
            freeReplyObject(reply);
        }
    }

    reply = redisCommand(redis, "HSET %s meter_%s_current %.6f", data_type, metrics->make, metrics->currenttime);
    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : 2 - Error storing meter status: %s", fun_name, redis->errstr);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
    }
    else
    {
        freeReplyObject(reply);
    }

    reply = redisCommand(redis, "HSET %s meter_%s_avg %.6f", data_type, metrics->make, metrics->currentAverages);
    if (reply == NULL)
    {
        printf("Error storing meter status: %s\n", redis->errstr);
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : 3 - Error storing meter status: %s", fun_name, redis->errstr);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
    }
    else
    {
        freeReplyObject(reply);
    }
}

int delete_old_metercommn_status(char *meter_id, char *hash_name)
{
    redisReply *reply;
    char cursor[32] = "0"; // Cursor for the SCAN command
    int deleted_count = 0;

    printf("hask : %s\n", hash_name);

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", "delete_old_metercommn_status()", redis->errstr);
        return -1;
    }

    // SCAN through all the keys
    do
    {
        reply = redisCommand(redis, "HSCAN %s %s MATCH %s*", hash_name, cursor, meter_id);
        if (reply == NULL || reply->type != REDIS_REPLY_ARRAY)
        {
            fprintf(stderr, "Redis SCAN failed: %s\n", redis->errstr);
            if (reply != NULL)
            {
                freeReplyObject(reply);
            }
            return -1;
        }

        // First element in reply is the new cursor
        snprintf(cursor, sizeof(cursor), "%s", reply->element[0]->str);

        // Second element in reply contains the keys matching "meter0_*"
        for (size_t i = 0; i < reply->element[1]->elements; i++)
        {
            // Delete the key
            redisReply *del_reply = redisCommand(redis, "HDEL %s %s", hash_name, reply->element[1]->element[i]->str);
            if (del_reply == NULL || del_reply->type == REDIS_REPLY_ERROR)
            {
                fprintf(stderr, "Failed to delete key %s\n", reply->element[1]->element[i]->str);
                freeReplyObject(del_reply);
                continue;
            }
            printf("Deleted key: %s\n", reply->element[1]->element[i]->str);
            deleted_count++;
            freeReplyObject(del_reply);
        }

        freeReplyObject(reply);
    } while (strcmp(cursor, "0") != 0); // Continue until cursor is 0 (end of scan)

    return deleted_count;
}

/**
 * Updates the communication status of a meter in Redis.
 *
 * @param midx The meter index.
 * @param commn_state The communication state (1 for "Communicating", 0 for "Not communicating").
 *
 * @return 0 on success, -1 on failure.
 *
 * This function constructs a key for the meter's communication status,
 * stores the state in Redis, and logs any errors that occur.
 */

int meter_commn_status(int midx, int commn_state)
{
    // printf("!!!!!!!!!!!enetered into meter comn !!!!!!!!!!!!!\n");
    static char fun_name[] = "meter_commn_status()";
    char key[64];
    char temp_buff[64];
    // char *serial_number = NULL;

    // printf("################################################### np_polled_comp : %d ################################\n", np_polled_comp);
    // dbg_log(INFORM, "[Meter_%d]:[%-25s] :entered into meter_commn_status \n", midx, fun_name);

    time_t now = time(NULL);
    struct tm *last_commn_time = localtime(&now);
    char last_commn_time_buf[32];
    strftime(last_commn_time_buf, sizeof(last_commn_time_buf), "%Y-%m-%d %H:%M:%S", last_commn_time);
    // dbg_log(INFORM, "[Meter_%d]:[%-25s] :entered into meter_commn_status 1\n", midx, fun_name);
    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }
    // dbg_log(INFORM, "[Meter_%d]:[%-25s] :entered into meter_commn_status 2\n", midx, fun_name);
    if (np_polled_comp[midx] != 1)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] :returning from meter_commn_status np_polled_comp[%d] %d\n", midx, fun_name, midx, np_polled_comp[midx]);
        return 0;
    }

    // dbg_log(INFORM, "[Meter_%d]:[%-25s] :entered into meter_commn_status 3\n", midx, fun_name);
    // if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    // {
    //     serial_number = get_meter_2_ser_num(midx);
    // }
    // else
    // {
    //     serial_number = get_met_ser_num_serialcommn(midx + 1);
    //     printf("############ serial number %s ################\n", serial_number);
    // }

    // serial_number = get_meter_2_ser_num(midx);
    // // printf("################################################### SER NUM : %s ################################\n", serial_number);
    // if (serial_number == NULL)
    // {
    //     printf("Failed to get meter serial number\n");
    //     return -1;
    // }

    char serial_number[64] = {0};
    if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
        return -1;
    }
    else
    {
        // printf("Meter serial number: %s\n", serial_number);
        // rithika 29/08/2025
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            snprintf(key, sizeof(key), "meter%d_%s", (midx + 1), serial_number);
        }
        else
        {
            snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);
        }

        // free(serial_number);
    }

    // snprintf(key, sizeof(key), "meter%d", midx);

#if 0
    printf("Redis name plate data query :%s\n", ((commn_state == 1) ? "Communicating" : "Not commnuicating"));
#endif

    memset(temp_buff, 0, sizeof(temp_buff));
    // rithika 29/08/2025
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        snprintf(temp_buff, sizeof(temp_buff), "meter%d_", (midx + 1));
    }
    else
    {
        snprintf(temp_buff, sizeof(temp_buff), "meter_%d_%d_", g_pidx, (midx + 1));
    }

    // rithika 26Nov2025
    //  int deleted_count = delete_old_metercommn_status(temp_buff, "meter_commn_status");

    // if (deleted_count < 0)
    // {
    //     fprintf(stderr, "Failed to delete keys starting with %s\n", temp_buff);
    //     return -1;
    // }
    // else if (deleted_count > 0)
    // {
    //     printf("Deleted %d keys with prefix %s\n", deleted_count, temp_buff);
    // }
    // else
    // {
    //     printf("No keys with prefix %s found\n", temp_buff);
    // }

    // redisReply *reply = redisCommand(redis, "HSET meter_commn_status %s %s", key, (commn_state == 1) ? "Communicating" : "Not Communicating");

    redisReply *reply;
    // rithika 14nov2025
    if (commn_state == 1)
    {
        reply = redisCommand(redis, "HSET meter_commn_status %s %s", key, "Communicating");
        char new_key[64];
        snprintf(new_key, sizeof(new_key), "%s_time", key);
        reply = redisCommand(redis, "HSET meter_commn_status %s %s", new_key, last_commn_time_buf);
        dbg_log(INFORM, "[Meter_%d]:[%-25s] :commn_state %d meter comm status added successfully\n", midx, fun_name, commn_state);
    }
    else
    {
        reply = redisCommand(redis, "HSET meter_commn_status %s %s", key, "Not Communicating");
    }

    print_colored_message(RED, PRINT, "===================1==============================\n");
    if (reply == NULL)
    {
        print_colored_message(RED, PRINT, "================22=================================\n");
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        print_colored_message(RED, PRINT, "================23=================================\n");
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        print_colored_message(RED, PRINT, "================24=================================\n");
        add_event_data(&redis_event_data);
        print_colored_message(RED, PRINT, "================25=================================\n");

        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Redis command failed \n", midx, fun_name);
        return -1;
    }

    print_colored_message(RED, PRINT, "========================3=========================\n");
    if (reply->type == REDIS_REPLY_ERROR)
    {
        print_colored_message(RED, PRINT, "=======================41==========================\n");
        memset(event_msg_str, 0, sizeof(event_msg_str));
        print_colored_message(RED, PRINT, "=======================42==========================\n");
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        print_colored_message(RED, PRINT, "=======================43==========================\n");
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        print_colored_message(RED, PRINT, "=======================44==========================\n");
        add_event_data(&redis_event_data);
        print_colored_message(RED, PRINT, "=======================45==========================\n");
        freeReplyObject(reply);
        print_colored_message(RED, PRINT, "======================5===========================\n");
        dbg_log(INFORM, "[Meter_%d]:[%-25s] :Redis command failed reply error\n", midx, fun_name);
        return -1;
    }
    print_colored_message(RED, PRINT, "=============================6====================\n");
    freeReplyObject(reply);
    print_colored_message(RED, PRINT, "==========================7=======================\n");

    return 0;
}

/**
 * Adds event data to Redis in JSON format.
 *
 * @param data Pointer to the event data structure containing message type, process name,
 *             data type, event message, and timestamp.
 *
 * @return 0 on success, -1 on failure.
 *
 * This function constructs a JSON string from the event data and stores it in a Redis hash
 * under the key "gen_event_list". It logs any errors encountered during the process.
 */

int add_event_data(const events_data_t *data)
{
    static char fun_name[] = "add_event_data";
    redisReply *reply;
    char key[64];

    char event_send_buff[1024];

    // Reset JSON buffers
    memset(event_send_buff, 0, sizeof(event_send_buff));
    memset(json_buff, 0, sizeof(json_buff));

    // Create JSON message
    sprintf(json_buff, "{\"msgType\" : \"%s\",\n", data->msgType);
    strcat(event_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"proc\" : \"%s\",\n", data->proc);
    strcat(event_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"dataType\" : \"%s\",\n", data->datatype);
    strcat(event_send_buff, json_buff);

    memset(json_buff, 0, sizeof(json_buff));
    sprintf(json_buff, "\"data\" : {\"eventmsg\" : \"%s\",\"timestamp\" : \"%s\"}\n}", data->eventmsg, data->timestamp);
    strcat(event_send_buff, json_buff);

    // Set the key for the Redis List
    sprintf(key, "gen_event_list");

    if (redis == NULL || redis->err)
    {
        fprintf(stderr, "%s: Redis not connected or connection error\n", fun_name);
        return -1;
    }

    // Check the type of the key "gen_event_list"
    reply = redisCommand(redis, "TYPE %s", key);

    if (reply == NULL)
    {
        fprintf(stderr, "%s: Redis command failed: %s\n", fun_name, redis->errstr);
        return -1;
    }

    // If the key is not of type list, delete it to avoid the WRONGTYPE error
    if (reply->type == REDIS_REPLY_STRING && strcmp(reply->str, "none") != 0 && strcmp(reply->str, "list") != 0)
    {
        printf("%s: Key %s is of type %s, deleting it to proceed with LPUSH.\n", fun_name, key, reply->str);
        redisCommand(redis, "DEL %s", key); // Delete the existing key
    }

    freeReplyObject(reply);

    reply = redisCommand(redis, "LPUSH %s %s", key, event_send_buff);
    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        snprintf(redis_event_data.eventmsg, sizeof(redis_event_data.eventmsg), "%s : Error: %s", fun_name, redis->errstr);
        snprintf(redis_event_data.timestamp, sizeof(redis_event_data.timestamp), "%s", time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
        return 1;
    }
    else
    {
        redisReply *reply_trim = redisCommand(redis, "LTRIM %s 0 %d", key, MAX_DIAG_MESSAGE_LIMIT - 1);
        if (reply_trim != NULL)
        {
            freeReplyObject(reply_trim);
        }
    }

    freeReplyObject(reply);

    return 0;
}

int check_diag_enable(int *diag_enbl)
{
     static char fun_name[] = "check_diag_enable()";
    char key[64];

    sprintf(key, "DA_SER:%d", g_pidx);
    printf("hash name %s \n", key);
    
     if (redis == NULL || redis->err)
    {
       
        // dbg_log(REPORT, "[%-25s] :Redis connection error\n",fun_name);
        return -1;
    }

    redisReply *reply = redisCommand(redis, "HGET %s %s", DIAG_ENBL_HASH, key);

    if (reply == NULL)
    {
        return -1;
    }
    if ( reply->type == REDIS_REPLY_NIL || reply->str == NULL)
    {
        *diag_enbl = 0;
        // dbg_log(REPORT, "[%-25s] : Failed to read diag enable status from Redis\n", fun_name);
        if (reply)
            freeReplyObject(reply);
        return 0;
    }

    *diag_enbl = atoi(reply->str);
    freeReplyObject(reply);
    return 0;
   
}


/**
 * Adds a process diagnostic message to a Redis list.
 *
 * @param buffer The diagnostic message to be added.
 *
 * @return 1 on error, or 0 on success.
 *
 * This function constructs a diagnostic message string and pushes it onto the
 * "proc_diag_msg" Redis list if process diagnostics are enabled.
 * It logs any errors encountered during the operation.
 */
int add_process_diag(char *buffer)
{
    static char fun_name[] = "add_process_diag";

    char key[64];
    int diag_enbl = 0;

    memset(key, 0, sizeof(key));
    if (RECONNECT)
    {
        // rithika 13April2026
       if( check_diag_enable(&diag_enbl)  == -1){
        return -1;
       }
        snprintf(key, sizeof(key), "diag_msgs:DA_SER:%d", g_pidx);
    }
    else
    {
        snprintf(key, sizeof(key), "dlmsda_%d_diag_msg", g_pidx);
    }

    // rithika 14nov2025
    char json_buffer[1024];

    memset(json_buffer, 0, sizeof(json_buffer));
    // snprintf(json_buff, sizeof(json_buff), "dlmsda_%d-%s", g_pidx, buffer);

    snprintf(json_buffer, sizeof(json_buffer), "%s", buffer);

    if (redis == NULL || redis->err)
    {
        printf("+++++++++++++123++++++++++++++++++\n");
        // dbg_log(REPORT, "[%-25s] :Redis connection error\n",fun_name);
        printf("+++++++++++++1234++++++++++++++++++\n");

        return -1;
    }

    if (RECONNECT && diag_enbl)
    {
        
        redisReply *reply = redisCommand(redis, "LPUSH %s %s", key, json_buffer);

        if (reply == NULL)
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            snprintf(redis_event_data.eventmsg, sizeof(redis_event_data.eventmsg), "%s : Error: %s", fun_name, redis->errstr);
            snprintf(redis_event_data.timestamp, sizeof(redis_event_data.timestamp), "%s", time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            return 1;
        }
        else
        {
            redisReply *reply_trim = redisCommand(redis, "LTRIM %s 0 %d", key, MAX_DIAG_MESSAGE_LIMIT - 1);
            if (reply_trim != NULL)
            {
                freeReplyObject(reply_trim);
            }
        }

        freeReplyObject(reply);
    }
    else if (poll_cfg.enable_proc_diag == 1)
    {

        redisReply *reply = redisCommand(redis, "LPUSH %s %s", key, json_buffer);

        if (reply == NULL)
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            snprintf(redis_event_data.eventmsg, sizeof(redis_event_data.eventmsg), "%s : Error: %s", fun_name, redis->errstr);
            snprintf(redis_event_data.timestamp, sizeof(redis_event_data.timestamp), "%s", time_format_conversion(time(NULL)));
            add_event_data(&redis_event_data);
            return 1;
        }
        else
        {
            redisReply *reply_trim = redisCommand(redis, "LTRIM %s 0 %d", key, MAX_DIAG_MESSAGE_LIMIT - 1);
            if (reply_trim != NULL)
            {
                freeReplyObject(reply_trim);
            }
        }

        freeReplyObject(reply);
    }

    return 0;
}

int extract_block_int(const char *key_name)
{
    redisReply *reply;

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", "extract_block_int()", redis->errstr);
        return -1;
    }

    reply = redisCommand(redis, "EXISTS %s", "meter_status");
    if (reply == NULL)
    {
        printf("Failed to execute Redis command\n");
        return -1;
    }

    if (reply->integer == 0)
    {
        printf("Key '%s' does not exist in Redis.\n", key_name);
        freeReplyObject(reply);
    }
    else
    {
        freeReplyObject(reply);
    }

    reply = redisCommand(redis, "HGET %s %s", "meter_status", key_name);
    if (reply == NULL)
    {
        fprintf(stderr, "Failed to execute Redis command\n");
        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        fprintf(stderr, "Redis error: %s\n", reply->str);
        freeReplyObject(reply);
        return -1;
    }

    if (reply->type != REDIS_REPLY_STRING || reply->str == NULL)
    {
        freeReplyObject(reply);
        return -1;
    }

    const char *json_str = reply->str;

    const nx_json *root = nx_json_parse(json_str, 0);
    if (!root)
    {
        printf("Failed to parse JSON\n");
        freeReplyObject(reply);
        return -1;
    }

    const char *p_gen_ptr = nx_json_get(root, "block_int") ? nx_json_get(root, "block_int")->text_value : NULL;
    if (!p_gen_ptr)
    {
        printf("'block_int' key not found in JSON\n");
        nx_json_free(root);
        freeReplyObject(reply);
        return -1;
    }

    int block_int = atoi(p_gen_ptr);
    printf("Extracted block_int: %d\n", block_int);

    int num_blocks = ((60 / block_int) * 24);

    nx_json_free(root);
    freeReplyObject(reply);

    return num_blocks;
}

// int send_hanging_check()
// {
//     char path[40];
//     char name[256];
//     time_t current_time;
//     char cur_time[128];
//     pid_t pid = getpid();

//     snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

//     // Open the cmdline file
//     FILE *fp = fopen(path, "r");
//     if (fp == NULL)
//     {
//         perror("Failed to open cmdline");
//         return 1;
//     }

//     // Read the process name (command line)
//     fgets(name, sizeof(name), fp);
//     fclose(fp);

//     // Extract the basename (process name)
//     char *base_name = basename(name);
//     if (base_name == NULL)
//     {
//         fprintf(stderr, "Failed to extract the base name of the process\n");
//         return 1;
//     }

//     // Get the current time (timestamp)
//     current_time = time(NULL);
//     if (current_time == (time_t)(-1))
//     {
//         perror("Failed to get current time");
//         return 1;
//     }

//     // Format the timestamp
//     snprintf(cur_time, sizeof(cur_time), "%ld", current_time);

//     // Check if Redis is connected
//     if (redis == NULL)
//     {
//         fprintf(stderr, "Redis connection is not established\n");
//         return -1;
//     }

//     // Execute the Redis command to store the hanging check data in Redis hash
//     redisReply *reply = redisCommand(redis, "HSET hc_msgs %s_hc_up_time %s", base_name, cur_time);
//     if (reply == NULL)
//     {
//         printf("Failed to execute Redis command\n");
//         return -1;
//     }

//     // Check if the Redis command was successful
//     if (reply->type == REDIS_REPLY_ERROR)
//     {
//         printf("Redis command error: %s\n", reply->str);
//         freeReplyObject(reply);
//         return -1;
//     }
//     else
//     {
//         printf("Successfully sent hanging check: %s_hc_up_time = %s\n", base_name, cur_time);
//     }

//     // Free Redis reply object after use
//     freeReplyObject(reply);

//     return 0;
// }

int send_hanging_check()
{
    char path[40];
    char name[256];

    // struct tm *time_info;
    char cur_time[128];
    char temp_buff[256];

    pid_t pid = getpid();
    snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

    // Open the cmdline file
    FILE *fp = fopen(path, "r");
    if (fp == NULL)
    {
        perror("Failed to open cmdline");
        return 1;
    }

    // Read the process name
    fgets(name, sizeof(name), fp);
    fclose(fp);

    // Extract the basename
    char *base_name = basename(name);

    // rithika 12March2026
    if (RECONNECT)
    {
        // struct timespec ts;
        // // Get monotonic time
        // clock_gettime(CLOCK_MONOTONIC, &ts);
        // snprintf(cur_time, sizeof(cur_time), "%ld", ts.tv_sec);

        // rithika 09April2026
        sprintf(temp_buff, "%s_%d_hc_up_time", base_name, (g_pidx));
        hc_update(redis, temp_buff);
        return 0;
    }
    else
    {
        time_t current_time;
        // Get the current time
        // rithika 7March2026
        current_time = time(NULL);
        snprintf(cur_time, sizeof(cur_time), "%ld", current_time);
    }
    // Execute the Redis command
    // execute_redis_command("hset hc_msgs %s_%d_hc_up_time %s", base_name,(g_pidx - 1), cur_time);

    // Execute the Redis command to store the hanging check data in Redis hash

    // rithika 23sep
    memset(temp_buff, 0, sizeof(temp_buff));
    if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
    {
        sprintf(temp_buff, "hset hc_msgs %s_%d_hc_up_time %s", base_name, (g_pidx - 1), cur_time);
    }
    else
    {
        sprintf(temp_buff, "hset hc_msgs %s_%d_hc_up_time %s", base_name, (g_pidx), cur_time);
    }
    // printf("temp buff : %s \n", temp_buff);

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", "send_hanging_check()", redis->errstr);
        return -1;
    }

    redisReply *reply = redisCommand(redis, temp_buff);
    if (reply == NULL)
    {
        printf("Failed to execute Redis command\n");
        return -1;
    }

    // Check if the Redis command was successful
    if (reply->type == REDIS_REPLY_ERROR)
    {
        printf("Redis command error: %s\n", reply->str);
        freeReplyObject(reply);
        return -1;
    }
    else
    {
        if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
        {
            hc_status_log("[%s] : Successfully sent hanging check:  %s_%d_hc_up_time = %s\n", time_format_conversion(time(NULL)), base_name, (g_pidx - 1), cur_time);
        }
        else
        {
            hc_status_log("[%s] : Successfully sent hanging check:  %s_%d_hc_up_time = %s\n", time_format_conversion(time(NULL)), base_name, (g_pidx), cur_time);
        }
        // printf("Successfully sent hanging check: %s_hc_up_time = %s\n", base_name, cur_time);
    }

    // Free Redis reply object after use
    freeReplyObject(reply);

    return 0;
}

int check_ls(char *buffer)
{
    static char fun_name[] = "check_ls()";

    char key[64];

    memset(key, 0, sizeof(key));
    snprintf(key, sizeof(key), "check_ls");

    if (redis == NULL || redis->err)
    {
        dbg_log(REPORT, "[%-25s] :Redis connection error: %s\n", fun_name, redis->errstr);
        return -1;
    }

    redisReply *reply = redisCommand(redis, "LPUSH %s %s", key, buffer);
    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        snprintf(redis_event_data.eventmsg, sizeof(redis_event_data.eventmsg), "%s : Error: %s", fun_name, redis->errstr);
        snprintf(redis_event_data.timestamp, sizeof(redis_event_data.timestamp), "%s", time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);
        return 1;
    }
    else
    {
        redisReply *reply_trim = redisCommand(redis, "LTRIM %s 0 %d", key, MAX_DIAG_MESSAGE_LIMIT - 1);
        if (reply_trim != NULL)
        {
            freeReplyObject(reply_trim);
        }
    }

    freeReplyObject(reply);

    return 0;
}

void update_serial_status_in_redis(const char *val)
{
    if (!redis || redis->err)
    {
        printf("Redis context is invalid or not connected.\n");
        return;
    }

    // Set value
    redisReply *reply = redisCommand(redis, "HSET check_plcc_serial_num ser_num_update_sts %s", val);
    if (reply)
    {
        printf("Redis HSET reply: %lld\n", reply->integer); // 1 = new field, 0 = updated existing
        freeReplyObject(reply);
    }
    else
    {
        printf("Failed to execute HSET command.\n");
        return;
    }

    // Get value back
    reply = redisCommand(redis, "HGET check_plcc_serial_num ser_num_update_sts");
    if (reply && reply->type == REDIS_REPLY_STRING)
    {
        printf("Updated value: %s\n", reply->str);
    }
    else
    {
        printf("Failed to fetch value or value is not a string.\n");
    }

    if (reply)
        freeReplyObject(reply);
}

int update_inst_det_ftp(int midx)
{
    static char fun_name[] = "update_inst_det";

    time_t current_time = time(NULL);
    struct tm *tm_info = localtime(&current_time);
    char datetime_str[32];

    strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);
    cJSON *root = cJSON_CreateObject();

    if (!root)
    {
        printf("Failed to create JSON object\n");
        return -1;
    }

    // char *serial_num = get_meter_2_ser_num(midx);

    // if (!serial_num)
    // {
    //     serial_num = strdup(""); // fallback empty string
    //     return -1;               // rithika 06oct
    // }
    char serial_num[64] = {0};
    if (get_meter_2_ser_num(midx, serial_num, sizeof(serial_num)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }
    // Add string fields to the object
    char meter_id_str[16] = {0};
    char port_id_str[16] = {0};

    midx += 1;

    sprintf(meter_id_str, "%d", midx);
    sprintf(port_id_str, "%d", g_pidx);

    cJSON_AddStringToObject(root, "meter_id", meter_id_str);
    cJSON_AddStringToObject(root, "port_id", port_id_str);
    cJSON_AddStringToObject(root, "meter_ser_num", serial_num);
    cJSON_AddStringToObject(root, "update_time", datetime_str);

    // Convert to JSON string
    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        printf("Failed to print JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    redisReply *reply = redisCommand(redis, "LPUSH inst_det_ftp %s", json_string);

    if (reply == NULL)
    {
        fprintf(stderr, "Redis command failed\n");

        print_colored_message(RED, 1, "Redis command failed\n");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : LTRIM  Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }
    else
    {
        redisReply *reply_trim = redisCommand(redis, "LTRIM inst_det_ftp 0 %d", MAX_DIAG_MESSAGE_LIMIT - 1);
        print_colored_message(RED, 1, "Else Redis command failed\n");
        if (reply_trim != NULL)
        {
            freeReplyObject(reply_trim);
        }
    }

    // free(serial_num);
    freeReplyObject(reply);
    free(json_string);
    cJSON_Delete(root);
}

int add_inst_scalar_info(int g_pidx, int midx, const inst_val_info_t *data)
{

    print_colored_message(GREEN, 1, "------------------------------- > data->num_scaler_vals %d\n", data->num_scaler_vals);
    static char fun_name[] = "add_inst_scalar_info";
    redisReply *reply;
    char hash_key[64];
    char key[64];
    int i;
    char temp_met_sn[64];

    cJSON *root = cJSON_CreateObject();

    char num_vals_str[32];
    snprintf(num_vals_str, sizeof(num_vals_str), "%d", data->num_scaler_vals);

    cJSON_AddStringToObject(root, "num_vals", num_vals_str);
    cJSON *obis_list = cJSON_AddArrayToObject(root, "obis_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        cJSON_AddItemToArray(obis_list, cJSON_CreateString(data->scalar_values[i].obis_code));
    }

    cJSON *val_list = cJSON_AddArrayToObject(root, "val_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        char str_val[64];

        sprintf(str_val, "%lf", data->scalar_values[i].multiplier);

        cJSON_AddItemToArray(val_list, cJSON_CreateString(str_val));
    }

    char *json_string = cJSON_Print(root);

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "inst_scalar_info");
    memset(key, 0, sizeof(key));

    memset(temp_met_sn, 0, sizeof(temp_met_sn));

    // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    // // rithika 06oct
    // else
    // {
    //     return -1;
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    reply = redisCommand(redis, "HMSET %s %s %s", hash_key, key, json_string);

    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }

    dbg_log(INFORM, "[%-25s] :Redis hash inst_scalar_info updated successfully\n", fun_name);

    freeReplyObject(reply);
    free(json_string);
    return 0;
}

int add_ls_scalar_info(int g_pidx, int midx, const load_survey_data_value_t *data)
{

    print_colored_message(GREEN, 1, "------------------------------- > data->num_scaler_vals %d\n", data->num_scaler_vals);
    static char fun_name[] = "add_ls_scalar_info";
    redisReply *reply;
    char hash_key[64];
    char key[64];
    int i;
    char temp_met_sn[64];

    cJSON *root = cJSON_CreateObject();

    char num_vals_str[32];
    snprintf(num_vals_str, sizeof(num_vals_str), "%d", data->num_scaler_vals);

    cJSON_AddStringToObject(root, "num_vals", num_vals_str);
    cJSON *obis_list = cJSON_AddArrayToObject(root, "obis_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        cJSON_AddItemToArray(obis_list, cJSON_CreateString(data->scalar_values[i].obis_code));
    }

    cJSON *val_list = cJSON_AddArrayToObject(root, "val_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        char str_val[64];

        sprintf(str_val, "%lf", data->scalar_values[i].multiplier);

        cJSON_AddItemToArray(val_list, cJSON_CreateString(str_val));
    }

    char *json_string = cJSON_Print(root);

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "ls_scalar_info");
    memset(key, 0, sizeof(key));

    memset(temp_met_sn, 0, sizeof(temp_met_sn));
    // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    // // rithika 06oct
    // else
    // {
    //     return -1;
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    reply = redisCommand(redis, "HMSET %s %s %s", hash_key, key, json_string);

    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }

    dbg_log(INFORM, "[%-25s] :Redis hash ls_scalar_info updated successfully\n", fun_name);

    freeReplyObject(reply);
    free(json_string);
    return 0;
}

int add_mn_scalar_info(int g_pidx, int midx, const daily_profile_data_value_t *data)
{

    print_colored_message(GREEN, 1, "------------------------------- > data->num_scaler_vals %d\n", data->num_scaler_vals);
    static char fun_name[] = "add_mn_scalar_info";
    redisReply *reply;
    char hash_key[64];
    char key[64];
    int i;
    char temp_met_sn[64];

    cJSON *root = cJSON_CreateObject();

    char num_vals_str[32];
    snprintf(num_vals_str, sizeof(num_vals_str), "%d", data->num_scaler_vals);

    cJSON_AddStringToObject(root, "num_vals", num_vals_str);
    cJSON *obis_list = cJSON_AddArrayToObject(root, "obis_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        cJSON_AddItemToArray(obis_list, cJSON_CreateString(data->scalar_values[i].obis_code));
    }

    cJSON *val_list = cJSON_AddArrayToObject(root, "val_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        char str_val[64];

        sprintf(str_val, "%lf", data->scalar_values[i].multiplier);

        cJSON_AddItemToArray(val_list, cJSON_CreateString(str_val));
    }

    char *json_string = cJSON_Print(root);

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "mn_scalar_info");
    memset(key, 0, sizeof(key));

    memset(temp_met_sn, 0, sizeof(temp_met_sn));
    // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    // // rithika 06oct
    // else
    // {
    //     return -1;
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }
    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    reply = redisCommand(redis, "HMSET %s %s %s", hash_key, key, json_string);

    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }

    dbg_log(INFORM, "[%-25s] :Redis hash midnight_scalar_info updated successfully\n", fun_name);

    freeReplyObject(reply);
    free(json_string);
    return 0;
}

int add_bill_scalar_info(int g_pidx, int midx, const billing_data_value_t *data)
{

    print_colored_message(GREEN, 1, "------------------------------- > data->num_scaler_vals %d\n", data->num_scaler_vals);
    static char fun_name[] = "add_bill_scalar_info";
    redisReply *reply;
    char hash_key[64];
    char key[64];
    int i;
    char temp_met_sn[64];

    cJSON *root = cJSON_CreateObject();

    char num_vals_str[32];
    snprintf(num_vals_str, sizeof(num_vals_str), "%d", data->num_scaler_vals);

    cJSON_AddStringToObject(root, "num_vals", num_vals_str);
    cJSON *obis_list = cJSON_AddArrayToObject(root, "obis_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        cJSON_AddItemToArray(obis_list, cJSON_CreateString(data->scalar_values[i].obis_code));
    }

    cJSON *val_list = cJSON_AddArrayToObject(root, "val_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        char str_val[64];

        sprintf(str_val, "%lf", data->scalar_values[i].multiplier);

        cJSON_AddItemToArray(val_list, cJSON_CreateString(str_val));
    }

    char *json_string = cJSON_Print(root);

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "bill_scalar_info");
    memset(key, 0, sizeof(key));

    memset(temp_met_sn, 0, sizeof(temp_met_sn));
    // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    // // rithika 06oct
    // else
    // {
    //     return -1;
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    reply = redisCommand(redis, "HMSET %s %s %s", hash_key, key, json_string);

    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }

    dbg_log(INFORM, "[%-25s] :Redis hash bill_scalar_info updated successfully\n", fun_name);

    freeReplyObject(reply);
    free(json_string);
    return 0;
}

int add_event_scalar_info(int g_pidx, int midx, const events_data_value_t *data)
{

    print_colored_message(GREEN, 1, "------------------------------- > data->num_scaler_vals %d\n", data->num_scaler_vals);
    static char fun_name[] = "add_event_scalar_info";
    redisReply *reply;
    char hash_key[64];
    char key[64];
    int i;
    char temp_met_sn[64];

    cJSON *root = cJSON_CreateObject();

    char num_vals_str[32];
    snprintf(num_vals_str, sizeof(num_vals_str), "%d", data->num_scaler_vals);

    cJSON_AddStringToObject(root, "num_vals", num_vals_str);
    cJSON *obis_list = cJSON_AddArrayToObject(root, "obis_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        cJSON_AddItemToArray(obis_list, cJSON_CreateString(data->scalar_values[i].obis_code));
    }

    cJSON *val_list = cJSON_AddArrayToObject(root, "val_list");

    for (i = 0; i < data->num_scaler_vals; i++)
    {

        char str_val[64];

        sprintf(str_val, "%lf", data->scalar_values[i].multiplier);

        cJSON_AddItemToArray(val_list, cJSON_CreateString(str_val));
    }

    char *json_string = cJSON_Print(root);

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "event_scalar_info");
    memset(key, 0, sizeof(key));

    memset(temp_met_sn, 0, sizeof(temp_met_sn));
    // strcpy(temp_met_sn, get_meter_2_ser_num((midx)));
    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    // // rithika 06oct
    // else
    // {
    //     return -1;
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    reply = redisCommand(redis, "HMSET %s %s %s", hash_key, key, json_string);

    if (reply == NULL)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : Redis error", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        freeReplyObject(reply);
        return -1;
    }

    dbg_log(INFORM, "[%-25s] :Redis hash event_scalar_info updated successfully\n", fun_name);

    freeReplyObject(reply);
    free(json_string);
    return 0;
}

// int check_od_command_resp()
// {
//     static char fun_name[] = "check_od_command_resp()";
//     int i;
//     if (!redis || redis->err)
//     {
//         printf("Redis not connected or error: %s\n", redis ? redis->errstr : "null");
//         return -1;
//     }

//     const char *list_key = "ftp_od_resp_sts";

//     redisReply *llen_reply = redisCommand(redis, "LLEN %s", list_key);
//     if (!llen_reply || llen_reply->type != REDIS_REPLY_INTEGER)
//     {
//         printf("Failed to get list length from Redis\n");
//         if (llen_reply)
//             freeReplyObject(llen_reply);
//         return -1;
//     }

//     int list_len = llen_reply->integer;
//     freeReplyObject(llen_reply);

//     print_colored_message(GREEN, 1, "list length ftp  %d\n", list_len);

//     // Arrays to store previously seen msg_type and meter_ser_num
//     char seen_msg_types[128][64] = {0};
//     char seen_sernums[128][64] = {0};
//     int seen_count = 0;
//     char seen_json[128][1024]= {0};

//     for (i = 0; i < list_len; i++)
//     {
//         redisReply *item_reply = redisCommand(redis, "LINDEX %s %d", list_key, i);
//         if (!item_reply || item_reply->type != REDIS_REPLY_STRING)
//         {
//             if (item_reply)
//                 freeReplyObject(item_reply);
//             printf("00000000000000\n");
//             continue;
//         }

//         cJSON *root = cJSON_Parse(item_reply->str);
//         if (!root)
//         {
//             freeReplyObject(item_reply);
//             printf("121212121212\n");
//             continue;
//         }

//         char portid[4];
//         snprintf(portid, sizeof(portid), "%u", g_pidx);

//         cJSON *port = cJSON_GetObjectItemCaseSensitive(root, "port_id");
//         if (!cJSON_IsString(port) || port->valuestring == NULL)
//         {
//             // Invalid or missing port_id field; skip this item
//             cJSON_Delete(root);
//             freeReplyObject(item_reply);
//             printf("111111111111\n");
//             continue;
//         }

//         if (strcmp(port->valuestring, portid) != 0)
//         {
//             // port_id doesn't match; skip this item
//             cJSON_Delete(root);
//             freeReplyObject(item_reply);
//             printf("2222222222\n");
//             continue;
//         }

//         cJSON *msg_type = cJSON_GetObjectItemCaseSensitive(root, "msg_type");
//         cJSON *sernum = cJSON_GetObjectItemCaseSensitive(root, "meter_ser_num");
//         printf("33333333333333333333\n");
//         if (cJSON_IsString(msg_type) && cJSON_IsString(sernum))
//         {
//             // Check if this combination was seen before
//             for (int j = 0; j < seen_count; j++)
//             {
//                 print_colored_message(YELLOW, 1,"inside for loop seen_msg_types[j] %s\nmsg_type->valuestring %s\nseen_sernums[j] %s\nsernum->valuestring %s\nsen count %d\n ", seen_msg_types[j], msg_type->valuestring, seen_sernums[j], sernum->valuestring, seen_count);

//                 if (strcmp(seen_msg_types[j], msg_type->valuestring) == 0 &&
//                     strcmp(seen_sernums[j], sernum->valuestring) == 0)
//                 {
//                     // Duplicate found
//                     char table_name[100];
//                     char datatype[64];
//                     char temp_manufacturer[64];
//                     printf("Duplicate found at index %d: msg_type=%s, seen_msg_types[j] %s meter_ser_num=%s\n", i, msg_type->valuestring, seen_msg_types[j], sernum->valuestring);

//                     if (strcmp(msg_type->valuestring, "OD_LS_DATA") == 0)
//                     {
//                         memset(datatype, 0, sizeof(datatype));
//                         snprintf(datatype, sizeof(datatype), "ls_data");
//                     }
//                     else if (strcmp(msg_type->valuestring, "OD_MN_DATA") == 0)
//                     {
//                         memset(datatype, 0, sizeof(datatype));
//                         snprintf(datatype, sizeof(datatype), "daily_profile_data");
//                     }
//                     else if (strcmp(msg_type->valuestring, "OD_BILL_DATA") == 0)
//                     {
//                         memset(datatype, 0, sizeof(datatype));
//                         snprintf(datatype, sizeof(datatype), "bill_data");
//                     }
//                     else if (strcmp(msg_type->valuestring, "OD_EVENT_DATA") == 0)
//                     {
//                         memset(datatype, 0, sizeof(datatype));
//                         snprintf(datatype, sizeof(datatype), "event_data");
//                     }

//                     int meter_id = get_ser_num_to_metid_serial(seen_sernums[j]);
//                     // char *manufacturer = get_manufacturer(meter_id, seen_sernums[j]);

//                     // // strcpy(manufacturer, get_manufacturer(midx, temp_met_sn));
//                     // if (manufacturer == NULL)
//                     // {
//                     //     dbg_log(INFORM, "[%-25s] : [Meter - %d] : Failed to get meter manufacturer.\n", fun_name, meter_id);
//                     //     cJSON_Delete(root);
//                     //     freeReplyObject(item_reply);
//                     //     continue;
//                     // }

//                     char manufacturer[64];
//                     if (get_manufacturer(meter_id, seen_sernums[j], manufacturer, sizeof(manufacturer)) != 0)
//                     {
//                         dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter manufacturer.\n", meter_id, fun_name);
//                         cJSON_Delete(root);
//                         freeReplyObject(item_reply);
//                         printf("4444444444\n");
//                         continue;
//                     }
//                     else
//                     {
//                         print_colored_message(GREEN, 1, "check_od_command_resp manufacturer %s temp_met_sn %s\n", manufacturer, seen_sernums[j]);

//                         snprintf(temp_manufacturer, sizeof(temp_manufacturer), "%s", manufacturer);
//                     }

//                     memset(table_name, 0, sizeof(table_name));
//                     snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%s", datatype, temp_manufacturer, dcu_ser_num, g_pidx, seen_sernums[j]);
//                     printf("table_name : %s\n", table_name);

//                     drop_table(table_name);

//                     print_colored_message(YELLOW, 1, "------------- %s ----------\n", seen_json[j]);

//                     print_colored_message(YELLOW, 1, "Removing matched JSON:\n1. %s\n2. %s\n", seen_json[j], item_reply->str);

//                     redisReply *rem1 = redisCommand(redis, "LREM %s 1 %b", list_key, seen_json[j], strlen(seen_json[j]));
//                     redisReply *rem2 = redisCommand(redis, "LREM %s 1 %b", list_key, item_reply->str, strlen(item_reply->str));

//                     if (rem1 && rem1->type == REDIS_REPLY_INTEGER)
//                         print_colored_message(GREEN, 1, "Removed 1st instance, count: %lld\n", rem1->integer);
//                     if (rem2 && rem2->type == REDIS_REPLY_INTEGER)
//                         print_colored_message(GREEN, 1, "Removed 2nd instance, count: %lld\n", rem2->integer);

//                     if (rem1)
//                         freeReplyObject(rem1);
//                     if (rem2)
//                         freeReplyObject(rem2);

//                     cJSON_Delete(root);
//                     freeReplyObject(item_reply);
//                     return 0;
//                 }
//             }

//             // Store this combination
//             strncpy(seen_msg_types[seen_count], msg_type->valuestring, sizeof(seen_msg_types[0]) - 1);
//             strncpy(seen_sernums[seen_count], sernum->valuestring, sizeof(seen_sernums[0]) - 1);
//             strncpy(seen_json[seen_count], item_reply->str, sizeof(seen_json[0]) - 1);

//             printf("outside for loop seen_msg_types[j] %s\nmsg_type->valuestring %s\nseen_sernums[j] %s\nsernum->valuestring %s\nsen count %d\n ", seen_msg_types[seen_count], msg_type->valuestring, seen_sernums[seen_count], sernum->valuestring, seen_count);
//        seen_count++;
//         }

//         cJSON_Delete(root);
//         freeReplyObject(item_reply);
//     }

//     // No duplicate found
//     return 0;
// }

int check_od_command_resp()
{
    static char fun_name[] = "check_od_command_resp()";
    int i;
    if (!redis || redis->err)
    {
        printf("Redis not connected or error: %s\n", redis ? redis->errstr : "null");
        return -1;
    }

    const char *list_key = "ftp_od_resp_sts";

    redisReply *llen_reply = redisCommand(redis, "LLEN %s", list_key);
    if (!llen_reply || llen_reply->type != REDIS_REPLY_INTEGER)
    {
        printf("Failed to get list length from Redis\n");
        if (llen_reply)
            freeReplyObject(llen_reply);
        return -1;
    }

    int list_len = llen_reply->integer;
    freeReplyObject(llen_reply);

    print_colored_message(GREEN, 1, "list length ftp  %d\n", list_len);

    for (i = 0; i < list_len; i++)
    {
        redisReply *item_reply = redisCommand(redis, "LINDEX %s %d", list_key, i);
        if (!item_reply || item_reply->type != REDIS_REPLY_STRING)
        {
            if (item_reply)
                freeReplyObject(item_reply);

            continue;
        }

        cJSON *root = cJSON_Parse(item_reply->str);
        if (!root)
        {
            freeReplyObject(item_reply);

            continue;
        }

        char portid[4];
        snprintf(portid, sizeof(portid), "%u", g_pidx);

        cJSON *port = cJSON_GetObjectItemCaseSensitive(root, "port_id");
        if (!cJSON_IsString(port) || port->valuestring == NULL)
        {
            // Invalid or missing port_id field; skip this item
            cJSON_Delete(root);
            freeReplyObject(item_reply);

            continue;
        }

        if (strcmp(port->valuestring, portid) != 0)
        {
            // port_id doesn't match; skip this item
            cJSON_Delete(root);
            freeReplyObject(item_reply);

            continue;
        }

        cJSON *msg_type = cJSON_GetObjectItemCaseSensitive(root, "msg_type");
        cJSON *sernum = cJSON_GetObjectItemCaseSensitive(root, "meter_ser_num");

        if (cJSON_IsString(msg_type) && cJSON_IsString(sernum))
        {

            // Duplicate found
            char table_name[100];
            char datatype[64];
            char temp_manufacturer[64];
            printf("Duplicate found at index %d: msg_type=%s,  meter_ser_num=%s\n", i, msg_type->valuestring, sernum->valuestring);

            if (strcmp(msg_type->valuestring, "OD_LS_DATA") == 0)
            {
                memset(datatype, 0, sizeof(datatype));
                snprintf(datatype, sizeof(datatype), "ls_data");
            }
            else if (strcmp(msg_type->valuestring, "OD_MN_DATA") == 0)
            {
                memset(datatype, 0, sizeof(datatype));
                snprintf(datatype, sizeof(datatype), "daily_profile_data");
            }
            else if (strcmp(msg_type->valuestring, "OD_BILL_DATA") == 0)
            {
                memset(datatype, 0, sizeof(datatype));
                snprintf(datatype, sizeof(datatype), "bill_data");
            }
            else if (strcmp(msg_type->valuestring, "OD_EVENT_DATA") == 0)
            {
                memset(datatype, 0, sizeof(datatype));
                snprintf(datatype, sizeof(datatype), "event_data");
            }

            int meter_id = get_ser_num_to_metid_serial(sernum->valuestring);
            // char *manufacturer = get_manufacturer(meter_id, seen_sernums[j]);

            // // strcpy(manufacturer, get_manufacturer(midx, temp_met_sn));
            // if (manufacturer == NULL)
            // {
            //     dbg_log(INFORM, "[%-25s] : [Meter - %d] : Failed to get meter manufacturer.\n", fun_name, meter_id);
            //     cJSON_Delete(root);
            //     freeReplyObject(item_reply);
            //     continue;
            // }

            char manufacturer[64];
            if (get_manufacturer(meter_id, sernum->valuestring, manufacturer, sizeof(manufacturer)) != 0)
            {
                dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter manufacturer.\n", meter_id, fun_name);
                cJSON_Delete(root);
                freeReplyObject(item_reply);

                continue;
            }
            else
            {
                print_colored_message(GREEN, 1, "check_od_command_resp manufacturer %s temp_met_sn %s\n", manufacturer, sernum->valuestring);

                snprintf(temp_manufacturer, sizeof(temp_manufacturer), "%s", manufacturer);
            }

            memset(table_name, 0, sizeof(table_name));
            // rithika 28Nov2025
            char temp_met_serial_no[64] = {0};
            strcpy(temp_met_serial_no, sernum->valuestring);

            for (int i = 0; i < strlen(temp_met_serial_no); i++)
            {

                if (temp_met_serial_no[i] == '-')
                {
                    temp_met_serial_no[i] = '_';
                }
            }

            snprintf(table_name, sizeof(table_name), "%s_od_%s_%s_%d_%s", datatype, temp_manufacturer, dcu_ser_num, g_pidx, temp_met_serial_no);
            printf("table_name : %s\n", table_name);

            drop_table(table_name);

            print_colored_message(YELLOW, 1, "Removing  JSON: %s\n", item_reply->str);

            redisReply *rem1 = redisCommand(redis, "LREM %s 1 %b", list_key, item_reply->str, strlen(item_reply->str));

            if (rem1 && rem1->type == REDIS_REPLY_INTEGER)
                print_colored_message(GREEN, 1, "Removed 1st instance, count: %lld\n", rem1->integer);

            if (rem1)
                freeReplyObject(rem1);
        }

        cJSON_Delete(root);
        freeReplyObject(item_reply);
    }

    // No duplicate found
    return 0;
}

// rithika 9sep
int get_event_obis()
{
    char fun_name[64] = "get_event_obis";
    int i;

    redisReply *reply = redisCommand(redis, "HGET master_obis_codes event_obis_list");
    if (!reply)
    {
        fprintf(stderr, "Redis error: %s\n", redis->errstr);
        return -1;
    }

    // Parse the JSON string with cJSON
    cJSON *obis_json_list = cJSON_Parse(reply->str);
    freeReplyObject(reply);
    if (!obis_json_list)
    {
        fprintf(stderr, "Failed to parse event_obis_list JSON\n");
        return -1;
    }

    // Get the array length dynamically
    obis_count = cJSON_GetArraySize(obis_json_list);

    // Loop over the array items dynamically
    for (i = 0; i < obis_count; i++)
    {
        cJSON *obis_item = cJSON_GetArrayItem(obis_json_list, i);

        if (!cJSON_IsString(obis_item))
        {
            // Invalid item, skip or break based on your logic
            continue;
        }

        snprintf(event_obis.event_obis_codes[i],
                 sizeof(event_obis.event_obis_codes[i]),
                 "%s",
                 obis_item->valuestring);
    }

    // Cleanup
    cJSON_Delete(obis_json_list);

    return 0;
}

void update_old_data_poll_status_in_redis(const char *val)
{
    if (!redis || redis->err)
    {
        printf("Redis context is invalid or not connected.\n");
        return;
    }

    // Set value
    redisReply *reply = redisCommand(redis, "HSET old_data_poll_sts poll_status %s", val);
    if (reply)
    {
        printf("Redis HSET reply: %lld\n", reply->integer); // 1 = new field, 0 = updated existing
        freeReplyObject(reply);
    }
    else
    {
        printf("Failed to execute HSET command.\n");
        return;
    }
}

int fetch_inst_scalar_info(int midx, inst_val_info_t *out_data)
{

    static char fun_name[] = "fetch_inst_scalar_info";
    redisReply *reply;
    char hash_key[64], key[64], temp_met_sn[64];

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "inst_scalar_info");

    memset(key, 0, sizeof(key));
    memset(temp_met_sn, 0, sizeof(temp_met_sn));

    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    // Fetch JSON string
    reply = redisCommand(redis, "HGET %s %s", hash_key, key);

    if (reply == NULL || reply->type == REDIS_REPLY_NIL)
    {
        dbg_log(REPORT, "[%-25s] : Redis returned NULL for key %s\n", fun_name, key);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    if (reply->type != REDIS_REPLY_STRING)
    {
        dbg_log(REPORT, "[%-25s] : Unexpected Redis reply type for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(reply->str);
    if (!root)
    {
        dbg_log(REPORT, "[%-25s] : Failed to parse JSON for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // num_vals
    cJSON *num_vals = cJSON_GetObjectItem(root, "num_vals");
    if (num_vals && cJSON_IsString(num_vals))
    {
        out_data->num_scaler_vals = atoi(num_vals->valuestring);
    }
    else
    {
        out_data->num_scaler_vals = 0;
    }

    // obis_list
    cJSON *obis_list = cJSON_GetObjectItem(root, "obis_list");
    cJSON *val_list = cJSON_GetObjectItem(root, "val_list");

    int count = out_data->num_scaler_vals;
    // if (count > MAX_SCALAR_VALS) count = MAX_SCALAR_VALS;

    for (int i = 0; i < count; i++)
    {
        cJSON *obis_item = cJSON_GetArrayItem(obis_list, i);
        cJSON *val_item = cJSON_GetArrayItem(val_list, i);

        if (obis_item && val_item && cJSON_IsString(obis_item) && cJSON_IsString(val_item))
        {
            strncpy(out_data->scalar_values[i].obis_code, obis_item->valuestring,
                    sizeof(out_data->scalar_values[i].obis_code) - 1);

            out_data->scalar_values[i].multiplier = atof(val_item->valuestring);
        }
    }

    cJSON_Delete(root);
    freeReplyObject(reply);

    dbg_log(INFORM, "[%-25s] : Loaded %d scaler values from Redis for meter %s\n",
            fun_name, out_data->num_scaler_vals, key);

    return 0;
}

int fetch_ls_scalar_info(int midx, load_survey_data_value_t *out_data)
{

    static char fun_name[] = "fetch_ls_scalar_info";
    redisReply *reply;
    char hash_key[64], key[64], temp_met_sn[64];

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "ls_scalar_info");

    memset(key, 0, sizeof(key));
    memset(temp_met_sn, 0, sizeof(temp_met_sn));

    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }
    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, " [Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    // Fetch JSON string
    reply = redisCommand(redis, "HGET %s %s", hash_key, key);

    if (reply == NULL || reply->type == REDIS_REPLY_NIL)
    {
        dbg_log(REPORT, "[%-25s] : Redis returned NULL for key %s\n", fun_name, key);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    if (reply->type != REDIS_REPLY_STRING)
    {
        dbg_log(REPORT, "[%-25s] : Unexpected Redis reply type for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(reply->str);
    if (!root)
    {
        dbg_log(REPORT, "[%-25s] : Failed to parse JSON for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // num_vals
    cJSON *num_vals = cJSON_GetObjectItem(root, "num_vals");
    if (num_vals && cJSON_IsString(num_vals))
    {
        out_data->num_scaler_vals = atoi(num_vals->valuestring);
    }
    else
    {
        out_data->num_scaler_vals = 0;
    }

    // obis_list
    cJSON *obis_list = cJSON_GetObjectItem(root, "obis_list");
    cJSON *val_list = cJSON_GetObjectItem(root, "val_list");

    int count = out_data->num_scaler_vals;
    // if (count > MAX_SCALAR_VALS) count = MAX_SCALAR_VALS;

    for (int i = 0; i < count; i++)
    {
        cJSON *obis_item = cJSON_GetArrayItem(obis_list, i);
        cJSON *val_item = cJSON_GetArrayItem(val_list, i);

        if (obis_item && val_item && cJSON_IsString(obis_item) && cJSON_IsString(val_item))
        {
            strncpy(out_data->scalar_values[i].obis_code, obis_item->valuestring,
                    sizeof(out_data->scalar_values[i].obis_code) - 1);

            out_data->scalar_values[i].multiplier = atof(val_item->valuestring);
        }
    }

    cJSON_Delete(root);
    freeReplyObject(reply);

    dbg_log(INFORM, "[%-25s] : Loaded %d scaler values from Redis for meter %s\n",
            fun_name, out_data->num_scaler_vals, key);

    return 0;
}

int fetch_mn_scalar_info(int midx, daily_profile_data_value_t *out_data)
{

    static char fun_name[] = "fetch_mn_scalar_info";
    redisReply *reply;
    char hash_key[64], key[64], temp_met_sn[64];

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "mn_scalar_info");

    memset(key, 0, sizeof(key));
    memset(temp_met_sn, 0, sizeof(temp_met_sn));

    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    // Fetch JSON string
    reply = redisCommand(redis, "HGET %s %s", hash_key, key);

    if (reply == NULL || reply->type == REDIS_REPLY_NIL)
    {
        dbg_log(REPORT, "[%-25s] : Redis returned NULL for key %s\n", fun_name, key);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    if (reply->type != REDIS_REPLY_STRING)
    {
        dbg_log(REPORT, "[%-25s] : Unexpected Redis reply type for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(reply->str);
    if (!root)
    {
        dbg_log(REPORT, "[%-25s] : Failed to parse JSON for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // num_vals
    cJSON *num_vals = cJSON_GetObjectItem(root, "num_vals");
    if (num_vals && cJSON_IsString(num_vals))
    {
        out_data->num_scaler_vals = atoi(num_vals->valuestring);
    }
    else
    {
        out_data->num_scaler_vals = 0;
    }

    // obis_list
    cJSON *obis_list = cJSON_GetObjectItem(root, "obis_list");
    cJSON *val_list = cJSON_GetObjectItem(root, "val_list");

    int count = out_data->num_scaler_vals;
    // if (count > MAX_SCALAR_VALS) count = MAX_SCALAR_VALS;

    for (int i = 0; i < count; i++)
    {
        cJSON *obis_item = cJSON_GetArrayItem(obis_list, i);
        cJSON *val_item = cJSON_GetArrayItem(val_list, i);

        if (obis_item && val_item && cJSON_IsString(obis_item) && cJSON_IsString(val_item))
        {
            strncpy(out_data->scalar_values[i].obis_code, obis_item->valuestring,
                    sizeof(out_data->scalar_values[i].obis_code) - 1);

            out_data->scalar_values[i].multiplier = atof(val_item->valuestring);
        }
    }

    cJSON_Delete(root);
    freeReplyObject(reply);

    dbg_log(INFORM, "[%-25s] : Loaded %d scaler values from Redis for meter %s\n",
            fun_name, out_data->num_scaler_vals, key);

    return 0;
}

int fetch_bill_scalar_info(int midx, billing_data_value_t *out_data)
{

    static char fun_name[] = "fetch_bill_scalar_info";
    redisReply *reply;
    char hash_key[64], key[64], temp_met_sn[64];

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "bill_scalar_info");

    memset(key, 0, sizeof(key));
    memset(temp_met_sn, 0, sizeof(temp_met_sn));

    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    // Fetch JSON string
    reply = redisCommand(redis, "HGET %s %s", hash_key, key);

    if (reply == NULL || reply->type == REDIS_REPLY_NIL)
    {
        dbg_log(REPORT, "[%-25s] : Redis returned NULL for key %s\n", fun_name, key);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    if (reply->type != REDIS_REPLY_STRING)
    {
        dbg_log(REPORT, "[%-25s] : Unexpected Redis reply type for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(reply->str);
    if (!root)
    {
        dbg_log(REPORT, "[%-25s] : Failed to parse JSON for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // num_vals
    cJSON *num_vals = cJSON_GetObjectItem(root, "num_vals");
    if (num_vals && cJSON_IsString(num_vals))
    {
        out_data->num_scaler_vals = atoi(num_vals->valuestring);
    }
    else
    {
        out_data->num_scaler_vals = 0;
    }

    // obis_list
    cJSON *obis_list = cJSON_GetObjectItem(root, "obis_list");
    cJSON *val_list = cJSON_GetObjectItem(root, "val_list");

    int count = out_data->num_scaler_vals;
    // if (count > MAX_SCALAR_VALS) count = MAX_SCALAR_VALS;

    for (int i = 0; i < count; i++)
    {
        cJSON *obis_item = cJSON_GetArrayItem(obis_list, i);
        cJSON *val_item = cJSON_GetArrayItem(val_list, i);

        if (obis_item && val_item && cJSON_IsString(obis_item) && cJSON_IsString(val_item))
        {
            strncpy(out_data->scalar_values[i].obis_code, obis_item->valuestring,
                    sizeof(out_data->scalar_values[i].obis_code) - 1);

            out_data->scalar_values[i].multiplier = atof(val_item->valuestring);
        }
    }

    cJSON_Delete(root);
    freeReplyObject(reply);

    dbg_log(INFORM, "[%-25s] : Loaded %d scaler values from Redis for meter %s\n",
            fun_name, out_data->num_scaler_vals, key);

    return 0;
}

int fetch_event_scalar_info(int midx, events_data_value_t *out_data)
{

    static char fun_name[] = "fetch_event_scalar_info";
    redisReply *reply;
    char hash_key[64], key[64], temp_met_sn[64];

    memset(hash_key, 0, sizeof(hash_key));
    sprintf(hash_key, "event_scalar_info");

    memset(key, 0, sizeof(key));
    memset(temp_met_sn, 0, sizeof(temp_met_sn));

    // char *sn = get_meter_2_ser_num(midx);
    // if (sn)
    // {
    //     strncpy(temp_met_sn, sn, sizeof(temp_met_sn) - 1);
    //     free(sn);
    // }

    if (get_meter_2_ser_num(midx, temp_met_sn, sizeof(temp_met_sn)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
    }

    sprintf(key, "meter_%d_%d_%s", g_pidx, midx + 1, temp_met_sn);

    // Fetch JSON string
    reply = redisCommand(redis, "HGET %s %s", hash_key, key);

    if (reply == NULL || reply->type == REDIS_REPLY_NIL)
    {
        dbg_log(REPORT, "[%-25s] : Redis returned NULL for key %s\n", fun_name, key);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    if (reply->type != REDIS_REPLY_STRING)
    {
        dbg_log(REPORT, "[%-25s] : Unexpected Redis reply type for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // Parse JSON
    cJSON *root = cJSON_Parse(reply->str);
    if (!root)
    {
        dbg_log(REPORT, "[%-25s] : Failed to parse JSON for key %s\n", fun_name, key);
        freeReplyObject(reply);
        return -1;
    }

    // num_vals
    cJSON *num_vals = cJSON_GetObjectItem(root, "num_vals");
    if (num_vals && cJSON_IsString(num_vals))
    {
        out_data->num_scaler_vals = atoi(num_vals->valuestring);
    }
    else
    {
        out_data->num_scaler_vals = 0;
    }

    // obis_list
    cJSON *obis_list = cJSON_GetObjectItem(root, "obis_list");
    cJSON *val_list = cJSON_GetObjectItem(root, "val_list");

    int count = out_data->num_scaler_vals;
    // if (count > MAX_SCALAR_VALS) count = MAX_SCALAR_VALS;

    for (int i = 0; i < count; i++)
    {
        cJSON *obis_item = cJSON_GetArrayItem(obis_list, i);
        cJSON *val_item = cJSON_GetArrayItem(val_list, i);

        if (obis_item && val_item && cJSON_IsString(obis_item) && cJSON_IsString(val_item))
        {
            strncpy(out_data->scalar_values[i].obis_code, obis_item->valuestring,
                    sizeof(out_data->scalar_values[i].obis_code) - 1);

            out_data->scalar_values[i].multiplier = atof(val_item->valuestring);
        }
    }

    cJSON_Delete(root);
    freeReplyObject(reply);

    dbg_log(INFORM, "[%-25s] : Loaded %d scaler values from Redis for meter %s\n",
            fun_name, out_data->num_scaler_vals, key);

    return 0;
}

int update_mn_poll_time(int midx, const char *last_poll_time_str)
{
    static char fun_name[] = "update_mn_poll_time";
    redisReply *reply = NULL;
    char key[64], next_time_str[32];
    // char *serial_number = NULL;
    int mn_poll_int = 0;

    reply = redisCommand(redis, "HGET poll_cfg mn_poll_int");
    if (reply == NULL || reply->type == REDIS_REPLY_NIL || reply->str == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to read mn_poll_int from Redis\n", fun_name);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    mn_poll_int = atoi(reply->str);
    freeReplyObject(reply);

    if (mn_poll_int <= 0)
    {
        dbg_log(REPORT, "[%-25s] : Invalid mn_poll_int value: %d\n", fun_name, mn_poll_int);
        return -1;
    }

    struct tm tm_info = {0};
    if (strptime(last_poll_time_str, "%Y-%m-%d %H:%M:%S", &tm_info) == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Invalid last_poll_time_str format: %s\n", fun_name, last_poll_time_str);
        return -1;
    }

    time_t poll_time = mktime(&tm_info);
    poll_time += mn_poll_int;

    struct tm *next_tm = localtime(&poll_time);
    strftime(next_time_str, sizeof(next_time_str), "%Y-%m-%d %H:%M:%S", next_tm);

    // serial_number = get_meter_2_ser_num(midx);
    // if (serial_number == NULL)
    // {
    //     dbg_log(REPORT, "[%-25s] : Failed to get meter serial number\n", fun_name);
    //     return -1;
    // }

    // snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);
    // free(serial_number);

    char serial_number[64] = {0};
    if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);

    cJSON *root = cJSON_CreateObject();

    if (!root)
    {
        printf("Failed to create JSON object\n");
        return -1;
    }
    cJSON_AddStringToObject(root, "last_poll_time", last_poll_time_str);
    cJSON_AddStringToObject(root, "next_poll_time", next_time_str);

    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        printf("Failed to print JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    reply = redisCommand(redis, "HSET mn_poll_time %s %s", key, json_string);
    if (reply == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to store mn_poll_time: %s\n", fun_name, redis->errstr);
        return -1;
    }

    freeReplyObject(reply);

    dbg_log(REPORT, "[%-25s] : Updated poll time for %s -> %s, %s (interval: %d sec)\n",
            fun_name, key, last_poll_time_str, next_time_str, mn_poll_int);

    return 0;
}

int update_ls_poll_time(int midx, const char *last_poll_time_str)
{
    static char fun_name[] = "update_ls_poll_time";
    redisReply *reply = NULL;
    char key[64], next_time_str[32];
    // char *serial_number = NULL;
    int ls_poll_int = 0;

    reply = redisCommand(redis, "HGET poll_cfg ls_poll_int");
    if (reply == NULL || reply->type == REDIS_REPLY_NIL || reply->str == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to read ls_poll_int from Redis\n", fun_name);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    ls_poll_int = atoi(reply->str);
    freeReplyObject(reply);

    if (ls_poll_int <= 0)
    {
        dbg_log(REPORT, "[%-25s] : Invalid ls_poll_int value: %d\n", fun_name, ls_poll_int);
        return -1;
    }

    struct tm tm_info = {0};
    if (strptime(last_poll_time_str, "%Y-%m-%d %H:%M:%S", &tm_info) == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Invalid last_poll_time_str format: %s\n", fun_name, last_poll_time_str);
        return -1;
    }

    time_t poll_time = mktime(&tm_info);
    poll_time += ls_poll_int;

    struct tm *next_tm = localtime(&poll_time);
    strftime(next_time_str, sizeof(next_time_str), "%Y-%m-%d %H:%M:%S", next_tm);

    // serial_number = get_meter_2_ser_num(midx);
    // if (serial_number == NULL)
    // {
    //     dbg_log(REPORT, "[%-25s] : Failed to get meter serial number\n", fun_name);
    //     return -1;
    // }

    // snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);
    // free(serial_number);

    char serial_number[64] = {0};
    if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);

    cJSON *root = cJSON_CreateObject();

    if (!root)
    {
        printf("Failed to create JSON object\n");
        return -1;
    }
    cJSON_AddStringToObject(root, "last_poll_time", last_poll_time_str);
    cJSON_AddStringToObject(root, "next_poll_time", next_time_str);

    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        printf("Failed to print JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    reply = redisCommand(redis, "HSET ls_poll_time %s %s", key, json_string);
    if (reply == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to store ls_poll_time: %s\n", fun_name, redis->errstr);
        return -1;
    }

    freeReplyObject(reply);

    dbg_log(REPORT, "[%-25s] : Updated poll time for %s ->%s, %s (interval: %d sec)\n",
            fun_name, key, last_poll_time_str, next_time_str, ls_poll_int);

    return 0;
}

int update_bill_poll_time(int midx, const char *last_poll_time_str)
{
    static char fun_name[] = "update_bill_poll_time";
    redisReply *reply = NULL;
    char key[64], next_time_str[32];
    // char *serial_number = NULL;
    int bill_poll_int = 0;

    reply = redisCommand(redis, "HGET poll_cfg bill_poll_int");
    if (reply == NULL || reply->type == REDIS_REPLY_NIL || reply->str == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to read bill_poll_int from Redis\n", fun_name);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    bill_poll_int = atoi(reply->str);
    freeReplyObject(reply);

    if (bill_poll_int <= 0)
    {
        dbg_log(REPORT, "[%-25s] : Invalid bill_poll_int value: %d\n", fun_name, bill_poll_int);
        return -1;
    }

    struct tm tm_info = {0};
    if (strptime(last_poll_time_str, "%Y-%m-%d %H:%M:%S", &tm_info) == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Invalid last_poll_time_str format: %s\n", fun_name, last_poll_time_str);
        return -1;
    }

    time_t poll_time = mktime(&tm_info);
    poll_time += bill_poll_int;

    struct tm *next_tm = localtime(&poll_time);
    strftime(next_time_str, sizeof(next_time_str), "%Y-%m-%d %H:%M:%S", next_tm);

    // serial_number = get_meter_2_ser_num(midx);
    // if (serial_number == NULL)
    // {
    //     dbg_log(REPORT, "[%-25s] : Failed to get meter serial number\n", fun_name);
    //     return -1;
    // }

    // snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);
    // free(serial_number);

    char serial_number[64] = {0};
    if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);

    cJSON *root = cJSON_CreateObject();

    if (!root)
    {
        printf("Failed to create JSON object\n");
        return -1;
    }
    cJSON_AddStringToObject(root, "last_poll_time", last_poll_time_str);
    cJSON_AddStringToObject(root, "next_poll_time", next_time_str);

    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        printf("Failed to print JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    reply = redisCommand(redis, "HSET billing_poll_time %s %s", key, json_string);
    if (reply == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to store billing_poll_time: %s\n", fun_name, redis->errstr);
        return -1;
    }

    freeReplyObject(reply);

    dbg_log(REPORT, "[%-25s] : Updated  poll time for %s -> %s, %s (interval: %d sec)\n",
            fun_name, key, last_poll_time_str, next_time_str, bill_poll_int);

    return 0;
}

int update_event_poll_time(int midx, const char *last_poll_time_str)
{
    static char fun_name[] = "update_event_poll_time";
    redisReply *reply = NULL;
    char key[64], next_time_str[32];
    // char *serial_number = NULL;
    int event_poll_int = 0;

    reply = redisCommand(redis, "HGET poll_cfg event_poll_int");
    if (reply == NULL || reply->type == REDIS_REPLY_NIL || reply->str == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to read event_poll_int from Redis\n", fun_name);
        if (reply)
            freeReplyObject(reply);
        return -1;
    }

    event_poll_int = atoi(reply->str);
    freeReplyObject(reply);

    if (event_poll_int <= 0)
    {
        dbg_log(REPORT, "[%-25s] : Invalid event_poll_int value: %d\n", fun_name, event_poll_int);
        return -1;
    }

    struct tm tm_info = {0};
    if (strptime(last_poll_time_str, "%Y-%m-%d %H:%M:%S", &tm_info) == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Invalid last_poll_time_str format: %s\n", fun_name, last_poll_time_str);
        return -1;
    }

    time_t poll_time = mktime(&tm_info);
    poll_time += event_poll_int;

    struct tm *next_tm = localtime(&poll_time);
    strftime(next_time_str, sizeof(next_time_str), "%Y-%m-%d %H:%M:%S", next_tm);

    // serial_number = get_meter_2_ser_num(midx);
    // if (serial_number == NULL)
    // {
    //     dbg_log(REPORT, "[%-25s] : Failed to get meter serial number\n", fun_name);
    //     return -1;
    // }

    // snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);
    // free(serial_number);

    char serial_number[64] = {0};
    if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);

        return -1;
    }

    snprintf(key, sizeof(key), "meter_%d_%d_%s", g_pidx, (midx + 1), serial_number);

    cJSON *root = cJSON_CreateObject();

    if (!root)
    {
        printf("Failed to create JSON object\n");
        return -1;
    }
    cJSON_AddStringToObject(root, "last_poll_time", last_poll_time_str);
    cJSON_AddStringToObject(root, "next_poll_time", next_time_str);

    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        printf("Failed to print JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    reply = redisCommand(redis, "HSET event_poll_time %s %s", key, json_string);
    if (reply == NULL)
    {
        dbg_log(REPORT, "[%-25s] : Failed to store event_poll_time: %s\n", fun_name, redis->errstr);
        return -1;
    }

    freeReplyObject(reply);

    dbg_log(REPORT, "[%-25s] : Updated  poll time for %s -> %s, %s (interval: %d sec)\n",
            fun_name, key, last_poll_time_str, next_time_str, event_poll_int);

    return 0;
}

int get_dev_model()
{
    static char fun_name[] = "get_dev_model()";
    redisReply *reply = NULL;
    char key[64];
    sprintf(key, "dcu_info");
    reply = redisCommand(redis, "HGET %s device_model", key);

    if (reply->str == NULL)
    {
        freeReplyObject(reply);
        return -1;
    }
    memset(dev_model, 0, sizeof(dev_model));
    snprintf(dev_model, sizeof(dev_model), "%s", reply->str);
    freeReplyObject(reply);
    return 0;
}

int update_nxt_poll_time(char *datatype, char *next_poll_time)
{

    redisReply *reply = NULL;
    reply = redisCommand(redis, "HSET next_poll_time %s %s", datatype, next_poll_time);
    freeReplyObject(reply);

    return 0;
}

int get_missing_old_data_check()
{
    static char fun_name[] = "get_missing_old_data_check()";
    redisReply *reply = NULL;
    char key[64];
    sprintf(key, "poll_cfg");
    reply = redisCommand(redis, "HGET %s missing_old_data_chk_serial", key);

    if (reply->str == NULL)
    {
        freeReplyObject(reply);
        return -1;
    }
    char old_data_chk_str[4];
    memset(old_data_chk_str, 0, sizeof(old_data_chk_str));
    snprintf(old_data_chk_str, sizeof(old_data_chk_str), "%s", reply->str);
    int miss_old_data_chk = atoi(old_data_chk_str);

    freeReplyObject(reply);
    return miss_old_data_chk;
}

int update_ls_list(int midx, char *meter_ser_num, char *date)
{
    static char fun_name[] = "update_od_sts_ftp";

    cJSON *root = cJSON_CreateObject();

    if (!root)
    {
        printf("Failed to create JSON object\n");
        return -1;
    }

    // Add string fields to the object
    cJSON_AddStringToObject(root, "meter_ser_num", meter_ser_num);
    cJSON_AddStringToObject(root, "date", date);

    char portid[4];
    snprintf(portid, sizeof(portid), "%u", g_pidx);
    cJSON_AddStringToObject(root, "port_id", portid);

    char meterid[4];
    int m_idx = midx + 1;
    snprintf(meterid, sizeof(meterid), "%u", m_idx);
    cJSON_AddStringToObject(root, "meter_id", meterid);

    char *json_string = cJSON_PrintUnformatted(root);
    if (json_string == NULL)
    {
        printf("Failed to print JSON\n");
        cJSON_Delete(root);
        return -1;
    }

    redisReply *reply = redisCommand(redis, "LPUSH check_missing_ls %s", json_string);

    if (reply == NULL)
    {
        fprintf(stderr, "Redis command failed\n");

        print_colored_message(RED, 1, "Redis command failed\n");

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(redis_event_data.eventmsg, "%s : LTRIM  Redis command failed", fun_name);
        strcpy(redis_event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&redis_event_data);

        return -1;
    }
    else
    {
        redisReply *reply_trim = redisCommand(redis, "LTRIM ftp_od_cmd_resp 0 %d", MAX_DIAG_MESSAGE_LIMIT - 1);
        print_colored_message(RED, 1, "Else Redis command failed\n");
        if (reply_trim != NULL)
        {
            freeReplyObject(reply_trim);
        }
    }

    freeReplyObject(reply);
    free(json_string);
    cJSON_Delete(root);
}