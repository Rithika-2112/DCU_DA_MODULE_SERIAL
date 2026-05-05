//
// --------------------------------------------------------------------------
//  Gurux Ltd
//
//
//
// Filename:        $HeadURL:  $
//
// Version:         $Revision:  $,
//                  $Date:  $
//                  $Author: $
//
// Copyright (c) Gurux Ltd
//
//---------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#if defined(_WIN32) || defined(_WIN64) // Windows includes
#include <conio.h>
#include "../include/getopt.h"
#include <process.h> //Add support for threads
#if _MSC_VER > 1000
#include <crtdbg.h>
#endif
// Windows doesn't implement strcasecmp. It uses strcmpi.
#define strcasecmp _strcmpi
#else
#include <string.h> /* memset */
#include <unistd.h>
#include <strings.h>
#include <errno.h>
#include <error.h>
#endif

#include "../include/communication.h"
#include "../../GuruxDLMS.c-master/development/include/gxserializer.h"

////////////////////////////////////////////////// PRIYA /////////////////////////////////////////////////////
#define COMMENT 0
#define CMS 1

#define OLD_DATA_FILES_TIME ((60) * 24 * 60 * 60)
// #define OLD_DATA_FILES_TIME (24 * 60 * 60)

#define NAME_PLATE 0
#define INST 1

#define TT_FILE_SIZE_EXCEEDS 2 * 1024 * 1024

typedef struct
{
    char obis[30][20];
    int tot_obis; // Number of OBIS codes
} NAMEPLATE_OBIS;

typedef struct
{
    char obis[30][20];
    int tot_obis; // Number of OBIS codes
} INST_OBIS;

// typedef struct
// {
//     uint8_t day;
//     uint8_t month;
//     uint16_t year;
//     uint8_t hour;
//     uint8_t minute;
//     uint8_t second;
// } date_time_t;

NAMEPLATE_OBIS nameplate_obis;
INST_OBIS inst_obis;
int position = 0;
time_t poll_start_time;
time_t poll_end_time;
time_t process_end_time;
time_t poll_diff;
int poll_min, poll_sec, proc_min, proc_sec;
char *poll_start_str = NULL;
char *poll_end_str = NULL;
char *proc_end_str = NULL;
char poll_details[256];
////////////////////////////////////////////////// PRIYA /////////////////////////////////////////////////////
// Client don't need this.
unsigned char svr_isTarget(
    dlmsSettings *settings,
    unsigned long serverAddress,
    unsigned long clientAddress)
{
    return 0;
}

// Client don't need this.
int svr_connected(
    dlmsServerSettings *settings)
{
    return 0;
}

// Client don't need this.
DLMS_ACCESS_MODE svr_getAttributeAccess(
    dlmsSettings *settings,
    gxObject *obj,
    unsigned char index)
{
    return DLMS_ACCESS_MODE_READ_WRITE;
}

// Client don't need this.
DLMS_METHOD_ACCESS_MODE svr_getMethodAccess(
    dlmsSettings *settings,
    gxObject *obj,
    unsigned char index)
{
    return DLMS_METHOD_ACCESS_MODE_ACCESS;
}

