#include "../include/plcc_rel.h"
#include "../include/dlms_config.h"
#include "../include/general_config.h"

#define WRAPPER_START_PATTERN_SIZE 6

static tcflag_t g_baudrate, g_databits, g_stopbits, g_parity_on, g_parity;
static tcflag_t baudrate[16] = {0, B50, B75, B110, B200, B300, B600, B1200, B2400, B4800, B9600, B19200, B38400, B57600, B115200};

extern poll_cfg_t poll_cfg;

int32_t g_serial_fd;
int32_t ser_read_ret = -1;

int RS485_ENABLE_FLAG = 0;
int global_check_type = -1;
extern int curr_nc_idx;

uint8_t cksum = 0;
uint8_t g_recv_buff[2048];
uint8_t cmd_data[4096];

unsigned char frame[4096];

char loc_buff[2048];
char event_msg_str[128];

uint8_t soft_reset_resp[23] = {0xcc, 0x03, 0x00, 0x01, 0x04, 0x01, 0x09, 0xca, 0x03, 0x00, 0x01, 0x20, 0x40, 0x64, 0xca, 0x04, 0x00, 0x02, 0xbf};

const uint8_t WRAPPER_START_PATTERN_1[] = {0x00, 0x01, 0x00, 0x01, 0x00, 0x10};
const uint8_t WRAPPER_START_PATTERN_2[] = {0x00, 0x01, 0x00, 0x01, 0x00, 0x30};

extern connection con;
extern char p_comm_port_det[16];
extern time_t last_rs_check_time;
extern uint8_t g_pidx;

events_data_t event_data;

uint8_t nc_soft_reset(int type);

uint16_t bytes_to_decimal(const uint8_t *bytes)
{
    // Convert from big-endian (most significant byte first)
    return (bytes[0] << 8) | bytes[1];
}

uint16_t bytes_to_decimal_le(const uint8_t *bytes)
{
    // Convert from little-endian (least significant byte first)
    return (bytes[1] << 8) | bytes[0];
}

int check_nc_nw(int check_type)
{
    static char fun_name[] = "check_nc_nw()";

    time_t curr_time = 0;

    curr_time = time(NULL);

    if ((curr_time - last_rs_check_time) > (poll_cfg.plcc_nw_refresh_int))
    {
        com_close(&con);

        init_plcc_modem(check_type);
        last_rs_check_time = time(NULL);

        con.comPort = -1;
        
        com_open(&con, p_comm_port_det);
        discover_meter_manf();
    }

    return 0;
}

int plcc_response_parsing(char *msg, int len)
{
    static char fun_name[] = "plcc_response_parsing()";

    memset(event_msg_str, 0, sizeof(event_msg_str));

    if (msg[0] != 0xCA)
    {
        dbg_log(REPORT, "[%-25s] :NC ACK RECEIVE FAILED\n", fun_name);
        sprintf(event_data.eventmsg, "%d:%d NC ACK RECEIVE FAILED",
                g_pidx, curr_nc_idx);

        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));

        add_event_data(&event_data);
        return -1;
    }
    else if (msg[11] != 0xca)
    {
        dbg_log(REPORT, "[%-25s] :NC FAILED TO SEND THE QUERY TO RS\n", fun_name);
        sprintf(event_data.eventmsg, "%d:%d NC FAILED TO SEND THE QUERY TO RS",
                g_pidx, curr_nc_idx);

        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));

        add_event_data(&event_data);
        return -1;
    }
    else if (msg[24] != 0xca)
    {
        dbg_log(REPORT, "[%-25s] :NC DIDN'T GOT THE ACK FROM THE RS\n", fun_name);
        sprintf(event_data.eventmsg, "%d:%d NC DIDN'T GOT THE ACK FROM THE RS",
                g_pidx, curr_nc_idx);

        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));

        add_event_data(&event_data);
        return -1;
    }
    else if (msg[65] != 0x7E)
    {
        dbg_log(REPORT, "[%-25s] :RS FAILED TO GET THE METER RESPONSE\n", fun_name);
        sprintf(event_data.eventmsg, "%d:%d RS FAILED TO GET THE METER RESPONSE",
                g_pidx, curr_nc_idx);

        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));

        add_event_data(&event_data);
        return -1;
    }

    return 0;
}

