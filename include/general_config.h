#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <sys/time.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <termios.h>
#include <pthread.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/errno.h>
#include <linux/serial.h>
#include <asm/ioctls.h>
#include <assert.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdbool.h>
#include "cJSON.h"
#include "../include/sqlite3.h"
#include "../include/hiredis.h"
#include "../include/nxjson.h"

#if RECONNECT 
#define MAX_NO_OF_METER_PER_PORT 5
#else
#define MAX_NO_OF_METER_PER_PORT 10
#endif

// Define ANSI escape codes for colors
#define RESET       "\033[0m"
#define BLACK       "\033[0;30m"
#define RED         "\033[0;31m"
#define GREEN       "\033[0;32m"
#define YELLOW      "\033[0;33m"
#define BLUE        "\033[0;34m"
#define MAGENTA     "\033[0;35m"
#define CYAN        "\033[0;36m"
#define WHITE       "\033[0;37m"

#define PRINT 1
#define IMX 1

#define REDIS_HOST_NAME "127.0.0.1"
#define REDIS_PORT       6379

#define DCU_DUAL_PORT_TYPE 1
#define DCU_FOUR_PORT_TYPE 0

#if IMX
#define PORT_1_FILE "/dev/ttymxc1"
#define PORT_2_FILE "/dev/ttymxc2"

#else
#define PORT_1_FILE "/dev/ttyS1"
#define PORT_2_FILE "/dev/ttyS4"
#endif

#define COMM_PORT0_DET ""
#define COMM_PORT1_DET ""
#define COMM_PORT2_DET ""
#define COMM_PORT3_DET ""
#define COMM_PORT4_DET ""
#define COMM_PORT5_DET ""

#if DCU_DUAL_PORT_TYPE
#define ATSAM91G 0
#define ATSAM91X 1
#define MAX_NO_OF_ETH_PORT 2
#define MAX_NO_OF_SERIAL_PORT 2
#undef COMM_PORT0_DET
#undef COMM_PORT1_DET
#define COMM_PORT0_DET PORT_1_FILE
#define COMM_PORT1_DET PORT_2_FILE
#endif

#if DCU_FOUR_PORT_TYPE
#define ATSAM91G 0
#define ATSAM91X 1
#define MAX_NO_OF_ETH_PORT 2
#define MAX_NO_OF_SERIAL_PORT 4
#undef COMM_PORT0_DET
#undef COMM_PORT1_DET
#undef COMM_PORT2_DET
#undef COMM_PORT3_DET
#define COMM_PORT0_DET "/dev/ttyUSB0"
#define COMM_PORT1_DET "/dev/ttyUSB1"
#define COMM_PORT2_DET "/dev/ttyUSB2"
#define COMM_PORT3_DET "/dev/ttyUSB3"
#endif

#define RET_OK 0

#define DEBUG 1

#define INFORM 0
#define WARNING 1
#define SEVERE 2
#define FATAL 3
#define REPORT 4

#define DCU_SERIAL_NUM "/srv/www/htdocs/info/serial.txt"
//rithika 
#define SD_LOC "/run/media/mmcblk0p1/"
#define OD_CHANGES 1

typedef struct
{
     unsigned char enable_port;
    unsigned char device_type[64];
    unsigned char port_name[64];
    unsigned char ser_portid;    // Serial port name
    uint16_t baud_rate; // Baudrate of serial port
    unsigned char data_bits;     // Num of databits
    unsigned char stop_bits;     // Num of stop bits
    unsigned char parity;    // Parity - ODD/EVEN/NONE
    unsigned char handshake[64]; // Handshake - SW/HW/NONE
    unsigned char inf_mode[64];  // RS232 / 485
} ser_port_cfg_t;

typedef struct
{
    unsigned char enable_meter;
    unsigned char num_retries;
    unsigned int resp_timeouts;
    unsigned char apply_ctpt_ratio;
    unsigned char ct_ratio;
    unsigned char pt_ratio;
    uint32_t meter_addr;
    unsigned char meter_addr_size;
    unsigned char auth_type[64];
    char sys_title[64];
    char cipher_key[64];
    char auth_key[64];
    char met_pw[64];  
    char meter_loc[64];
} meter_cfg_t;

typedef struct
{
    int chan_no;
    int num_dev;
    char meter_make[64];
    ser_port_cfg_t ser_port_cfg;
    meter_cfg_t meter_cfg[MAX_NO_OF_METER_PER_PORT];
} ser_port_channel_t;

// typedef struct
// {
// 	ser_port_channel_t				ser_port_channel[MAX_NO_OF_SERIAL_PORT];
// }dev_config_t;

typedef struct
{
    int num_ser_ports;
    int num_eth_ports;
    int num_ipsec_tunnels;
    int num_ftp_instances;
    char meter_inf_type[64];
} feature_cfg_t;

typedef struct
{
    char msg_type[64];
    char seq_no[20];
    char init_source[20];
    char meter[20];
    char start_date[20];
    char num_days[10];
    char time_stamp[30];
    char nw_size[8];
    char db_size[8];
    char event_type[16];
    char port_id[2];
    char apn_name[32];
    char apn_bearer[32];
    char commn_msg[2048];
    char resp_msg[2056];
} od_cmd_t;

typedef struct
{
    char make[32];
    double totalAverages[20]; // Total average timings for each operation
    int cycleCount;           // Cycle count for this make
    double currenttime;
    double currentAverages;
} meter_metrics_t;

typedef struct
{
    int send_qry_cnt;
    int recv_resp_cnt;
    int no_resp_cnt;
    int vaild_resp_cnt;
    int invaild_resp_cnt;
    char update_time[32];
} serial_port_diag_t;

typedef struct
{
    char msgType[64];
    char proc[64];
    char datatype[64];
    char eventmsg[64];
    char timestamp[64];
} events_data_t;

void print_colored_message(const char *color,int,const char *message, ...);
int snrm_aarq_failed_cnt(const char *data,...);
int32_t dbg_log(int mode, const char *p_format, ...);
double elapsed_ms(struct timeval start, struct timeval end);