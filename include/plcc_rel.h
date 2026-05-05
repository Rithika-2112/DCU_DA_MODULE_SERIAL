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



#define PLCC_START_CHAR 0xCA
#define PLCC_START_CHAR_SR 0xCC
// Protocol Version
#define PROTOCOL_VERSION 0x00

// Packet Type
#define PACKET_TYPE_REQUEST 0x00
#define PACKET_TYPE_RESPONSE 0x01
#define PACKET_TYPE_INDICATION 0x02

// Service Type
#define SERVICE_TYPE_EMBEDDED_SERVICES 0x00
#define SERVICE_TYPE_STACK_SERVICES 0x01
#define SERVICE_TYPE_CONF_AND_MON 0x02
#define SERVICE_TYPE_DATA_SERVICES 0x03
#define SERVICE_TYPE_MGMT_SERVICES 0x05

// PLCC Commands - OP Codes

// Soft Reset
#define OPCODE_NOP 0x00
#define OPCODE_RESET 0x20
#define OPCODE_DB_ENTRY 0x65
#define OPCODE_NODE_INFO 0x69

// Config and Monitoring Commands
#define OPCODE_SET_PREDEF_PARAMS 0x40
#define OPCODE_SET_DEVICE_PARAMS 0x41
#define OPCODE_GET_DEVICE_PARAMS 0x42
#define OPCODE_SAVE_DEVICE_PARAMS 0x43
#define OPCODE_REMOTE_PARAMS_CHANGED 0x4C

// Data Commands
#define OPCODE_TX_PKT 0x60
#define OPCODE_GET_NC_DB_SIZE 0x65
#define OPCODE_RX_PKT 0x68
#define OPCODE_GET_NODE_INFO 0x69
#define OPCODE_DELETE_NODE_INFO 0x6A

// NL Management Commands
#define OPCODE_ADMISSION_APPROVAL_REQ_RESP 0xA4
#define OPCODE_LEAVE_NETWORK_REQ 0xA6
#define OPCODE_CONNECTIVITY_STATUS_IND 0xB1
#define OPCODE_NODE_LEFT_NETWORK_IND 0xB3
#define OPCODE_GET_ADMISSION_APPROVAL_IND 0xB8
#define OPCODE_ADMISSION_REFUSE_IND 0xB9
#define OPCODE_CONNECTED_TO_NC_IND 0xBA
#define OPCODE_DISCONNECTED_FROM_NC_IND 0xBE
#define OPCODE_NETWORK_ID_ASSIGNED_IND 0xBF

//  Command Execution status
#define CMDEXE_STATUS_SUCCESS 0x01
#define CMDEXE_STATUS_FAILURE 0x00

#define PLCC_FRAME 1

#define MAX_RETRY 10

#define RS_NW_CHECK_PERIOD 5 * 60
#define DELETE_RS_PERIOD RS_NW_CHECK_PERIOD * 2

#define NC_SN   0x55
#define NC_SIZE 0x56
#define DB_SIZE 0x57

// #define MAX_NO_OF_METER_PER_PORT 50 
#define MAX_NO_OF_METER_PER_PORT 10 // rithika changed for tcs dcu

typedef struct
{
    uint8_t plcc_ser[8];
    uint8_t dcu_meter_idx;
    uint8_t rs_status;
    time_t rs_conn_fail_time;
    char meter_ser_num[32];
    char meter_manf_name[64];
    uint8_t node_id;
    uint8_t parent_id;
    int signal_quality;
} plcc_meter_map_t;

typedef struct
{
    int no_of_nodes;
    uint8_t nc_sn[16];
    int nw_size;
    int db_size;
    uint8_t nc_id;
    char last_update_time[64];
    plcc_meter_map_t plcc_meter_map[MAX_NO_OF_METER_PER_PORT];
} plcc_modem_info_t;

plcc_modem_info_t plcc_modem_info;
int plcc_response_parsing(char *msg, int len);