/// Client don't need this.
void svr_preGet(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

// Client don't need this.
void svr_postGet(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

// Client don't need this.
void svr_preRead(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

// Client don't need this.
void svr_preWrite(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

// Client don't need this.
void svr_preAction(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

// Client don't need this.
extern void svr_postRead(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

// Client don't need this.
extern void svr_postWrite(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

// Client don't need this.
extern void svr_postAction(
    dlmsSettings *settings,
    gxValueEventCollection *args)
{
}

/**
 * Updates clock and reads it.
 */
// int com_updateClock(connection *connection)
// {
//     int ret;
//     gxClock clock;
//     unsigned char ln[] = {0, 0, 1, 0, 0, 255};
//     INIT_OBJECT(clock, DLMS_OBJECT_TYPE_CLOCK, ln);
//     // Initialize connection.
//     ret = com_initializeConnection(connection);
//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         return ret;
//     }
//     // Write clock.
//     time_init(&clock.time, 2000, 1, 1, 0, 0, 0, 0, 0x8000);
//     ret = com_write(connection, &clock.base, 2);
//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         return ret;
//     }

//     // Read clock.
//     ret = com_read(connection, &clock.base, 2);
//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         return ret;
//     }
//     // Print clock.
//     ret = time_print(NULL, &clock.time);
//     return ret;
// }

/**
 * Calls disconnect method.
 */
int disconnect(connection *connection)
{
    int ret;
    gxDisconnectControl dc;
    unsigned char ln[] = {0, 0, 96, 3, 10, 255};
    INIT_OBJECT(dc, DLMS_OBJECT_TYPE_DISCONNECT_CONTROL, ln);
    // Call Disconnect action.
    dlmsVARIANT param;
    GX_INT8(param) = 0;
    ret = com_method(connection, &dc.base, 1, &param);
    return ret;
}

/**
 * Calls capture of the profile generic object..
 */
int CaptureProfileGeneric(connection *connection)
{
    int ret;
    gxProfileGeneric pg;
    unsigned char ln[] = {1, 0, 99, 1, 0, 255};
    INIT_OBJECT(pg, DLMS_OBJECT_TYPE_PROFILE_GENERIC, ln);
    // Invokes capture action.
    dlmsVARIANT param;
    GX_INT8(param) = 0;
    ret = com_method(connection, BASE(pg), 2, &param);
    return ret;
}

/**
* Activates and strengthens the security policy
  for the security setup object
*/
int SecurityActivate(connection *connection,
                     DLMS_SECURITY_POLICY policy)
{
    int ret;
    gxSecuritySetup ss;
    unsigned char ln[] = {0, 0, 43, 0, 0, 255};
    INIT_OBJECT(ss, DLMS_OBJECT_TYPE_SECURITY_SETUP, ln);
    // Invokes capture action.
    dlmsVARIANT param;
    var_setEnum(&param, policy);
    GX_ENUM(param) = policy;
    ret = com_method(connection, BASE(ss), 1, &param);
    return ret;
}

// Show PDU before it's encrypted or
// after it's decrypted when DLMS_TRACE_PDU
// is defined.
void cip_tracePdu(
    unsigned char encrypt,
    gxByteBuffer *pdu)
{
    const char *direction = encrypt ? "TX" : "RX";
    char *str = bb_toHexString(pdu);
    printf("\r\n%s PDU: %s\r\n", direction, str);
    free(str);
}

/**
 * Executes selected script.
 * params:
 * ln: Logical name of the script table object.
 * scriptId: Script Id.
 */
int execute(connection *connection, const unsigned char *ln, uint16_t scriptId)
{
    int ret;
    gxScriptTable st;
    INIT_OBJECT(st, DLMS_OBJECT_TYPE_SCRIPT_TABLE, ln);
    // Call script id.
    dlmsVARIANT param;
    GX_UINT16(param) = scriptId;
    ret = com_method(connection, &st.base, 1, &param);
    return ret;
}
////////////////////////////////////////////////////////////////////
int polling_time_clc_file(char *data)
{
    FILE *fpt;
    printf("======================================= > >  > %s", data);
    fpt = fopen("/usr/cms/log/yitran_polling_time_calc.csv", "a");
    if (fpt == NULL)
    {
        printf("open call failed for file : %s , Error : %s\n", "/usr/cms/log/yitran_polling_time_calc.csv", strerror(errno));
        return -1;
    }

    fprintf(fpt, data);
    fflush(fpt);
    position = ftell(fpt);

    if (position >= TT_FILE_SIZE_EXCEEDS)
    {
         fclose(fpt);
        char bkup_file[64];
        char tmp_buf[128];  

        memset(bkup_file, 0, sizeof(bkup_file));
        sprintf(bkup_file, "%s.csv", "/usr/cms/log/yitran_polling_time_calc_bkup");

        memset(tmp_buf, 0, sizeof(tmp_buf));
        sprintf(tmp_buf, "mv %s %s", "/usr/cms/log/yitran_polling_time_calc.csv", bkup_file);
        system(tmp_buf);

        fpt = fopen("/usr/cms/log/yitran_polling_time_calc.csv", "w");
        // perror("fopen:");
    }

    fclose(fpt);

    return 0;
}
/////////////////////////////////////////////////////////////////////////////////
/**
* Update firmware of the meter.
*
* In image update following steps are made:
1. Image_transfer_enabled is read.
2. Image block size is read.
3. image_transferred_blocks_status is read to check is image try to update before.
4. image_transfer_initiate
5. image_transfer_status is read.
6. image_block_transfer
7. image_transfer_status is read.
8. image_transfer_status is read.
9. image_verify is called.
10. image_transfer_status is read.
11. image_activate is called.
*/
int imageUpdate(connection *connection, const unsigned char *identification, uint16_t identificationSize, unsigned char *image, uint32_t imageSize)
{
    int ret;
    gxByteBuffer bb;
    bb_init(&bb);
    dlmsVARIANT param;
    gxImageTransfer im;
    unsigned char ln[] = {0, 0, 44, 0, 0, 255};
    INIT_OBJECT(im, DLMS_OBJECT_TYPE_IMAGE_TRANSFER, ln);

    // 1. Image_transfer_enabled is read.
    if ((ret = com_read(connection, BASE(im), 5)) == 0 &&
        // 2. Image block size is read.
        (ret = com_read(connection, BASE(im), 2)) == 0 &&
        // 3. image_transferred_blocks_status is read to check is image try to update before.
        (ret = com_read(connection, BASE(im), 3)) == 0 &&
        // 4. image_transfer_initiate
        (ret = bb_setInt8(&bb, DLMS_DATA_TYPE_STRUCTURE)) == 0 &&
        (ret = bb_setInt8(&bb, 2)) == 0 &&
        (ret = bb_setInt8(&bb, DLMS_DATA_TYPE_OCTET_STRING)) == 0 &&
        (ret = hlp_setObjectCount(identificationSize, &bb)) == 0 &&
        (ret = bb_set(&bb, identification, identificationSize)) == 0 &&
        (ret = bb_setInt8(&bb, DLMS_DATA_TYPE_UINT32)) == 0 &&
        (ret = bb_setInt32(&bb, imageSize)) == 0 &&
        (ret = com_method3(connection, BASE(im), 1, &bb)) == 0)
    {
        // 5. image_transfer_status is read.
        if ((ret = com_read(connection, BASE(im), 6)) == 0)
        {
            // 6. image_block_transfer
            uint32_t count = im.imageBlockSize;
            uint32_t blockNumber = 0;
            while (imageSize != 0)
            {
                if (imageSize < im.imageBlockSize)
                {
                    count = imageSize;
                }
                bb_clear(&bb);
                if ((ret = bb_setInt8(&bb, DLMS_DATA_TYPE_STRUCTURE)) != 0 ||
                    (ret = bb_setInt8(&bb, 2)) != 0 ||
                    (ret = bb_setInt8(&bb, DLMS_DATA_TYPE_UINT32)) != 0 ||
                    (ret = bb_setInt32(&bb, blockNumber)) != 0 ||
                    (ret = bb_setInt8(&bb, DLMS_DATA_TYPE_OCTET_STRING)) != 0 ||
                    (ret = hlp_setObjectCount(count, &bb)) != 0 ||
                    (ret = bb_set(&bb, image, count)) != 0 ||
                    (ret = com_method3(connection, BASE(im), 2, &bb)) != 0)
                {
                    break;
                }
                imageSize -= count;
                ++blockNumber;
            }
            if (ret == 0)
            {
                // 7. image_transfer_status is read.
                ret = com_read(connection, BASE(im), 6);
                if (ret == 0)
                {
                    // 9. image_verify is called.
                    GX_INT8(param) = 0;
                    if ((ret = com_method(connection, BASE(im), 3, &param)) == 0 ||
                        ret == DLMS_ERROR_CODE_TEMPORARY_FAILURE)
                    {
                        while (1)
                        {
                            // 10. image_transfer_status is read.
                            ret = com_read(connection, BASE(im), 6);
                            if (im.imageTransferStatus == DLMS_IMAGE_TRANSFER_STATUS_VERIFICATION_SUCCESSFUL)
                            {
                                break;
                            }
                            if (im.imageTransferStatus == DLMS_IMAGE_TRANSFER_STATUS_VERIFICATION_FAILED)
                            {
                                ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
                                break;
                            }

                            // Wait until image is activated.
#if defined(_WIN32) || defined(_WIN64) // Windows
                            Sleep(10000);
#else
                            usleep(1000000);
#endif // defined(_WIN32) || defined(_WIN64)
                        }
                        if (ret == 0)
                        {
                            ret = com_read(connection, BASE(im), 6);
                            // 11. image_activate is called.
                            ret = com_method(connection, BASE(im), 4, &param);
                        }
                    }
                }
            }
        }
    }
    bb_clear(&bb);
    return ret;
}

/*
 * This method can be used to update firmware from the hex file.
 */
int imageUpdateFromFile(
    connection *connection,
    const char *identifier,
    const char *fileName)
{
#if _MSC_VER > 1400
    FILE *f = NULL;
    fopen_s(&f, fileName, "r");
#else
    FILE *f = fopen(fileName, "r");
#endif
    int ret = DLMS_ERROR_CODE_INVALID_PARAMETER;
    if (f != NULL)
    {
        size_t imagesize;
        char image[101];
        gxByteBuffer bb;
        bb_init(&bb);
        while (feof(f) == 0)
        {
            imagesize = fread(image, sizeof(char), sizeof(image) - 1, f);
            image[imagesize] = 0;
            bb_addHexString(&bb, image);
        }
        fclose(f);
        ret = imageUpdate(connection, identifier, (uint16_t)strlen(identifier), bb.data, bb.size);
        bb_clear(&bb);
    }
    return ret;
}

/*Read DLMS meter using TCP/IP connection.*/
int readTcpIpConnection(
    connection *connection,
    const char *address,
    const int port,
    char *readObjects,
    const char *invocationCounter,
    const char *outputFile)
{
    int ret;
#if defined(_WIN32) || defined(_WIN64) // Windows
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        // Tell the user that we could not find a usable WinSock DLL.
        return 1;
    }
#endif
    if (connection->trace > GX_TRACE_LEVEL_WARNING)
    {
        printf("Connecting to %s:%d\n", address, port);
    }
    // Make connection to the meter.
    ret = com_makeConnect(connection, address, port, 5000);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        con_close(connection);
        com_close(connection);
        return ret;
    }
    // if (readObjects != NULL)
    // {
    if ((ret = com_updateInvocationCounter(connection, invocationCounter)) == 0 &&
        (ret = com_initializeConnection(connection)) == 0 &&
        (ret = com_getAssociationView(connection, outputFile)) == 0)
    {
#if COMMENT
        int index;
        unsigned char buff[200];
        gxObject *obj = NULL;
        char *p2, *p = readObjects;
        do
        {
            if (p != readObjects)
            {
                ++p;
            }

            p2 = strchr(p, ':');
            *p2 = '\0';
            ++p2;
#if defined(_WIN32) || defined(_WIN64) // Windows
            sscanf_s(p2, "%d", &index);
#else
            sscanf(p2, "%d", &index);
#endif
            hlp_setLogicalName(buff, p);
            if ((ret = oa_findByLN(&connection->settings.objects, DLMS_OBJECT_TYPE_NONE, buff, &obj)) == 0)
            {
                if (obj == NULL)
                {
                    printf("Object '%s' not found from the association view.\n", p);
                    break;
                }
                // Capture objects are read first if the buffer of the profile generic is read.
                if (obj->objectType == DLMS_OBJECT_TYPE_PROFILE_GENERIC && index == 2)
                {
                    if ((ret = com_readValue(connection, obj, 3)) != 0)
                    {
                        break;
                    }
                }
                ret = com_readValue(connection, obj, index);
            }
        } while ((p = strchr(p2, ',')) != NULL);
#endif
#if CMS
        // read_nameplate(connection);
        // single_obis_read(connection);
        // read_inst(connection);
        // read_billing(connection);
        // read_ls(connection);
        // read_event(connection);
        // read_midnight(connection);
#endif
    }
    // }
#if COMMENT

    else
    {
        // Initialize connection.
        if ((ret = com_updateInvocationCounter(connection, invocationCounter)) != 0 ||
            (ret = com_initializeConnection(connection)) != 0 ||
            (ret = com_readAllObjects(connection, outputFile)) != 0)
        // Read all objects from the meter.
        {
            // Error code is returned at the end of the function.
        }
    }

#endif
    // Close connection.
    com_close(connection);
    con_close(connection);
    return ret;
}

/*Read DLMS meter using serial port connection.*/
int readSerialPort(
    connection *connection,
    const char *port,
    char *readObjects,
    const char *invocationCounter,
    const char *outputFile)
{
    int ret;
    printf("-------------------------------------- Inside Reading serial port obj : %s port : %s-------------------------------\n", readObjects, port);
    ret = com_open(connection, port);

    if (ret == 0)
    {
        if ((ret = com_initializeOpticalHead(connection)) == 0 &&
            (ret = com_updateInvocationCounter(connection, invocationCounter)) == 0 &&
            (ret = com_initializeOpticalHead(connection)) == 0 &&
            (ret = com_initializeConnection(connection)) == 0 &&
            (ret = com_getAssociationView(connection, outputFile)) == 0)
        {
#if COMMENT
            int index;
            unsigned char buff[200];
            gxObject *obj = NULL;
            char *p2, *p = readObjects;
            do
            {
                if (p != readObjects)
                {
                    ++p;
                }

                p2 = strchr(p, ':');
                *p2 = '\0';
                ++p2;
#if defined(_WIN32) || defined(_WIN64) // Windows
                sscanf_s(p2, "%d", &index);
#else
                sscanf(p2, "%d", &index);
#endif
                printf("p : %s\n", p);
                hlp_setLogicalName(buff, p);
                printf("buff : %s p : %s\n", buff, p);
                oa_findByLN(&connection->settings.objects, DLMS_OBJECT_TYPE_NONE, buff, &obj);
                if (obj == NULL)
                {
                    printf("Object '%s' not found from the association view.\n", p);
                    break;
                }
                // Capture objects are read first if the buffer of the profile generic is read.
                if (obj->objectType == DLMS_OBJECT_TYPE_PROFILE_GENERIC && index == 2)
                {
                    printf("********************************************************\n");
                    if ((ret = com_readValue(connection, obj, 3)) != 0)
                    {
                        break;
                    }
                }
                printf("*************************read*******************************\n");
                ret = com_readValue(connection, obj, index);
            } while ((p = strchr(p2, ',')) != NULL);
#endif
#if CMS
            // read_nameplate(connection);
            // single_obis_read(connection);
            // read_inst(connection);
            // read_billing(connection);
            // read_ls(connection);
            // read_event(connection);
            // read_midnight(connection);
#endif
        }
    }
#if COMMENT
    else if (ret == 0)
    {
        // Initialize connection.
        printf("-------------------------------------- Inside Reading serial port else obj : %s-------------------------------\n", readObjects);
        if ((ret = com_initializeOpticalHead(connection)) != 0 ||
            (ret = com_updateInvocationCounter(connection, invocationCounter)) != 0 ||
            (ret = com_initializeOpticalHead(connection)) != 0 ||
            (ret = com_initializeConnection(connection)) != 0 ||
            (ret = com_readAllObjects(connection, outputFile)) != 0)
        // Read all objects from the meter.
        {
            // Error code is returned at the end of the function.
        }
    }
#endif
    // Close connection.
    com_close(connection);
    con_close(connection);
    return ret;
}

static void ShowHelp()
{
    printf("GuruxDlmsSample reads data from the DLMS/COSEM device.\n");
    printf("GuruxDlmsSample -h [Meter IP Address] -p [Meter Port No] -c 16 -s 1 -r SN\n");
    printf(" -h \t host name or IP address.\n");
    printf(" -p \t port number or name (Example: 1000).\n");
    printf(" -u \t UDP is used.");
    printf(" -S \t serial port.\n");
    printf(" -a \t Authentication (None, Low, High, HighMd5, HighSha1, HighGmac, HighSha256).\n");
    printf(" -P \t Password for authentication.\n");
    printf(" -c \t Client address. (Default: 16)\n");
    printf(" -s \t Server address. (Default: 1)\n");
    printf(" -n \t Server address as serial number.\n");
    printf(" -r \t [sn, sn]\t Short name or Logican Name (default) referencing is used.\n");
    printf(" -w \t WRAPPER profile is used. HDLC is default.\n");
    printf(" -t \t Trace messages.\n");
    printf(" -g \t Get selected object(s) with given attribute index. Ex -g \"0.0.1.0.0.255:1; 0.0.1.0.0.255:2\" \n");
    printf(" -C \t Security Level. (None, Authentication, Encrypted, AuthenticationEncryption)\n");
    printf(" -v \t Invocation counter data object Logical Name. Ex. 0.0.43.1.1.255\n");
    printf(" -I \t Auto increase invoke ID\n");
    printf(" -o \t Cache association view to make reading faster. Ex. -o C:\\device.bin");
    printf(" -T \t System title that is used with chiphering. Ex -T 4775727578313233\n");
    printf(" -A \t Authentication key that is used with chiphering. Ex -A D0D1D2D3D4D5D6D7D8D9DADBDCDDDEDF\n");
    printf(" -B \t Block cipher key that is used with chiphering. Ex -B 000102030405060708090A0B0C0D0E0F\n");
    printf(" -b \t Broadcast Block cipher key that is used with chiphering. Ex -b 000102030405060708090A0B0C0D0E0F");
    printf(" -D \t Dedicated key that is used with chiphering. Ex -D 00112233445566778899AABBCCDDEEFF\n");
    printf(" -i \t Used communication interface. Ex. -i WRAPPER.");
    printf(" -m \t Used PLC MAC address. Ex. -m 1.");
    printf(" -R \t Data is send as a broadcast (UnConfirmed, Confirmed).");
    printf("Example:\n");
    printf("Read LG device using TCP/IP connection.\n");
    printf("GuruxDlmsSample -r SN -c 16 -s 1 -h [Meter IP Address] -p [Meter Port No]\n");
    printf("Read LG device using serial port connection.\n");
    printf("GuruxDlmsSample -r SN -c 16 -s 1 -sp COM1 -i\n");
    printf("Read Indian device using serial port connection.\n");
    printf("GuruxDlmsSample -S COM1 -c 16 -s 1 -a Low -P [password]\n");
}

int connectMeter(int argc, char *argv[])
{
    connection con;
    gxByteBuffer item;
    bb_init(&item);
    con_init(&con, GX_TRACE_LEVEL_INFO);
    // Initialize settings using Logical Name referencing and HDLC.
    cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);

    int ret, opt = 0;
    int port = 0;
    char *address = NULL;
    char *serialPort = NULL;
    char *p, *readObjects = NULL, *outputFile = NULL;
    int index, a, b, c, d, e, f;
    char *invocationCounter = NULL;
    while ((opt = getopt(argc, argv, "h:p:c:s:r:i:It:a:p:wP:g:S:C:v:T:A:B:D:l:F:o:V:M:b:R:")) != -1)
    {
        switch (opt)
        {
        case 'r':
            if (strcasecmp("sn", optarg) == 0)
            {
                con.settings.useLogicalNameReferencing = 0;
            }
            else if (strcasecmp("ln", optarg) == 0)
            {
                con.settings.useLogicalNameReferencing = 1;
            }
            else
            {
                printf("Invalid reference option.\n");
                return 1;
            }
            break;
        case 'h':
            // Host address.
            address = optarg;
            break;
        case 't':
            // Trace.
            if (strcasecmp("Error", optarg) == 0)
                con.trace = GX_TRACE_LEVEL_ERROR;
            else if (strcasecmp("Warning", optarg) == 0)
                con.trace = GX_TRACE_LEVEL_WARNING;
            else if (strcasecmp("Info", optarg) == 0)
                con.trace = GX_TRACE_LEVEL_INFO;
            else if (strcasecmp("Verbose", optarg) == 0)
                con.trace = GX_TRACE_LEVEL_VERBOSE;
            else if (strcasecmp("Off", optarg) == 0)
                con.trace = GX_TRACE_LEVEL_OFF;
            else
            {
                printf("Invalid trace option '%s'. (Error, Warning, Info, Verbose, Off)", optarg);
                return 1;
            }
            break;
        case 'p':
            // Port.
            port = atoi(optarg);
            break;
        case 'P':
            bb_init(&con.settings.password);
            bb_addString(&con.settings.password, optarg);
            break;
        case 'i':
            // Interface
            if (strcasecmp("HDLC", optarg) == 0)
                con.settings.interfaceType = DLMS_INTERFACE_TYPE_HDLC;
            else if (strcasecmp("WRAPPER", optarg) == 0)
                con.settings.interfaceType = DLMS_INTERFACE_TYPE_WRAPPER;
            else if (strcasecmp("HdlcModeE", optarg) == 0)
                con.settings.interfaceType = DLMS_INTERFACE_TYPE_HDLC_WITH_MODE_E;
            else if (strcasecmp("Plc", optarg) == 0)
                con.settings.interfaceType = DLMS_INTERFACE_TYPE_PLC;
            else if (strcasecmp("PlcHdlc", optarg) == 0)
                con.settings.interfaceType = DLMS_INTERFACE_TYPE_PLC_HDLC;
            else if (strcasecmp("PlcPrime", optarg) == 0)
                con.settings.interfaceType = DLMS_INTERFACE_TYPE_PLC_PRIME;
            else if (strcasecmp("Pdu", optarg) == 0)
                con.settings.interfaceType = DLMS_INTERFACE_TYPE_PDU;
            else
            {
                printf("Invalid interface option '%s'. (HDLC, WRAPPER, HdlcModeE, Plc, PlcHdlc)", optarg);
                return 1;
            }
            // Update PLC settings.
#ifndef DLMS_IGNORE_PLC
            plc_reset(&con.settings);
#endif // DLMS_IGNORE_PLC
            break;
        case 'I':
            // AutoIncreaseInvokeID.
            con.settings.autoIncreaseInvokeID = 1;
            break;
        case 'C':
            if (strcasecmp("None", optarg) == 0)
            {
                con.settings.cipher.security = DLMS_SECURITY_NONE;
            }
            else if (strcasecmp("Authentication", optarg) == 0)
            {
                con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION;
            }
            else if (strcasecmp("Encryption", optarg) == 0)
            {
                con.settings.cipher.security = DLMS_SECURITY_ENCRYPTION;
            }
            else if (strcasecmp("AuthenticationEncryption", optarg) == 0)
            {
                con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION_ENCRYPTION;
            }
            else
            {
                printf("Invalid Ciphering option '%s'. (None, Authentication, Encryption, AuthenticationEncryption)", optarg);
                return 1;
            }
            break;
        case 'V':
            if (strcasecmp("Suite0", optarg) == 0)
            {
                con.settings.cipher.suite = DLMS_SECURITY_SUITE_V0;
            }
            else if (strcasecmp("Suite1", optarg) == 0)
            {
                con.settings.cipher.suite = DLMS_SECURITY_SUITE_V1;
            }
            else if (strcasecmp("Suite2", optarg) == 0)
            {
                con.settings.cipher.suite = DLMS_SECURITY_SUITE_V2;
            }
            else
            {
                printf("Invalid Security suite option '%s'. (Suite0, Suite1, Suite2)", optarg);
                return 1;
            }
            break;
        case 'T':
            bb_clear(&con.settings.cipher.systemTitle);
            bb_addHexString(&con.settings.cipher.systemTitle, optarg);
            if (con.settings.cipher.systemTitle.size != 8)
            {
                printf("Invalid system title '%s'.", optarg);
                return 1;
            }
            break;
        case 'M':
            if (con.settings.preEstablishedSystemTitle == NULL)
            {
                con.settings.preEstablishedSystemTitle = malloc(sizeof(gxByteBuffer));
                bb_init(con.settings.preEstablishedSystemTitle);
            }
            bb_clear(con.settings.preEstablishedSystemTitle);
            bb_addHexString(con.settings.preEstablishedSystemTitle, optarg);
            if (con.settings.preEstablishedSystemTitle->size != 8)
            {
                printf("Invalid pre-established system title '%s'.", optarg);
                return 1;
            }
            break;
        case 'A':
            bb_clear(&con.settings.cipher.authenticationKey);
            bb_addHexString(&con.settings.cipher.authenticationKey, optarg);
            if (con.settings.cipher.authenticationKey.size != 16 &&
                con.settings.cipher.authenticationKey.size != 32)
            {
                printf("Invalid authentication key '%s'.", optarg);
                return 1;
            }
            break;
        case 'B':
            bb_clear(&con.settings.cipher.blockCipherKey);
            bb_addHexString(&con.settings.cipher.blockCipherKey, optarg);
            if (con.settings.cipher.blockCipherKey.size != 16 &&
                con.settings.cipher.blockCipherKey.size != 32)
            {
                printf("Invalid block cipher key '%s'.", optarg);
                return 1;
            }
            break;
        case 'b':
            bb_clear(&con.settings.cipher.broadcastBlockCipherKey);
            bb_addHexString(&con.settings.cipher.broadcastBlockCipherKey, optarg);
            if (con.settings.cipher.broadcastBlockCipherKey.size != 16 &&
                con.settings.cipher.broadcastBlockCipherKey.size != 32)
            {
                printf("Invalid broadcast block cipher key '%s'.", optarg);
                return 1;
            }
            break;
        case 'D':
            con.settings.cipher.dedicatedKey = (gxByteBuffer *)malloc(sizeof(gxByteBuffer));
            bb_init(con.settings.cipher.dedicatedKey);
            bb_addHexString(con.settings.cipher.dedicatedKey, optarg);
            if (con.settings.cipher.dedicatedKey->size != 16)
            {
                printf("Invalid dedicated key '%s'.", optarg);
                return 1;
            }
            break;
        case 'F':
            con.settings.cipher.invocationCounter = atol(optarg);
            break;
        case 'o':
            outputFile = optarg;
            break;
        case 'v':
            invocationCounter = optarg;
#if defined(_WIN32) || defined(_WIN64) // Windows
            if ((ret = sscanf_s(optarg, "%d.%d.%d.%d.%d.%d", &a, &b, &c, &d, &e, &f)) != 6)
#else
            if ((ret = sscanf(optarg, "%d.%d.%d.%d.%d.%d", &a, &b, &c, &d, &e, &f)) != 6)
#endif
            {
                ShowHelp();
                return 1;
            }
            break;
        case 'g':
            // Get (read) selected objects.
            p = optarg;
            do
            {
                if (p != optarg)
                {
                    ++p;
                }
#if defined(_WIN32) || defined(_WIN64) // Windows
                if ((ret = sscanf_s(p, "%d.%d.%d.%d.%d.%d:%d", &a, &b, &c, &d, &e, &f, &index)) != 7)
#else
                if ((ret = sscanf(p, "%d.%d.%d.%d.%d.%d:%d", &a, &b, &c, &d, &e, &f, &index)) != 7)
#endif
                {
                    ShowHelp();
                    return 1;
                }
            } while ((p = strchr(p, ',')) != NULL);
            readObjects = optarg;
            break;
        case 'S':
            serialPort = optarg;
            break;
        case 'a':
            if (strcasecmp("None", optarg) == 0)
            {
                con.settings.authentication = DLMS_AUTHENTICATION_NONE;
            }
            else if (strcasecmp("Low", optarg) == 0)
            {
                con.settings.authentication = DLMS_AUTHENTICATION_LOW;
            }
            else if (strcasecmp("High", optarg) == 0)
            {
                con.settings.authentication = DLMS_AUTHENTICATION_HIGH;
            }
            else if (strcasecmp("HighMd5", optarg) == 0)
            {
                con.settings.authentication = DLMS_AUTHENTICATION_HIGH_MD5;
            }
            else if (strcasecmp("HighSha1", optarg) == 0)
            {
                con.settings.authentication = DLMS_AUTHENTICATION_HIGH_SHA1;
            }
            else if (strcasecmp("HighGmac", optarg) == 0)
            {
                con.settings.authentication = DLMS_AUTHENTICATION_HIGH_GMAC;
            }
            else if (strcasecmp("HighSha256", optarg) == 0)
            {
                con.settings.authentication = DLMS_AUTHENTICATION_HIGH_SHA256;
            }
            else
            {
                printf("Invalid Authentication option. (None, Low, High, HighMd5, HighSha1, HighGmac, HighSha256)\n");
                return 1;
            }
            break;
        case 'c':
            con.settings.clientAddress = atoi(optarg);
            break;
        case 's':
            con.settings.serverAddress = atoi(optarg);
            break;
        case 'l':
            con.settings.serverAddress = cl_getServerAddress(atoi(optarg), (unsigned short)con.settings.serverAddress, 0);
            break;
        case 'n':
            // TODO: Add support for serial number. con.settings.serverAddress = cl_getServerAddress(atoi(optarg));
            break;
        case 'm':
            con.settings.plcSettings.macDestinationAddress = atoi(optarg);
            break;
        case 'R':
            con.settings.cipher.broadcast = 1;
            if (strcasecmp("UnConfirmed", optarg) == 0)
            {
                con.settings.serviceClass = DLMS_SERVICE_CLASS_UN_CONFIRMED;
            }
            else if (strcasecmp("Confirmed", optarg) == 0)
            {
                con.settings.serviceClass = DLMS_SERVICE_CLASS_CONFIRMED;
            }
            else
            {
                printf("Invalid Broadcast type option. (UnConfirmed or Confirmed)\n");
                return 1;
            }
            break;
        case '?':
        {
            if (optarg[0] == 'c')
            {
                printf("Missing mandatory client option.\n");
            }
            else if (optarg[0] == 's')
            {
                printf("Missing mandatory server option.\n");
            }
            else if (optarg[0] == 'h')
            {
                printf("Missing mandatory host name option.\n");
            }
            else if (optarg[0] == 'p')
            {
                printf("Missing mandatory port option.\n");
            }
            else if (optarg[0] == 'r')
            {
                printf("Missing mandatory reference option.\n");
            }
            else if (optarg[0] == 'a')
            {
                printf("Missing mandatory authentication option.\n");
            }
            else if (optarg[0] == 'S')
            {
                printf("Missing mandatory Serial port option.\n");
            }
            else if (optarg[0] == 'g')
            {
                printf("Missing mandatory OBIS code option.\n");
            }
            else if (optarg[0] == 'C')
            {
                printf("Missing mandatory Ciphering option.\n");
            }
            else if (optarg[0] == 'v')
            {
                printf("Missing mandatory invocation counter logical name option.\n");
            }
            else if (optarg[0] == 'T')
            {
                printf("Missing mandatory system title option.");
            }
            else if (optarg[0] == 'A')
            {
                printf("Missing mandatory authentication key option.");
            }
            else if (optarg[0] == 'B')
            {
                printf("Missing mandatory block cipher key option.");
            }
            else if (optarg[0] == 'D')
            {
                printf("Missing mandatory dedicated key option.");
            }
            else
            {
                return 1;
            }
        }
        break;
        default:
            return 1;
        }
    }

    if (port != 0 || address != NULL)
    {
        if (port == 0)
        {
            printf("Missing mandatory port option.\n");
            return 1;
        }
        if (address == NULL)
        {
            printf("Missing mandatory host option.\n");
            return 1;
        }
        ret = readTcpIpConnection(&con, address, port, readObjects, invocationCounter, outputFile);
    }
    else if (serialPort != NULL)
    {
        printf("-------------------------------------- Reading serial port -------------------------------\n");
        ret = readSerialPort(&con, serialPort, readObjects, invocationCounter, outputFile);
    }
    else
    {
        printf("Missing mandatory connection information for TCP/IP or serial port connection.\n");
        return 1;
    }

    // Clear objects.
    if (ret != 0)
    {
        printf("%s\n", hlp_getErrorMessage(ret));
    }
    else
    {
        printf("All items are read.\n");
    }
    cl_clear(&con.settings);
    return 0;
}
//////////////////////////////////////////////////////////////////// PRIYA ///////////////////////////////////////////////////////////
// int config_parameter()
// {
//     connection con;
//     gxByteBuffer item;
//     bb_init(&item);
//     con_init(&con, GX_TRACE_LEVEL_INFO);
//     cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);
//     int ret, opt = 0;
//     int port = 0;
//     char *address = NULL;
//     char *serialPort = NULL;
//     char *p, *readObjects = NULL, *outputFile = NULL;
//     int index, a, b, c, d, e, f;
//     char *invocationCounter = NULL;

//     serialPort = "/dev/ttymxc2";
//     // address = "192.168.10.13";
//     // port = 4059;
//     // serialPort = "/dev/ttyS1";
//     con.settings.clientAddress = 32;
//     con.settings.authentication = DLMS_AUTHENTICATION_LOW;
//     // con.settings.interfaceType = DLMS_INTERFACE_TYPE_WRAPPER;

//     // con.settings.serverAddress = 7553;
//     con.settings.serverAddress = 25;
//     con.settings.serverAddress = cl_getServerAddress(1, (unsigned short)con.settings.serverAddress, 0);
//     con.settings.cipher.security = DLMS_SECURITY_NONE;

//     // con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION_ENCRYPTION;

//     // bb_clear(&con.settings.cipher.systemTitle);
//     // bb_addHexString(&con.settings.cipher.systemTitle, "474F453030303030");

//     // bb_clear(&con.settings.cipher.authenticationKey);
//     // bb_addHexString(&con.settings.cipher.authenticationKey, "41654D6C456B416B6761506C30316162");

//     // bb_clear(&con.settings.cipher.blockCipherKey);
//     // bb_addHexString(&con.settings.cipher.blockCipherKey, "41654D6C456B416B6761506C30316162");

//     bb_init(&con.settings.password);

//     bb_addString(&con.settings.password, "1A2B3C4D");

//     // bb_addString(&con.settings.password, "ABCD0001");
//     // bb_addString(&con.settings.password, "lnt1");

//     con.trace = GX_TRACE_LEVEL_VERBOSE;

//     if (port != 0 || address != NULL)
//     {
//         printf("-------------------------------------- Reading TCP -------------------------------\n");
//         if (port == 0)
//         {
//             printf("Missing mandatory port option.\n");
//             return 1;
//         }
//         if (address == NULL)
//         {
//             printf("Missing mandatory host option.\n");
//             return 1;
//         }
//         ret = readTcpIpConnection(&con, address, port, readObjects, invocationCounter, outputFile);
//     }
//     else if (serialPort != NULL)
//     {
//         printf("-------------------------------------- Reading serial port -------------------------------\n");
//         ret = readSerialPort(&con, serialPort, readObjects, invocationCounter, outputFile);
//     }
//     else
//     {
//         printf("Missing mandatory connection information for TCP/IP or serial port connection.\n");
//         return 1;
//     }

//     // Clear objects.
//     if (ret != 0)
//     {
//         printf("%s\n", hlp_getErrorMessage(ret));
//     }
//     else
//     {
//         printf("All items are read.\n");
//     }
//     cl_clear(&con.settings);
//     return 0;
// }

// int obis_parsing(char *text, int type)
// {
//     int count = -1, i = 0;
//     // char text[] = "Index: 2 Value: [\n]\nIndex: 3 Value: [Data 0.0.96.1.0.255, Data 0.0.96.1.2.255, Data 0.0.96.1.1.255, Data 1.0.0.2.0.255, Data 0.0.94.91.9.255, Data 0.0.94.91.11.255, Data 0.0.94.91.12.255, Data 0.0.96.1.4.255, Data 1.0.0.3.0.255]\nIndex: 4 Value: 0\nIndex: 5 Value: 0\nIndex: 6 Value:\nIndex: 7 Value: 0\nIndex: 8 Value: 0";

//     char *start = strstr(text, "Index: 3 Value: [") + strlen("Index: 3 Value: [");
//     if (start == NULL)
//     {
//         printf("No data found within square brackets.\n");
//         return 1;
//     }

//     char *end = strstr(start, "]");
//     if (end == NULL)
//     {
//         printf("No data found within square brackets.\n");
//         return 1;
//     }

//     *end = '\0';

//     printf("Data: %s\n", start);
//     char *token = strtok(start, " ");

//     while (token != NULL)
//     {

//         token = strtok(NULL, ", ");

//         count++;
//         // printf("parse : %s count %d\n",token, count);
//         if (count % 2 == 0)
//         {
//             if (type == NAME_PLATE)
//             {
//                 nameplate_obis.obis[nameplate_obis.tot_obis][20];
//                 strcpy(nameplate_obis.obis[i], token);
//                 printf(" token :  '%s' %d\n", nameplate_obis.obis[i], i);
//             }
//             if (type == INST)
//             {
//                 inst_obis.obis[inst_obis.tot_obis][20];
//                 strcpy(inst_obis.obis[i], token);
//                 printf(" token :  '%s' %d\n", inst_obis.obis[i], i);
//             }

//             i++;
//         }
//     }

//     return 0;
// }
// int countOBISCodes(const char *str, int type)
// {
//     const char *index3Start = strstr(str, "Index: 3");
//     if (!index3Start)
//         return 0;

//     const char *start = strstr(index3Start, "Value: [");
//     if (!start)
//         return 0;

//     const char *end = strstr(start, "]");
//     if (!end)
//         return 0;

//     int count = 0;
//     for (const char *ptr = start; ptr < end; ++ptr)
//     {
//         if (*ptr == ',')
//         {
//             ++count;
//         }
//     }
//     if (type == NAME_PLATE)
//     {
//         nameplate_obis.tot_obis = count + 1;
//     }
//     if (type == INST)
//     {
//         inst_obis.tot_obis = count + 1;
//     }
//     return count + 1;
// }
// int read_ls(connection *connection)
// {
//     gxByteBuffer ba;
//     date_time_t g_st_date_time, g_end_date_time;
//     char *data = NULL;
//     int ret = -1;

//     printf("READ LS...................\n");
//     gxProfileGeneric pg;
//     gxtime strt;
//     gxtime nd;
//     int count;
//     int idx = 0;

//     for (idx = 0; idx < 2; idx++)
//     {
//         poll_start_time = time(NULL);
//         poll_start_str = ctime(&poll_start_time);
//         poll_start_str[strlen(poll_start_str) - 1] = '\0';

//         if (idx == 0)
//         {
//             memset(poll_details, 0, sizeof(poll_details));
//             sprintf(poll_details, "LS_2BLOCK(15mins)\t\t%s\t\t%s\t\t", "Genus", poll_start_str);
//             polling_time_clc_file(poll_details);
//         }
//         else
//         {
//             memset(poll_details, 0, sizeof(poll_details));
//             sprintf(poll_details, "LS_1DAY(15mins)\t\t%s\t\t%s\t\t", "Genus", poll_start_str);
//             polling_time_clc_file(poll_details);
//         }

//         time_now(&strt);
//         time_now(&nd);
//         int desired_year = 2024;
//         int desired_month = 8; // May
//         int desired_day = 11;

//         if (idx == 0)
//         {

//             g_st_date_time.year = desired_year;
//             g_st_date_time.month = desired_month;
//             g_st_date_time.day = desired_day;
//             g_st_date_time.hour = 0;
//             g_st_date_time.minute = 0;

//             g_end_date_time.year = desired_year;
//             g_end_date_time.month = desired_month;
//             g_end_date_time.day = desired_day;
//             g_end_date_time.hour = 0;
//             g_end_date_time.minute = 15;
//         }
//         else
//         {
//             g_st_date_time.year = desired_year;
//             g_st_date_time.month = desired_month;
//             g_st_date_time.day = desired_day;
//             g_st_date_time.hour = 0;
//             g_st_date_time.minute = 0;

//             g_end_date_time.year = desired_year;
//             g_end_date_time.month = desired_month;
//             g_end_date_time.day = desired_day;
//             g_end_date_time.hour = 23;
//             g_end_date_time.minute = 59;
//         }

//         strt.value.tm_year = g_st_date_time.year - 1900; // Adjust for struct tm format
//         strt.value.tm_mon = g_st_date_time.month - 1;    // Adjust for struct tm format
//         strt.value.tm_mday = g_st_date_time.day;
//         strt.value.tm_hour = g_st_date_time.hour;
//         strt.value.tm_min = g_st_date_time.minute;

//         nd.value.tm_year = g_end_date_time.year - 1900; // Adjust for struct tm format
//         nd.value.tm_mon = g_end_date_time.month - 1;    // Adjust for struct tm format
//         nd.value.tm_mday = g_end_date_time.day;
//         nd.value.tm_hour = g_end_date_time.hour;
//         nd.value.tm_min = g_end_date_time.minute;

//         // strt.value.tm_year =desired_year - 1900; // Adjust for struct tm format
//         // strt.value.tm_mon = desired_month - 1;    // Adjust for struct tm format
//         // strt.value.tm_mday = desired_day;
//         // strt.value.tm_hour = 0;
//         // strt.value.tm_min = 0;

//         // nd.value.tm_year = desired_year - 1900; // Adjust for struct tm format
//         // nd.value.tm_mon = desired_month - 1;    // Adjust for struct tm format
//         // nd.value.tm_mday = desired_day;
//         // nd.value.tm_hour = 23;
//         // nd.value.tm_min = 00;

//         // nd = strt;
//         // time_clearTime(&strt);

//         // time_addDays(&strt, -1); // no.of.days load survey

//         printf("start Year: %d, Month: %d, Day: %d\n",
//                strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday);
//         printf("end Year: %d, Month: %d, Day: %d\n",
//                nd.value.tm_year + 1900, nd.value.tm_mon + 1, nd.value.tm_mday);
//         cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, "1.0.99.1.0.255");
//         printf("READ LS 1.0.99.1.0.255...................\n");
//         com_read(connection, BASE(pg), 3);
//         if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         {
//             ret = obj_toString(&pg, &data);
//             if (ret != DLMS_ERROR_CODE_OK)
//             {
//                 return ret;
//             }
//             if (data != NULL)
//             {
//                 printf("%s\r\n", data);
//                 free(data);
//                 data = NULL;
//             }
//         }
//         // com_read(connection, BASE(pg), 2);
//         // ret = com_readValue(connection, BASE(pg), 2);
//         // if (ret != DLMS_ERROR_CODE_OK)
//         // {
//         //     printf("Failed to read object  rows by entry\r\n");
//         // }
//         // else
//         // {
//         //     printf("========================== REAding all data success=======================\n");
//         //     // if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         //     // {
//         //         bb_init(&ba);
//         //         obj_rowsToString(&ba, pg.buffer);
//         //         data = bb_toString(&ba);
//         //         bb_clear(&ba);
//         //         printf("###################################%s\r\n", data);
//         //         free(data);
//         //     // }
//         // }
//         ret = com_readRowsByRange(connection, &pg, &strt, &nd);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             printf("Failed to read object  rows by entry\r\n");
//         }
//         else
//         {
//             printf("========================== REAding com_readRowsByRange all data success=======================\n");
//             if (connection->trace > GX_TRACE_LEVEL_WARNING)
//             {
//                 bb_init(&ba);
//                 obj_rowsToString(&ba, &pg.buffer);
//                 data = bb_toString(&ba);
//                 bb_clear(&ba);
//                 printf("%s\r\n", data);
//                 free(data);
//             }
//         }
//         poll_end_time = time(NULL);
//         poll_end_str = ctime(&poll_end_time);
//         poll_end_str[strlen(poll_end_str) - 1] = '\0';

//         poll_diff = poll_end_time - poll_start_time;

//         poll_min = poll_diff / 60;
//         poll_sec = poll_diff % 60;

//         memset(poll_details, 0, sizeof(poll_details));
//         sprintf(poll_details, "%s\t\t%d:%d\n", poll_end_str, poll_min, poll_sec);
//         polling_time_clc_file(poll_details);
//     }

//     return 0;
// }
// int read_event(connection *connection)
// {
//     // char obis[18] = "0.0.99.98.0.255";
//     char obis[6][18] = {"0.0.99.98.0.255", "0.0.99.98.1.255", "0.0.99.98.2.255", "0.0.99.98.3.255", "0.0.99.98.4.255", "0.0.99.98.5.255"};
//     uint32_t st_event_num = 0, end_event_num = 0;
//     int attr = 3;
//     gxProfileGeneric *obj;
//     gxByteBuffer ba;
//     gxtime startTime, endTime;
//     char *data = NULL;
//     unsigned char buff[200];
//     int ret = -1;
//     int idx = 0;

//     for (idx = 0; idx < 6; idx++)
//     {
//         poll_start_time = time(NULL);
//         poll_start_str = ctime(&poll_start_time);
//         poll_start_str[strlen(poll_start_str) - 1] = '\0';

//         memset(poll_details, 0, sizeof(poll_details));
//         sprintf(poll_details, "EVENT_%d_10EVENTS\t\t%s\t\t%s\t\t", idx, "Genus", poll_start_str);
//         polling_time_clc_file(poll_details);

//         hlp_setLogicalName(buff, obis[idx]);
//         oa_findByLN(&connection->settings.objects, DLMS_OBJECT_TYPE_NONE, buff, &obj);
//         if (obj == NULL)
//         {
//             printf("Object '%s' not found from the association view.\n", obis[idx]);
//             return -1;
//         }

//         ret = com_read(connection, (gxObject *)obj, 7);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             if (connection->trace > GX_TRACE_LEVEL_OFF)
//             {
//                 printf("Failed to read object attribute index %d\r\n", 7);
//             }
//             // Do not clear objects list because it will free also objects from association view list.
//             // oa_empty(&objects);
//             return ret;
//         }

//         ret = com_read(connection, (gxObject *)obj, 5);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             if (connection->trace > GX_TRACE_LEVEL_OFF)
//             {
//                 printf("Failed to read object attribute index %d\r\n", 8);
//             }
//             // Do not clear objects list because it will free also objects from association view list.
//             // oa_empty(&objects);
//             return ret;
//         }

//         if (obj->entriesInUse > 10)
//         {
//             printf("condition success\n");
//             st_event_num = obj->entriesInUse - 10;
//             end_event_num = obj->entriesInUse;
//         }
//         else if (obj->entriesInUse < 1)
//         {
//             st_event_num = 1;
//             end_event_num = 0;
//         }

//         com_readValue(connection, (gxObject *)obj, attr);
//         ret = obj_toString(obj, &data);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             return ret;
//         }
//         if (data != NULL)
//         {
//             printf("Event data obis : %s\n", data);
//             free(data);
//             data = NULL;
//         }
//         // com_readValue(connection, (gxObject *)obj, 2);
//         printf("======================== entry in use : %d =================\n", obj->entriesInUse);
//         ret = com_readRowsByEntry(connection, obj, st_event_num, obj->entriesInUse);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             printf("Failed to read object  rows by entry\r\n");
//         }
//         else
//         {
//             if (connection->trace > GX_TRACE_LEVEL_WARNING)
//             {
//                 bb_init(&ba);
//                 obj_rowsToString(&ba, &obj->buffer);
//                 data = bb_toString(&ba);
//                 bb_clear(&ba);
//                 printf("###################################%s\r\n", data);
//                 free(data);
//             }
//         }
//         poll_end_time = time(NULL);
//         poll_end_str = ctime(&poll_end_time);
//         poll_end_str[strlen(poll_end_str) - 1] = '\0';

//         poll_diff = poll_end_time - poll_start_time;

//         poll_min = poll_diff / 60;
//         poll_sec = poll_diff % 60;

//         memset(poll_details, 0, sizeof(poll_details));
//         sprintf(poll_details, "%s\t\t%d:%d\n", poll_end_str, poll_min, poll_sec);
//         polling_time_clc_file(poll_details);
//     }
//     // gxByteBuffer ba;
//     // char *data = NULL;
//     // int ret = -1;

//     // printf("READ LS...................\n");
//     // gxProfileGeneric pg;
//     // gxtime strt;
//     // gxtime nd;

//     // time_now(&strt);
//     // nd = strt;
//     // time_clearTime(&strt);
//     // time_addDays(&strt, -10); // no.of.days load survey
//     // cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, "0.0.99.98.1.255");
//     // printf("READ event 0.0.99.98.1.255...................\n");
//     // com_read(connection, BASE(pg), 3);
//     // com_read(connection, BASE(pg), 2);
//     // ret = com_readRowsByRange(connection, &pg, &strt, &nd);
//     // if (ret != DLMS_ERROR_CODE_OK)
//     // {
//     //     printf("Failed to read object  rows by entry\r\n");
//     // }
//     // else
//     // {
//     //     if (connection->trace > GX_TRACE_LEVEL_WARNING)
//     //     {
//     //         bb_init(&ba);
//     //         obj_rowsToString(&ba, &pg.buffer);
//     //         data = bb_toString(&ba);
//     //         bb_clear(&ba);
//     //         printf("%s\r\n", data);
//     //         free(data);
//     //     }
//     // }

//     return 0;
// }
// int read_midnight(connection *connection)
// {
//     char obis[18] = "1.0.99.2.0.255";
//     int attr = 3;
//     gxProfileGeneric pg;
//     gxByteBuffer ba;
//     gxtime startTime, endTime;
//     char *data = NULL;
//     unsigned char buff[200];
//     int ret = -1;
//     // hlp_setLogicalName(buff, obis);
//     // oa_findByLN(&connection->settings.objects, DLMS_OBJECT_TYPE_NONE, buff, &obj);

//     poll_start_time = time(NULL);
//     poll_start_str = ctime(&poll_start_time);
//     poll_start_str[strlen(poll_start_str) - 1] = '\0';

//     memset(poll_details, 0, sizeof(poll_details));
//     sprintf(poll_details, "MID_NIGHT_60DAY\t\t%s\t\t%s\t\t", "Genus", poll_start_str);
//     polling_time_clc_file(poll_details);

//     cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, "1.0.99.2.0.255");

//     // if (obj == NULL)
//     // {
//     //     printf("Object '%s' not found from the association view.\n", obis);
//     //     return -1;
//     // }
//     // com_readValue(connection, (gxObject *)obj, attr);
//     // com_readValue(connection, (gxObject *)obj, 2);
//     // ret = com_readRowsByEntry(connection, obj,1, 1);
//     // if (ret != DLMS_ERROR_CODE_OK)
//     // {
//     //     printf("Failed to read object  rows by entry\r\n");
//     // }
//     // else
//     // {
//     //     if (connection->trace > GX_TRACE_LEVEL_WARNING)
//     //     {
//     //         bb_init(&ba);
//     //         obj_rowsToString(&ba, &obj->buffer);
//     //         data = bb_toString(&ba);
//     //         bb_clear(&ba);
//     //         printf("###################################%s\r\n", data);
//     //         free(data);
//     //     }
//     // }

//     gxtime strt;
//     int count;
//     gxtime nd;
//     time_t curr_time = 0, st_time_in_sec = 0, end_time_in_sec = 0;
//     curr_time = time(NULL);
//     // time_now(&strt);
//     st_time_in_sec = curr_time - OLD_DATA_FILES_TIME;
//     end_time_in_sec = curr_time;
//     struct tm *timeinfo = localtime(&st_time_in_sec);
//     strt.value = *timeinfo;
//     printf("Year: %d, Month: %d, Day: %d\n",
//            strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday);

//     struct tm *end_timeinfo = localtime(&end_time_in_sec);
//     nd.value = *end_timeinfo;
//     // nd = strt;

//     // time_clearTime(&strt);
//     com_read(connection, BASE(pg), 3);
//     if (connection->trace > GX_TRACE_LEVEL_WARNING)
//     {
//         ret = obj_toString(&pg, &data);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             return ret;
//         }
//         if (data != NULL)
//         {
//             printf("###################################%s\r\n", data);
//             free(data);
//             data = NULL;
//         }
//     }
//     // com_read(connection, BASE(pg), 2);
//     ret = com_readRowsByRange(connection, &pg, &strt, &nd);

//     poll_end_time = time(NULL);
//     poll_end_str = ctime(&poll_end_time);
//     poll_end_str[strlen(poll_end_str) - 1] = '\0';

//     poll_diff = poll_end_time - poll_start_time;

//     poll_min = poll_diff / 60;
//     poll_sec = poll_diff % 60;

//     memset(poll_details, 0, sizeof(poll_details));
//     sprintf(poll_details, "%s\t\t%d:%d\n", poll_end_str, poll_min, poll_sec);
//     polling_time_clc_file(poll_details);

//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         printf("Failed to read object  rows by entry\r\n");
//     }
//     else
//     {
//         if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         {
//             bb_init(&ba);
//             obj_rowsToString(&ba, &pg.buffer);
//             data = bb_toString(&ba);
//             bb_clear(&ba);
//             printf("###################################%s\r\n", data);
//             free(data);
//         }
//     }

//     return 0;
// }
// int read_billing(connection *connection)
// {
//     char obis[18] = "1.0.98.1.0.255";
//     int attr = 3;
//     gxProfileGeneric *obj;
//     gxByteBuffer ba;
//     objectArray objects;
//     gxtime startTime, endTime;
//     char *data = NULL;
//     unsigned char buff[200];
//     int ret = -1;
//     int idx = 0;

//     for (idx = 0; idx < 2; idx++)
//     {
//         poll_start_time = time(NULL);
//         poll_start_str = ctime(&poll_start_time);
//         poll_start_str[strlen(poll_start_str) - 1] = '\0';

//         if (idx == 0)
//         {
//             memset(poll_details, 0, sizeof(poll_details));
//             sprintf(poll_details, "BILLING_2_MONTH\t\t%s\t\t%s\t\t", "Genus", poll_start_str);
//             polling_time_clc_file(poll_details);
//         }
//         else
//         {
//             memset(poll_details, 0, sizeof(poll_details));
//             sprintf(poll_details, "BILLING_12_MONTH\t\t%s\t\t%s\t\t", "Genus", poll_start_str);
//             polling_time_clc_file(poll_details);
//         }

//         hlp_setLogicalName(buff, obis);
//         oa_findByLN(&connection->settings.objects, DLMS_OBJECT_TYPE_NONE, buff, &obj);
//         if (obj == NULL)
//         {
//             printf("Object '%s' not found from the association view.\n", obis);
//             return -1;
//         }
//         com_readValue(connection, (gxObject *)obj, attr);
//         ret = obj_toString(obj, &data);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             return ret;
//         }
//         if (data != NULL)
//         {
//             printf("Billing data obis : %s\n", data);
//             free(data);
//             data = NULL;
//         }
//         // com_readValue(connection, (gxObject *)obj, 2);
//         // ret = com_read(connection, (gxObject *)obj, 7);
//         // if (ret != DLMS_ERROR_CODE_OK)
//         // {
//         //     if (connection->trace > GX_TRACE_LEVEL_OFF)
//         //     {
//         //         printf("Failed to read object attribute index %d\r\n", 7);
//         //     }
//         //     // Do not clear objects list because it will free also objects from association view list.
//         //     oa_empty(&objects);
//         //     return ret;
//         // }
//         // // Read entries.
//         // ret = com_read(connection, (gxObject *)obj, 8);
//         // if (ret != DLMS_ERROR_CODE_OK)
//         // {
//         //     if (connection->trace > GX_TRACE_LEVEL_OFF)
//         //     {
//         //         printf("Failed to read object attribute index %d\r\n", 8);
//         //     }
//         //     // Do not clear objects list because it will free also objects from association view list.
//         //     oa_empty(&objects);
//         //     return ret;
//         // }
//         // if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         // {
//         //     printf("Entries: %ld/%ld\r\n", obj->entriesInUse, obj->profileEntries);
//         // }
//         // printf("entries : %u\n", obj->entriesInUse);
//         // ret = com_readValue(connection, (gxObject *)obj, 2);
//         // if (ret != DLMS_ERROR_CODE_OK)
//         // {
//         //     printf("Failed to read object  rows by entry\r\n");
//         // }
//         // else
//         // {
//         //     printf("========================== REAding all data success=======================\n");
//         //     // if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         //     // {
//         //     bb_init(&ba);
//         //     obj_rowsToString(&ba, &obj->buffer);
//         //     data = bb_toString(&ba);
//         //     bb_clear(&ba);
//         //     printf("###################################%s\r\n", data);
//         //     free(data);
//         //     // }
//         // }
//         if (idx == 0)
//         {
//             ret = com_readRowsByEntry(connection, obj, 1, 2);
//             if (ret != DLMS_ERROR_CODE_OK)
//             {
//                 printf("Failed to read object  rows by entry\r\n");
//             }
//             else
//             {
//                 printf("========================== REAding rows by entry all data success=======================\n");
//                 if (connection->trace > GX_TRACE_LEVEL_WARNING)
//                 {
//                     bb_init(&ba);
//                     obj_rowsToString(&ba, &obj->buffer);
//                     data = bb_toString(&ba);
//                     bb_clear(&ba);
//                     printf("###################################%s\r\n", data);
//                     free(data);
//                 }
//             }
//         }
//         else
//         {
//             ret = com_readRowsByEntry(connection, obj, 1, 12);
//             if (ret != DLMS_ERROR_CODE_OK)
//             {
//                 printf("Failed to read object  rows by entry\r\n");
//             }
//             else
//             {
//                 printf("========================== REAding rows by entry all data success=======================\n");
//                 if (connection->trace > GX_TRACE_LEVEL_WARNING)
//                 {
//                     bb_init(&ba);
//                     obj_rowsToString(&ba, &obj->buffer);
//                     data = bb_toString(&ba);
//                     bb_clear(&ba);
//                     printf("###################################%s\r\n", data);
//                     free(data);
//                 }
//             }
//         }

//         poll_end_time = time(NULL);
//         poll_end_str = ctime(&poll_end_time);
//         poll_end_str[strlen(poll_end_str) - 1] = '\0';

//         poll_diff = poll_end_time - poll_start_time;

//         poll_min = poll_diff / 60;
//         poll_sec = poll_diff % 60;

//         memset(poll_details, 0, sizeof(poll_details));
//         sprintf(poll_details, "%s\t\t%d:%d\n", poll_end_str, poll_min, poll_sec);
//         polling_time_clc_file(poll_details);
//     }

//     return 0;
// }
// int read_inst(connection *connection)
// {
//     char obis[18] = "1.0.94.91.0.255";
//     int attr = 3;
//     int count = 0;
//     gxProfileGeneric *obj;
//     gxByteBuffer ba;
//     char *data = NULL;
//     unsigned char buff[200];
//     int ret = -1;

//     poll_start_time = time(NULL);
//     poll_start_str = ctime(&poll_start_time);
//     poll_start_str[strlen(poll_start_str) - 1] = '\0';

//     memset(poll_details, 0, sizeof(poll_details));
//     sprintf(poll_details, "INST\t\t%s\t\t%s\t\t", "Genus", poll_start_str);
//     polling_time_clc_file(poll_details);

//     hlp_setLogicalName(buff, obis);
//     oa_findByLN(&connection->settings.objects, DLMS_OBJECT_TYPE_NONE, buff, &obj);
//     if (obj == NULL)
//     {
//         printf("Object '%s' not found from the association view.\n", obis);
//         return -1;
//     }
//     com_readValue(connection, (gxObject *)obj, attr);
//     if (connection->trace > GX_TRACE_LEVEL_WARNING)
//     {
//         ret = obj_toString(obj, &data);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             return ret;
//         }
//         if (data != NULL)
//         {
//             // count = countOBISCodes(data, INST);
//             // printf("NO.OF OBIS : %d\n", count);
//             // obis_parsing(data, INST);
//             free(data);
//             data = NULL;
//         }
//     }
//     ret = com_readValue(connection, (gxObject *)obj, 2);
//     printf("========================== REAding data : %d=======================\n", ret);
//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         printf("Failed to read object  rows by entry\r\n");
//     }
//     else
//     {
//         printf("========================== REAding data all success=======================\n");
//         // if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         bb_init(&ba);
//         obj_rowsToString(&ba, &obj->buffer);
//         data = bb_toString(&ba);
//         bb_clear(&ba);
//         printf("###################################%s\r\n", data);
//         free(data);
//     }

//     poll_end_time = time(NULL);
//     poll_end_str = ctime(&poll_end_time);
//     poll_end_str[strlen(poll_end_str) - 1] = '\0';

//     poll_diff = poll_end_time - poll_start_time;

//     poll_min = poll_diff / 60;
//     poll_sec = poll_diff % 60;

//     memset(poll_details, 0, sizeof(poll_details));
//     sprintf(poll_details, "%s\t\t%d:%d\n", poll_end_str, poll_min, poll_sec);
//     polling_time_clc_file(poll_details);

//     ret = com_readRowsByEntry(connection, obj, 1, 1);
//     printf("========================== REAding data : %d=======================\n", ret);
//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         printf("Failed to read object  rows by entry\r\n");
//     }
//     else
//     {
//         printf("========================== REAding data success=======================\n");
//         // if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         bb_init(&ba);
//         obj_rowsToString(&ba, &obj->buffer);
//         data = bb_toString(&ba);
//         bb_clear(&ba);
//         printf("###################################%s\r\n", data);
//         free(data);
//     }
//     return 0;
// }

// int read_nameplate(connection *connection)
// {
//     char obis[18] = "0.0.94.91.10.255";
//     int attr = 3;
//     int count = 0;
//     gxProfileGeneric *obj;
//     gxByteBuffer ba;
//     char *data = NULL;
//     unsigned char buff[200];
//     int ret = -1;
//     hlp_setLogicalName(buff, obis);
//     oa_findByLN(&connection->settings.objects, DLMS_OBJECT_TYPE_NONE, buff, &obj);
//     if (obj == NULL)
//     {
//         printf("Object '%s' not found from the association view.\n", obis);
//         return -1;
//     }
//     com_readValue(connection, (gxObject *)obj, attr);
//     if (connection->trace > GX_TRACE_LEVEL_WARNING)
//     {
//         ret = obj_toString(obj, &data);
//         if (ret != DLMS_ERROR_CODE_OK)
//         {
//             return ret;
//         }
//         if (data != NULL)
//         {
//             // count = countOBISCodes(data, NAME_PLATE);
//             // printf("NO.OF OBIS : %d\n", count);
//             // obis_parsing(data, NAME_PLATE);
//             free(data);
//             data = NULL;
//         }
//     }
//     // obj_ArbitratorToString((gxArbitrator*)obj,&data);
//     // printf("DATA : %s\n",data);

//     printf("========================== REAding data =======================\n");

//     ret = com_readValue(connection, (gxObject *)obj, 2);
//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         printf("Failed to read object  rows by entry\r\n");
//     }
//     else
//     {
//         printf("========================== REAding all data success=======================\n");
//         // if (connection->trace > GX_TRACE_LEVEL_WARNING)
//         // {
//         bb_init(&ba);
//         obj_rowsToString(&ba, &obj->buffer);
//         data = bb_toString(&ba);
//         bb_clear(&ba);
//         printf("###################################%s\r\n", data);
//         free(data);
//         // }
//     }
//     // ret = com_readRowsByEntry(connection, obj, 1, 1);
//     // printf("========================== REAding data : %d=======================\n",ret);
//     // if (ret != DLMS_ERROR_CODE_OK)
//     // {
//     //     printf("Failed to read object  rows by entry\r\n");
//     // }
//     // else
//     // {
//     //     printf("========================== REAding data success=======================\n");
//     //     // if (connection->trace > GX_TRACE_LEVEL_WARNING)
//     //     // {
//     //         bb_init(&ba);
//     //         obj_rowsToString(&ba, &obj->buffer);
//     //         data = bb_toString(&ba);
//     //         bb_clear(&ba);
//     //         printf("###################################%s\r\n", data);
//     //         free(data);
//     //     // }
//     // }
//     return 0;
// }
// int single_obis_read(connection *connection)
// {
//     int ret;
//     unsigned char logicalname_dn[] = {1, 0, 0, 8, 4, 255}; // device name
//     // Read - Device Name
//     gxData *_deviceName = (gxData *)malloc(sizeof(gxData));
//     INIT_OBJECT((*_deviceName), DLMS_OBJECT_TYPE_DATA, logicalname_dn);
//     ret = com_read(connection, &_deviceName->base, 2);
//     if (ret != DLMS_ERROR_CODE_OK)
//     {
//         return ret;
//     }
//     printf("\n Device Name: %d %d", _deviceName->value.vt, _deviceName->value.iVal);
//     // Value is "ISK0070777807016ýýýý"
// }
// /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// int main(int argc, char *argv[])
// {
//     // FILE *fpt;

//     // fpt = fopen("/usr/cms/log/yitran_polling_time_calc.csv", "w");
//     // if (fpt != NULL)
//     // {

//     // 	fprintf(fpt, "COMMAND_TYPE\t\tMETER_TYPE\t\tPOLL_START_TIME\t\tPOLL_END_TIME\t\tPOLL_TIME_TAKEN\n");

//     // 	fclose(fpt);
//     // }
// #if COMMENT
//     int ret = connectMeter(argc, argv);
//     if (ret != 0)
//     {
//         ShowHelp();
//     }
// #endif
// #if CMS
//     int ret = config_parameter();
// #endif
// #if defined(_WIN32) || defined(_WIN64) // Windows
//     WSACleanup();
// #if _MSC_VER > 1000
//     // Show memory leaks.
//     _CrtDumpMemoryLeaks();
// #endif //_MSC_VER
// #endif
//     return ret;
// }