int32_t init_serial(int32_t serial_fd, char *port_name, uint32_t recv_baudrate, uint8_t recv_databits, uint8_t recv_parity, uint8_t recv_stopbits)
{

    static char fun_name[] = "init_serial()";
#if DEBUG
    printf("%s, %u, %d %d \n", port_name, recv_baudrate, recv_databits, recv_stopbits);
#endif

    struct termios set_termios;

    memset((void *)&set_termios, 0, sizeof(struct termios));

    g_baudrate = baudrate[recv_baudrate];

    g_parity_on = 0;

    switch (recv_databits)
    {
    case 5:
        g_databits = CS5;
        break;

    case 6:
        g_databits = CS6;
        break;

    case 7:
        g_databits = CS7;
        break;

    case 8:
    default:
        g_databits = CS8;
        break;
    }

    switch (recv_parity)
    {
    case 1:
        g_parity_on = PARENB;
        g_parity = PARODD;
        break;

    case 2:
        g_parity_on = PARENB;
        g_parity = 0;
        break;

    case 0:
    default:
        g_parity_on = 0;
        g_parity = 0;
        break;
    }

    switch (recv_stopbits)
    {
    case 1:
    default:
        g_stopbits = 0;
        break;

    case 2:
        g_stopbits = CSTOPB;
        break;
    }

    if ((serial_fd = open(port_name, O_RDWR | O_NOCTTY)) == -1)
    {
        dbg_log(REPORT, "[%-25s] :Unable to open Port : %s\n", fun_name, port_name);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(event_data.eventmsg, "Unable to open Port : %s", port_name);
        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&event_data);

        return -1;
    }

    dbg_log(INFORM, "[%-25s] :Port fd %d\n", fun_name, serial_fd);

    if (strstr(port_name, "ttymxc"))
    {
        RS485_ENABLE_FLAG = 1;
    }
    else
    {
        RS485_ENABLE_FLAG = 0;
    }

#if IMX
    if (RS485_ENABLE_FLAG == 1)
        rs485_enable(serial_fd, 1);
#endif

    set_termios.c_cflag = g_baudrate | g_databits | g_stopbits | g_parity | g_parity_on | CLOCAL | CREAD;

    if (g_parity_on > 0)
        set_termios.c_iflag = (tcflag_t)(INPCK);
    else
        set_termios.c_iflag = IGNPAR;

    set_termios.c_oflag = 0;
    set_termios.c_lflag = 0;

    (void)tcflush(serial_fd, TCIFLUSH);
    if (tcsetattr(serial_fd, TCSANOW, &set_termios) == -1)
    {
        dbg_log(REPORT, "[%-25s] :Unable to set attribute, error : %s\n", fun_name, strerror(errno));
        return -1;
    }

    return serial_fd;
}

int32_t write_ser(int32_t serial_fd, uint8_t *msg, int32_t len)
{

#if 0
    printf("init write_ser_port success with fd : %d\n", serial_fd);
#endif

#if IMX
    if (RS485_ENABLE_FLAG == 1)
        rs485_enable(serial_fd, 1);
#endif

    if (write(serial_fd, msg, len) < 0)
    {
        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(event_data.eventmsg, "PLCC WRITE FAILED.");
        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&event_data);
        return -1;
    }

    return 0;
}

int32_t read_ser_port(int32_t serial_fd, uint8_t *trav, uint8_t time_out, uint8_t midx)
{
    int32_t tot_byte_read = 0, byte_present = 0, loc_byte_read = 0;
    int32_t wait_cnt = 0, max_wait_cnt = 0, break_cnt = 0;

    max_wait_cnt = (time_out * 1000) / 50;

    while (wait_cnt < max_wait_cnt)
    {
        if (ioctl(serial_fd, FIONREAD, &byte_present) == -1)
        {
            printf("read_ser_port ::: ioctl Failed : Error : %s\n", strerror(errno));

            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(event_data.eventmsg, "PLCC read_ser_port ::: ioctl Failed : Error : %s", strerror(errno));
            strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&event_data);

            return -1;
        }

        if (byte_present > 0)
            break;

        else if (byte_present == 0)
        {
            usleep(50000);
            wait_cnt++;
        }
    }

    if (wait_cnt == max_wait_cnt)
    {
        printf("Reched Max WaitCount : %d\n", max_wait_cnt);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(event_data.eventmsg, "PLCC Reched Max WaitCount : %d", max_wait_cnt);
        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&event_data);

        return -1;
    }

    // printf("Start Recv Data at  WaitCount : %d\n", wait_cnt);

    while (1)
    {
        memset(loc_buff, 0, sizeof(loc_buff));
        if ((loc_byte_read = read(serial_fd, loc_buff, byte_present)) == -1)
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(event_data.eventmsg, "PLCC Failed to read serial");
            strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&event_data);

            return -1;
        }

        memcpy((void *)&trav[tot_byte_read], loc_buff, loc_byte_read);
        tot_byte_read += loc_byte_read;

        byte_present = 0;
        if (ioctl(serial_fd, FIONREAD, &byte_present) == -1)
        {
            memset(event_msg_str, 0, sizeof(event_msg_str));
            sprintf(event_data.eventmsg, "PLCC 2 read_ser_port ::: ioctl Failed : Error : %s", strerror(errno));
            strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
            add_event_data(&event_data);

            return -1;
        }

        if (byte_present > 0)
        {
            if (tot_byte_read == 1024)
                break;
            else
                continue;
        }
        else if (byte_present == 0)
        {
            usleep(50000);
            break_cnt = 1;

            while (wait_cnt < max_wait_cnt)
            {
                if (tot_byte_read >= 2)

                    byte_present = 0;
                if (ioctl(serial_fd, FIONREAD, &byte_present) == -1)
                {
                    memset(event_msg_str, 0, sizeof(event_msg_str));
                    sprintf(event_data.eventmsg, "PLCC 3 read_ser_port ::: ioctl Failed : Error : %s", strerror(errno));
                    strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
                    add_event_data(&event_data);
                    return -1;
                }

                if (byte_present > 0)
                {
                    // printf("Inside Inner while Some extra bytes aval to read. Breaking inner loop\n");
                    wait_cnt++;
                    break;
                }
                else
                {
                    usleep(50000);
                    wait_cnt++;
                    break_cnt++;
                    continue;
                }
            }

            if (tot_byte_read >= 2)

                if (wait_cnt >= max_wait_cnt)
                {
                    // printf("Main while loop Reached Max Wait Cnt, breaking loop\n");
                    break;
                }
        }
    }

    return tot_byte_read;
}

