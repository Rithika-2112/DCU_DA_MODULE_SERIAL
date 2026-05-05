#include "../../GuruxDLMS.c-master/development/include/gxserializer.h"
#include "../../GuruxDLMS.c-master/development/include/date.h"
#include "include/communication.h"
#include "/home/vishnu/Projects/REConnect/hc_file/hc_heartbeat.h"
#include "/home/vishnu/Projects/REConnect/net_logger/dcu_netlog.h"

#include <stdint.h>

#define NAMEPLATE 0
#define INSTDATA 1
#define DAILYPROFILE 2
#define BILLING 3
#define LSDATA 4
#define EVENTDATA 5
#define INST_SCALER 6
#define DAILYPROFILE_SCALER 7
#define BILLING_SCALER 8
#define LSDATA_SCALER 9
#define EVENTDATA_SCALER 10

// rithika 17feb2026
#define RECONNECT 1 

#if RECONNECT
#define MAX_NAMEPLATE_VALS 16
#else
#define MAX_NAMEPLATE_VALS 7
#endif


#define MAX_NO_OF_SERIAL_PORT 2

//rithika 27sep
#define MAX_INST_VALS 100
#define MAX_EVENT_VALS 50
#define MAX_LS_VALS 40
//rithika 27sep
#define MAX_BILLING_VALS 320
#define MAX_MN_VALS 40

// #define MAX_EVENTS 7 //uncomment for plcc

#define MAX_EVENTS 6

#define INST_OBIS "1.0.94.91.0.255"
#define EVENT_01_OBIS "0.0.99.98.0.255" //voltage
#define EVENT_02_OBIS "0.0.99.98.1.255" // current
#define EVENT_03_OBIS "0.0.99.98.2.255" //power
#define EVENT_04_OBIS "0.0.99.98.3.255" //transaction
#define EVENT_05_OBIS "0.0.99.98.4.255" //other tamper
#define EVENT_06_OBIS "0.0.99.98.5.255" //non roll over
#define EVENT_07_OBIS "0.0.99.98.6.255" //control
#define LS_OBIS "1.0.99.1.0.255"
#define BILLING_OBIS "1.0.98.1.0.255"
#define MN_OBIS "1.0.99.2.0.255"

#define INST_SCALER_OBIS "1.0.94.91.3.255"
#define EVENT_SCALER_OBIS "1.0.94.91.7.255"
#define LS_SCALER_OBIS "1.0.94.91.4.255"
#define BILLING_SCALER_OBIS "1.0.94.91.6.255"
#define MN_SCALER_OBIS "1.0.94.91.5.255"

#define FI_FO 0x01
#define LI_FO 0x02

#define OBIS_ATTR 3
#define VAL_ATTR 2

typedef struct
{
    char obis_code[32];
    char val[32]; 
    double multiplier;
} val_t;


typedef struct
{
    char location[32];
    char serial_number[32];
    char manufacturer[64];
    char meter_type[32];
    char firmware_version[32];
    float ct_ratio;
    float pt_ratio;
    char ct_ratio_str[8];
    char pt_ratio_str[8];
    int block_int;
    char meter_category[32];
    char curr_rating[32];
    char year_of_manuf[32];
    int demand_int_period;
    int hdlc_device_address;
    int transfrmr_ratio_volt;
    char ipv4_address[16];
    int extra_obis_1;
    int extra_obis_2;
    int extra_obis_3;

} nameplate_info_t;

typedef struct
{
    int num_vals;
    int num_scaler_vals;
    uint8_t midx;
    char make[64];
    unsigned char load_status;
    char loc_name[128];
    val_t values[MAX_INST_VALS];
    val_t scalar_values[MAX_INST_VALS];
} inst_val_info_t;

typedef struct
{
    int num_vals;
    int num_scaler_vals;
    uint8_t midx;
    char make[64];
    char loc_name[128];
    val_t values[MAX_BILLING_VALS];
    val_t scalar_values[MAX_BILLING_VALS];
} billing_data_value_t;

typedef struct
{
    uint8_t midx;
    char make[64];
    char loc_name[128];
    int num_vals;
    int num_scaler_vals;
    int blk_period;
    val_t values[MAX_LS_VALS];
    val_t scalar_values[MAX_LS_VALS];
} load_survey_data_value_t;

typedef struct
{
    uint8_t midx;
    char make[64];
    int num_vals;
    int num_scaler_vals;
    char loc_name[128];
    val_t values[MAX_MN_VALS];
    val_t scalar_values[MAX_MN_VALS];
} daily_profile_data_value_t;

typedef struct
{
    uint8_t midx;
    char make[64];
    char loc_name[128];
    uint8_t event_type;
    uint32_t num_event;

    int num_scaler_vals;
    val_t values[MAX_EVENT_VALS];
    val_t scalar_values[MAX_EVENT_VALS];
} events_data_value_t;




// polling configuration
typedef struct
{
    unsigned char poll_inst_data;
    unsigned char poll_ls_data;
    unsigned char poll_mn_data;
    unsigned char poll_bill_data;
    unsigned char poll_event_data;
    unsigned char enable_stored_data_poll;
    unsigned char enable_proc_diag;
    unsigned char num_old_days;
    uint16_t num_of_bill_month;
    char num_of_events[8];
    uint16_t num_of_mn_data;
    uint16_t num_blocks;
    int32_t inst_poll_int;
    int32_t ls_poll_int;
    int32_t mn_poll_int;
    int32_t bill_poll_int;
    int32_t event_poll_int;
    int32_t plcc_nw_refresh_int;
}poll_cfg_t;


typedef struct
{
    char initial_poll_starts[512];
    char store_poll_starts[512];
    char idle_poll_starts[512];
    char periodic_poll_starts[512];
}strategy_cfg_t;


typedef struct
{
	unsigned char day;
	unsigned char month;
	unsigned short year;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
} Date;

typedef struct
{
	int	      	pow_val;
    float 		mul_factor;
}param_det_t;

#define MAX_EVENT_OBIS 50
#define OBIS_CODE_LEN  32

typedef struct {
    char event_obis_codes[MAX_EVENT_OBIS][OBIS_CODE_LEN];
} event_obis_t;

//rithika 30oct2025
typedef struct{
    char obis_code[MAX_EVENT_VALS][32];
    int num_event_params;
}event_val_obis_t;

//rithika 3nov2025

typedef struct{
    char obis_code[MAX_INST_VALS][32];
    int num_val_params;
}inst_val_obis_t;

typedef struct{
    char obis_code[MAX_LS_VALS][32];
    int num_val_params;
}ls_val_obis_t;

typedef struct{
    char obis_code[MAX_MN_VALS][32];
    int num_val_params;
}daily_profile_val_obis_t;

typedef struct{
    char obis_code[MAX_BILLING_VALS][32];
    int num_val_params;
}billing_val_obis_t;
