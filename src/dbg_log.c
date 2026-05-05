/*START OF FILE*/
#include "../include/general_config.h"
#include "../include/dlms_config.h"
/*** Local Macros ***/
#define FILE_SIZE_EXCEED 2 * 1024 * 1024 // 1 MB
extern uint8_t g_pidx;

#define DEBUG_LOG_LEVEL "/usr/cms/config/debuglog.cfg"

// priya
#define CMS_RMS_FLAG 1
#define DLMS_MASTER_PROC 0x12

// rithika 06Sep
//  #define LOG_FILE_NAME "/usr/cms/log/dcu_da_ser_module.log"
//  #define LOG_FILENAME_BK "/usr/cms/log/dcu_da_ser_module_1.log"

typedef struct
{
    int procId;
    char dcuSer[16];
    char msg[256];
} CMS_RMS_DBG_LOG_MSG;
CMS_RMS_DBG_LOG_MSG cms_rms_log_msg;

extern int cms_rms_log_level;
extern char serial_number[16];
/*** Externs ***/
// extern device_config_modem_t		dev_cfg_modem;

/*** Globals ***/
static char *p_dbg_msgs[5] = {"[Info]", "[Warning]", "[Severe]", "[Fatal]", "[Report]"};

// rithika 08oct2025
//  static char g_msg_str[32768], g_dbg_buff[32768], g_file_path[64];
// rithika 13nov2025
//  static char g_msg_str[1024], g_dbg_buff[1024], g_file_path[64];
static char g_msg_str[1024], g_dbg_buff[1024], g_file_path[64];
// 5120
char dbg_buf_2[1024];

static FILE *p_dbg_fptr_arr = NULL;

/* =========================================================================== */

/*************************************
 *Function 		: dbg_log()
 *Params 		: mode ,data
 *Return			: int
 *Description 	: forwriting log
 **************************************/

// Globals
static char LOG_FILE_NAME[128];
static char LOG_FILENAME_BK[128];

void initialize_log_filenames()
{
    if (g_pidx == 0)
    {
        strcpy(LOG_FILE_NAME, "/usr/cms/log/dcu_da_serport_0.log");
        strcpy(LOG_FILENAME_BK, "/usr/cms/log/dcu_da_serport_0_bkp.log");

        // for plcc
        // strcpy(LOG_FILE_NAME, "/usr/cms/log/dcu_damodule.log");
        // strcpy(LOG_FILENAME_BK, "/usr/cms/log/dcu_damodule_1.log");
    }
    else if (g_pidx == 1)
    {
        strcpy(LOG_FILE_NAME, "/usr/cms/log/dcu_da_serport_1.log");
        strcpy(LOG_FILENAME_BK, "/usr/cms/log/dcu_da_serport_1_bkp.log");

        // for plcc
        // strcpy(LOG_FILE_NAME, "/usr/cms/log/dcu_damodule.log");
        // strcpy(LOG_FILENAME_BK, "/usr/cms/log/dcu_damodule_1.log");
    }
}

// int32_t dbg_log(uint8_t mode, const char *p_format, ...)
// {
//     va_list arg;
//     int done = 0;
//     memset(g_dbg_buff, 0, sizeof(g_dbg_buff));

//     va_start(arg, p_format);
//     done = vsnprintf(g_dbg_buff, sizeof(g_dbg_buff), p_format, arg);
//     va_end(arg);

//     time_t curr_time_sec = time(NULL);
//     struct tm *p_curr_time_tm = localtime(&curr_time_sec);
//     char time_str[64] = {0};
//     strftime(time_str, sizeof(time_str), "%d_%b_%Y %H_%M_%S", p_curr_time_tm);

//     printf("#######\n%s : %s : %s\n", time_str, p_dbg_msgs[mode], g_dbg_buff);

//     snprintf(g_msg_str, sizeof(g_msg_str), "%s : %s : %s", time_str, p_dbg_msgs[mode], g_dbg_buff);

//     add_process_diag(g_msg_str);
//     write_dbglog(g_msg_str);

//     return done;
// }

// int pprintf(const char *format, ...)
// {
//     va_list arg;
//     int done;
//     char dbg_buf_2[2048];

//     va_start(arg, format);
//     done = vsnprintf(dbg_buf_2, sizeof(dbg_buf_2), format, arg);
//     va_end(arg);

//     printf("++++++ %s\n", dbg_buf_2);

//     // Pass as literal string — not a format string
//     dbg_log(INFORM, "%s", dbg_buf_2);

//     return done;
// }

void iec104_log_sink_poll_network(void)
{
    dcu_netlog_poll();
}