int read_resp(int read_fd, uint8_t *resp_buf, int timeout, uint8_t *len_of_resp)
{
    static char fun_name[] = "read_resp()";
    int count = 0, count1 = 0;
    int temp = 0;
    uint8_t max = 0;
    uint16_t read_ret = 0;
    char write_port_buffer[1024];
    int offset = 0;

    for (count = 0; count < 1024; count++)
    {
        resp_buf[count] = 0;
    }

#if DEBUG
    printf("resp_buf : %u\n", *resp_buf);
#endif

    while (count < 5 * 100)
    {

        if ((ioctl(read_fd, FIONREAD, &temp)) == -1)
        {
            return -1;
        }
        if (temp == 0)
        {
            usleep(10000);
            count++;
            continue;
        }
        break;
    }
    if (count >= 5 * 100)
    {
        return -1;
    }

    while (1)
    {

        read_ret = read(read_fd, resp_buf, 1024);
        printf("-------------------8-----------------\n");
        if (read_ret == -1)
        {
            dbg_log(REPORT, "[%-25s] :Failed to read from serial port\n", fun_name);
            return -1;
        }
        printf("-------------------7-----------------\n");
        max += read_ret;
        if (read_ret < 0)
        {
            dbg_log(REPORT, "[%-25s] :Serial read port failed\n", fun_name);
            return -1;
        }
        printf("-----------------6-------------------\n");
        if ((ioctl(read_fd, FIONREAD, &read_ret)) == -1)
        {
            return -1;
        }
        printf("--------------------5----------------\n");
        if (read_ret > 0)
        {

            if (max >= 256)
            {
                break;
            }
            else
                continue;
        }
        else if (read_ret == 0)
        {
            usleep(10000);
            if ((ioctl(read_fd, FIONREAD, &read_ret)) == -1)
            {
                return -1;
            }
            if (read_ret > 0)
            {
                continue;
            }
            else
            {
                break;
            }
        }
    }
    printf("---------------4---------------------\n");
    *len_of_resp = max;
    printf("------------------3------------------\n");
    for (count1 = 0; count1 < max; count1++)
    {
        offset += snprintf(write_port_buffer + offset, 256 - offset, "%02x\t", resp_buf[count1]);
    }

#if DEBUG
    printf("RESPONSE RECEIVED : %s \n\n", write_port_buffer);
#endif

    return 0;
}

int32_t close_port(int32_t serial_fd)
{
    close(serial_fd);
    return 0;
}

int validate_plcc_resp(uint8_t midx, uint8_t *msg, int32_t len, int type)
{
    static char fun_name[] = "validate_plcc_resp()";
    int idx = 0;

    if (type == OPCODE_RESET)
    {
        if (len > 22)
        {
            dbg_log(REPORT, "[%-25s] :len : %d\n", fun_name, len);
            return -11;
        }
        for (idx = 0; idx < 19; idx++)
        {
            // dbg_log(REPORT, "[%-25s] :msg[idx] : %02x   soft_reset_resp[idx] : %02x\n", fun_name, msg[idx], soft_reset_resp[idx]);
            if (msg[idx] != soft_reset_resp[idx])
            {
                dbg_log(REPORT, "[%-25s] :msg[idx] : %02x   soft_reset_resp[idx] : %02x\n", fun_name, msg[idx], soft_reset_resp[idx]);
                return -12;
            }
        }
        return 0;
    }

    if (msg[0] != PLCC_START_CHAR)
    {
        dbg_log(REPORT, "[%-25s] :Invalid Start Byte\n", fun_name);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(event_data.eventmsg, "PLCC Invalid Start Byte");
        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&event_data);

        return -13;
    }
    else if (msg[3] != PACKET_TYPE_RESPONSE)
    {
        dbg_log(REPORT, "[%-25s] :Invalid Packet Type - Received\n", fun_name);

        memset(event_msg_str, 0, sizeof(event_msg_str));
        sprintf(event_data.eventmsg, "PLCC Invalid Packet Type - Received");
        strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
        add_event_data(&event_data);

        return -14;
    }

    if (type == OPCODE_NODE_INFO)
    {
        for (idx = 10; idx <= 17; idx++)
        {
            if (msg[idx] != 0x00)
            {
                return 0;
            }
        }
        return -15;
    }

    return 0;
}

