#define DATETIME_BUFFER_SIZE 12

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <strings.h>

#include "../include/meter_params.h"
#include "../include/communication.h"
#include "../include/general_config.h"

typedef unsigned char uint8_t;

extern connection con;
extern int curr_nc_idx;
extern char temp_json[2048];

int set_clock_time(int midx, uint16_t year, uint8_t month, uint8_t day, uint8_t hour, uint8_t minute, uint8_t second, char *out_buf)
{

    static char fun_name[] = "set_clock_time()";

    int ret;
    gxClock clock;
    curr_nc_idx = midx;

    printf("time %u %u %u %u %u %u\n", year, month, day, hour, minute, second);

    ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name ,(hlp_getErrorMessage(ret)));
        return -1;
    }

    unsigned char ln[] = {0, 0, 1, 0, 0, 255};
    INIT_OBJECT(clock, DLMS_OBJECT_TYPE_CLOCK, ln);

    time_init(&clock.time, year, month, day, hour, minute, second, 0, 0x8000);

    ret = com_write(&con, &clock.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_clock_time write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &clock.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    char buff[50];
    if ((ret = time_toString2(&clock.time, buff, 50)) != 0)
    {
        return ret;
    }

    strcpy(out_buf, buff);
    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_demand_period(int midx, int val, char *out_buf)
{
    static char fun_name[] = "set_demand_period()";

    gxData demand_interval_d;

    curr_nc_idx = midx;

    unsigned char ln[] = {1, 0, 0, 8, 0, 255};

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    INIT_OBJECT(demand_interval_d, DLMS_OBJECT_TYPE_DATA, ln);

      ret = com_read(&con, &demand_interval_d.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
 
    GX_UINT16(demand_interval_d.value) = val;

    ret = com_write(&con, &demand_interval_d.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_demand_period write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &demand_interval_d.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", demand_interval_d.value.uiVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_profile_capture_period(int midx, int capture_period, char *out_buf)
{
    static char fun_name[] = "set_profile_capture_period()";

    gxData block_interval_d;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n",midx, fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(block_interval_d), DLMS_OBJECT_TYPE_DATA, CAPTURE_PERIOD_OBIS);

    GX_UINT16(block_interval_d.value) = capture_period;

    ret = com_write(&con, BASE(block_interval_d), CAPTURE_PERIOD_ATTR_INDEX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_profile_capture_period write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &block_interval_d.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", block_interval_d.value.uiVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_metering_mode(int midx, int metering_mode, char *out_buf)
{
    static char fun_name[] = "set_metering_mode()";

    gxData meter_mode_d;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx,fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(meter_mode_d), DLMS_OBJECT_TYPE_DATA, METERING_MODE_OBIS);

    GX_UINT8(meter_mode_d.value) = metering_mode;

    ret = com_write(&con, BASE(meter_mode_d), METERING_MODE_ATTR_INDEX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &meter_mode_d.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", meter_mode_d.value.boolVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_payment_mode(int midx, int payment_mode, char *out_buf)
{
    static char fun_name[] = "set_payment_mode()";

    gxData payment_mode_d;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n",midx, fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(payment_mode_d), DLMS_OBJECT_TYPE_DATA, PAYMENT_MODE_OBIS);

    GX_UINT8(payment_mode_d.value) = payment_mode;

    ret = com_write(&con, BASE(payment_mode_d), PAYMENT_MODE_ATTR_INDEX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_payment_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &payment_mode_d.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", payment_mode_d.value.boolVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_lst_token_recharge_amt(int midx, int recharge_amt, char *out_buf)
{

    static char fun_name[] = "set_lst_token_recharge_amt()";

    gxData rc_amt_d;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(rc_amt_d), DLMS_OBJECT_TYPE_DATA, LAST_TKN_RECHARGE_AMT_OBIS);

    GX_INT32(rc_amt_d.value) = recharge_amt;

    ret = com_write(&con, BASE(rc_amt_d), LAST_TKN_RECHARGE_AMT_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &rc_amt_d.base, 2);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", rc_amt_d.value.ulVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_tot_amt_lst_recharge(int midx, int tot_amt, char *out_buf)
{
    static char fun_name[] = "set_tot_amt_lst_recharge()";

    gxData tol_amt_d;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n",midx, fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(tol_amt_d), DLMS_OBJECT_TYPE_DATA, TOT_AMT_LST_RECHARGE_OBIS);

    GX_INT32(tol_amt_d.value) = tot_amt;

    ret = com_write(&con, BASE(tol_amt_d), TOT_AMT_LST_RECHARGE_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &tol_amt_d.base, 2);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", tol_amt_d.value.ulVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_curr_bal_amt(int midx, int bal_amt, char *out_buf)
{
    static char fun_name[] = "set_curr_bal_amt()";

    gxData bal_amt_d;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(bal_amt_d), DLMS_OBJECT_TYPE_DATA, CUR_BAL_AMT_OBIS);

    GX_INT32(bal_amt_d.value) = bal_amt;

    ret = com_write(&con, BASE(bal_amt_d), CUR_BAL_AMT_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &bal_amt_d.base, 2);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", bal_amt_d.value.ulVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_relay_op_overload_overcurr(int midx, int relay_val, char *out_buf)
{
    static char fun_name[] = "set_relay_op_overload_overcurr()";

    gxData ol_oc_d;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(ol_oc_d), DLMS_OBJECT_TYPE_DATA, RELAY_OP_OVERLOAD_OVERCURR_OBIS);

    GX_UINT8(ol_oc_d.value) = relay_val;

    ret = com_write(&con, BASE(ol_oc_d), RELAY_OP_OVERLOAD_OVERCURR_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &ol_oc_d.base, 2);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", ol_oc_d.value.boolVal);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_eswf(int midx, char *str, char *out_buf)
{

    static char fun_name[] = "set_eswf()";

    printf("============ start of reading set_eswf =============\n");

    gxData obj;
    char *data = NULL;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx,fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_DATA, ESWF_OBIS);
    ret = com_read(&con, &obj.base, ESWF_ATTR_IDX);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    printf("TYPE OF THE VALUE :  %d\n", obj.value.vt);

    ret = obj_toString(&obj.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    // if (data != NULL)
    // {
        printf("%s", data);
    // }

    const char *key = "Index: 2 Value:";
    char *position = strstr(data, key);

    char value[256];

    memset(value, 0, sizeof(value));

    if (position != NULL)
    {
        position += strlen(key); // Move pointer to the value after the key

        if (sscanf(position, "  %255s", value) == 1)
        {
            printf("Value at index 4: %s\n", value);
        }
        else
        {
            printf("No numeric value found at index 4.\n");
        }
    }
    else
    {
        printf("Index 4 not found.\n");
    }

    free(data);
    data = NULL;

    strcpy(out_buf, value);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int enbl_disbl_load_limit_fn(int midx, int val, char *out_buf)
{
    static char fun_name[] = "enbl_disbl_load_limit_fn()";

    int ret;

    curr_nc_idx = midx;

    ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    // Step 1: Initialize the control mode variant
    dlmsVARIANT controlModeVariant;
    var_init(&controlModeVariant);
    controlModeVariant.vt = DLMS_DATA_TYPE_ENUM;
    controlModeVariant.bVal = val; // Set the control mode to the value passed (0 or 4)

    // Step 2: Initialize the gxDisconnectControl object
    gxDisconnectControl dc;
    // unsigned char obis[] = RELAY_CONN_DISCONN_OBIS; // Use correct OBIS for DisconnectControl
    cosem_init(BASE(dc), DLMS_OBJECT_TYPE_DISCONNECT_CONTROL, RELAY_CONN_DISCONN_OBIS);

    // Step 3: Set the value for Control Mode (attribute index 4)
    dc.controlMode = (DLMS_CONTROL_MODE)controlModeVariant.bVal; // Set the value of the control mode attribute

    // Step 4: Write the value to the device
    ret = com_write(&con, BASE(dc), ENBL_DISBL_LOAD_LIMIT_FN_ATTR_IDX);

    if (ret == DLMS_ERROR_CODE_OK)
    {
        printf("Successfully wrote Control Mode: %d\n", val);
    }
    else
    {
        printf("Failed to write Control Mode. Error: %d\n", ret);
        return -17;
    }

    ret = com_read(&con, &dc.base, 4);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20/09

    sprintf(out_buf, "%d", dc.controlMode);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_md_reset(int midx, int md_reset_val, char *out_buf)
{
    static char fun_name[] = "set_md_reset()";

    gxScriptTable dc;
    int ret;
    char *data = NULL;

    curr_nc_idx = midx;

    ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx,fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    unsigned char ln[] = {0, 0, 10, 0, 1, 255};

    INIT_OBJECT(dc, DLMS_OBJECT_TYPE_SCRIPT_TABLE, ln);

    dlmsVARIANT param;
    GX_UINT16(param) = 1;

    ret = com_method(&con, &dc.base, 1, &param);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    ret = com_read(&con, &dc.base, 2);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    // memset(out_buf, 0, sizeof(out_buf)); //rithika 20sep

    ret = obj_toString(&dc.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    // if (data != NULL)
    // {
        printf("%s", data);
    // }

    const char *key = "Index: 2 Value: [";
    char *position = strstr(data, key);

    char value[64];

    memset(value, 0, sizeof(value));

    if (position != NULL)
    {
        position += strlen(key); // Move pointer to the value after the key

        sscanf(position, "%[^]]", value);
    }
    else
    {
        printf("Index 4 not found.\n");
    }

    free(data);
    data = NULL;

    strcpy(out_buf, value);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_load_limit(int midx, int limit_val, char *out_buf)
{
    static char fun_name[] = "set_load_limit()";

    gxLimiter lv_d;
    char *data = NULL;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n",midx,  fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(lv_d), DLMS_OBJECT_TYPE_LIMITER, LOAD_LIMIT_OBIS);

    GX_UINT32(lv_d.thresholdNormal) = limit_val;

    ret = com_write(&con, BASE(lv_d), LOAD_LIMIT_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    gxLimiter obj;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_LIMITER, LOAD_LIMIT_OBIS);
    ret = com_read(&con, &obj.base, LOAD_LIMIT_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    else
    {
        ret = obj_toString(&obj.base, &data);
        if (ret != DLMS_ERROR_CODE_OK)
        {
            return ret;
        }
        if (data != NULL)
        {
            printf("%s", data);
        }
    }

    const char *key = "Index: 4 Value:";
    char *position = strstr(data, key);

    char value[8];

    memset(value, 0, sizeof(value));

    if (position != NULL)
    {
        position += strlen(key); // Move pointer to the value after the key

        if (sscanf(position, "  %7s", value) == 1)
        {
            printf("Value at index 4: %s\n", value);
        }
        else
        {
            printf("No numeric value found at index 4.\n");
        }
    }
    else
    {
        printf("Index 4 not found.\n");
    }

    free(data);
    data = NULL;

    strcpy(out_buf, value);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_apn(int midx, char *apn_name, unsigned char apn_bearer, char *out_buf)
{
    static char fun_name[] = "set_apn()";

    gxData apnObject;

    curr_nc_idx = midx;

    dlmsVARIANT apnStruct;
    var_init(&apnStruct);
    apnStruct.vt = DLMS_DATA_TYPE_STRUCTURE;
    apnStruct.Arr = (gxArray *)malloc(sizeof(gxArray));
    arr_init(apnStruct.Arr);

    // Add Bearer type enum (e.g., 6 = GPRS)
    dlmsVARIANT bearer;
    var_init(&bearer);
    bearer.vt = DLMS_DATA_TYPE_ENUM;
    bearer.bVal = 6;
    arr_push(apnStruct.Arr, &bearer);

    // Add APN string
    dlmsVARIANT apnName;
    var_init(&apnName);
    apnName.vt = DLMS_DATA_TYPE_STRING;
    apnName.strVal = (gxByteBuffer *)malloc(sizeof(gxByteBuffer));
    BYTE_BUFFER_INIT(apnName.strVal);
    bb_addString(apnName.strVal, apn_name);

    arr_push(apnStruct.Arr, &apnName);

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n",  midx, fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(apnObject), DLMS_OBJECT_TYPE_DATA, APN_OBIS);

    apnObject.value = apnStruct;
    ret = com_write(&con, BASE(apnObject), APN_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_demand_period write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    gxData obj;
    char *data = NULL;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_DATA, APN_OBIS);
    ret = com_read(&con, &obj.base, APN_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    ret = obj_toString(&obj.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    if (data != NULL)
    {
        printf("%s", data);
        strcpy(out_buf, data);
        free(data);
        data = NULL;
    }

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

char *hex_date(char *input_str)
{
    char hex_values[12][3]; // 12 hex bytes as 2-digit strings
    int byte_values[12];

    // Skip to the hex part
    const char *hex_part = strstr(input_str, "Value:");
    if (!hex_part)
    {
        printf("Invalid input string.\n");
        return NULL;
    }

    hex_part += strlen("Value: "); // Move past "Value: "

    // Extract 12 hex values from string
    for (int i = 0; i < 12; i++)
    {
        sscanf(hex_part, "%2s", hex_values[i]);
        byte_values[i] = (int)strtol(hex_values[i], NULL, 16);
        hex_part = strchr(hex_part, ' ');
        if (hex_part)
            hex_part++; // Move to next hex byte
    }

    // Extract date & time
    int year = (byte_values[0] << 8) | byte_values[1];
    int month = byte_values[2];
    int day = byte_values[3];
    int hour = byte_values[5];
    int minute = byte_values[6];
    int second = byte_values[7];

    // Format the date string
    // char date_str[20];
    // snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d %02d:%02d:%02d",
    //          year, month, day, hour, minute, second);

    // printf("Parsed date string: %s\n", date_str);

    // return date_str;

    char *date_str = malloc(20);
    if (!date_str)
        return NULL;

    snprintf(date_str, 20, "%04d-%02d-%02d %02d:%02d:%02d",
             year, month, day, hour, minute, second);

    return date_str;
}

#define DLMS_IGNORE_MALLOC
int set_lst_token_recharge_time(int midx, unsigned char *token_time, int len, char *out_buf)
{
    static char fun_name[] = "set_lst_token_recharge_time()";

    int ret;
    unsigned char datetimeBuffer[DATETIME_BUFFER_SIZE];

    gxData dataObject;

    curr_nc_idx = midx;

    ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    memset(&dataObject, 0, sizeof(gxData));
    cosem_init(BASE(dataObject), DLMS_OBJECT_TYPE_DATA, LAST_TKN_RECHARGE_TIME_OBIS);
    dataObject.base.objectType = DLMS_OBJECT_TYPE_DATA;

    gxByteBuffer *buf = (gxByteBuffer *)malloc(sizeof(gxByteBuffer));
    if (buf == NULL)
    {
        printf("Memory allocation failed\n");
        return -1;
    }

    // Step 2: Initialize and attach the buffer
    bb_init(buf);
    bb_attach(buf, datetimeBuffer, 0, DATETIME_BUFFER_SIZE);

    // Step 3: Assign the buffer to the variant
    dataObject.value.vt = DLMS_DATA_TYPE_OCTET_STRING;
    dataObject.value.byteArr = buf;

    memcpy(buf->data, token_time, len);
    buf->size = len;

    // Send the write request to the device (for the Last Token Receive Time)
    ret = com_write(&con, BASE(dataObject), LAST_TKN_RECHARGE_TIME_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_payment_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    bb_clear(buf);
    free(buf);

    gxData obj;
    char *data = NULL;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_DATA, LAST_TKN_RECHARGE_TIME_OBIS);
    ret = com_read(&con, &obj.base, LAST_TKN_RECHARGE_TIME_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    ret = obj_toString(&obj.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    if (data != NULL)
    {
        printf("%s", data);
        // rithika 20/09
        char *tmp = hex_date(data); // malloc'ed string
        if (tmp != NULL)
        {
            strcpy(out_buf, tmp); // copy into out_buf
            free(tmp);            // free malloc'ed string
        }

        // strcpy(out_buf, hex_date(data));
        free(data);
        data = NULL;
    }

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int set_curr_bal_time(int midx, unsigned char *curr_bal_time, int len, char *out_buf)
{
    static char fun_name[] = "set_curr_bal_time()";

    int ret;
    unsigned char datetimeBuffer[DATETIME_BUFFER_SIZE];

    curr_nc_idx = midx;

    ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n",midx, fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    // Initialize the gxData object
    gxData dataObject;
    memset(&dataObject, 0, sizeof(gxData)); // Clear the object
    cosem_init(BASE(dataObject), DLMS_OBJECT_TYPE_DATA, CURR_BAL_TIME_OBIS);
    dataObject.base.objectType = DLMS_OBJECT_TYPE_DATA;

    // Step 1: Allocate the gxByteBuffer
    gxByteBuffer *buf = (gxByteBuffer *)malloc(sizeof(gxByteBuffer));
    if (buf == NULL)
    {
        printf("Memory allocation failed\n");
        return -1;
    }

    // Step 2: Initialize and attach the buffer
    bb_init(buf);
    bb_attach(buf, datetimeBuffer, 0, DATETIME_BUFFER_SIZE);

    // Step 3: Assign the buffer to the variant
    dataObject.value.vt = DLMS_DATA_TYPE_OCTET_STRING;
    dataObject.value.byteArr = buf;

    memcpy(buf->data, curr_bal_time, len);
    buf->size = len;

    // Send the write request to the device (for the Last Token Receive Time)
    ret = com_write(&con, BASE(dataObject), CURR_BAL_TIME_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    bb_clear(buf);
    free(buf);

    gxData obj;
    char *data = NULL;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_DATA, CURR_BAL_TIME_OBIS);
    ret = com_read(&con, &obj.base, CURR_BAL_TIME_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    ret = obj_toString(&obj.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    if (data != NULL)
    {
        printf("%s", data);

         // rithika 20/09
        char *tmp = hex_date(data); // malloc'ed string
        if (tmp != NULL)
        {
            strcpy(out_buf, tmp); // copy into out_buf
            free(tmp);            // free malloc'ed string
        }

        // strcpy(out_buf, hex_date(data));
        free(data);
        data = NULL;
    }

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);
    return 0;
}

void hex_to_ascii(const char *hex_str, char *output)
{
    char hex[3] = {0};
    while (*hex_str)
    {
        if (*hex_str == ' ')
        {
            hex_str++;
            continue;
        }
        hex[0] = *hex_str++;
        hex[1] = *hex_str++;
        *output++ = (char)strtol(hex, NULL, 16);
    }
    *output = '\0';
}

int count_elements_by_braces(const char *input)
{
    int count = 0;
    while (*input)
    {
        if (*input == '{')
        {
            count++;
        }
        input++;
    }
    return count;
}

void parse_season(char *buffer, activity_calendar *act_cal_val)
{
    int season_count;

    season_count = 0;
    char *start = strchr(buffer, '{');
    while (start)
    {

        char *end = strchr(start, '}');
        if (!end)
            break;

        *end = '\0'; // Terminate this item
        char season_hex[256] = {0}, week_hex[256] = {0};
        char temp[1024];
        strcpy(temp, start + 1); // Skip '{'

        // Split into parts: season_hex, time, week_hex
        char *token = strtok(temp, ",");
        if (token)
            strcpy(season_hex, token);
        token = strtok(NULL, ","); // Skip time
        if (token)
            strcpy(act_cal_val->season_profile[season_count].start_time, token);
        token = strtok(NULL, ",");
        if (token)
            strcpy(week_hex, token);

        // Convert to ASCII

        hex_to_ascii(season_hex, act_cal_val->season_profile[season_count].season_name); //
        hex_to_ascii(week_hex, act_cal_val->season_profile[season_count].week_name);

        printf("Season Name: %s, Week Name: %s time stamp %s\n", act_cal_val->season_profile[season_count].season_name, act_cal_val->season_profile[season_count].week_name, act_cal_val->season_profile[season_count].start_time);

        // Move to next item
        start = strchr(end + 1, '{');
        season_count++;
    }
}

void parse_week(const char *temp, activity_calendar *act_cal_val)
{
    char *start = strchr(temp, '[');
    char *end = strrchr(temp, ']');
    if (start && end && end > start)
    {
        *end = '\0';
        memmove(temp, start + 1, strlen(start)); // Move content left after removing '['
    }

    printf("Cleaned week value: %s\n", temp);

    int week_idx = 0;

    // Loop through comma-separated weeks
    char *cursor = temp;
    while (*cursor && week_idx < MAX_WEEKS)
    {
        // Find comma or end
        char *comma = strchr(cursor, ',');
        char week_buf[256] = {0};

        if (comma)
        {
            size_t len = comma - cursor;
            strncpy(week_buf, cursor, len);
            cursor = comma + 1;
        }
        else
        {
            strncpy(week_buf, cursor, sizeof(week_buf) - 1);
            cursor += strlen(cursor);
        }

        // Trim leading/trailing spaces
        char *trim = week_buf;
        while (isspace((unsigned char)*trim))
            trim++;

        // Now split by space
        char *token = strtok(trim, " ");
        int day_idx = 0;

        if (token)
        {
            strncpy(act_cal_val->week_profile[week_idx].week_name, token, sizeof(act_cal_val->week_profile[week_idx].week_name) - 1);
            token = strtok(NULL, " ");
        }

        while (token && day_idx < 7)
        {
            act_cal_val->week_profile[week_idx].weekday[day_idx++] = atoi(token);
            token = strtok(NULL, " ");
        }

        printf("Parsed Week %d -> Name: %s | Days: ", week_idx, act_cal_val->week_profile[week_idx].week_name);
        for (int j = 0; j < day_idx; j++)
        {
            printf("%d ", act_cal_val->week_profile[week_idx].weekday[j]);
        }
        printf("\n");

        week_idx++;
    }

    act_cal_val->num_weeks = week_idx;
    printf("Total weeks parsed: %d\n", act_cal_val->num_weeks);
}

void trim(char *str)
{
    // Remove leading/trailing whitespace
    char *end;
    while (isspace((unsigned char)*str))
        str++;
    if (*str == 0)
        return;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;
    *(end + 1) = 0;
}

void parse_day(const char *input, activity_calendar *cal)
{
    cal->num_days = 0;

    const char *ptr = input;
    while (*ptr)
    {
        if (isdigit(*ptr))
        {
            // Start of a day entry
            day d;
            memset(&d, 0, sizeof(day));

            d.day_id = atoi(ptr);
            while (*ptr && *ptr != '[')
                ptr++; // move to [
            if (*ptr != '[')
                break;
            ptr++; // skip [

            char timezone_entry[128];
            int tz_count = 0;
            while (*ptr && *ptr != ']')
            {
                // Read one timezone entry
                int len = 0;
                while (*ptr && *ptr != ',' && *ptr != ']')
                {
                    timezone_entry[len++] = *ptr++;
                }
                timezone_entry[len] = '\0';
                trim(timezone_entry);

                // Parse the entry: script_table selector time
                if (strlen(timezone_entry) > 0 && tz_count < MAX_TIME_ZONES)
                {
                    time_zones *tz = &d.timezones[tz_count];
                    sscanf(timezone_entry, "%s %d %s", tz->script_table, &tz->script_selector, tz->day_start_time);
                    tz_count++;
                }

                if (*ptr == ',')
                    ptr++; // move past comma
            }
            d.max_time_zones = tz_count;

            if (*ptr == ']')
                ptr++; // move past ]

            // Store in calendar
            if (cal->num_days < MAX_DAYS)
            {
                cal->day_profile[cal->num_days++] = d;
            }
        }
        else
        {
            ptr++;
        }
    }

    printf("Parsed %d day(s):\n", cal->num_days);
    for (int i = 0; i < cal->num_days; ++i)
    {
        printf("Day ID: %d\n", cal->day_profile[i].day_id);
        for (int j = 0; j < cal->day_profile[i].max_time_zones; ++j)
        {
            time_zones tz_day = cal->day_profile[i].timezones[j];
            printf("  Script Table: %s, Selector: %d, Start Time: %s\n",
                   tz_day.script_table, tz_day.script_selector, tz_day.day_start_time);
        }
    }
}

char *escape_json_string(const char *input)
{
    size_t len = strlen(input);
    size_t out_len = len * 2 + 1; // worst case: all characters escaped
    char *output = malloc(out_len);
    if (!output)
        return NULL;

    char *out = output;
    for (const char *p = input; *p; p++)
    {
        switch (*p)
        {
        case '\"':
            *out++ = '\\';
            *out++ = '\"';
            break;
        case '\\':
            *out++ = '\\';
            *out++ = '\\';
            break;
        case '\b':
            *out++ = '\\';
            *out++ = 'b';
            break;
        case '\f':
            *out++ = '\\';
            *out++ = 'f';
            break;
        case '\n':
            *out++ = '\\';
            *out++ = 'n';
            break;
        case '\r':
            *out++ = '\\';
            *out++ = 'r';
            break;
        case '\t':
            *out++ = '\\';
            *out++ = 't';
            break;
        default:
            *out++ = *p;
        }
    }
    *out = '\0';
    return output;
}

int activity_calnd(int midx, activity_calendar ac)
{
    static char fun_name[] = "activity_calnd()";

    int ret = 0;
    int i, j, k;

    curr_nc_idx = midx;

    activity_calendar act_cal_val = {0};

    int year, month, days, hour, minute, second;
    gxActivityCalendar calendar;

    ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n",  midx,fun_name, (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(calendar), DLMS_OBJECT_TYPE_ACTIVITY_CALENDAR, ACT_CALND_OBIS);

    // Create a ByteBuffer to hold the passive calendar name
    gxByteBuffer calendarNameBuffer;
    bb_init(&calendarNameBuffer);                        // Initialize the buffer
    bb_addString(&calendarNameBuffer, ac.calendar_name); // Add the string as ByteBuffer

    // Set the ByteBuffer for the passive calendar name in the gxActivityCalendar structure

    calendar.calendarNamePassive = calendarNameBuffer;

    printf("start season\n");

    // === Season Profile ===
    for (i = 0; i < ac.num_seasons; i++)
    {
        gxSeasonProfile *season = (gxSeasonProfile *)malloc(sizeof(gxSeasonProfile));
        memset(season, 0, sizeof(gxSeasonProfile));
        bb_init(&season->name);
        bb_addString(&season->name, ac.season_profile[i].season_name);

        if (sscanf(ac.season_profile[i].start_time, "%d-%d-%d %d:%d:%d", &year, &month, &days, &hour, &minute, &second) != 6)
        {
            printf("Invalid date format\n");
            return -17;
        }

        gxtime startTimeseason;
        time_init(&startTimeseason, year, month, days, hour, minute, second, 500, 0);
        season->start = startTimeseason;

        bb_init(&season->weekName);
        bb_addString(&season->weekName, ac.season_profile[i].week_name);
        arr_push(&calendar.seasonProfilePassive, season);
    }

    printf("end of season\n");
    // // // === Week Profile ===

    for (i = 0; i < ac.num_weeks; i++)
    {
        gxWeekProfile *week = (gxWeekProfile *)malloc(sizeof(gxWeekProfile));

        memset(week, 0, sizeof(gxWeekProfile));
        bb_init(&week->name);
        bb_addString(&week->name, ac.week_profile[i].week_name);

        week->monday = ac.week_profile[i].weekday[0];
        week->tuesday = ac.week_profile[i].weekday[1];
        week->wednesday = ac.week_profile[i].weekday[2];
        week->thursday = ac.week_profile[i].weekday[3];
        week->friday = ac.week_profile[i].weekday[4];
        week->saturday = ac.week_profile[i].weekday[5];
        week->sunday = ac.week_profile[i].weekday[6];
        arr_push(&calendar.weekProfileTablePassive, week);
    }

    // // === Day Profile  ===

    for (j = 0; j < ac.num_days; j++)
    {

        gxDayProfile *day = (gxDayProfile *)malloc(sizeof(gxDayProfile));
        memset(day, 0, sizeof(gxDayProfile));

        day->dayId = ac.day_profile[j].day_id;
        arr_init(&day->daySchedules);

        for (i = 0; i < ac.day_profile[j].max_time_zones; i++)
        {
            gxScriptTable *script = (gxScriptTable *)malloc(sizeof(gxScriptTable));
            memset(script, 0, sizeof(gxScriptTable));
            cosem_init(BASE((*script)), DLMS_OBJECT_TYPE_SCRIPT_TABLE, ac.day_profile[j].timezones[i].script_table);

            gxDayProfileAction *day_actions = (gxDayProfileAction *)malloc(sizeof(gxDayProfileAction));
            gxtime t1;

            if (sscanf(ac.day_profile[j].timezones[i].day_start_time, "%d-%d-%d %d:%d:%d", &year, &month, &days, &hour, &minute, &second) != 6)
            {
                printf("Invalid date format\n");
                return -1;
            }

            time_init(&t1, year, month, days, hour, minute, second, 0, -1);
            day_actions->startTime = t1;
            day_actions->script = (gxObject *)script;
            day_actions->scriptSelector = ac.day_profile[j].timezones[i].script_selector;

            arr_push(&day->daySchedules, day_actions);
        }
        arr_push(&calendar.dayProfileTablePassive, day);
    }

    // printf("time !!!!!!!!!!!!\n");
    // gxtime param_time;

    // if (sscanf(cal_time, "%d-%d-%d %d:%d:%d", &year, &month, &days, &hour, &minute, &second) != 6)
    // {
    //     printf("Invalid date format\n");
    // }

    // time_init(&param_time, year, month, days, hour, minute, second, 500, 0);
    // calendar.time = param_time;

    int attr;
    for (attr = 6; attr <= 9; ++attr)
    {
        ret = com_write(&con, BASE(calendar), attr);
        if (ret != DLMS_ERROR_CODE_OK)
        {
            dbg_log(REPORT, "[%-25s] :set_demand_period write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
            return -17;
        }
    }

    // Activate passive calendar (method 1)
    gxByteBuffer params;
    bb_init(&params); // Empty buffer
    bb_attach(&params, NULL, 0, 0);

    ret = com_method(&con, &calendar.base, 1, &params);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_demand_period write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    // printf("Activity calendar configured and activated successfully.\n");

    /////////////////////////////////////
    printf("============ start of activity_calnd=============\n");

    gxActivityCalendar obj;
    char *data = NULL;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_ACTIVITY_CALENDAR, ACT_CALND_OBIS);

    for (i = 6; i <= 9; i++)
    {
        ret = com_read(&con, &obj.base, i);

        if (ret != DLMS_ERROR_CODE_OK)
        {
            return ret;
        }

        ret = obj_toString(&obj.base, &data);
        if (ret != DLMS_ERROR_CODE_OK)
        {
            return ret;
        }
        if (data != NULL)
        {
            printf("!!!!!!!!!!!!!!!!!!!!!!!\n%s\n!!!!!!!!!!\n", data);

            // char *line = strtok(data, "\n");
            char *saveptr;
            char *line = strtok_r(data, "\n", &saveptr);

            while (line != NULL)
            {
                int index;
                char value[1024];

                // Extract index and value
                if (sscanf(line, "Index: %d Value: %[^\n]", &index, value) == 2)
                {
                    printf("Parsed -> Index: %d, Value: %s\n", index, value);
                    if (index == 6)
                    {
                        strcpy(act_cal_val.calendar_name, value);
                        printf("Calendar Name: %s\n", act_cal_val.calendar_name);
                    }
                    if (index == 7)
                    {

                        char temp[1024];
                        strcpy(temp, value);

                        int num_seasons = count_elements_by_braces(temp);
                        act_cal_val.num_seasons = num_seasons;
                        printf("num seasons %d\n", act_cal_val.num_seasons);
                        // char *token = strtok(value, ",");
                        // while (token != NULL)
                        // {
                        //     printf("token %s\n", token);
                        //     for (i = 2; i < strlen(token); i++)
                        //     {
                        //         strcat(seasn_name, token[i]);
                        //     }
                        //     printf("------- sean name %s ---------------\n",seasn_name);
                        //     token = strtok(NULL, ",");
                        // }
                        char buffer[1024];
                        strcpy(buffer, value);

                        parse_season(buffer, &act_cal_val);
                    }

                    if (index == 8)
                    {
                        char temp[1024] = {0};
                        strncpy(temp, value, sizeof(temp) - 1);

                        parse_week(temp, &act_cal_val);
                    }

                    if (index == 9)
                    {
                        char temp[1024] = {0};
                        strcpy(temp, value);

                        parse_day(temp, &act_cal_val);
                        // print_day(&act_cal_val);
                    }
                }
                else
                {
                    printf("Could not parse line: %s\n", line);
                }

                // line = strtok(NULL, "\n"); // Move to next line
                line = strtok_r(NULL, "\n", &saveptr);
            }

            // printf("*******************act_cal_val.calendar_name %s season name %s *********\n", act_cal_val.calendar_name, seasn_name);
            free(data);
            data = NULL;
        }
    }

    // ------- PRINT ALL VALUES ---------
    printf("Calendar Name: %s\n", act_cal_val.calendar_name);
    printf("\n-- Day Profiles (%d) --\n", act_cal_val.num_days);
    for ( i = 0; i < act_cal_val.num_days; i++)
    {
        printf("Day ID: %d\n", act_cal_val.day_profile[i].day_id);
        for ( j = 0; j < act_cal_val.day_profile[i].max_time_zones; j++)
        {
            printf("  Time Zone %d:\n", j + 1);
            printf("    Start Time : %s\n", act_cal_val.day_profile[i].timezones[j].day_start_time);
            printf("    Script Table: %s\n", act_cal_val.day_profile[i].timezones[j].script_table);
            printf("    Selector   : %d\n", act_cal_val.day_profile[i].timezones[j].script_selector);
        }
    }

    printf("\n-- Season Profiles (%d) --\n", act_cal_val.num_seasons);
    for ( i = 0; i < act_cal_val.num_seasons; i++)
    {
        printf("Season Name: %s\n", act_cal_val.season_profile[i].season_name);
        printf("Start Time : %s\n", act_cal_val.season_profile[i].start_time);
        printf("Week Name  : %s\n", act_cal_val.season_profile[i].week_name);
    }

    printf("\n-- Week Profiles (%d) --\n", act_cal_val.num_weeks);
    for ( i = 0; i < act_cal_val.num_weeks; i++)
    {
        printf("Week Name: %s\n", act_cal_val.week_profile[i].week_name);
        printf("Week Days: ");
        for ( j = 0; j < 7; j++)
        {
            printf("%d ", act_cal_val.week_profile[i].weekday[j]);
        }
        printf("\n");
    }

    // Create JSON
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "cal_name", act_cal_val.calendar_name);
    cJSON_AddNumberToObject(root, "num_days", act_cal_val.num_days);

    // Day Profile
    cJSON *day_prof = cJSON_CreateArray();
    for ( i = 0; i < act_cal_val.num_days; i++)
    {
        cJSON *day_item = cJSON_CreateObject();
        cJSON_AddNumberToObject(day_item, "id", act_cal_val.day_profile[i].day_id);
        cJSON_AddNumberToObject(day_item, "num_tz", act_cal_val.day_profile[i].max_time_zones);

        cJSON *time_prof = cJSON_CreateArray();
        for ( j = 0; j < act_cal_val.day_profile[i].max_time_zones; j++)
        {
            cJSON *tz_item = cJSON_CreateObject();
            cJSON_AddStringToObject(tz_item, "start_time", act_cal_val.day_profile[i].timezones[j].day_start_time);
            cJSON_AddStringToObject(tz_item, "table", act_cal_val.day_profile[i].timezones[j].script_table);
            cJSON_AddNumberToObject(tz_item, "selc", act_cal_val.day_profile[i].timezones[j].script_selector);
            cJSON_AddItemToArray(time_prof, tz_item);
        }

        cJSON_AddItemToObject(day_item, "time_prof", time_prof);
        cJSON_AddItemToArray(day_prof, day_item);
    }
    cJSON_AddItemToObject(root, "day_prof", day_prof);

    // Season
    cJSON_AddNumberToObject(root, "num_season", act_cal_val.num_seasons);
    cJSON *seasons = cJSON_CreateArray();
    for ( i = 0; i < act_cal_val.num_seasons; i++)
    {
        cJSON *season_item = cJSON_CreateObject();
        cJSON *season_prof = cJSON_CreateObject();
        cJSON_AddStringToObject(season_prof, "season_name", act_cal_val.season_profile[i].season_name);
        cJSON_AddStringToObject(season_prof, "start_time", act_cal_val.season_profile[i].start_time);

        // Match week
        // for (int j = 0; j < act_cal_val.num_weeks; j++)
        // {
        //     if (strcmp(act_cal_val.week_profile[j].week_name, act_cal_val.season_profile[i].week_name) == 0)
        //     {
        //         cJSON *week_prof = cJSON_CreateObject();
        //         cJSON_AddStringToObject(week_prof, "week_name", act_cal_val.week_profile[j].week_name);
        //         cJSON *week_days = cJSON_CreateIntArray((const int *)act_cal_val.week_profile[j].weekday, 7);
        //         cJSON_AddItemToObject(week_prof, "week_days", week_days);
        //         cJSON_AddItemToObject(season_prof, "week_prof", week_prof);
        //         break;
        //     }
        // }

        for ( j = 0; j < act_cal_val.num_weeks; j++)
        {
            if (strcmp(act_cal_val.week_profile[j].week_name, act_cal_val.season_profile[i].week_name) == 0)
            {
                cJSON *week_prof = cJSON_CreateObject();
                cJSON_AddStringToObject(week_prof, "week_name", act_cal_val.week_profile[j].week_name);

                int week_days_int[7];
                for ( k = 0; k < 7; k++)
                {
                    week_days_int[k] = act_cal_val.week_profile[j].weekday[k];
                }

                cJSON *week_days = cJSON_CreateIntArray(week_days_int, 7);
                cJSON_AddItemToObject(week_prof, "week_days", week_days);
                cJSON_AddItemToObject(season_prof, "week_prof", week_prof);
                break;
            }
        }

        cJSON_AddItemToObject(season_item, "season_prof", season_prof);
        cJSON_AddItemToArray(seasons, season_item);
    }
    cJSON_AddItemToObject(root, "season", seasons);

    char *temp = cJSON_PrintUnformatted(root);
    if (temp == NULL)
    {
        cJSON_Delete(root);
        return -1; // failed to generate JSON
    }

    size_t len1 = sizeof(temp_json);

    printf("size_t len1 : %zu\n", len1);
    snprintf(temp_json, len1, "%s", escape_json_string(temp));

    free(temp);
    cJSON_Delete(root);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", temp_json);
    return 0;
}

int single_action_schl_billing(int midx, char *scdl_time, char *out_buf)
{
    static char fun_name[] = "single_action_schl_billing()";

    int ret;
    int month, day, hour, minute, second, year;

    curr_nc_idx = midx;

    gxActionSchedule sched;
    cosem_init(BASE(sched), DLMS_OBJECT_TYPE_ACTION_SCHEDULE, SNG_ACT_SCH_BIL_OBIS);

    // Initialize the execution time list
    arr_init(&sched.executionTime);

    gxtime *execTime = (gxtime *)malloc(sizeof(gxtime));

    if (sscanf(scdl_time, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) != 6)
    {
        printf("Invalid date format\n");
    }

    time_init(execTime, 0xFFFF, month, day, hour, minute, second, 0, -1); // year as wildcard entry

    arr_push(&sched.executionTime, execTime);

    ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx, fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    // Write attribute 4 (execution time list)
    ret = com_write(&con, BASE(sched), SNG_ACT_SCH_BIL_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :single_action_schl_billing write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    printf("============ start of single action schl =============\n");

    gxActionSchedule obj;
    char *data = NULL;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_ACTION_SCHEDULE, SNG_ACT_SCH_BIL_OBIS);
    ret = com_read(&con, &obj.base, SNG_ACT_SCH_BIL_ATTR_IDX);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    ret = obj_toString(&obj.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    // if (data != NULL)
    // {
        printf("%s", data);
        // free(data);
        // data = NULL;
    // }

    const char *key = "Index: 4 Value: [";
    char *position = strstr(data, key);

    char value[32];

    memset(value, 0, sizeof(value));

    if (position != NULL)
    {
        position += strlen(key); // Move pointer to the value after the key

        sscanf(position, "%[^]]", value);
    }
    else
    {
        printf("Index 4 not found.\n");
    }

    free(data);
    data = NULL;

    strcpy(out_buf, value);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);

    return 0;
}

int instant_push_setup(int midx, const char *destination_address, char *out_buf)
{
    static char fun_name[] = "instant_push_setup()";
    // Initialize the PushSetup object
    gxPushSetup push;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx,fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(push), DLMS_OBJECT_TYPE_PUSH_SETUP, INSTANT_PUSH_OBIS);

    // Create the structure for the destination
    dlmsVARIANT destinationStruct;
    var_init(&destinationStruct);

    destinationStruct.vt = DLMS_DATA_TYPE_OCTET_STRING;
    destinationStruct.byteArr = (gxByteBuffer *)malloc(sizeof(gxByteBuffer));
    BYTE_BUFFER_INIT(destinationStruct.byteArr);
    bb_addString(destinationStruct.byteArr, destination_address); // Add the destination address as a string

    // Set the value for the PushSetup object (destination attribute)
    push.destination = *destinationStruct.byteArr;

    // Write the value to the meter
    ret = com_write(&con, BASE(push), INSTANT_PUSH_ATTR_IDX);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        dbg_log(REPORT, "[%-25s] :set_metering_mode write failed: %s\n", fun_name, hlp_getErrorMessage(ret));
        return -17;
    }

    // Clean up allocated memory
    bb_clear(&destinationStruct.byteArr);

    printf("============ start of push notifications =============\n");

    gxPushSetup obj;
    char *data = NULL;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_PUSH_SETUP, INSTANT_PUSH_OBIS);
    ret = com_read(&con, &obj.base, INSTANT_PUSH_ATTR_IDX);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    ret = obj_toString(&obj.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    // if (data != NULL)
    // {
        printf("%s", data);
    // }

    const char *key = "Index: 3 Value:";
    char *position = strstr(data, key);

    char value[64];

    memset(value, 0, sizeof(value));

    if (position != NULL)
    {
        position += strlen(key); // Move pointer to the value after the key

        if (sscanf(position, "  %63s", value) == 1)
        {
            printf("Value at index 4: %s\n", value);
        }
        else
        {
            printf("No numeric value found at index 4.\n");
        }
    }
    else
    {
        printf("Index 4 not found.\n");
    }

    free(data);
    data = NULL;

    strcpy(out_buf, value);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);
    return 0;
}

int alert_push_setup(int midx, const char *destination_address, char *out_buf)
{
    static char fun_name[] = "alert_push_setup()";

    gxPushSetup push;

    curr_nc_idx = midx;

    int ret = init_dlms(midx);
    if (ret < 0)
    {
        dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ FAILED : ret - %s\n", midx,fun_name,  (hlp_getErrorMessage(ret)));
        return -1;
    }

    cosem_init(BASE(push), DLMS_OBJECT_TYPE_PUSH_SETUP, ALERT_PUSH_OBIS);

    // Create the structure for the destination
    dlmsVARIANT destinationStruct;
    var_init(&destinationStruct);

    destinationStruct.vt = DLMS_DATA_TYPE_OCTET_STRING;
    destinationStruct.byteArr = (gxByteBuffer *)malloc(sizeof(gxByteBuffer));
    BYTE_BUFFER_INIT(destinationStruct.byteArr);
    bb_addString(destinationStruct.byteArr, destination_address); // Add the destination address as a string

    // Set the value for the PushSetup object (destination attribute)
    push.destination = *destinationStruct.byteArr;

    // Write the value to the meter
    ret = com_write(&con, BASE(push), ALERT_PUSH_ATTR_IDX);

    if (ret == DLMS_ERROR_CODE_OK)
    {
        printf(" ===================== Write alert_push_setup destination address finished ===========\n");
    }
    else
    {
        printf("Write failed with error code: %d\n", ret);
    }

    // Clean up allocated memory
    bb_clear(&destinationStruct.byteArr);

    printf("============ start of alert_push_setup =============\n");

    gxPushSetup obj;
    char *data = NULL;

    cosem_init(BASE(obj), DLMS_OBJECT_TYPE_PUSH_SETUP, ALERT_PUSH_OBIS);
    ret = com_read(&con, &obj.base, ALERT_PUSH_ATTR_IDX);

    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }

    ret = obj_toString(&obj.base, &data);
    if (ret != DLMS_ERROR_CODE_OK)
    {
        return ret;
    }
    // if (data != NULL)
    // {
        printf("%s", data);
    // }

    const char *key = "Index: 3 Value:";
    char *position = strstr(data, key);

    char value[64];

    memset(value, 0, sizeof(value));

    if (position != NULL)
    {
        position += strlen(key); // Move pointer to the value after the key

        if (sscanf(position, "  %63s", value) == 1)
        {
            printf("Value at index 4: %s\n", value);
        }
        else
        {
            printf("No numeric value found at index 4.\n");
        }
    }
    else
    {
        printf("Index 4 not found.\n");
    }

    free(data);
    data = NULL;

    strcpy(out_buf, value);

    print_colored_message(YELLOW, 1, "------------------ out_buf : %s -----------\n", out_buf);
    return 0;
}