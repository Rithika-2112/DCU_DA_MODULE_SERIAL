#define CAPTURE_PERIOD_OBIS "1.0.0.8.4.255"
#define DEMAND_INTERVAL_OBIS "1.0.0.8.0.255"
#define CLOCK_OBIS "0.0.1.0.0.255"
#define METERING_MODE_OBIS "0.0.94.96.19.255"
#define PAYMENT_MODE_OBIS "0.0.94.96.20.255"
#define LAST_TKN_RECHARGE_AMT_OBIS "0.0.94.96.21.255"
#define TOT_AMT_LST_RECHARGE_OBIS "0.0.94.96.23.255" 
#define CUR_BAL_AMT_OBIS "0.0.94.96.24.255"
#define MD_RESET_OBIS "0.0.10.0.1.255"
#define RELAY_OP_OVERLOAD_OVERCURR_OBIS "0.0.96.128.1.255"
#define RELAY_CONN_DISCONN_OBIS "0.0.96.3.10.255"
#define LOAD_LIMIT_OBIS "0.0.17.0.0.255"
#define ESWF_OBIS "0.0.94.91.26.255"
#define INSTANT_PUSH_OBIS "0.0.25.9.0.255"
#define APN_OBIS "0.0.155.1.1.255"
#define LAST_TKN_RECHARGE_TIME_OBIS "0.0.94.96.22.255"
#define CURR_BAL_TIME_OBIS "0.0.94.96.25.255"
#define ACT_CALND_OBIS "0.0.13.0.0.255"
#define SNG_ACT_SCH_BIL_OBIS "0.0.15.0.0.255"
#define ALERT_PUSH_OBIS "0.4.25.9.0.255"

#define CAPTURE_PERIOD_ATTR_INDEX 2
#define CLOCK_ATTR_INDEX 2
#define DEMAND_ATTR_INDEX 2
#define METERING_MODE_ATTR_INDEX 2
#define PAYMENT_MODE_ATTR_INDEX 2
#define LAST_TKN_RECHARGE_AMT_ATTR_IDX 2
#define TOT_AMT_LST_RECHARGE_ATTR_IDX 2
#define CUR_BAL_AMT_ATTR_IDX 2
#define MD_RESET_ATTR_IDX 2
#define RELAY_OP_OVERLOAD_OVERCURR_ATTR_IDX 2
#define RELAY_CONN_DISCONN_ATTR_IDX 2
#define LOAD_LIMIT_ATTR_IDX 4
#define ESWF_ATTR_IDX 2
#define ENBL_DISBL_LOAD_LIMIT_FN_ATTR_IDX 4
#define APN_ATTR_IDX 2
#define LAST_TKN_RECHARGE_TIME_ATTR_IDX 2
#define CURR_BAL_TIME_ATTR_IDX 2
#define ACT_CALND_ATTR_IDX 0
#define PAS_CALNAME_ATTR_INDX 6
#define PAS_SEASON_PROF_ATTR_INDX 7
#define PAS_WEEK_PROF_TBL_ATTR_INDX 8
#define PAS_DAY_PROF_TBL_ATTR_INDX 9
#define ACT_CAL_TIME_ATTR_INDX 10
#define SNG_ACT_SCH_BIL_ATTR_IDX 4
#define INSTANT_PUSH_ATTR_IDX 3
#define ALERT_PUSH_ATTR_IDX 3

#define MAX_SEASONS 2
#define MAX_WEEKS 2
#define MAX_DAYS 5
#define MAX_TIME_ZONES 8

typedef struct {
    char day_start_time[32];
    char script_table[64];
    int  script_selector;
}time_zones;

typedef struct{
    int day_id;
    int max_time_zones;
    time_zones timezones[MAX_TIME_ZONES];
}day;

typedef struct{
    char week_name[32];
    unsigned char weekday[7];
}week;

typedef struct {
    char season_name[32];
    char start_time[64];
    char week_name[32];
}season;

typedef struct{
    char calendar_name[32];
    int num_seasons;
    season season_profile[MAX_SEASONS];
    int num_weeks;
    week week_profile[MAX_WEEKS];
    int num_days;
    day day_profile[MAX_DAYS];

}activity_calendar;