int32_t proc_resp(uint8_t midx, uint8_t *msg, int32_t len, int type)
{
    static char fun_name[] = "proc_resp()";
    int32_t fun_ret = RET_OK;
    uint8_t temp_msg[2];
    char temp_buff[255];
    int i;

#if 0
    printf("proc_resp.. : %d\n", len);

    printf("Response : ");
    for (i = 0; i < len; i++)
    {
        printf("%02X ", msg[i]);
    }
    printf("\n");
#endif

    fun_ret = validate_plcc_resp(midx, msg, len, type);
    if (fun_ret < 0)
    {
        dbg_log(REPORT, "[%-25s] :validate resp failed : %d\n", fun_name, fun_ret);
        return fun_ret;
    }

    // printf("++++++++++++++++ TYpe : %d ++++++++++++++++++\n", type);

    switch (type)
    {
    case OPCODE_DB_ENTRY:
        plcc_modem_info.no_of_nodes = msg[9] << 8 | msg[8];
        break;

    case OPCODE_NODE_INFO:
        get_nodes_ser_num(midx, msg, len);
        break;

    case OPCODE_GET_DEVICE_PARAMS:
        memcpy(temp_msg, &msg[6], 2 * sizeof(msg[0]));
        plcc_modem_info.nc_id = bytes_to_decimal_le(temp_msg);
        dbg_log(REPORT, "[%-25s] :NODE ID : %02x\n", fun_name, plcc_modem_info.nc_id);

        break;

    case NC_SN:
        memcpy(plcc_modem_info.nc_sn, &msg[6], 16 * sizeof(msg[0]));

        memset(temp_buff, 0, sizeof(temp_buff));
        memcpy(temp_buff, &plcc_modem_info.nc_sn, 15);

        memset(temp_buff, 0, sizeof(temp_buff));

        for (i = 0; i < 16; i++)
        {
            snprintf(temp_buff + 2 * i, sizeof(temp_buff) - 2 * i, "%02x", plcc_modem_info.nc_sn[i]);
        }

        dbg_log(REPORT, "[%-25s] :NODE SN : %s\n", fun_name, temp_buff);
        break;

    case NC_SIZE:
        plcc_modem_info.nw_size = (msg[7] << 8) | msg[6];
        dbg_log(REPORT, "[%-25s] :NW_SIZE : %02x\n", fun_name, plcc_modem_info.nw_size);

        break;

    case DB_SIZE:
        plcc_modem_info.db_size = (msg[7] << 8) | msg[6];
        dbg_log(REPORT, "[%-25s] :DB_SIZE : %02x\n", fun_name, plcc_modem_info.db_size);
        break;

    default:
        break;
    }

    return 0;
}

void print_data_plcc(uint8_t midx, uint8_t *msg, int32_t len, int type)
{
    static char fun_name[] = "print_data_plcc()";
    uint32_t idx = 0, frame_cnt = 1;
    uint8_t frame_len = 32;
    char temp_buff[32];

    memset(loc_buff, 0, sizeof(loc_buff));

    if (len >= 1)
    {
        for (idx = 0; idx < len; idx++)
        {
            memset(temp_buff, 0, sizeof(temp_buff));
            sprintf(temp_buff, "%02x ", msg[idx] & 0xff);
            strcat(loc_buff, temp_buff);

            if ((frame_cnt * frame_len - 1 == idx) && (idx > 0))
            {
                // dbg_log(INFORM, "[%-25s] :%s\n", fun_name, loc_buff);
                frame_cnt++;
                memset(loc_buff, 0, sizeof(loc_buff));
            }
        }
        if (type == 0)
        {
            dbg_log(INFORM, "[%-25s] :QUERY              : %s\n", fun_name, loc_buff);
        }
        else
        {
            dbg_log(INFORM, "[%-25s] :RESPONSE[%d bytes] : %s\n", fun_name, len, loc_buff);
        }
    }
}