static dcu_netlog_level_t netlog_level_of(int lvl)
{
    switch (lvl)
    {

    case 5:
        return DCU_NETLOG_DEBUG;
    case 0:
        return DCU_NETLOG_INFO;
    case 1:
        return DCU_NETLOG_WARN;
    case 4:
        return DCU_NETLOG_ERROR;
    }
    return DCU_NETLOG_INFO;
}

// int32_t dbg_log(uint8_t mode, const char *p_format, ...)
int32_t dbg_log(int mode, const char *p_format, ...)
{
    // printf("________________________________ ENTER ____________________________________\n");
    uint32_t done = 0;
    va_list arg;

    // memset(g_dbg_buff, 0, 32768);
    memset(g_dbg_buff, 0, 1024);
    va_start(arg, p_format);
    // done = vsprintf((char *)g_dbg_buff, (const char *)p_format, arg);

    // done = vsprintf(g_dbg_buff, p_format, arg);
    done = vsnprintf(g_dbg_buff, sizeof(g_dbg_buff), p_format, arg);

    va_end(arg);

    time_t curr_time_sec = 0;
    struct tm *p_curr_time_tm;
    char time_str[64] = {0};

    /* time_t  T= time(NULL);

    struct  tm tm = *localtime(&T); */

    // printf("System Date is: %02d/%02d/%04d\n",tm.tm_mday, tm.tm_mon+1, tm.tm_year+1900);
    // printf("System Time is: %02d:%02d:%02d\n",tm.tm_hour, tm.tm_min, tm.tm_sec);

    curr_time_sec = time(NULL);
    p_curr_time_tm = localtime(&curr_time_sec);
    // strftime(time_str, 64, "%d_%b_%Y %H_%M_%S", p_curr_time_tm);
    strftime(time_str, 64, "[%Y-%m-%d %H:%M:%S]", p_curr_time_tm);

    // printf("+++++++++++++++++ timestr %s\n",time_str);
    // memset(g_msg_str, 0, 32768);
    memset(g_msg_str, 0, 1024);
    // strcpy(g_msg_str, time_str);
    // strcat(g_msg_str, " [");
    // strcat(g_msg_str, p_dbg_msgs[mode]);
    // // strcat(g_msg_str, "]:[Port_%d]:"); // rithika 10nov
    // strcat(g_msg_str, g_dbg_buff);

    snprintf(g_msg_str, sizeof(g_msg_str), "%s %s:[Port_%d]: %s", time_str, p_dbg_msgs[mode], g_pidx, g_dbg_buff);

    /* if(g_msg_str[strlen(g_msg_str)-1] == '\n')
    {
        g_msg_str[strlen(g_msg_str)]='\0';
    }
    else
    {
        g_msg_str[strlen(g_msg_str)]='\n';
        g_msg_str[strlen(g_msg_str)+1]='\0';
    } */
    printf("%s", g_msg_str);
    // len = strlen(g_msg_str);

    add_process_diag(g_msg_str);
    write_dbglog(g_msg_str);

    // rithika 14April2026
    snprintf(g_msg_str, sizeof(g_msg_str), "[Port_%d]: %s", g_pidx, g_dbg_buff);

    if (RECONNECT)
    {
        dcu_netlog_send(netlog_level_of(mode), "%s", g_msg_str);
    }

    // va_end(arg);
    // printf("________________________________ EXIT ____________________________________\n");
    return done;
}

int pprintf(const char *format, ...)
{
    // Get Arg list and convert to string
    va_list arg;
    volatile unsigned int done;
    memset(dbg_buf_2, 0, 1024);
    va_start(arg, format);

    // Format the message into dbg_buf_2 using the provided format and arguments
    done = vsprintf(dbg_buf_2, format, arg);
    va_end(arg);
    printf("++++++ %s \n", dbg_buf_2);
    dbg_log(INFORM, dbg_buf_2);
    return done;
}

/* int file_status()
{
    int fd;
    fd=open((const INT8 *)LOG_FILE_NAME,O_RDONLY,MODE);
    if(fd==0)
        return -1;
    else
        return 1;

} */
/*************************************
 *Function 		: write_dbglog()
 *Params 		: data
 *Return			: int
 *Description 	: to write debug log
 **************************************/