int32_t test_plcc_comm(uint8_t midx, uint8_t *msg, int32_t len, int type)
{
    static char fun_name[] = "send_msg_meter()";
    uint8_t retry = 0;
    int ret = 0;

#if 0
    printf("Query: ");
    for (i = 0; i < len; i++)
    {
        printf("%02X ", msg[i]);
    }
    printf("\n");
#endif

    for (retry = 0; retry < 3; retry++)
    {
        if (write_ser(g_serial_fd, msg, len) < 0)
        {
            dbg_log(REPORT, "[%-25s] :Write serial failed, Retry Val : %d\n", fun_name, retry + 1);
            continue;
        }
        else
        {
            print_data_plcc(midx, msg, len, 0);

            memset(g_recv_buff, 0, sizeof(g_recv_buff));
            ser_read_ret = -1;

            ser_read_ret = read_ser_port(g_serial_fd, g_recv_buff, 3, midx);

            if (ser_read_ret < 0)
            {
                dbg_log(REPORT, "[%-25s] :Read serial failed,Retry: %d\n", fun_name, retry + 1);
                sleep(20);
                continue;
            }
            else
            {
                // dbg_log(REPORT, "[%-25s] :Num Bytes Read %d\n", fun_name, ser_read_ret);
                print_data_plcc(midx, g_recv_buff, ser_read_ret, 1);
                ret = proc_resp(midx, g_recv_buff, ser_read_ret, type);
                if (ret < 0)
                {
                    dbg_log(REPORT, "[%-25s] :Processing response failed, Retry Val : %d\n", fun_name, retry + 1);

                    memset(event_msg_str, 0, sizeof(event_msg_str));
                    sprintf(event_data.eventmsg, "PLCC Processing response failed, Retry Val : %d", retry + 1);
                    strcpy(event_data.timestamp, time_format_conversion(time(NULL)));
                    add_event_data(&event_data);
                    continue;
                }
                else
                {
                    break;
                }
            }
        }
    }
    if (retry < 3)
        return RET_OK;
    else
        return ret;
}

int get_nodes_ser_num(int midx, uint8_t *msg, int len)
{
    static char fun_name[] = "get_nodes_ser_num()";
    int j;
    uint8_t temp_msg[2];
    char temp_buff[255];

    dbg_log(INFORM, "[%-25s] :RS_%d OLD STATUS : %d CURR_STATUS : %d\n", fun_name, midx, plcc_modem_info.plcc_meter_map[midx].rs_status, msg[len - 2]);

    plcc_modem_info.plcc_meter_map[midx].rs_status = msg[len - 2];

    memcpy(temp_msg, &msg[6], 2 * sizeof(msg[0]));
    plcc_modem_info.plcc_meter_map[midx].node_id = bytes_to_decimal_le(temp_msg);

    memcpy(temp_msg, &msg[8], 2 * sizeof(msg[0]));
    plcc_modem_info.plcc_meter_map[midx].parent_id = bytes_to_decimal_le(temp_msg);

    memset(temp_buff, 0, sizeof(temp_buff));

    for (j = 10; j <= 17; j++)
    {
        plcc_modem_info.plcc_meter_map[midx].plcc_ser[j - 10] = msg[j];
        sprintf(temp_buff,"%02x", plcc_modem_info.plcc_meter_map[midx].plcc_ser[j - 10]);
    }

    dbg_log(REPORT, "[%-25s] :RS SN : %s\n", fun_name, temp_buff);

    return 0;
}

uint8_t calc_cksum(uint8_t *data, size_t len)
{
    cksum = 0;
    size_t i;
    // printf("data");
    // for (i = 1; i < len; i++)
    // {
    //     printf("%02x", data[i]);
    // }
    // printf("\n");
    for (i = 1; i < len; i++)
    {
        cksum = (cksum + data[i]) % 256;
    }
    return cksum;
}

// rithika 20/09
// uint8_t *build_PLCC_cmd_frame(uint8_t opcode, uint8_t *data, int cmd_size, char *plcc_frame)
// {
int build_PLCC_cmd_frame(uint8_t opcode, uint8_t *data, int cmd_size, char *plcc_frame)
{
    int i = 0;

    memset(plcc_frame, 0, sizeof(plcc_frame));

    plcc_frame[0] = PLCC_START_CHAR;

    plcc_frame[1] = 0x00; // LSB Length

    plcc_frame[2] = 0x00; // MSB Length

    plcc_frame[3] = (PROTOCOL_VERSION << 3) | PACKET_TYPE_REQUEST;

    plcc_frame[4] = opcode;

    for (i = 0; i < cmd_size; i++)
    {
        plcc_frame[i + 5] = data[i];
    }

    int plcc_frm_len = 2 + cmd_size;

    plcc_frame[1] = (plcc_frm_len) & 0xFF;      // LSB Length
    plcc_frame[2] = (plcc_frm_len >> 8) & 0xFF; // MSB Length

    cksum = calc_cksum(plcc_frame, cmd_size + 5);

    plcc_frame[cmd_size + 5] = cksum;

    return cmd_size + 6;
}

int nc_db_size()
{
    static char fun_name[] = "nc_db_size()";
    int len = 0;
    int i;

    // Get NC Database Size
    cmd_data[0] = PACKET_TYPE_RESPONSE;
    len = build_PLCC_cmd_frame(OPCODE_DB_ENTRY, cmd_data, 1, frame);
    test_plcc_comm(0, frame, len, OPCODE_DB_ENTRY);
    sleep(5);

    dbg_log(REPORT, "[%-25s] :NUMBER OF NODES : %d\n", fun_name, plcc_modem_info.no_of_nodes);

    for (i = 0; i < plcc_modem_info.no_of_nodes; i++)
    {
        send_hanging_check();
        
        memset(cmd_data, 0, sizeof(cmd_data));

        cmd_data[0] = (i >> 8) & 0xFF;
        cmd_data[1] = (i) & 0xFF;

#if 0
        for (j = 0; j <= 2; j++)
        {
            printf("%02X ", cmd_data[j]);
        }
        printf("len of cmd_data : %d \n", sizeof(cmd_data));
#endif

         len = build_PLCC_cmd_frame(OPCODE_NODE_INFO, cmd_data, 3, frame);
        test_plcc_comm(i, frame, len, OPCODE_NODE_INFO);
        sleep(1);
    }

    return 0;
}

uint32_t plcc_packetization(uint8_t *qry_data, int dlms_data_size, uint8_t *dlms_frame)
{
    static char fun_name[] = "plcc_packetization()";
    memset(frame, 0, sizeof(frame));
    int i;
    int len = 0;
    int cmd_data_len = 0;

#if DEBUG
    printf("CURR_RS_IDX  : %d \t RS_SN : ", curr_nc_idx);
    for (i = 9; i < 17; i++)
    {
        printf("%02x", plcc_modem_info.plcc_meter_map[curr_nc_idx].plcc_ser[i - 9]);
    }
    printf("\n");
#endif

    if (PLCC_FRAME == 0)
    {
        return qry_data;
    }
    else
    {
        if (curr_nc_idx >= plcc_modem_info.no_of_nodes)
        {
            printf("Meter index exceeded \n");
            return -1;
        }
        frame[0] = SERVICE_TYPE_CONF_AND_MON;
        frame[1] = PACKET_TYPE_REQUEST; // 0x01,0x08,0x07,0x01,0x00,0x00,0x00
        frame[2] = 0x01;
        frame[3] = 0x08;
        frame[4] = 0x07;
        frame[5] = 0x02;
        frame[6] = 0x00;
        frame[7] = 0x00;
        frame[8] = 0x02;

        for (i = 9; i < 17; i++)
        {
            frame[i] = plcc_modem_info.plcc_meter_map[curr_nc_idx].plcc_ser[i - 9];
        }

        frame[i++] = 0x00;
        frame[i++] = 0x00;
        frame[i++] = 0x00;
        frame[i++] = 0x00;
        frame[i++] = 0x00;
        frame[i++] = 0x00;
        frame[i++] = 0x00;
        frame[i++] = 0x00;
        cmd_data_len = i + dlms_data_size;

        memcpy(frame + i, qry_data, cmd_data_len);

        len = build_PLCC_cmd_frame(OPCODE_TX_PKT, frame, cmd_data_len, dlms_frame);
    }
    return len;
}

uint32_t plcc_depacketization(uint8_t *res_data, int data_size, uint8_t *dlms_frame)
{
    // uint8_t *plcc_frame;
    // memset(cmd_data, 0, sizeof(cmd_data));
    // print_plcc_frame(res_data,data_size,1);

    int len = 33;
    size_t i;
    int cmd_data_len = 0;

    // printf("ORIGINAL PACKET");
    // for (i = 0; i <= data_size; i++)
    // {
    //     printf("%02x ", res_data[i]);
    // }
    // printf("\n");

    if (PLCC_FRAME == 0)
    {
        return res_data;
    }
    else
    {

        size_t start_index = 0;
        while (start_index < data_size && res_data[start_index] != 0x7e)
        {
            start_index++;
        }

        if (start_index >= data_size)
        {
            // printf("No '7e' found in the res_data.\n");
            return;
        }

        size_t end_index = data_size - 1;
        while (end_index > 0 && res_data[end_index] != 0x7e)
        {
            end_index--;
        }
        // printf("start idx : %d end index : %d\n", start_index, end_index);
        if (start_index < end_index)
        {
            // printf("Values between first '7e' and last '7e': \n");
            for (i = start_index; i <= end_index; i++)
            {
                dlms_frame[i - start_index] = res_data[i];
                // printf("%02x ", dlms_frame[i - start_index]);
            }
            // printf("\n");
            cmd_data_len = (end_index - start_index) + 1;
            // printf("plcc_depacketization len : %d\n", cmd_data_len);
        }
        else
        {
            printf("No values between first '7e' and last '7e'.\n\n");
        }
    }
    return cmd_data_len;
}