int32_t write_dbglog(char *p_data)
{
    uint64_t position = 0;

    memset(g_file_path, 0, 64);

#if 0
	checkRmsLogStatus();

	p_data[255] = '\0';
	// printf("p_data :%s\n",p_data);
	cms_rms_log_msg.procId = DLMS_MASTER_PROC;
	memset(	cms_rms_log_msg.dcuSer,0,sizeof(cms_rms_log_msg.dcuSer));
	memcpy(cms_rms_log_msg.dcuSer,serial_number,16);
	// printf("cms_rms_log_msg.dcuSer : %s\n",cms_rms_log_msg.dcuSer);
	// printf("serial_number :%s\n",serial_number);
	memset(	cms_rms_log_msg.msg,0,sizeof(cms_rms_log_msg.msg));
	memcpy(cms_rms_log_msg.msg,p_data,256);

	// printf("cms_rms_log_msg.msg :%s\n",cms_rms_log_msg.msg);

	if ( cms_rms_log_level >= INFORM )
	{
		sendCmsRmsDbgLogMsg((char *)&cms_rms_log_msg,sizeof(CMS_RMS_DBG_LOG_MSG));
	}
	else if(cms_rms_log_level >=REPORT)
	{
		sendCmsRmsDbgLogMsg((char *)&cms_rms_log_msg,sizeof(CMS_RMS_DBG_LOG_MSG));
	}
	else if(cms_rms_log_level >=WARNING) 
	{
		sendCmsRmsDbgLogMsg((char *)&cms_rms_log_msg,sizeof(CMS_RMS_DBG_LOG_MSG));
	}	
	else if (cms_rms_log_level >=SEVERE) 
	{
		sendCmsRmsDbgLogMsg((char *)&cms_rms_log_msg,sizeof(CMS_RMS_DBG_LOG_MSG));
	}		
	else if (cms_rms_log_level >=FATAL) 
	{
		sendCmsRmsDbgLogMsg((char *)&cms_rms_log_msg,sizeof(CMS_RMS_DBG_LOG_MSG));
	}

#endif
    FILE *fp_log;
    int debug_log_level = 0;

    fp_log = fopen(DEBUG_LOG_LEVEL, "r");
    if (fp_log == NULL)
    {
        return 0;
    }
    fscanf(fp_log, "%d", &debug_log_level);
    fclose(fp_log);
    // printf("web_cgi_flag %d\n", debug_log_level);

    // int debug_log_level = 1;
    if (debug_log_level == 0)
    {
        return RET_OK;
    }

    if (p_dbg_fptr_arr == NULL)
    {
        p_dbg_fptr_arr = fopen(LOG_FILE_NAME, "a");
        if (p_dbg_fptr_arr == NULL)
        {
            printf("open call failed for file : %s , Error : %s\n", LOG_FILE_NAME, strerror(errno));
            return -1;
        }
    }

    // p_data[19246] = '\0';
    fprintf(p_dbg_fptr_arr, "%s", p_data);
    fflush(p_dbg_fptr_arr);

    position = ftell(p_dbg_fptr_arr);
    if (position >= FILE_SIZE_EXCEED)
    {
        if (g_pidx == 0)
        {
            system("mv /usr/cms/log/dcu_da_serport_0.log /usr/cms/log/dcu_da_serport_0_bkp.log");

            // for plcc
            //  system("mv /usr/cms/log/dcu_damodule.log /usr/cms/log/dcu_damodule_1.log");
        }
        else if (g_pidx == 1)
        {

            system("mv /usr/cms/log/dcu_da_serport_1.log /usr/cms/log/dcu_da_serport_1_bkp.log");

            // for plcc
            // system("mv /usr/cms/log/dcu_damodule.log /usr/cms/log/dcu_damodule_1.log");
        }

        fclose(p_dbg_fptr_arr);
        p_dbg_fptr_arr = NULL;
        p_dbg_fptr_arr = fopen(LOG_FILE_NAME, "w+");
        if (p_dbg_fptr_arr == NULL)
        {
            printf("open call failed for file : %s , Error : %s\n", LOG_FILE_NAME, strerror(errno));
            return -1;
        }
    }

    return RET_OK;
}

/* End OF FILE */

// rithika 13nov2025
void print_data(uint8_t midx, uint8_t *msg, int32_t len)

{
    uint32_t idx = 0, frame_cnt = 1;

    uint8_t frame_len = 32;

    char loc_buff[128], temp_buff[32];

    memset(loc_buff, 0, sizeof(loc_buff));

    if (len >= 1)

    {
        dbg_log(INFORM, "[Meter_%d]:[%-25s] : [Number of bytes : %d] \n", midx, "readData", len);

        for (idx = 0; idx < len; idx++)

        {

            memset(temp_buff, 0, sizeof(temp_buff));

            sprintf(temp_buff, "%02x ", msg[idx] & 0xff);

            strcat(loc_buff, temp_buff);

            if ((frame_cnt * frame_len - 1 == idx) && (idx > 0))

            {

                dbg_log(INFORM, "[Meter_%d]:[%-25s] : Rx : %s\n", midx, "readData", loc_buff);
                frame_cnt++;

                memset(loc_buff, 0, sizeof(loc_buff));
            }
        }

        dbg_log(INFORM, "[Meter_%d]:[%-25s] : Rx : %s\n", midx, "readData", loc_buff);
    }
}