void print_hex(uint8_t *data, int len)
{
    int i;
    for (i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

uint32_t wrapper_depacketization(uint8_t *res_data, int data_size, uint8_t *dlms_frame)
{
    size_t i, j;
    int cmd_data_len = 0;

    // Find start of wrapper pattern
    size_t start_index = 0;
    bool pattern_found = false;

    // Search for the start pattern
    for (i = 0; i < data_size - WRAPPER_START_PATTERN_SIZE; i++)
    {
        // Check for first pattern (WRAPPER_START_PATTERN_1)
        bool match_pattern_1 = true;
        for (j = 0; j < WRAPPER_START_PATTERN_SIZE; j++)
        {
            if (res_data[i + j] != WRAPPER_START_PATTERN_1[j])
            {
                match_pattern_1 = false;
                break;
            }
        }

        // If first pattern is found, set start_index and break
        if (match_pattern_1)
        {
            start_index = i;
            pattern_found = true;
            break;
        }

        // Check for second pattern (WRAPPER_START_PATTERN_2)
        bool match_pattern_2 = true;
        for (j = 0; j < WRAPPER_START_PATTERN_SIZE; j++)
        {
            if (res_data[i + j] != WRAPPER_START_PATTERN_2[j])
            {
                match_pattern_2 = false;
                break;
            }
        }

        // If second pattern is found, set start_index and break
        if (match_pattern_2)
        {
            start_index = i;
            pattern_found = true;
            break;
        }
    }

    if (!pattern_found)
    {
        printf("No wrapper pattern found in the res_data.\n");
        return 0;
    }

    // Calculate the wrapper length, excluding the last byte
    cmd_data_len = data_size - start_index - 1; // Exclude last byte

    // Ensure we don't exceed buffer size
    if (cmd_data_len <= 0)
    {
        printf("Calculated data length is invalid.\n");
        return 0;
    }

    // Copy wrapper data to output buffer, excluding the last byte
    for (i = start_index; i < start_index + cmd_data_len; i++)
    {
        dlms_frame[i - start_index] = res_data[i];
    }

    // printf("Wrapper start index: %zu, length: %d\n", start_index, cmd_data_len);

    // if (cmd_data_len > 0)
    // {
    //     printf("Extracted wrapper (%d bytes):\n", cmd_data_len);
    //     print_hex(dlms_frame, cmd_data_len);
    // }

    return cmd_data_len;
}

int delete_db()
{
    static char fun_name[] = "delete_db()";

    int ret = 0, index = 0;
   
    int len = 0;

    if (g_serial_fd > 0)
        close_port(g_serial_fd);

    g_serial_fd = init_serial(g_serial_fd, PORT_1_FILE, 12, 8, 0, 1);

    if (g_serial_fd < 0)
    {
        dbg_log(REPORT, "[%-25s] :Error to init serial\n", fun_name);
        return -10;
    }

    // Delet Operation
    memset(cmd_data, 0, sizeof(cmd_data));
    cmd_data[index++] = 0x03;
    len = build_PLCC_cmd_frame(OPCODE_DELETE_NODE_INFO, cmd_data, index, frame);
    dbg_log(INFORM, "%-20s :DELTE NC CMD EXCUTED\n", fun_name);
    test_plcc_comm(0, frame, len, OPCODE_DELETE_NODE_INFO);

    nc_soft_reset(1);

    // No Operation
    memset(cmd_data, 0, sizeof(cmd_data));
    len = build_PLCC_cmd_frame(OPCODE_NOP, cmd_data, 0, frame);
    dbg_log(INFORM, "%-20s :NOP CMD EXCUTED\n", fun_name);
    test_plcc_comm(0, frame, len, OPCODE_NOP);
    sleep(5);

    close_port(g_serial_fd);

    sleep(3);
    return 0;
}

uint8_t get_db_size(int type)
{
    static char fun_name[] = "get_db_size()";

    int len, ret;
    uint8_t index = 0;

    if (type != 0)
    {
        com_close(&con);
        
        if (g_serial_fd > 0)
            close_port(g_serial_fd);

        g_serial_fd = init_serial(g_serial_fd, PORT_1_FILE, 12, 8, 0, 1);

        if (g_serial_fd < 0)
        {
            dbg_log(REPORT, "[%-25s] :Error to init serial\n", fun_name);
            return -10;
        }
    }

    // Get db size
    memset(cmd_data, 0, sizeof(cmd_data));
    cmd_data[index++] = 0x06;
    cmd_data[index++] = 0x5B;
    cmd_data[index++] = 0x00;
    cmd_data[index++] = 0x01;
    cmd_data[index++] = 0x00;
    len = build_PLCC_cmd_frame(OPCODE_GET_DEVICE_PARAMS, cmd_data, index, frame);
    ret = test_plcc_comm(0, frame, len, DB_SIZE);
    sleep(2);

    if (type != 0)
    {
        con.comPort = -1;
        com_open(&con, p_comm_port_det);
    }
    
    return ret;
}

uint8_t get_nw_size(int type)
{
    static char fun_name[] = "get_nw_size()";

    int len, ret;
    uint8_t index = 0;

    if (type != 0)
    {
        com_close(&con);

        if (g_serial_fd > 0)
            close_port(g_serial_fd);

        g_serial_fd = init_serial(g_serial_fd, PORT_1_FILE, 12, 8, 0, 1);

        if (g_serial_fd < 0)
        {
            dbg_log(REPORT, "[%-25s] :Error to init serial\n", fun_name);
            return -10;
        }
    }

    // Get nc size
    memset(cmd_data, 0, sizeof(cmd_data));
    cmd_data[index++] = 0x06;
    cmd_data[index++] = 0x38;
    cmd_data[index++] = 0x00;
    cmd_data[index++] = 0x01;
    len = build_PLCC_cmd_frame(OPCODE_GET_DEVICE_PARAMS, cmd_data, index, frame);
    ret = test_plcc_comm(0, frame, len, NC_SIZE);
    sleep(2);

    if (type != 0)
    {
        con.comPort = -1;
        com_open(&con, p_comm_port_det);
    }

    return ret;
}

uint8_t nc_soft_reset(int type)
{
    static char fun_name[] = "nc_soft_reset()";

    int len, ret;
    uint8_t index = 0, idx;

    if (type != 0)
    {
        com_close(&con);

        if (g_serial_fd > 0)
            close_port(g_serial_fd);

        g_serial_fd = init_serial(g_serial_fd, PORT_1_FILE, 12, 8, 0, 1);

        if (g_serial_fd < 0)
        {
            dbg_log(REPORT, "[%-25s] :Error to init serial\n", fun_name);
            return -10;
        }
    }

    // Soft Reset
    for (idx = 0; idx < MAX_RETRY; idx++)
    {
        memset(cmd_data, 0, sizeof(cmd_data));

        len = build_PLCC_cmd_frame(OPCODE_RESET, cmd_data, 0, frame);
        ret = test_plcc_comm(0, frame, len, OPCODE_RESET);

        dbg_log(INFORM, "[%-25s] :ret : %d  idx : %d\n", fun_name, ret, idx);

        if (ret == -12)
        {
            dbg_log(INFORM, "[%-25s] :SLEEP 30\n", fun_name);
            sleep(30);
        }
        else
        {
            dbg_log(INFORM, "[%-25s] :SLEEP 5\n", fun_name);

            sleep(5);
        }
        if (ret < 0)
        {
            continue;
        }
        else
            break;
    }

    if (type != 0)
    {
        con.comPort = -1;
        com_open(&con, p_comm_port_det);
    }

    return ret;
}

int init_plcc_modem(int init_type)
{
    static char fun_name[] = "init_plcc_modem()";

    int len = 0, index = 0;

    if (init_type != 0)
    {
        com_close(&con);

        if (g_serial_fd > 0)
            close_port(g_serial_fd);

        g_serial_fd = init_serial(g_serial_fd, PORT_1_FILE, 12, 8, 0, 1);

        if (g_serial_fd < 0)
        {
            dbg_log(REPORT, "[%-25s] :Error to init serial\n", fun_name);
            return -10;
        }
    }
    else
    {
        g_serial_fd = init_serial(g_serial_fd, PORT_1_FILE, 12, 8, 0, 1);

        if (g_serial_fd < 0)
        {
            dbg_log(REPORT, "[%-25s] :Error to init serial\n", fun_name);
            return -1;
        }
    }

    send_hanging_check();

    nc_soft_reset(init_type);

    // No Operation
    memset(cmd_data, 0, sizeof(cmd_data));
    len = build_PLCC_cmd_frame(OPCODE_NOP, cmd_data, 0, frame);
    test_plcc_comm(0, frame, len, OPCODE_NOP);
    sleep(2);

    get_db_size(init_type);

    get_nw_size(init_type);

    // Get Node ID
    memset(cmd_data, 0, sizeof(cmd_data));
    cmd_data[index++] = 0x06;
    cmd_data[index++] = 0x19;
    cmd_data[index++] = 0x00;
    cmd_data[index++] = 0x01;
    cmd_data[index++] = 0x00;
    len = build_PLCC_cmd_frame(OPCODE_GET_DEVICE_PARAMS, cmd_data, index, frame);
    test_plcc_comm(0, frame, len, OPCODE_GET_DEVICE_PARAMS);
    sleep(2);

    send_hanging_check();

    // Get Node SN
    memset(cmd_data, 0, sizeof(cmd_data));
    index = 0;
    cmd_data[index++] = 0x05;
    cmd_data[index++] = 0xAB;
    cmd_data[index++] = 0xBA;
    len = build_PLCC_cmd_frame(OPCODE_GET_DEVICE_PARAMS, cmd_data, index, frame);
    test_plcc_comm(0, frame, len, NC_SN);
    sleep(2);

    // Get NC Database Size and serial num
    nc_db_size();

    send_hanging_check();

    close_port(g_serial_fd);

    if (init_type != 0)
    {
        con.comPort = -1;
        com_open(&con, p_comm_port_det);
    }

    set_plcc_modem_info_to_redis();
    return 0;
}
