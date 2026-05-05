#include "../include/dlms_config.h"
#include "../include/general_config.h"
#include "../include/plcc_rel.h"

#define true 1
#define false 0

// rithika 31oct2025
#define LATEST_EVE_CNT 1
// rithika 15nov2025
#define NUM_MIDNIGHT_DAYS_TCS 30
#define NUM_MIDNIGHT_DAYS 60

uint8_t g_date_time_obis[8] = {0, 0, 1, 0, 0, 255};
uint8_t init_one_time = 0;
extern int ftp_enable; // rithika

connection con;

nameplate_info_t nameplate_info;
inst_val_info_t inst_val_info;
billing_data_value_t billing_data_value;
daily_profile_data_value_t daily_profile_data_value;
load_survey_data_value_t load_survey_data_value;
events_data_value_t events_data_value;

// rithika 30oct2025
event_val_obis_t event_val_obis[MAX_NO_OF_METER_PER_PORT][MAX_EVENTS];

// rithika 03oct2025
inst_val_obis_t inst_val_obis[MAX_NO_OF_METER_PER_PORT];
ls_val_obis_t ls_val_obis[MAX_NO_OF_METER_PER_PORT];
daily_profile_val_obis_t daily_profile_val_obis[MAX_NO_OF_METER_PER_PORT];
billing_val_obis_t billing_val_obis[MAX_NO_OF_METER_PER_PORT];

feature_cfg_t feature_cfg;
// ser_port_channel_t ser_port_data;
ser_port_channel_t ser_port_data[MAX_NO_OF_SERIAL_PORT];

meter_metrics_t inst_meter_metrics;
meter_metrics_t bill_meter_metrics;
meter_metrics_t ls_meter_metrics;
meter_metrics_t mn_meter_metrics;
meter_metrics_t event_meter_metrics;
meter_metrics_t np_meter_metrics;

// rithika 16oct
gxProfileGeneric inst_pg[MAX_NO_OF_METER_PER_PORT];
// gxProfileGeneric ls_pg[MAX_NO_OF_METER_PER_PORT];
gxProfileGeneric ls_blk_pg[MAX_NO_OF_METER_PER_PORT]; // rithika 05nov2025 commented when used for all meters and declared inside the function
gxProfileGeneric mn_pg[MAX_NO_OF_METER_PER_PORT];
gxProfileGeneric bill_pg[MAX_NO_OF_METER_PER_PORT];
gxProfileGeneric event_pg[MAX_NO_OF_METER_PER_PORT][MAX_EVENTS];

Date meter_date_time[MAX_NO_OF_METER_PER_PORT];
char meter_time_str[MAX_NO_OF_METER_PER_PORT][20]; // rithika

char time_buff[32];
char process_diag_msg[255];
Date global_end_date;

// rithika 30oct2025
int g_event_idx = 0;
// rithika 15nov2025
extern char dev_model[64];

uint8_t name_plate_obis[MAX_NAMEPLATE_VALS][6] =
	{
		{0, 0, 96, 1, 0, 255},	//"Meter Serial No.",     "0.0.96.1.0.255",
		{0, 0, 96, 1, 1, 255},	//"Manufacturer Name",    "0.0.96.1.1.255",
		{1, 0, 0, 2, 0, 255},	//"Firmware Version",     "1.0.0.2.0.255",
		{0, 0, 94, 91, 9, 255}, //"Meter Type",           "0.0.94.91.9.255",
		{1, 0, 0, 4, 2, 255},	//"Int. CT Ratio",        "1.0.0.4.2.255",
		{1, 0, 0, 4, 3, 255},	// pt ratio
		{1, 0, 0, 8, 4, 255}	// block.int
								// RECONNECT 17Feb2026
#if RECONNECT
		,
		{0, 0, 94, 91, 11, 255} // meter category
		,
		{0, 0, 94, 91, 12, 255} // current rating
		,
		{0, 0, 96, 1, 4, 255} // year of manufacture
		,
		{1, 0, 0, 8, 0, 255} // demand_int_period
		,
		{1, 0, 0, 3, 0, 255}, // not in secure, lnt, genus
		{0, 0, 25, 1, 0, 255},
		{0, 0, 22, 0, 0, 255},
		{1, 0, 0, 4, 6, 255},  // not in lnt, genus
		{1, 0, 0, 3, 128, 255} // not in secure, lnt, genus

#endif
};

//[-3, 33] | [-3, 33] | [-3, 33] | [-2, 35] | [-2, 35] | [-2, 35] | [-3, 255] | [-3, 255] | [-3, 255] | [-3, 255] | [-3, 44] | [-3, 28] | [-3, 27] | [-3, 29] | [-5, 30] | [-5, 30] | [-5, 31] | [-5, 31] | [0, 6] | [0, 255] | [-5, 32] | [-5, 32] | [-5, 32] | [-5, 32] | [-5, 30] | [-5, 30] | [-5, 32] | [-5, 32] | [-5, 32] | [-4, 27] | [-4, 27] | [-4, 28] | [-4, 28] | [-4, 27] | [-4, 27] | [-4, 28] | [-4, 28] | [-1, 56]

param_det_t get_mul_fcator[] = {
	{-7, 0.0000001},
	{-6, 0.000001},
	{-5, 0.00001},
	{-4, 0.0001},
	{-3, 0.001},
	{-2, 0.01},
	{-1, 0.1},
	{0, 1},
	{1, 10},
	{2, 100},
	{3, 1000},
	{4, 10000},
	{5, 100000},
	{6, 1000000},
	{7, 10000000}};

char *outputFile = NULL;

extern char dcu_ser_num[16];
extern uint8_t g_pidx;
extern char g_old_ls_date_time[11];
extern bool old_ls_flag;
extern int curr_nc_idx;
extern char p_comm_port_det[16];
extern int OD_CMD_RECEIVED;

extern poll_cfg_t poll_cfg;

extern Date calculate_end_date(Date ls_date, int num_days);
extern Date calculate_hours_minutes(int num_blocks, int blk_int);
extern double string_to_double(const char *str);
extern char *replace_dot_with_underscore(char *str);
extern char *removeSpace(char *hexString, char *outputBuffer, size_t bufferSize, int type_of_data, char *manufacturer);
extern void convertDate(const char *input, char *output, int change, int type_of_data);
extern void calculate_start_and_end_date(const char *date_str, struct tm *start_date, struct tm *end_date, int);

// rithika 06oct
extern int inst_data_read[MAX_NO_OF_METER_PER_PORT];
extern int ls_data_read_day[MAX_NO_OF_METER_PER_PORT];
extern int ls_data_read_block[MAX_NO_OF_METER_PER_PORT];
extern int mn_data_read[MAX_NO_OF_METER_PER_PORT];
extern int bill_data_read[MAX_NO_OF_METER_PER_PORT];
extern int event_data_read[MAX_NO_OF_METER_PER_PORT];

// rithika 17oct
extern int inst_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
// extern int ls_day_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
extern int ls_block_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
extern int mn_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
extern int bill_obis_read_enb[MAX_NO_OF_METER_PER_PORT];
extern int event_obis_read_enb[MAX_NO_OF_METER_PER_PORT][MAX_EVENTS];

int num_event_entries[MAX_NO_OF_METER_PER_PORT][MAX_EVENTS] = {0};

// rithika 30oct2025
extern int latest_row_exist;
int discover_meter_manf()
{
	static char fun_name[] = "discover_meter_manf()";
	uint8_t obis[6] = {0, 0, 42, 0, 0, 255};
	// uint8_t obis[6] = {0, 0, 96, 1, 1, 255};

	char obis_val[64];
	int cnt;
	uint8_t inf_flag = -1;

	// char *outputFile = NULL, *invocationCounter = NULL; //rithika 19sep

	dbg_log(INFORM, "[%-25s] :Discover Meter Manf\n", fun_name);

	int no_of_meters;

	if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
	{
		no_of_meters = plcc_modem_info.no_of_nodes;
	}
	else
	{
		no_of_meters = ser_port_data[g_pidx].num_dev;
	}

	for (cnt = 0; cnt < no_of_meters; cnt++)
	{
		int ret = -1;
		curr_nc_idx = cnt;

		cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);
		sleep(2);

		con.settings.interfaceType = DLMS_INTERFACE_TYPE_WRAPPER;

		dbg_log(INFORM, "[%-25s] : [RS_%d] : Trying with Wrapper\n", fun_name, cnt);

		ret = com_initializeConnection(&con);

		if (ret != 0)
		{
			dbg_log(INFORM, "[%-25s] : [RS_%d] : It is not Wrapper so Trying with HDLC\n", fun_name, cnt);

			cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);
			sleep(2);

			con.settings.interfaceType = DLMS_INTERFACE_TYPE_HDLC;
			ret = com_initializeConnection(&con);
			if (ret != 0)
			{
				memset(plcc_modem_info.plcc_meter_map[cnt].meter_manf_name, 0, sizeof(plcc_modem_info.plcc_meter_map[cnt].meter_manf_name));
				strcpy(plcc_modem_info.plcc_meter_map[cnt].meter_manf_name, "NC");
				dbg_log(INFORM, "[%-25s] : [RS_%d] : Tried with both HDLC and Wrapper but meter didn't response\n", fun_name, cnt);
				continue;
			}
			else
			{
				inf_flag = 0;
			}
		}
		else
		{
			inf_flag = 1;
		}

		plcc_modem_info.plcc_meter_map[cnt].dcu_meter_idx = cnt;

		unsigned char *uchar_var = obis;
		// int val_type = single_obis_read(uchar_var, &obis_val, &con); rithika 19sep

		int val_type = single_obis_read(uchar_var, &obis_val);

		memset(plcc_modem_info.plcc_meter_map[cnt].meter_manf_name, 0, sizeof(plcc_modem_info.plcc_meter_map[cnt].meter_manf_name));
		strcpy(plcc_modem_info.plcc_meter_map[cnt].meter_manf_name, obis_val);

		if (inf_flag == 1)
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter Type - Wrapper. Manf name - %s\n", cnt, fun_name, plcc_modem_info.plcc_meter_map[cnt].meter_manf_name);
		else if (inf_flag == 0)
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter Type - HDLC. Manf name - %s\n", cnt, fun_name, plcc_modem_info.plcc_meter_map[cnt].meter_manf_name);
	}

	return 0;
}

/**
 * Counts the number of OBIS codes in a given string, categorized by data type.
 *
 * @param str The input string containing the OBIS code data.
 * @param type The type of data being processed, which determines where to store the count.
 * @param midx The meter index (not used in this function, but may be relevant in context).
 * @return 0 on success, indicating that the count has been successfully updated.
 */
int countOBISCodes(const char *str, int type, int midx)
{
	static char fun_name[] = "countOBISCodes()";

	const char *index3Start = strstr(str, "Index: 3");
	if (!index3Start)
		return 0;

	const char *start = strstr(index3Start, "Value: [");
	if (!start)
		return 0;

	const char *end = strstr(start, "]");
	if (!end)
		return 0;

	int count = 0;
	const char *ptr;
	for (ptr = start; ptr < end; ++ptr)
	{
		if (*ptr == ',')
		{
			++count;
		}
	}

	if (type == INSTDATA)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_INST_VALS)
		{
			inst_val_info.num_vals = MAX_INST_VALS;
			// rithika 03oct2025
			inst_val_obis[midx].num_val_params = MAX_INST_VALS;
		}
		else
		{
			inst_val_info.num_vals = tot_count;
			// rithika 03oct2025
			inst_val_obis[midx].num_val_params = tot_count;
		}

		// inst_val_info.num_vals = count +1;

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : INST No.of Obis - %d\n", midx, fun_name, inst_val_info.num_vals);
	}
	else if (type == DAILYPROFILE)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_MN_VALS)
		{
			daily_profile_data_value.num_vals = MAX_MN_VALS;
			// rithika 03oct2025
			daily_profile_val_obis[midx].num_val_params = MAX_MN_VALS;
		}
		else
		{
			daily_profile_data_value.num_vals = tot_count;
			// rithika 03oct2025
			daily_profile_val_obis[midx].num_val_params = tot_count;
		}

		// daily_profile_data_value.num_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : DAILYPROFILE No.of Obis - %d\n", midx, fun_name, daily_profile_data_value.num_vals);
	}
	else if (type == BILLING)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_BILLING_VALS)
		{
			billing_data_value.num_vals = MAX_BILLING_VALS;
			// rithika 03oct2025
			billing_val_obis[midx].num_val_params = MAX_BILLING_VALS;
		}
		else
		{
			billing_data_value.num_vals = tot_count;
			// rithika 03oct2025
			billing_val_obis[midx].num_val_params = tot_count;
		}

		// billing_data_value.num_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : BILLING No.of Obis - %d\n", midx, fun_name, billing_data_value.num_vals);
	}
	else if (type == LSDATA)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_LS_VALS)
		{
			load_survey_data_value.num_vals = MAX_LS_VALS;
			// rithika 03oct2025
			ls_val_obis[midx].num_val_params = MAX_LS_VALS;
		}
		else
		{
			load_survey_data_value.num_vals = tot_count;
			// rithika 03oct2025
			ls_val_obis[midx].num_val_params = tot_count;
		}

		// load_survey_data_value.num_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : LSDATA No.of Obis - %d\n", midx, fun_name, load_survey_data_value.num_vals);
	}
	else if (type == EVENTDATA)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_EVENT_VALS)
		{
			events_data_value.num_event = MAX_EVENT_VALS;
			// rithika 30oct2024
			event_val_obis[midx][g_event_idx].num_event_params = MAX_EVENT_VALS;
		}
		else
		{
			events_data_value.num_event = tot_count;
			// rithika 30oct2024
			event_val_obis[midx][g_event_idx].num_event_params = tot_count;
		}

		// events_data_value.num_event = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : EVENTDATA No.of Obis - %d\n", midx, fun_name, events_data_value.num_event);
	}
	else if (type == INST_SCALER)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_INST_VALS)
		{
			inst_val_info.num_scaler_vals = MAX_INST_VALS;
		}
		else
		{
			inst_val_info.num_scaler_vals = tot_count;
		}

		// inst_val_info.num_scaler_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : INST_SCALER No.of Obis - %d\n", midx, fun_name, inst_val_info.num_scaler_vals);
	}
	else if (type == DAILYPROFILE_SCALER)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_MN_VALS)
		{
			daily_profile_data_value.num_scaler_vals = MAX_MN_VALS;
		}
		else
		{
			daily_profile_data_value.num_scaler_vals = tot_count;
		}

		// daily_profile_data_value.num_scaler_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : DAILYPROFILE_SCALER No.of Obis - %d\n", midx, fun_name, daily_profile_data_value.num_scaler_vals);
	}
	else if (type == BILLING_SCALER)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_BILLING_VALS)
		{
			billing_data_value.num_scaler_vals = MAX_BILLING_VALS;
		}
		else
		{
			billing_data_value.num_scaler_vals = tot_count;
		}

		// billing_data_value.num_scaler_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : BILLING_SCALER No.of Obis - %d\n", midx, fun_name, billing_data_value.num_scaler_vals);
	}
	else if (type == LSDATA_SCALER)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_LS_VALS)
		{
			load_survey_data_value.num_scaler_vals = MAX_LS_VALS;
		}
		else
		{
			load_survey_data_value.num_scaler_vals = tot_count;
		}

		// load_survey_data_value.num_scaler_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : LSDATA_SCALER No.of Obis - %d\n", midx, fun_name, load_survey_data_value.num_scaler_vals);
	}
	else if (type == EVENTDATA_SCALER)
	{
		// rithika 07oct
		int tot_count = count + 1;
		if (tot_count > MAX_EVENT_VALS)
		{
			events_data_value.num_scaler_vals = MAX_EVENT_VALS;
		}
		else
		{
			events_data_value.num_scaler_vals = tot_count;
		}

		// events_data_value.num_scaler_vals = count + 1;
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : EVENTDATA_SCALER No.of Obis - %d\n", midx, fun_name, events_data_value.num_scaler_vals);
	}

	return 0;
}

/**
 * Parses OBIS codes from a given text and stores them based on the specified type.
 *
 * @param text The input text containing the OBIS codes.
 * @param type The type of data being parsed (e.g., INSTDATA, DAILYPROFILE, etc.).
 * @param midx An index for the meter (currently not used in the function).
 * @return 0 on success, or -1 if parsing fails.
 */
int obis_parsing(char *text, int type, int midx)
{
	static char fun_name[] = "obis_parsing()";

	int count = -1, i = 0;

	// print_colored_message(YELLOW, 1, "===============================\n");
	// print_colored_message(MAGENTA, 1, "TEXT %s", text);
	char *start = strstr(text, "Index: 3 Value: [") + strlen("Index: 3 Value: [");
	if (start == NULL)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : No data found within square brackets.\n", midx, fun_name);
		return -1;
	}

	char *end = strstr(start, "]");
	if (end == NULL)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : No data found within square brackets.\n", midx, fun_name);
		return -1;
	}

	*end = '\0';

	char *token = strtok(start, " ");
	int j;
	char token_cpy[256];
	char final_obis[64];

	while (token != NULL)
	{

		token = strtok(NULL, ", ");
		if (token != NULL)
		{
			strcpy(token_cpy, token);
		}
		else
		{
			dbg_log(REPORT, "[%-25s] :Parsing token value NULL\n", fun_name);
			break;
		}

		count++;

		memset(final_obis, 0, sizeof(final_obis));
		strcpy(final_obis, replace_dot_with_underscore(token));
		// print_colored_message(RED, 1, "final obs %s ", final_obis);

		if (count % 2 == 0)
		{
			if (type == INSTDATA)
			{

				// if (strcmp(inst_val_info.values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(inst_val_info.values[i].obis_code, final_obis);
				// }
				// else
				// {
				// 	strcpy(inst_val_info.values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(inst_val_info.values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}

				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(inst_val_info.values[i].obis_code, final_obis);

				// rithika 03nov2025
				strcpy(inst_val_obis[midx].obis_code[i], final_obis);
				i++;
			}
			else if (type == INST_SCALER)
			{

				// if (strcmp(inst_val_info.scalar_values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(inst_val_info.scalar_values[i].obis_code, final_obis);
				// }
				// else
				// {
				// 	strcpy(inst_val_info.scalar_values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(inst_val_info.scalar_values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}
				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(inst_val_info.scalar_values[i].obis_code, final_obis);
				i++;
			}
			else if (type == DAILYPROFILE)
			{
				// strcpy(daily_profile_data_value.values[i].obis_code, final_obis);
				// if (strcmp(daily_profile_data_value.values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(daily_profile_data_value.values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(daily_profile_data_value.values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}

				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(daily_profile_data_value.values[i].obis_code, final_obis);

				// rithika 03nov2025
				strcpy(daily_profile_val_obis[midx].obis_code[i], final_obis);

				i++;
			}
			else if (type == DAILYPROFILE_SCALER)
			{
				// strcpy(daily_profile_data_value.scalar_values[i].obis_code, final_obis);
				// if (strcmp(daily_profile_data_value.scalar_values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(daily_profile_data_value.scalar_values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(daily_profile_data_value.scalar_values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}

				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(daily_profile_data_value.scalar_values[i].obis_code, final_obis);
				i++;
			}
			else if (type == BILLING)
			{
				// strcpy(billing_data_value.values[i].obis_code, final_obis);
				// if (strcmp(billing_data_value.values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(billing_data_value.values[i].obis_code, final_obis);
				// 	print_colored_message(GREEN, 1, "obis %s ", billing_data_value.values[i].obis_code);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					// print_colored_message(GREEN, 1, "billing_data_value.values[%d].obis_code %s final_obis %s ", j,billing_data_value.values[j].obis_code, final_obis);
					if (strcmp(billing_data_value.values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}

				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
					// print_colored_message(GREEN, 1, "cpy obis %s ", final_obis);
				}

				strcpy(billing_data_value.values[i].obis_code, final_obis);

				// rithika 30oct2025
				strcpy(billing_val_obis[midx].obis_code[i], final_obis);
				i++;
			}
			else if (type == BILLING_SCALER)
			{
				// strcpy(billing_data_value.scalar_values[i].obis_code, final_obis);
				// if (strcmp(billing_data_value.scalar_values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(billing_data_value.scalar_values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(billing_data_value.scalar_values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}

				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}
				print_colored_message(GREEN, 1, "final_obis %s ", final_obis);
				strcpy(billing_data_value.scalar_values[i].obis_code, final_obis);
				i++;
			}
			else if (type == LSDATA)
			{
				// strcpy(load_survey_data_value.values[i].obis_code, final_obis);
				// if (strcmp(load_survey_data_value.values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(load_survey_data_value.values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(load_survey_data_value.values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}
				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(load_survey_data_value.values[i].obis_code, final_obis);
				// rithika 30oct2025
				strcpy(ls_val_obis[midx].obis_code[i], final_obis);
				i++;
			}
			else if (type == LSDATA_SCALER)
			{
				// strcpy(load_survey_data_value.scalar_values[i].obis_code, final_obis);
				// if (strcmp(load_survey_data_value.scalar_values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(load_survey_data_value.scalar_values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(load_survey_data_value.scalar_values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}
				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(load_survey_data_value.scalar_values[i].obis_code, final_obis);
				i++;
			}
			else if (type == EVENTDATA)
			{
				// strcpy(events_data_value.values[i].obis_code, final_obis);
				// if (strcmp(events_data_value.values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(events_data_value.values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(events_data_value.values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}
				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(events_data_value.values[i].obis_code, final_obis);

				// rithika 30oct2025
				strcpy(event_val_obis[midx][g_event_idx].obis_code[i], final_obis);
				i++;
			}
			else if (type == EVENTDATA_SCALER)
			{
				// strcpy(events_data_value.scalar_values[i].obis_code, final_obis);
				// if (strcmp(events_data_value.scalar_values[i - 1].obis_code, final_obis) == 0)
				// {
				// 	strcat(final_obis, "_DT");
				// 	strcpy(events_data_value.scalar_values[i].obis_code, final_obis);
				// }
				// i++;

				int isDuplicate = 0;
				for (j = 0; j < i; j++)
				{
					if (strcmp(events_data_value.scalar_values[j].obis_code, final_obis) == 0)
					{
						isDuplicate = 1;
						break;
					}
				}
				if (isDuplicate)
				{
					strcat(final_obis, "_DT");
				}

				strcpy(events_data_value.scalar_values[i].obis_code, final_obis);
				i++;
			}
		}
	}

#if DEBUG

	print_colored_message(RED, PRINT, "=================================================\n");
	char msg_buffer[64];
	memset(msg_buffer, 0, sizeof(msg_buffer));
	if (type == INSTDATA)
	{
		printf("INST OBIS: \n");

		sprintf(msg_buffer, "[Meter_%d]:[%-25s]: INSTDATA Obis Parsing Completed", midx, fun_name);
		add_process_diag(msg_buffer);
		// for (i = 0; i < inst_val_info.num_vals; i++)
		// {
		// 	printf("%s\n", inst_val_info.values[i].obis_code);
		// }
	}
	else if (type == INST_SCALER)
	{
		// printf("INST SCALAR OBIS: \n");
		sprintf(msg_buffer, "[Meter_%d]:[%-25s]: INST_SCALER Obis Parsing Completed", midx, fun_name);
		add_process_diag(msg_buffer);

		// for (i = 0; i < inst_val_info.num_scaler_vals; i++)
		// {
		// 	printf("%s\n", inst_val_info.scalar_values[i].obis_code);
		// }
	}
	else if (type == DAILYPROFILE)
	{
		sprintf(msg_buffer, "[Meter_%d]:[%-25s]: DAILYPROFILE Obis Parsing Completed", midx, fun_name);
		add_process_diag(msg_buffer);

		// printf("DAILYPROFILE OBIS: \n");
		// for (i = 0; i < daily_profile_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", daily_profile_data_value.values[i].obis_code);
		// }
	}
	else if (type == BILLING)
	{
		sprintf(msg_buffer, "[Meter_%d]:[%-25s]: BILLING Obis Parsing Completed", midx, fun_name);
		add_process_diag(msg_buffer);

		// printf("BILLING OBIS: \n");
		// for (i = 0; i < billing_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", billing_data_value.values[i].obis_code);
		// }
	}
	else if (type == LSDATA)
	{
		sprintf(msg_buffer, "[Meter_%d]:[%-25s]: LSDATA Obis Parsing Completed", midx, fun_name);
		add_process_diag(msg_buffer);

		// printf("LSDATA OBIS: \n");
		// for (i = 0; i < load_survey_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", load_survey_data_value.values[i].obis_code);
		// }
	}
	else if (type == LSDATA_SCALER)
	{
		sprintf(msg_buffer, "[Meter_%d]:[%-25s]: LSDATA_SCALER Obis Parsing Completed", midx, fun_name);
		add_process_diag(msg_buffer);

		// printf("LSDATA_SCALER OBIS: \n");
		// for (i = 0; i < load_survey_data_value.num_scaler_vals; i++)
		// {
		// 	printf("%s\n", load_survey_data_value.scalar_values[i].obis_code);
		// }
	}
	else if (type == EVENTDATA)
	{
		sprintf(msg_buffer, "[Meter_%d]:[%-25s]: EVENTDATA Obis Parsing Completed", midx, fun_name);
		add_process_diag(msg_buffer);

		// printf("EVENTDATA OBIS: \n");
		// for (i = 0; i < events_data_value.num_event; i++)
		// {
		// 	printf("%s\n", events_data_value.values[i].obis_code);
		// }
	}

	print_colored_message(RED, PRINT, "=================================================\n");
#endif

	return 0;
}

char *trim_spaces(char *str)
{
	int i, start, end;

	// Trim leading spaces
	for (start = 0; str[start] == ' ' || str[start] == '\t'; start++)
		;

	// Trim trailing spaces
	end = strlen(str) - 1;
	while (end >= start && (str[end] == ' ' || str[end] == '\t'))
	{
		end--;
	}

	// Shift characters to the front
	for (i = 0; start <= end; start++, i++)
	{
		str[i] = str[start];
	}
	str[i] = '\0'; // Null-terminate the string

	return str;
}

double elapsed_ms(struct timeval start, struct timeval end)
{

	return (end.tv_sec - start.tv_sec) * 1000.0 +

		   (end.tv_usec - start.tv_usec) / 1000.0;
}

/**
 * @brief Parses a string of data values and stores them in the appropriate structure based on the data type.
 *
 * This function takes a pipe-separated string of values, processes each value according to its
 * format, and stores the results in predefined structures based on the specified type of data.
 * It also handles date-time conversion and removes spaces from hexadecimal values as needed.
 *
 * @param data A pointer to a null-terminated string containing the data values in the format:
 *              "value1|value2|value3|...".
 * @param type_of_data An integer indicating the type of data being parsed:
 *                     - 0: Unused
 *                     - 1: Instantaneous data
 *                     - 2: Daily profile data
 *                     - 3: Billing data
 *                     - 4: Load survey data
 *                     - Other: Event data
 * @param midx An integer used as an index or identifier for the specific context of parsing.
 *
 * @return Returns 0 on success, or -1 if an error occurs during parsing.
 */
int val_parse(char *data, int type_of_data, int midx, int event_idx, char *manufacturer)
{
	static char fun_name[] = "val_parse()";
	int count;
	int ret = -1;
	const char *ret_val;
	int hasSpace = 0;
	int i;

	// rithika 06oct
	struct timeval start, end;

	// char *token;
	char temp_token[255];
	char temp_buff[255];
	printf("=========== data %s ==========\n", data);
	// token = strtok(data, "|");
	char *data_copy = strdup(data);
	if (!data_copy)
	{
		printf("Error no data to parse\n");
	}

	char *saveptr = NULL;
	char *token = strtok_r(data_copy, "|", &saveptr);

	count = 0;

	// char *serial_num = get_meter_2_ser_num(midx);
	// if (serial_num == NULL)
	// {
	// 	printf("[%s] : Serial number not found for meter index %d\n", fun_name, midx);
	// 	return -1;
	// }
	char serial_num[64] = {0};
	if (get_meter_2_ser_num(midx, serial_num, sizeof(serial_num)) != 0)
	{
		// handle error
		printf("[%s] : Serial number not found for meter index %d\n", fun_name, midx);
		free(data_copy);
		return -1;
	}

	gettimeofday(&start, NULL);

	while (token != NULL)
	{
		printf("+++++++++++++++++%s+++++++++++++++\n", token);
		memset(temp_buff, 0, sizeof(temp_buff));
		if (token[0] == ' ')
		{
			memmove(token, token + 1, strlen(token));

			if (strlen(token) != 1)
			{
				// 04/07/2025 rithika for handling val without space at the end eg:" 105"
				if (token[strlen(token) - 1] == ' ')
				{
					token[strlen(token) - 1] = '\0';
				}
			}
		}

#if 0 // DEBUG
		printf("Count : %d value : *%s* ser_num : *%s*\n", count, token, get_meter_2_ser_num(midx));
#endif
		memset(temp_token, 0, sizeof(temp_buff));
		strcpy(temp_token, token);

		// char *serial_num = get_meter_2_ser_num(midx);
		// if (serial_num == NULL)
		// {
		// 	printf("[%s] : Serial number not found for meter index %d\n", fun_name, midx);
		// }

		// if (serial_num && strcmp(trim_spaces(temp_token), serial_num) == 0)
		// {
		if (strlen(serial_num) > 0 && strcmp(trim_spaces(temp_token), serial_num) == 0)
		{
			// if (strcmp(get_meter_2_ser_num(midx), trim_spaces(temp_token)) == 0)
			// {

			strcpy(temp_buff, token);
		}
		else if (isdigit(token[0]) != 0)
		{

			ret_val = strstr(token, "/");

			//////////////////////
			/* --- begin replacement block for the "if (ret_val) { ... }" section --- */

			if (ret_val)
			{

				/* token appears to contain a date/time (it had a '/') */
				/* First, compute a trimmed copy safely into token_trim */
				char token_trim[256];
				size_t tlen = strlen(token);
				if (tlen >= sizeof(token_trim))
					tlen = sizeof(token_trim) - 1;
				strncpy(token_trim, token, tlen);
				token_trim[tlen] = '\0';
				trim_spaces(token_trim); /* assume you have a trim helper; if not, implement */

				/* check for common timezone indicators */
				const char *tz_ptr = NULL;
				int assume_local_default = 1; /* 1 => assume local (IST), 0 => assume UTC. Change as needed */

				if ((tz_ptr = strstr(token_trim, "UTC")) != NULL)
				{
					/* contains literal UTC */
					convertDate(token_trim, temp_buff, 1, type_of_data); /* treat as UTC */
				}
				else if (strstr(token_trim, "IST") != NULL)
				{

					convertDate(token_trim, temp_buff, 0, type_of_data); /* treat as local/IST */
				}
				else if (strchr(token_trim, 'Z') == (token_trim + strlen(token_trim) - 1))
				{
					/* ends with Z (ISO UTC designator) */
					convertDate(token_trim, temp_buff, 1, type_of_data);
				}
				else
				{
					/* look for +/- timezone offset patterns like +05:30, -0530, +0530 */
					char *plus = strrchr(token_trim, '+');
					char *minus = strrchr(token_trim, '-');
					if ((plus && plus > token_trim + 5) || (minus && minus > token_trim + 5))
					{

						/* assume presence of numeric timezone offset */
						convertDate(token_trim, temp_buff, 1, type_of_data); /* treat offset as explicit tz (use convertDate to handle) */
					}
					else
					{

						/* No timezone token found. Fall back to a default. */
						if (assume_local_default)
						{

							/* assume local timezone (IST) */
							convertDate(token_trim, temp_buff, 0, type_of_data);
						}
						else
						{
							/* assume UTC */
							convertDate(token_trim, temp_buff, 1, type_of_data);
						}
					}
				}
			}
			/* --- end replacement block --- */

			///////////////////////
			// 			if (ret_val)
			// 			{
			// 				const char *end_pos = strstr(token, "UTC");
			// 				if (end_pos == NULL)
			// 				{
			// 					printf("[%-25s] :UTC part not found in the input string.\n", fun_name);
			// 					return -1;
			// 				}

			// 				size_t length = end_pos - token;

			// 				char buffer[128];

			// 				// Open a pipe to run the "date" command and read its output
			// 				FILE *fp = popen("date", "r");
			// 				if (fp == NULL)
			// 				{
			// 					perror("Failed to run date command");
			// 					return 1;
			// 				}

			// 				// Read the output of the date command
			// 				fgets(buffer, sizeof(buffer), fp);

			// 				// Close the pipe
			// 				fclose(fp);

			// 				// Check if "UTC" or "IST" is in the string
			// 				if (strstr(buffer, "UTC") != NULL)
			// 				{
			// 					printf("The date and time is in UTC.\n");
			// 					convertDate(token, temp_buff, 1, type_of_data);
			// 				}
			// 				else if (strstr(buffer, "IST") != NULL)
			// 				{
			// 					printf("The date and time is in IST.\n");
			// 					convertDate(token, temp_buff, 0, type_of_data);
			// 				}
			// 				else
			// 				{
			// 					printf("Neither UTC nor IST found in the date string.\n");
			// 				}

			// 				// strncpy(temp_buff, token, length);
			// 				// temp_buff[length] = '\0';
			// #if 0 // DEBUG
			// 				printf("TIME STAMP : Temp BUFF : %s\n", temp_buff);
			// #endif
			// 			}
			else
			{

				for (i = 0; token[i] != '\0'; i++)
				{
					if (token[i] == ' ')
					{
						hasSpace = 1;
						break;
					}
				}
				if (hasSpace == 1)
				{

					// strcpy(temp_buff, removeSpace(token));
					removeSpace(token, temp_buff, sizeof(temp_buff), type_of_data, manufacturer);
#if 0 // DEBUG
					printf("HEX VALUE : Temp BUFF : %s\n", temp_buff);
#endif
					hasSpace = 0;
				}
				else
				{

					strcpy(temp_buff, token);
#if 0 // DEBUG
					printf("INT : Temp BUFF : %s\n", temp_buff);
#endif
				}
			}
		}
		else
		{
#if 0 // DEBUG
			printf("STRING : Temp BUFF : %s\n", temp_buff);
#endif
			strcpy(temp_buff, token);
		}

		// token = strtok(NULL, "|");
		token = strtok_r(NULL, "|", &saveptr);

		switch (type_of_data)
		{
		case 0:
			/* code */
			break;

		case 1:
			strcpy(inst_val_info.values[count].val, temp_buff);
			break;

		case 2:
		{

			strcpy(daily_profile_data_value.values[count].val, temp_buff);
		}

		break;

		case 3:
			strcpy(billing_data_value.values[count].val, temp_buff);
			break;

		case 4:
			strcpy(load_survey_data_value.values[count].val, temp_buff);
			// rithika 06oct2025
			//  ls_scaler_val_2(count);
			break;

		default:
			strcpy(events_data_value.values[count].val, temp_buff);
			break;
		}
		count++;
	}

	// rithika 30Dec2025 for removing the \n located at the end of last value eg :906\n
	if (type_of_data == 1)
	{
		char *val = inst_val_info.values[count - 1].val;
		printf("+++++++++++++++++++++++++++++++++++++++++++\n\n\nval %s inst_val_info.values[count - 1].val %s\n", val, inst_val_info.values[count - 1].val);
		size_t len = strlen(val);

		if (len > 0 && val[len - 1] == '\n')
		{
			val[len - 1] = '\0';
		}
	}

	gettimeofday(&end, NULL);

	// dbg_log(INFORM, "[%-25s] : function() took %.3f ms\n", "val_parse for parsing", elapsed_ms(start, end) );

	double elapsed = elapsed_ms(start, end);

	dbg_log(INFORM, "[val_parse()] : took %.3f ms\n", elapsed);
	printf("--------------- val_parse for parsing function() took %.3f ms\n", elapsed);

	// if (serial_num)
	// {
	// 	free(serial_num);
	// }

	free(data_copy);

	print_colored_message(RED, PRINT, "=================================================\n");

	gettimeofday(&start, NULL);

	if (type_of_data == INSTDATA)
	{
		// add_process_diag("INSTDATA Val Parsing Completed");
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : INSTDATA Val Parsing Completed\n", midx, fun_name);

		// printf("INST VALUES: \n");
		// for (i = 0; i < inst_val_info.num_vals; i++)
		// {
		// 	printf("%s\n", inst_val_info.values[i].val);
		// }

		// rithika 03nov2025
		inst_val_info.num_vals = inst_val_obis[midx].num_val_params;

		// rithika 07oct2025
		if (fetch_inst_scalar_info(midx, &inst_val_info) == 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Inst Scaler info fetched successfully\n", midx, fun_name);

			inst_scaler_val(midx); // now scalers will be applied correctly
		}
		else
		{
			// rithika 30Dec2025
			inst_data_read[midx] = 0;
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No inst scaler info found in redis for meter_%d\n", midx, fun_name, midx);
		}

		// inst_scaler_val();
		// createDataTable("inst_data", g_pidx, dcu_ser_num, midx);
	}
	else if (type_of_data == DAILYPROFILE)
	{

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : DAILYPROFILE Val Parsing Completed\n", midx, fun_name);

		// printf("BEFORE SCALLING DAILYPROFILE VALUES: \n");
		// for (i = 0; i < daily_profile_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", daily_profile_data_value.values[i].val);
		// }

		// rithika 03nov2025
		daily_profile_data_value.num_vals = daily_profile_val_obis[midx].num_val_params;

		// rithika 09oct2025
		if (fetch_mn_scalar_info(midx, &daily_profile_data_value) == 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] :  Midnight Scaler info fetched successfully\n", midx, fun_name);

			midnight_scaler_val(midx); // now scalers will be applied correctly
		}
		else
		{
			// rithika 30Dec2025
			mn_data_read[midx] = 0;
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No midnight scaler info found in redis for meter_%d\n", midx, fun_name);
		}

		// midnight_scaler_val();

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : SCALLING DAILYPROFILE Val Completed\n", midx, fun_name);

		// printf("AFTER SCALLING DAILYPROFILE VALUES: \n");
		// for (i = 0; i < daily_profile_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", daily_profile_data_value.values[i].val);
		// }
		createDataTable("daily_profile_data", g_pidx, dcu_ser_num, midx, event_idx);
	}
	else if (type_of_data == BILLING)
	{

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : BILLING Val Parsing Completed\n", midx, fun_name);

		// printf("BEFORE SCALLING BILLING VALUES: \n");
		// for (i = 0; i < billing_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", billing_data_value.values[i].val);
		// }
		// rithika 03nov2025
		billing_data_value.num_vals = billing_val_obis[midx].num_val_params;

		// rithika 09oct2025
		if (fetch_bill_scalar_info(midx, &billing_data_value) == 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Billing Scaler info fetched successfully\n", midx, fun_name);

			billing_scaler_val(midx); // now scalers will be applied correctly
		}
		else
		{
			// rithika 30Dec2025
			bill_data_read[midx] = 0;
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No Billing scaler info found in redis for meter_%d\n", midx, fun_name);
		}

		// billing_scaler_val();
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : SCALLING BILLING Val Completed\n", midx, fun_name);

		// printf("AFTER SCALLING BILLING VALUES: \n");
		// for (i = 0; i < billing_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", billing_data_value.values[i].val);
		// }
		createDataTable("bill_data", g_pidx, dcu_ser_num, midx, event_idx);
	}
	else if (type_of_data == LSDATA)
	{

		// printf("LSDATA VALUES: \n");
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : LSDATA Val Parsing Completed\n", midx, fun_name);

		// for (i = 0; i < load_survey_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", load_survey_data_value.values[i].val);
		// }

		// rithika 03nov2025
		load_survey_data_value.num_vals = ls_val_obis[midx].num_val_params;

		// rithika 09oct2025
		if (fetch_ls_scalar_info(midx, &load_survey_data_value) == 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Ls Scaler info fetched successfully\n", midx, fun_name);

			ls_scaler_val(midx); // now scalers will be applied correctly
		}
		else
		{
			// rithika 30Dec2025
			ls_data_read_day[midx] = 0;
			ls_data_read_block[midx] = 0;
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No ls scaler info found in redis for meter_%d\n", midx, fun_name);
		}

		// ls_scaler_val();
		// printf("AFTER LSDATA VALUES: \n");
		// dbg_log(INFORM, "[%-25s] : SCALLING LSDATA Val Completed\n", fun_name);

		// for (i = 0; i < load_survey_data_value.num_vals; i++)
		// {
		// 	printf("%s\n", load_survey_data_value.values[i].val);
		// }
		createDataTable("ls_data", g_pidx, dcu_ser_num, midx, event_idx);
	}
	else if (type_of_data == EVENTDATA)
	{

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : EVENTDATA Val Parsing Completed\n", midx, fun_name);

		// printf("EVENTDATA VALUES: \n");
		// for (i = 0; i < events_data_value.num_event; i++)
		// {
		// 	printf("%s\n", events_data_value.values[i].val);
		// }

		// rithika 30oct2025
		printf("------------------------ g_event_idx %d event_val_obis[midx][g_event_idx].num_event_params %d\n", g_event_idx, event_val_obis[midx][g_event_idx].num_event_params);
		events_data_value.num_event = event_val_obis[midx][g_event_idx].num_event_params;

		// rithika 09oct2025
		if (fetch_event_scalar_info(midx, &events_data_value) == 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Event Scaler info fetched successfully\n", midx, fun_name);

			event_scaler_val(midx); // now scalers will be applied correctly
		}
		else
		{
			// rithika 30Dec2025
			event_data_read[midx] = 0;
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No event scaler info found in redis for meter_%d\n", midx, fun_name);
		}

		// event_scaler_val();
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : SCALLING EVENTDATA Val Completed\n", midx, fun_name);

		createDataTable("event_data", g_pidx, dcu_ser_num, midx, event_idx);
	}

	print_colored_message(RED, PRINT, "=================================================\n");

	gettimeofday(&end, NULL);

	dbg_log(INFORM, "[val_parse()] : took %.3f ms\n", elapsed_ms(start, end));
	printf("--------------- val_parse for storing function() took %.3f ms\n", elapsed_ms(start, end));

	return 0;
}

/**
 * @brief Parses a multiline string into individual lines and processes each line.
 *
 * This function takes a string containing multiple lines of data and splits it into separate
 * lines. For each line, it calls the `val_parse` function to handle the specific parsing
 * and processing based on the given type.
 *
 * @param data A pointer to a null-terminated string containing data in multiple lines,
 *             where each line is terminated by a newline character.
 * @param type An integer indicating the type of data being parsed, which is passed to
 *             the `val_parse` function for further processing.
 * @param midx An integer used as an index or identifier for the specific context of parsing.
 */
void parsing_strings_to_lines(char *data, int type, int midx, int event_idx, char *manufacturer)
{
	int input_pos = 0;
	int line_start = 0;
	int line_length = 0;
	int line_count = 0;

	char val_data[8048]; // rithika 08Sep
	memset(val_data, 0, sizeof(val_data));
	struct timeval start, end;

	// poll_status_log("[%s] Met ser num  : %s.received data : %s\n", time_format_conversion(time(NULL)),get_meter_2_ser_num(midx),data);

	while (data[input_pos] != '\0' && line_count < 300)
	{
		// printf("++++++++++++++++++++++++++++++++++++++ input pos : %d +++++++++++++++++++++++\n",input_pos);
		if (data[input_pos] == '\n' || data[input_pos + 1] == '\0')
		{
			line_length = input_pos - line_start + 1;
			strncpy(val_data, data + line_start, line_length);
			val_data[line_length - 1] = '\0';

			line_start = input_pos + 1;
			line_count++;
			printf("---------- val_data %s-----------\n", val_data);
			////////////

			gettimeofday(&start, NULL);

			val_parse(val_data, type, midx, event_idx, manufacturer);

			gettimeofday(&end, NULL);

			dbg_log(INFORM, "[parsing_strings_to_lines()] : took %.3f ms\n", elapsed_ms(start, end));

			printf("--------------- function() took %.3f ms\n", elapsed_ms(start, end));
		}
		input_pos++;
	}
}

/**
 * @brief Retrieves the multiplier corresponding to a given key.
 *
 * This function searches for a specific power value (key) in the
 * `get_mul_fcator` array. If a match is found, it returns the
 * associated multiplication factor. If the key is not found, it
 * returns a default value of 1.0.
 *
 * @param key The power value for which the multiplier is to be retrieved.
 * @return The corresponding multiplication factor, or 1.0 if the key
 *         is not found in the array.
 */
double get_multiplier(int key)
{
	for (int i = 0; i < sizeof(get_mul_fcator) / sizeof(get_mul_fcator[0]); i++)
	{
		if (get_mul_fcator[i].pow_val == key)
		{
			return get_mul_fcator[i].mul_factor;
		}
	}
	return 1.0; // Default value if key not found
}

/**
 * @brief Parses scaler values from a given input string.
 *
 * This function splits the input string at the '[' character and
 * attempts to extract integer values. For each valid integer found,
 * it retrieves the corresponding multiplier using the `get_multiplier`
 * function and assigns it to the appropriate structure based on the
 * provided type of scaler.
 *
 * The function supports multiple scaler types:
 * - INST_SCALER
 * - DAILYPROFILE_SCALER
 * - BILLING_SCALER
 * - LSDATA_SCALER
 * - EVENTDATA_SCALER
 *
 * The function prints each extracted value along with its corresponding
 * multiplier for debugging purposes.
 *
 * @param input The input string containing scaler values.
 * @param type_of_scaler The type of scaler, which determines where
 *                       the extracted multipliers are stored.
 * @return Returns 0 on successful parsing.
 */
int scaler_parsing(char *input, int type_of_scaler)
{
	char *token;
	int value;
	int i = 0;

	token = strtok(input, "[");
	while (token != NULL)
	{
		if (sscanf(token, "%d", &value) == 1)
		{
			if (type_of_scaler == INST_SCALER)
			{
				inst_val_info.scalar_values[i].multiplier = get_multiplier(value);
				printf("Value: %d, Multiplier: %f\n", value, inst_val_info.scalar_values[i].multiplier);
			}
			else if (type_of_scaler == DAILYPROFILE_SCALER)
			{
				daily_profile_data_value.scalar_values[i].multiplier = get_multiplier(value);
				printf("Value: %d, Multiplier: %f\n", value, daily_profile_data_value.scalar_values[i].multiplier);
			}
			else if (type_of_scaler == BILLING_SCALER)
			{
				billing_data_value.scalar_values[i].multiplier = get_multiplier(value);
				printf("Value: %d, Multiplier: %f\n", value, billing_data_value.scalar_values[i].multiplier);
			}
			else if (type_of_scaler == LSDATA_SCALER)
			{
				load_survey_data_value.scalar_values[i].multiplier = get_multiplier(value);
				printf("Value: %d, Multiplier: %f\n", value, load_survey_data_value.scalar_values[i].multiplier);
			}
			else if (type_of_scaler == EVENTDATA_SCALER)
			{
				events_data_value.scalar_values[i].multiplier = get_multiplier(value);
				printf("Value: %d, Multiplier: %f\n", value, events_data_value.scalar_values[i].multiplier);
			}
			i++;
		}
		token = strtok(NULL, "[");
	}
	return 0;
}

/**
 * @brief Opens a serial port and initializes connection parameters.
 *
 * This function attempts to open a specified serial port for communication.
 * If the port opens successfully, it initializes the optical head and updates
 * the invocation counter. In case of any failures during these steps, appropriate
 * error messages are logged.
 *
 * @param connection A pointer to the connection structure that will hold
 *                   the details of the opened connection.
 * @param port The name of the serial port to be opened.
 * @param readObjects A string containing objects to be read from the serial port.
 * @param invocationCounter A string representing the invocation counter for
 *                          the connection.
 * @param outputFile The name of the output file where results will be saved.
 * @return Returns 0 on success, or an error code on failure.
 */
int open_serial_port(
	connection *connection,
	const char *port,
	char *readObjects,
	const char *invocationCounter,
	const char *outputFile)
{
	static char fun_name[] = "open_serial_port()";
	int ret;

	ret = com_open(connection, port);
	if (ret == 0)
	{
		dbg_log(INFORM, "[%-25s] :Serial port opened successfully\n", fun_name);

#if TESTING
		if ((ret = com_initializeOpticalHead(connection)) == 0 &&
			(ret = com_updateInvocationCounter(connection, invocationCounter)) == 0)
		{
			printf("************************ inside open_serial_port\n");
#if CMS
			init_dlms(0);
			read_nameplate(connection);
			printf("--------------- after nameplate\n");
			// meterDateTime_read(connection);
			// read_inst(connection);
			// read_billing(connection);
			// read_ls(connection);
			// read_event(connection);
			// read_midnight(connection);
#endif
		}

		else
		{
			dbg_log(REPORT, "[%-25s] :InitializeOpticalHead failed : ret - %s\n", fun_name, (hlp_getErrorMessage(ret)));
		}
#endif
	}
	else
	{
		dbg_log(INFORM, "[%-25s] :Serial port open failed : ret - %s\n", fun_name, (hlp_getErrorMessage(ret)));
	}

	return ret;
}

/**
 * @brief Initializes the serial port with the specified settings.
 *
 * This function sets up the connection parameters for the serial port,
 * initializes the necessary buffers, and opens the specified serial port.
 * It logs the initialization process and handles the case where the
 * serial port value is NULL, returning an error code in such cases.
 *
 * @param serialPort A string representing the name of the serial port to be initialized.
 * @return Returns 0 on success or 1 if the serial port value is NULL.
 */
int init_serial_port(char *serialPort)
{
	static char fun_name[] = "init_serial_port()";

	gxByteBuffer item;
	bb_init(&item);
	con_init(&con, GX_TRACE_LEVEL_INFO);
	// cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);/ rithika
	cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);

	int opt = 0;
	int port = 0;
	char *address = NULL;
	// char *serialPort = NULL;
	char *readObjects = NULL, *outputFile = NULL;

	char *invocationCounter = NULL;

	dbg_log(INFORM, "[%-25s] :Init serial port : %s\n", fun_name, serialPort);

	con.trace = GX_TRACE_LEVEL_VERBOSE;

	if (serialPort != NULL)
	{
		open_serial_port(&con, serialPort, readObjects, invocationCounter, outputFile);
	}
	else
	{
		dbg_log(REPORT, "[%-25s] :Serial port value is NULL\n", fun_name);
		return 1;
	}
}

void hpl_config()
{
	con.settings.interfaceType = DLMS_INTERFACE_TYPE_WRAPPER;
	con.settings.clientAddress = 48;						// us type
	con.settings.authentication = DLMS_AUTHENTICATION_HIGH; // high
	con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION_ENCRYPTION;

	bb_clear(&con.settings.cipher.systemTitle);
	bb_addHexString(&con.settings.cipher.systemTitle, "3232323232323232"); // 4245450000249F9C
	printf("system title %s\n", con.settings.cipher.systemTitle);

	bb_clear(&con.settings.cipher.authenticationKey);
	printf("before authenticationKey '%s' size : %d\n", con.settings.cipher.authenticationKey.data, con.settings.cipher.authenticationKey.size);
	bb_addHexString(&con.settings.cipher.authenticationKey, "31323334414243443132333441424344");
	printf("after  authenticationKey '%s' size : %d\n", con.settings.cipher.authenticationKey.data, con.settings.cipher.authenticationKey.size);

	bb_clear(&con.settings.cipher.blockCipherKey);
	bb_addHexString(&con.settings.cipher.blockCipherKey, "31323334414243443132333441424344");
	printf("blockCipherKey '%s'\n", con.settings.cipher.blockCipherKey);

	// rithika 27oct2025
	bb_clear(&con.settings.password);
	bb_init(&con.settings.password);
	bb_addString(&con.settings.password, "8888888888888888");
	printf("pass : %s\n", con.settings.password);

	con.trace = GX_TRACE_LEVEL_VERBOSE;
}

void lnt_cfg()
{
	con.settings.interfaceType = DLMS_INTERFACE_TYPE_HDLC;
	con.settings.clientAddress = 32;
	con.settings.authentication = DLMS_AUTHENTICATION_LOW;

	// rithika 27oct2025
	bb_clear(&con.settings.password);
	bb_init(&con.settings.password);
	bb_addString(&con.settings.password, "lnt1");

	con.trace = GX_TRACE_LEVEL_VERBOSE;
}
void genus_cfg()
{
	con.settings.interfaceType = DLMS_INTERFACE_TYPE_HDLC;
	con.settings.clientAddress = 32;
	con.settings.authentication = DLMS_AUTHENTICATION_LOW;
	con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION_ENCRYPTION;

	bb_clear(&con.settings.cipher.systemTitle);
	bb_addHexString(&con.settings.cipher.systemTitle, "474F453030303030");

	bb_clear(&con.settings.cipher.authenticationKey);
	bb_addHexString(&con.settings.cipher.authenticationKey, "41654D6C456B416B6761506C30316162");

	bb_clear(&con.settings.cipher.blockCipherKey);
	bb_addHexString(&con.settings.cipher.blockCipherKey, "41654D6C456B416B6761506C30316162");

	// rithika 27oct2025
	bb_clear(&con.settings.password);
	bb_init(&con.settings.password);
	bb_addString(&con.settings.password, "1A2B3C4D");

	con.trace = GX_TRACE_LEVEL_VERBOSE;
}

void bentec_cfg()
{
	con.settings.interfaceType = DLMS_INTERFACE_TYPE_WRAPPER;
	con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION_ENCRYPTION;
	con.trace = GX_TRACE_LEVEL_VERBOSE;
	con.settings.clientAddress = 48;						// us type
	con.settings.authentication = DLMS_AUTHENTICATION_HIGH; // high

	bb_clear(&con.settings.cipher.systemTitle);
	printf("before system title %s len : %d\n", con.settings.cipher.systemTitle, con.settings.cipher.systemTitle.size);
	bb_init(&con.settings.cipher.systemTitle);
	bb_addHexString(&con.settings.cipher.systemTitle, "4245450000989695"); // 4245450000249F9C
	printf("system title %s len : %d\n", con.settings.cipher.systemTitle, con.settings.cipher.systemTitle.size);

	bb_clear(&con.settings.cipher.authenticationKey);
	printf("before authenticationKey '%s' len : %d\n", con.settings.cipher.authenticationKey, con.settings.cipher.authenticationKey.size);
	bb_init(&con.settings.cipher.authenticationKey);
	bb_addHexString(&con.settings.cipher.authenticationKey, "42454E4B45595450444C323631313234");
	printf("authenticationKey '%s' len : %d\n", con.settings.cipher.authenticationKey, con.settings.cipher.authenticationKey.size);

	bb_clear(&con.settings.cipher.blockCipherKey);
	printf("before blockCipherKey '%s' len : %d\n", con.settings.cipher.blockCipherKey, con.settings.cipher.blockCipherKey.size);
	bb_init(&con.settings.cipher.blockCipherKey);
	bb_addHexString(&con.settings.cipher.blockCipherKey, "42454E4B45595450444C323631313234");
	printf("blockCipherKey '%s' len : %d\n", con.settings.cipher.blockCipherKey, con.settings.cipher.blockCipherKey.size);

	// rithika 27oct2025
	bb_clear(&con.settings.password);
	bb_init(&con.settings.password);
	bb_addString(&con.settings.password, "BENTECTPDL261124");
	printf("pass : %s len : %d\n", con.settings.password, con.settings.password.size);

	if (con.settings.cipher.dedicatedKey != NULL)
	{
		printf("Dedicate key not equal to null\n");

		// Clear the buffer contents
		bb_clear(con.settings.cipher.dedicatedKey);
		printf("Dedicate key cleared\n");

		// Free the buffer data if it exists
		if (con.settings.cipher.dedicatedKey->data != NULL)
		{
			free(con.settings.cipher.dedicatedKey->data);
			con.settings.cipher.dedicatedKey->data = NULL;
		}

		// Free the buffer structure
		free(con.settings.cipher.dedicatedKey);
		printf("Dedicate key freed\n");

		con.settings.cipher.dedicatedKey = NULL;
		printf("Dedicate key set to NULL\n");
	}
	else
	{
		printf("Dedicate key equal to null\n");

		// if (con.settings.password != NULL)
		// {
		//     printf("pass: %s\n", con.settings.password);
		// }

		// Allocate new ByteBuffer structure
		con.settings.cipher.dedicatedKey = (gxByteBuffer *)malloc(sizeof(gxByteBuffer));
		if (con.settings.cipher.dedicatedKey == NULL)
		{
			printf("Failed to allocate dedicatedKey buffer\n");
			return -1;
		}

		// Initialize the buffer
		con.settings.cipher.dedicatedKey->data = NULL;
		con.settings.cipher.dedicatedKey->size = 0;
		con.settings.cipher.dedicatedKey->position = 0;
		con.settings.cipher.dedicatedKey->capacity = 0;

		// Clear the buffer (although it's already empty, keeping for consistency)
		bb_clear(con.settings.cipher.dedicatedKey);
		printf("Dedicated key initialized\n");

		// Add hex string to buffer
		if (bb_addHexString(con.settings.cipher.dedicatedKey, "42454E4B45595450444C323631313234") != 0)
		{
			printf("Failed to add hex string to buffer\n");
			free(con.settings.cipher.dedicatedKey);
			con.settings.cipher.dedicatedKey = NULL;
			return -1;
		}

		printf("Hex string added to dedicated key\n");

		// Print the dedicated key contents safely
		if (con.settings.cipher.dedicatedKey->data != NULL)
		{
			printf("dedicatedKey contents: ");
			int i;
			for (i = 0; i < con.settings.cipher.dedicatedKey->size; i++)
			{
				printf("%02X", con.settings.cipher.dedicatedKey->data[i]);
			}
			printf("\n");
		}
	}
}

uint8_t close_dlms_port()
{
	printf("********************************* close port *********************\n");
	dbg_log(WARNING, "%s : close_dlms_port\n", "close_dlms_port");
	com_close(&con);
	sleep(2);
	// con_close(&con);

	con.comPort = -1;

	com_open(&con, p_comm_port_det);

	// con_init(&con, GX_TRACE_LEVEL_INFO);
	// sleep(2);
	// cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);
	// sleep(2);
}

uint8_t init_buffer()
{
	dbg_log(INFORM, "%s : init_buffer\n", "init_buffer");

	// printf("con.settings.password.data = %p\n", con.settings.password.data);

	cl_clear(&con.settings);

	bb_clear(&con.data);

	con_initializeBuffers(&con, 0);

	con_init(&con, GX_TRACE_LEVEL_INFO);
	// sleep(2);
	// sleep(1);

	cl_init(&con.settings, 1, 16, 1, DLMS_AUTHENTICATION_NONE, NULL, DLMS_INTERFACE_TYPE_HDLC);
	// sleep(2);
	// sleep(1);
}

void meter_cfg(int midx)
{

	int dlms_auth_type = DLMS_AUTHENTICATION_LOW;
	con.settings.interfaceType = DLMS_INTERFACE_TYPE_HDLC;
	con.settings.clientAddress = 32;
	con.settings.authentication = dlms_auth_type;
	// con.settings.cipher.security = DLMS_SECURITY_AUTHENTICATION_ENCRYPTION;

	// rithika 23/10/2025
	con.settings.maxInfoTX = 128;
	con.settings.maxInfoRX = 128;
	// con.settings.hdlc.maxFrameCount = 1;
	con.waitTime = 2000; // ms timeout per read

	con.settings.serverAddress = ser_port_data[g_pidx].meter_cfg[midx].meter_addr;
	printf("------------------ before server address %d\n", con.settings.serverAddress);
	con.settings.serverAddress = cl_getServerAddress(1, (unsigned short)con.settings.serverAddress, 4);
	printf("------------------ after server address %d\n", con.settings.serverAddress);

	// bb_clear(&con.settings.cipher.systemTitle);
	// bb_addHexString(&con.settings.cipher.systemTitle, "474F453030303030");

	// bb_clear(&con.settings.cipher.authenticationKey);
	// bb_addHexString(&con.settings.cipher.authenticationKey, "41654D6C456B416B6761506C30316162");

	// bb_clear(&con.settings.cipher.blockCipherKey);
	// bb_addHexString(&con.settings.cipher.blockCipherKey, "41654D6C456B416B6761506C30316162");

	// char *pwd = NULL;
	// pwd = ser_port_data[g_pidx].meter_cfg[midx].met_pw;

	// rithika 27oct2025
	bb_clear(&con.settings.password);
	bb_init(&con.settings.password);

	bb_addString(&con.settings.password, ser_port_data[g_pidx].meter_cfg[midx].met_pw);

	con.trace = GX_TRACE_LEVEL_VERBOSE;
}

// rithika 17oct
int dis_connect()
{

	// rithika 18oct20225
	int ret = com_disconnect(&con);
	// rithika 23/10/2025
	tcflush(&con.comPort, TCIOFLUSH); // Flush both input and output queues
	// connection->comPort
	// usleep(20000);
	usleep(100000); // 100 millisec

	return ret;
}

int init_dlms_serial(int midx)
{
	int ret = 0;
	char fun_name[] = "init_dlms_serial";
	// 23oct
	meter_cfg(midx);
	con.settings.connected = 1;

	// sleep for 10s

	char temp_met_sn[64];

	// char *serial_number = get_meter_2_ser_num(midx);

	// if (serial_number == NULL)
	// {
	// 	dbg_log(INFORM, "[Meter_%d]:[%-25s]:  Failed to get meter serial number.\n", fun_name, midx);
	// }
	char serial_number[64] = {};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number.\n", midx, fun_name);
	}
	else
	{
		snprintf(temp_met_sn, sizeof(temp_met_sn), "%s", serial_number);
	}

	// char *manufacturer = get_manufacturer(midx, temp_met_sn);

	// if (manufacturer == NULL)
	// {
	// 	dbg_log(INFORM, "[Meter_%d]:[%-25s]:  Failed to get meter manufacturer.\n", fun_name, midx);
	// }
	// else
	// {
	// 	snprintf(temp_manufacturer, sizeof(temp_manufacturer), "%s", manufacturer);
	// }

	char temp_manufacturer[64] = {0};
	if (get_manufacturer(midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter manufacturer.\n", midx, fun_name);
	}

	if (strcmp(temp_manufacturer, "secure") == 0)
	{
		sleep(8);
		ret = com_disconnect(&con);
		if (ret != 0)
		{
			sleep(8);
		}
	}
	// else if(strcmp(temp_manufacturer, "genus") == 0){
	// 	sleep(2);
	// 	ret = com_disconnect(&con);
	// 	if (ret != 0)
	// 	{
	// 		sleep(2);
	// 	}
	// }
	else
	{
		com_disconnect(&con);
	}
	// if failed sleep for 8 seconds

	init_buffer();

	meter_cfg(midx);

	// printf("after con.settings.password.data = %p\n", con.settings.password.data);

	ret = com_initializeConnection(&con);
	if (ret != 0)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ failed\n", midx, fun_name);
		// snrm_aarq_failed_cnt("[%s] MANF : %s.SNRM-AARQ failed.\n", time_format_conversion(time(NULL)), plcc_modem_info.plcc_meter_map[midx].meter_manf_name);
		return -1;
	}
	else
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ Success\n", midx, fun_name);
	}
	return 0;
}

/**
 * @brief Initializes the DLMS connection with specified settings.
 *
 * This function configures the connection settings based on the meter information type.
 * It initializes parameters such as client and server addresses, authentication methods,
 * and cipher settings. It attempts to establish the connection and retrieve the association view.
 *
 * @param midx An integer index for the meter (not used in this implementation).
 * @return Returns 0 on success or a negative error code if initialization fails.
 */
int init_dlms(int midx)
{

	static char fun_name[] = "init_dlms()";
	int ret;

	if (strcmp(feature_cfg.meter_inf_type, "PLCC") != 0)
	{
		ret = init_dlms_serial(midx);
		return ret;
	}
	// strcpy(plcc_modem_info.plcc_meter_map[midx].meter_manf_name, "LARSEN"); // rithika
	// printf("################ %s\n", plcc_modem_info.plcc_meter_map[midx].meter_manf_name);
	print_colored_message(YELLOW, 1, "++++++++++++++++++++++++++++++++++++ MANF NAME : %s +++++++++++++++++++++++++++\n", plcc_modem_info.plcc_meter_map[midx].meter_manf_name);

	curr_nc_idx = midx;

	if ((strstr(plcc_modem_info.plcc_meter_map[midx].meter_manf_name, "GOESPHWCN")) || (strstr(plcc_modem_info.plcc_meter_map[midx].meter_manf_name, "GOERAPDRP")))
	{
		genus_cfg();

		con.settings.connected = 1;

		com_disconnect(&con);

		init_buffer();

		genus_cfg();

		ret = com_initializeConnection(&con);
		if (ret != 0)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ failed\n", midx, fun_name);
			// snrm_aarq_failed_cnt("[%s] MANF : %s.SNRM-AARQ failed.\n", time_format_conversion(time(NULL)), plcc_modem_info.plcc_meter_map[midx].meter_manf_name);
			return -1;
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ Success\n", midx, fun_name);
		}
	}
	else if (strstr(plcc_modem_info.plcc_meter_map[midx].meter_manf_name, "HPL"))
	{
		printf("++++++++++++++++++++++++++++++++++++ INSIDE HPL +++++++++++++++++++++++++++\n");

		hpl_config();

		con.settings.connected = 1;

		com_disconnect(&con);

		hpl_config();

		init_buffer();

		ret = com_initializeConnection(&con);
		if (ret != 0)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ failed\n", midx, fun_name);
			// snrm_aarq_failed_cnt("[%s] MANF : %s.SNRM-AARQ failed.\n", time_format_conversion(time(NULL)), plcc_modem_info.plcc_meter_map[midx].meter_manf_name);
			return -1;
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ Success\n", midx, fun_name);
		}
	}
	else if (strstr(plcc_modem_info.plcc_meter_map[midx].meter_manf_name, "LARSEN"))
	{

		lnt_cfg();
		ret = com_initializeConnection(&con);

		if (ret != 0)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ failed\n", midx, fun_name);
			// snrm_aarq_failed_cnt("[%s] MANF : %s.SNRM-AARQ failed.\n", time_format_conversion(time(NULL)), plcc_modem_info.plcc_meter_map[midx].meter_manf_name);
			return -1;
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ Success\n", midx, fun_name);
		}
	}
	else if (strstr(plcc_modem_info.plcc_meter_map[midx].meter_manf_name, "BEE"))
	{
		bentec_cfg();

		con.settings.connected = 1;

		com_disconnect(&con);

		init_buffer();

		bentec_cfg();

		char *invocationCounter = "0.0.43.1.3.255";
		printf("invocationCounter '%s'.", invocationCounter);

		struct timeval start, end;

		gettimeofday(&start, NULL);

		ret = com_updateInvocationCounter(&con, invocationCounter);
		if (ret == 0)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : updateInvocationCounter Success\n", midx, fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : updateInvocationCounter failed\n", midx, fun_name);
			// snrm_aarq_failed_cnt("[%s] MANF : %s.updateInvocationCounter failed.\n", time_format_conversion(time(NULL)), plcc_modem_info.plcc_meter_map[midx].meter_manf_name);
			return -1;
		}

		// bentec_cfg();
		if ((ret = com_initializeConnection(&con)) == 0)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ Success\n", midx, fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : SNRM-AARQ failed\n", midx, fun_name);
			snrm_aarq_failed_cnt("[%s] MANF : %s.SNRM-AARQ failed.\n", time_format_conversion(time(NULL)), plcc_modem_info.plcc_meter_map[midx].meter_manf_name);
			return -1;
		}

		int clk_ret = gettimeofday(&end, NULL);
		// Compute duration in seconds (with microsecond precision)
		double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		// Print duration
		// print_colored_message(YELLOW, 1, "disc Time: %.6f seconds\n", duration);
	}

	return 0;
}

int get_load_status(int midx, unsigned char *status)
{
	static char fun_name[] = "get_load_status()";
	int ret;
	gxDisconnectControl dc;
	// Same Logical Name as used for disconnect/reconnect
	unsigned char ln[] = {0, 0, 96, 3, 10, 255};

	// Initialize disconnect control object
	INIT_OBJECT(dc, DLMS_OBJECT_TYPE_DISCONNECT_CONTROL, ln);

	curr_nc_idx = midx;

	// Read attribute 1.0.96.3.10.255.2 (output state)
	// Attribute index 2 represents the output state in DisconnectControl class
	ret = com_read(&con, &dc.base, 2);

	if (ret == 0)
	{
		// Get the boolean status
		*status = dc.outputState;
		// true (1) means connected
		// false (0) means disconnected
	}

	return ret;
}

int load_reconnect(int midx)
{
	static char fun_name[] = "load_reconnect()";
	int ret;
	gxDisconnectControl dc;
	unsigned char loadStatus;

	curr_nc_idx = midx;

	if (init_dlms(midx) < 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : load_reconnect Failed\n", midx, fun_name);
		return -1;
	}

	// Logical Name (LN) for the disconnect control object
	unsigned char ln[] = {0, 0, 96, 3, 10, 255};

	// Initialize a disconnect control object with the logical name
	INIT_OBJECT(dc, DLMS_OBJECT_TYPE_DISCONNECT_CONTROL, ln);

	// Set up parameter for the reconnect method
	dlmsVARIANT param;
	GX_INT8(param) = 900; // 1 for reconnect instead of 0

	// Call the reconnect method
	ret = com_method(&con, &dc.base, 2, &param);

	if (ret == 0)
	{
		int result = get_load_status(midx, &loadStatus);
		if (result == 0)
		{
			if (loadStatus)
			{
				dbg_log(REPORT, "[Meter_%d]:[%-25s] : Load status -  Connected.\n", midx, fun_name);
			}
			else
			{
				dbg_log(REPORT, "[Meter_%d]:[%-25s] : Load status -  Disonnected.\n", midx, fun_name);
			}
		}
	}
	else
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : Error reading load status: %d\n", midx, fun_name, ret);
		return ret;
	}
	get_inst_data(midx);
	return 0;
}

int load_disconnect(int midx)
{
	static char fun_name[] = "load_disconnect()";
	int ret;
	gxDisconnectControl dc;
	unsigned char loadStatus;

	curr_nc_idx = midx;

	if (init_dlms(midx) < 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : load_disconnect Failed\n", midx, fun_name);
		return -1;
	}

	// Logical Name (LN) for the disconnect control object
	unsigned char ln[] = {0, 0, 96, 3, 10, 255};

	// Initialize a disconnect control object with the logical name
	INIT_OBJECT(dc, DLMS_OBJECT_TYPE_DISCONNECT_CONTROL, ln);

	// Set up parameter for the reconnect method
	dlmsVARIANT param;
	GX_INT8(param) = 1; // 1 for reconnect instead of 0

	// Call the reconnect method
	ret = com_method(&con, &dc.base, 1, &param);

	if (ret == 0)
	{
		int result = get_load_status(midx, &loadStatus);
		if (result == 0)
		{
			if (loadStatus)
			{
				dbg_log(REPORT, "[Meter_%d]:[%-25s] : Load status -  Connected.\n", midx, fun_name);
			}
			else
			{
				dbg_log(REPORT, "[Meter_%d]:[%-25s] : Load status -  Disonnected.\n", midx, fun_name);
			}
		}
	}
	else
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : Error reading load status: %d\n", midx, fun_name, ret);
		return ret;
	}

	get_inst_data(midx);
	return 0;
}

/**
 * @brief Reads a single OBIS value from a device.
 *
 * This function initializes a data object with the provided logical name,
 * performs a read operation, and stores the value in the provided output buffer.
 * The function handles different data types (string, uint16, uint8, octet string).
 *
 * @param logicalname_dn A pointer to the logical name of the device.
 * @param obis_val A character buffer to store the read OBIS value.
 * @return Returns the data type of the value read or a negative error code on failure.
 */
// int single_obis_read(unsigned char *logicalname_dn, char *obis_val)
// {
// 	int ret;
// 	uint16_t obis_val1;

// 	memset(obis_val, 0, sizeof(*obis_val));

// 	gxData *_deviceName = (gxData *)malloc(sizeof(gxData));

// 	INIT_OBJECT((*_deviceName), DLMS_OBJECT_TYPE_DATA, logicalname_dn);

// 	ret = com_read(&con, &_deviceName->base, 2);
// 	if (ret != DLMS_ERROR_CODE_OK)
// 	{
// 		return ret;
// 	}

// 	printf("TYPE OF THE VALUE :  %d\n", _deviceName->value.vt);

// 	obis_val = _deviceName->value.strVal->data;
//     char *tmp = bb_toString(_deviceName->value.strVal);
//     printf("\n Device Name: %s", tmp);
//     free(tmp);
//     printf("\n Device Name: %s\n", obis_val);

// 	switch (_deviceName->value.vt)
// 	{
// 	case DLMS_DATA_TYPE_STRING:
// 		if (_deviceName && _deviceName->value.strVal)
// 		{
// 			char *result = bb_toString(_deviceName->value.strVal);
// 			if (result)
// 			{
// 				strncpy(obis_val, result, 128);
// 				obis_val[128 - 1] = '\0';
// #if 0
// 				printf("VALUE :  %s\n", obis_val);
// #endif
// 			}
// 			else
// 			{
// 				printf("bb_toString returned NULL");
// 			}
// 		}
// 		else
// 		{
// 			strncpy(obis_val,"NA", 128);
// 		}

// 		break;

// 	case DLMS_DATA_TYPE_UINT16:

// 		memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
// #if 1
// 		printf("VALUE :  %d\n", obis_val[0]);
// #endif
// 		break;

// 	case DLMS_DATA_TYPE_UINT8:

// 		memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
// #if 0
// 		printf("VALUE :  %d\n", obis_val[0]);
// #endif
// 		break;

// 	case DLMS_DATA_TYPE_OCTET_STRING:

// 		strcpy(obis_val, bb_toString(_deviceName->value.strVal));
// #if 0
// 		printf("VALUE :  %s\n", obis_val);
// #endif
// 		break;

// 	case DLMS_DATA_TYPE_ENUM:

// 		memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
// #if 0
// 		printf("VALUE :  %d\n", obis_val[0]);
// #endif

// 		break;

// 	default:
// 		return -1;
// 	}

// 	return _deviceName->value.vt;
// }

int transfrmr_ratio_voltage()
{
	int ret;

	gxRegister reg;
	ret = cosem_init(&reg.base, DLMS_OBJECT_TYPE_REGISTER, "1.0.0.4.6.255");

	ret = com_read(&con, &reg.base, 2);

	if (ret != DLMS_ERROR_CODE_OK)
	{
		printf("failed to read\n");
		return ret;
	}

	float val = reg.value.uiVal;
	printf("UINT16 VALUE : %f\n", val);

	// snprintf(obis_val, 128, "%u", val); // store as string if needed

	printf("succeeded to read\n");
}

int ipv4_read(char *obis_val)
{
	int ret;

	gxIp4Setup ipv4;
	ret = cosem_init(&ipv4.base, DLMS_OBJECT_TYPE_IP4_SETUP, "0.0.25.1.0.255");

	ret = com_read(&con, &ipv4.base, 3);

	if (ret != DLMS_ERROR_CODE_OK)
	{
		printf("failed to read\n");
		return ret;
	}

	uint32_t ip = ipv4.ipAddress;

	unsigned char b1 = (ip >> 24) & 0xFF;
	unsigned char b2 = (ip >> 16) & 0xFF;
	unsigned char b3 = (ip >> 8) & 0xFF;
	unsigned char b4 = ip & 0xFF;

	printf("IP Address: %u.%u.%u.%u\n", b1, b2, b3, b4);

	snprintf(obis_val, 128, "%u.%u.%u.%u", b1, b2, b3, b4);

	// printf("succeeded to read %s\n",obis_val);
}

int hdlc_read(char *obis_val)
{
	int ret;

	gxIecHdlcSetup hdlc;
	ret = cosem_init(&hdlc.base, DLMS_OBJECT_TYPE_IEC_HDLC_SETUP, "0.0.22.0.0.255");

	ret = com_read(&con, &hdlc.base, 9);

	if (ret != DLMS_ERROR_CODE_OK)
	{
		printf("failed to read\n");
		return ret;
	}

	uint16_t val = hdlc.deviceAddress;
	printf("UINT16 VALUE : %u\n", val);
	snprintf(obis_val, 128, "%u", val);
	// snprintf(obis_val, 128, "%u", val); // store as string if needed

	printf("succeeded to read\n");
}

int single_obis_read(unsigned char *logicalname_dn, char *obis_val)
{
	int ret;
	// unsigned char logicalname_dn[] = {0, 0, 94, 91, 11, 255}; // device name
	// Read - Device Name

	memset(obis_val, 0, sizeof(obis_val));

	gxData *_deviceName = (gxData *)malloc(sizeof(gxData));

	if (!_deviceName)
	{
		printf("Memory allocation failed for _deviceName\n");
		strcpy(obis_val, "NA");
		return -1; // or an appropriate error code
	}

	INIT_OBJECT((*_deviceName), DLMS_OBJECT_TYPE_DATA, logicalname_dn);

	ret = com_read(&con, &_deviceName->base, 2);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		free(_deviceName);
		return ret;
	}

	printf("TYPE OF THE VALUE :  %d\n", _deviceName->value.vt);

	switch (_deviceName->value.vt)
	{
	case DLMS_DATA_TYPE_STRING:
	{
		// if (_deviceName && _deviceName->value.strVal)
		// {
		if (_deviceName->value.strVal) // ✅ no need to check _deviceName anymore
		{
			char *result = bb_toString(_deviceName->value.strVal);
			if (result)
			{
				// Ensure that obis_val has enough space
				strncpy(obis_val, result, 128); // Use a safe method like strncpy to avoid overflow
				obis_val[128 - 1] = '\0';		// Null-terminate in case of truncation
				printf("VALUE :  %s\n", obis_val);
				free(result);
			}
			else
			{
				printf("Handle error: bb_toString returned NULL");
			}
		}
		else
		{
			printf("Handle error: _deviceName or _deviceName->value.strVal is NULL");
			strcpy(obis_val, "NA");
		}

		break;
	}

	case DLMS_DATA_TYPE_OCTET_STRING:
	{

		// rithika 31oct2025 to handle the block interval that is coming as octet string
		if (logicalname_dn[0] == 1 && logicalname_dn[1] == 0 && logicalname_dn[2] == 0 && logicalname_dn[3] == 8 && logicalname_dn[4] == 4 && logicalname_dn[5] == 255 && _deviceName->value.strVal->size == 2)
		{
			uint32_t data1 = 0;
			data1 = (_deviceName->value.strVal->data[0] & 0XFF) << 8;
			data1 = data1 | _deviceName->value.strVal->data[1] & 0XFF;
			printf("block interval data1 %u\n", data1);
			memcpy(obis_val, &data1, 2);
		}
		else
		{
			// rithika 05/12/2025
			char *tmp = bb_toString(_deviceName->value.strVal);
			if (tmp)
			{
				strcpy(obis_val, tmp);
				free(tmp);
			}
			// strcpy(obis_val, bb_toString(_deviceName->value.strVal));
			printf("before parsing  :  %s*******\n", obis_val);
		}

		break;
	}

	case DLMS_DATA_TYPE_UINT16:
	{
		// memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
		// printf("with * %d\n", obis_val);
		// break;
		// rithika 06nov2025
		uint16_t val = _deviceName->value.uiVal;
		printf("UINT16 VALUE : %u\n", val);
		snprintf(obis_val, 128, "%u", val); // store as string if needed
		break;
	}
	case DLMS_DATA_TYPE_UINT32:
		memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
		printf("with * %d\n", obis_val[0]);
		break;

	case DLMS_DATA_TYPE_UINT8:

		memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
		printf("with * %d\n", obis_val[0]);
		break;

	case DLMS_DATA_TYPE_ENUM:

		memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
#if 1
		printf("VALUE :  %d\n", obis_val[0]);
#endif
		break;

	case DLMS_DATA_TYPE_FLOAT32:
		// memcpy(obis_val, &_deviceName->value.uiVal, sizeof(_deviceName->value.uiVal));
		// printf("VALUE :  %d\n", obis_val[0]);
		// break;
		{
			float fval;
			memcpy(&fval, &_deviceName->value.uiVal, sizeof(fval));
			snprintf(obis_val, 128, "%f", fval);
			break;
		}

	default:
		printf("----\n");
		free(_deviceName);
		return -1;
	}

	// obis_value = _deviceName->value.strVal->data;
	// char *tmp = bb_toString(_deviceName->value.strVal);
	// printf("\n Device Name: %s", tmp);
	// free(tmp);
	// printf("\n Device Name: %s\n", obis_value);

	// return _deviceName->value.vt; // 19sep rithika
	int return_vt = _deviceName->value.vt; // save return value
	free(_deviceName);					   // ✅ free memory before return
	return return_vt;

	// Value is "ISK0070777807016ýýýý"
}
/**
 * @brief Retrieves the block period from a device.
 *
 * This function constructs an OBIS identifier for the block period,
 * reads the corresponding value from the device, and converts it
 * from seconds to minutes.
 *
 * @return The block period in minutes, or -1 on error if reading fails.
 */
int get_blk_period()
{
	static char fun_name[] = "get_int_blk_period()";
	uint8_t obis[6] = {1, 0, 0, 8, 4, 255};

	char obis_val[256];
	uint32_t blk_period;

	unsigned char *uchar_var = obis;
	int val_type = single_obis_read(uchar_var, &obis_val);

	if (val_type == 18)
	{
		// val points to ASCII string like "900"
		printf("!!!!!!!! val for block %s\n", obis_val);
		blk_period = (uint32_t)atoi(obis_val); // convert string to int safely
	}
	else
	{
		memcpy(&blk_period, obis_val, 4);
	}

	printf("blk_period : %u\n", blk_period);

	return (blk_period / 60);
}

/**
 * @brief Reads the current date and time from the meter.
 *
 * This function initializes the connection to the meter, reads the
 * clock object, and formats the date and time into a string.
 *
 * @return A string containing the date and time in "YYYY-MM-DD HH:MM" format,
 *         or "-1" on error.
 */
char *meterDateTime_read()
{
	int ret;

	gxClock clock;
	ret = cosem_init(&clock.base, DLMS_OBJECT_TYPE_CLOCK, "0.0.1.0.0.255");

	// // Initialize connection.
	// ret = com_initializeConnection(&con);
	// if (ret != DLMS_ERROR_CODE_OK)
	// {
	// 	return "-1";
	// }
	ret = com_read(&con, &clock.base, 2);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		return NULL;
	}
	else if (clock.time.value.tm_mday == 0 || clock.time.value.tm_year == 0)
	{
		return NULL;
	}

	printf("Time Zone Offset: %d minutes deviatoion %d\n", clock.timeZone, clock.deviation);
	memset(time_buff, 0, sizeof(time_buff));

	// rithika 08oct2025
	// sprintf(time_buff, "%d-%d-%d %d:%d", clock.time.value.tm_year + 1900, clock.time.value.tm_mon + 1, clock.time.value.tm_mday, clock.time.value.tm_hour, clock.time.value.tm_min);

	sprintf(time_buff, "%d-%d-%d %d:%d:%02d", clock.time.value.tm_year + 1900, clock.time.value.tm_mon + 1, clock.time.value.tm_mday, clock.time.value.tm_hour, clock.time.value.tm_min, clock.time.value.tm_sec);

#if 0
	printf("meterDateTime : %s\n", time_buff);
#endif
	return time_buff;
}

/**
 * @brief Parses the date and time from a string and stores it in a meter_date_time array.
 *
 * This function extracts the year, month, day, hour, and minute from the provided
 * message string and assigns them to the appropriate fields in the meter_date_time
 * structure for the given meter index (midx).
 *
 * @param midx The index of the meter for which the date and time is being set.
 * @param msg The string containing the date and time in the format "YYYY-MM-DD HH:MM".
 * @param len The length of the input string (not used in this implementation).
 *
 * @return Returns RET_OK on success.
 */
int32_t get_meter_date_time(uint8_t midx, char *msg, int32_t len)
{
	static char fun_name[] = "get_meter_date_time()";
	uint16_t year = 0;

	sscanf(msg, "%4hu", &year);

	meter_date_time[midx].year = year;
	// sscanf(msg, "%*4hu-%2hhu-%2hhu %2hhu:%2hhu", &meter_date_time[midx].month, &meter_date_time[midx].day, &meter_date_time[midx].hour, &meter_date_time[midx].minute,&meter_date_time[midx].second);

	sscanf(msg, "%4hu-%2hhu-%2hhu %2hhu:%2hhu:%2hhu",
		   &meter_date_time[midx].year,
		   &meter_date_time[midx].month,
		   &meter_date_time[midx].day,
		   &meter_date_time[midx].hour,
		   &meter_date_time[midx].minute,
		   &meter_date_time[midx].second);

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Curr MtrDT : %02d_%02d_%04d %02d:%02d:%02d\n", midx, fun_name, meter_date_time[midx].day,
			meter_date_time[midx].month,
			meter_date_time[midx].year,
			meter_date_time[midx].hour,
			meter_date_time[midx].minute,
			meter_date_time[midx].second);

	// rithika 03/09/2025

	snprintf(meter_time_str[midx], sizeof(meter_time_str[midx]), "%04d-%02d-%02d %02d:%02d:%02d",
			 meter_date_time[midx].year,
			 meter_date_time[midx].month,
			 meter_date_time[midx].day,
			 meter_date_time[midx].hour,
			 meter_date_time[midx].minute,
			 meter_date_time[midx].second);

	return RET_OK;
}

/**
 * @brief Retrieves the current date and time from the meter and stores it in the meter_date_time array.
 *
 * This function reads the current date and time from the meter using the
 * `meterDateTime_read` function, then parses this data and stores it for
 * the specified meter index (midx).
 *
 * @param midx The index of the meter for which the current date and time are being retrieved.
 *
 * @return Returns RET_OK on success.
 */
int32_t get_curr_date_time(uint8_t midx)
{
	static char fun_name[] = "get_curr_date_time()";

	char *charString = meterDateTime_read();

	// printf("%s\n", charString);
	// rithika 02/12/2025
	if (charString == NULL)
	{
		return -1;
	}
	get_meter_date_time(midx, charString, 1);
	return RET_OK;
}

/**
 * @brief Retrieves and stores the nameplate details of the meter based on the provided index.
 *
 * This function updates the `nameplate_info` structure with various details of the meter
 * based on the specified index (np_idx). The values are passed as a string and are
 * stored in the corresponding fields of the nameplate information.
 *
 * @param np_idx The index indicating which nameplate detail to retrieve.
 * @param val The value to store in the specified nameplate detail field.
 *
 * @return Returns RET_OK on success.
 */
int32_t get_name_plate_det(uint8_t np_idx, char *val, int return_type)
{
	static char fun_name[] = "get_name_plate_det()";

	uint8_t temp = 0;

	uint16_t *int_ptr;
	printf("val :: %s \n", val);

#if 0
	printf("get_name_plate_det np_idx : %d  val : %u\n", np_idx, (unsigned int)val[0]);
#endif

	switch (np_idx)
	{
	case 0:
		memset(nameplate_info.serial_number, 0, sizeof(nameplate_info.serial_number));

		// rithika 18/08/2025 to handle space at the end
		// int size = strlen(val);
		// for (int i = 0; i < size; i++)
		// {
		// 	if (val[i] == ' ')
		// 	{
		// 		val[i] = '\0';
		// 		break;
		// 	}
		// }

		// strcpy(nameplate_info.serial_number, val);
		char ser_num[32];
		int i, j = 0;
		for (i = 0; i < strlen(val); i++)
		{
			if (val[i] != ' ')
			{
				ser_num[j] = val[i];
				j++;
			}
		}
		ser_num[j] = '\0';

		// // rithika 28Nov2025
		// for (i = 0; i < strlen(ser_num); i++)
		// {
		// 	if (ser_num[i] == "-")
		// 	{
		// 		ser_num[i] = "_";
		// 	}
		// }

		strcpy(nameplate_info.serial_number, ser_num);
		break;

	case 1:
		memset(nameplate_info.manufacturer, 0, 64);
		strcpy(nameplate_info.manufacturer, val);
		break;

	case 2:
		memset(nameplate_info.firmware_version, 0, 32);
		strcpy(nameplate_info.firmware_version, val);
		break;

	case 3:
		memset(nameplate_info.meter_type, 0, 32);
		// rithika 20Feb2026
		uint8_t mtr_type = (unsigned int)val[0];
		sprintf(nameplate_info.meter_type, "%u", mtr_type);
		break;
	case 4:
		memset(nameplate_info.ct_ratio_str, 0, 8);
		printf("^^^^^^^^^^^^ ct val %s ^^^^^^^^^^^^\n", val);
		if (!val || val[0] == '\0')
		{

			strcpy(nameplate_info.ct_ratio_str, "NA");
		}
		else
		{
			// int_ptr = (uint16_t *)val;

			// rithika 27sep
			nameplate_info.ct_ratio = atof(val);
		}
		break;
	case 5:
		memset(nameplate_info.pt_ratio_str, 0, 8);

		if (!val || val[0] == '\0')
		{
			strcpy(nameplate_info.pt_ratio_str, "NA");
		}
		else
		{
			// int_ptr = (uint16_t *)val;
			// nameplate_info.pt_ratio = (float)*val;
			// rithika 27sep
			nameplate_info.pt_ratio = atof(val);
		}
		break;

	case 6:
	{
		if (return_type == 18)
		{
			// val points to ASCII string like "900"

			int numeric_val = atoi(val); // convert string to int safely
			nameplate_info.block_int = numeric_val / 60;
		}
		else
		{
			uint16_t *int_ptr = (uint16_t *)val;

			nameplate_info.block_int = ((int)(*int_ptr)) / 60;
		}

		printf("nameplate_info.block_int %d\n", nameplate_info.block_int);
		break;
	}

	case 7:
	{
		memset(nameplate_info.meter_category, 0, 32);
		strcpy(nameplate_info.meter_category, val);
		break;
	}
	case 8:
	{
		memset(nameplate_info.curr_rating, 0, 32);
		strcpy(nameplate_info.curr_rating, val);
		break;
	}
	case 9:
	{
		memset(nameplate_info.year_of_manuf, 0, 32);
		strcpy(nameplate_info.year_of_manuf, val);
		break;
	}
	case 10:
	{
		nameplate_info.demand_int_period = atoi(val);

		break;
	}
	case 12:
	{
		memset(nameplate_info.ipv4_address, 0, 32);
		strcpy(nameplate_info.ipv4_address, val);
		break;
	}
	case 13:
	{
		nameplate_info.hdlc_device_address = atoi(val);

		break;
	}
	default:
		break;
	}

	return RET_OK;
}

/**
 * @brief Retrieves and logs the nameplate details of the meter.
 *
 * This function fetches the nameplate information such as serial number, manufacturer,
 * firmware version, meter type, CT ratio, PT ratio, and block interval from the meter
 * using OBIS codes. It also updates the nameplate information in a specified structure
 * and logs the retrieved details.
 *
 * @param midx The index of the meter for which the nameplate details are to be retrieved.
 *
 * @return Returns RET_OK on success, or -1 on failure to retrieve current date time.
 */
int32_t get_name_plate(uint8_t midx)
{

	static char fun_name[] = "get_name_plate()";

	char obis_val[256];
	char buffer[64];

	uint8_t idx = 0;

	float val = 0;

	struct timeval start, end;

	time_t curr_time = time(NULL);

	update_np_obis(); // rithika 17Feb2026

	gettimeofday(&start, NULL);

	if (get_curr_date_time(midx) < 0)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", midx, fun_name);
		return -1;
	}

	for (idx = 0; idx < MAX_NAMEPLATE_VALS; idx++)
	{
#if DEBUG
		printf("%s :NAME_PLATE_PARAMS_TYPE : %d\n", fun_name, idx);
#endif
		unsigned char *uchar_var = name_plate_obis[idx];

		memset(obis_val, 0, sizeof(obis_val));
		int val_type;

		if (idx == 12)
		{
			ipv4_read(&obis_val);
		}
		else if (idx == 13)
		{
			hdlc_read(&obis_val);
			val_type = 0;
		}
		else if (idx == 14)
		{
			transfrmr_ratio_voltage();
			val_type = 0;
		}

		else
		{
			val_type = single_obis_read(uchar_var, &obis_val);
		}
		print_colored_message(MAGENTA, 1, "************** val type : %d ***********\n", val_type);
		if (val_type < 0)
		{
			return -1;
		}
#if 1
		printf("obis val before calling   %u\n", (unsigned int)obis_val[0]);
#endif
		get_name_plate_det(idx, obis_val, val_type);
	}

	// printf("6666666 %f \n", nameplate_info.ct_ratio);
	// printf("7777777 %f \n", nameplate_info.pt_ratio);
	gettimeofday(&end, NULL);

	double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%.6f", duration);

	data_status_log("NP_DATA Total Time: %s seconds\n", buffer);

	// Print duration
	print_colored_message(YELLOW, 1, "NP_DATA Time: %.6f seconds\n", duration);

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter Serial Num : %s\n", midx, fun_name, nameplate_info.serial_number);
	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter MNF Name : %s\n", midx, fun_name, nameplate_info.manufacturer);
	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter FW Name : %s\n", midx, fun_name, nameplate_info.firmware_version);
	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter Type : %s\n", midx, fun_name, nameplate_info.meter_type);
	// rithika 27sep
	if (nameplate_info.pt_ratio_str[0] != '\0' || nameplate_info.ct_ratio_str[0] != '\0')
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter CT Ratio str: %s\n", midx, fun_name, nameplate_info.ct_ratio_str);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] :  Meter PT Ratio str: %s\n", midx, fun_name, nameplate_info.pt_ratio_str);
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter CT Ratio : %f \n", midx, fun_name, nameplate_info.ct_ratio);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter PT Ratio : %f\n", midx, fun_name, nameplate_info.pt_ratio);
	}
	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter block int : %d\n", midx, fun_name, nameplate_info.block_int);
	// rithika 18Feb2026
	if (RECONNECT)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter category : %s\n", midx, fun_name, nameplate_info.meter_category);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Meter current rating : %s\n", midx, fun_name, nameplate_info.curr_rating);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Year of manufacture : %s\n", midx, fun_name, nameplate_info.year_of_manuf);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Demand integration period : %d\n", midx, fun_name, nameplate_info.demand_int_period);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : HDLC device address : %d\n", midx, fun_name, nameplate_info.hdlc_device_address);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : IPV4 address : %s\n", midx, fun_name, nameplate_info.ipv4_address);

	}
	update_name_plate(midx, &nameplate_info);
	if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
	{
		strcpy(plcc_modem_info.plcc_meter_map[midx].meter_ser_num, nameplate_info.serial_number);
		set_plcc_modem_info_to_redis();
	}

	return RET_OK;
}

/**
 * @brief Reads scalar values from a meter based on the type of data specified.
 *
 * This function retrieves scalar values from a meter by identifying the correct
 * OBIS code based on the provided type of data. It handles reading both the
 * OBIS string and the scalar value, processing them accordingly.
 *
 * @param midx The index of the meter from which to read the scalar values.
 * @param type_of_data The type of data to be read, which determines the OBIS code.
 *
 * @return Returns RET_OK on success, or an error code on failure.
 */
int read_scalar_values(int midx, int type_of_data)
{
	static char fun_name[] = "read_scalar_values()";

	// gxProfileGeneric *obj;
	gxProfileGeneric pg;
	gxByteBuffer ba;

	char *data = NULL;
	char obis[32];

	struct timeval start, end;

	int ret = -1;

	memset(obis, 0, sizeof(obis));

	char serial_number[64] = {0};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		// handle error
	}

	// char *serial_number = get_meter_2_ser_num(midx);

	// if (serial_number == NULL)
	// {
	// 	printf("Failed to get meter serial number\n");

	// 	// return -1;
	// }

	switch (type_of_data)
	{
	case 6:
		strcpy(obis, INST_SCALER_OBIS);
		if (strlen(serial_number) > 0)
		{
			data_status_log("INST_SCALER_OBIS:%s ", serial_number);
		}

		break;

	case 7:
		strcpy(obis, MN_SCALER_OBIS);
		if (strlen(serial_number) > 0)
		{
			data_status_log("MN_SCALER_OBIS:%s ", serial_number);
		}
		break;

	case 8:
		strcpy(obis, BILLING_SCALER_OBIS);
		if (strlen(serial_number) > 0)
		{
			data_status_log("BILLING_SCALER_OBIS:%s ", serial_number);
		}
		break;

	case 9:
		strcpy(obis, LS_SCALER_OBIS);
		if (strlen(serial_number) > 0)
		{
			data_status_log("LS_SCALER_OBIS:%s ", serial_number);
		}
		break;

	case 10:
		strcpy(obis, EVENT_SCALER_OBIS);
		if (strlen(serial_number) > 0)
		{
			data_status_log("EVENT_SCALER_OBIS:%s ", serial_number);
		}
		break;

	default:
		break;
	}

	cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, obis);

	gettimeofday(&start, NULL);

	ret = com_read(&con, BASE(pg), OBIS_ATTR);
	if (ret != DLMS_ERROR_CODE_OK) // 03/09
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : SCALER VAL OBIS FAILED : RET : %d\n", midx, fun_name, ret);
		return -2;
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : SCALER VAL OBIS SUCCCESS : RET : %d\n", midx, fun_name, ret);
		ret = obj_toString(&pg, &data);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			return ret;
		}
		if (data != NULL)
		{
#if 0
			printf("SCALAR OBIS STRING : %s\n", data);
#endif
			countOBISCodes(data, type_of_data, midx); // needs to uncomment by priya
			obis_parsing(data, type_of_data, midx);
			free(data);
			data = NULL;
		}
	}

	ret = com_read(&con, BASE(pg), VAL_ATTR);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : SCALER VAL READING FAILED!!! RET : %d\n", midx, fun_name, ret);
		return -3;
	}
	else
	{

		int clk_ret = gettimeofday(&end, NULL);

		double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		// Print duration
		print_colored_message(YELLOW, 1, "scaler Time: %.6f seconds\n", duration);

		char buffer[64];

		snprintf(buffer, sizeof(buffer), "%.6f", duration);

		data_status_log("Total Time: %s S\n", buffer);

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : SCALER VAL SUCCCESS : RET : %d\n", midx, fun_name, ret);
		bb_init(&ba);
		obj_rowsToString(&ba, &pg.buffer);
		data = bb_toString(&ba);
#if DEBUG
		printf("SCALAR VAL %s\n", data);
#endif
		scaler_parsing(data, type_of_data);
		free(data);
		bb_clear(&ba);
		obj_clear(BASE(pg)); // rithika 05Sep
	}

	return RET_OK;
}

/**
 * @brief Scales instantaneous values based on corresponding scalar values.
 *
 * This function iterates through the instantaneous values and their
 * corresponding scalar values. If an OBIS code matches, it calculates
 * the scaled value and updates the instantaneous value.
 *
 * @return Returns 0 on success.
 */
int inst_scaler_val(int midx)
{
	char result_array[MAX_INST_VALS][32];
	int count1 = 4;
	int count2 = 4;
	int result_count = 0;

	for (int i = 0; i < inst_val_info.num_vals; i++)
	{
		for (int j = 0; j < inst_val_info.num_scaler_vals; j++)
		{
			// printf("i : %d j : %d [i].obis_code : %s [j].obis_code : %s value : %s\n", i, j, inst_val_info.values[i].obis_code, inst_val_info.scalar_values[j].obis_code, inst_val_info.values[i].val);

			// rithika 03nov2025
			//  if (strcmp(inst_val_info.values[i].obis_code, inst_val_info.scalar_values[j].obis_code) == 0)
			//  {
			if (strcmp(inst_val_obis[midx].obis_code[i], inst_val_info.scalar_values[j].obis_code) == 0)
			{
				double value = string_to_double(inst_val_info.values[i].val);
				// printf("%lf\n", value);
				double result = value * inst_val_info.scalar_values[j].multiplier;
				// printf("-----------------%lf\n", result);
				snprintf(inst_val_info.values[i].val, 30, "%lf", result);

				// printf("Matching OBIS: %s\n", inst_val_info.values[i].obis_code);
				// printf("Value: %s\n", inst_val_info.values[i].val);
				// printf("Scaling Factor: %f\n", inst_val_info.scalar_values[j].multiplier);
				// printf("Result: %s\n\n", result_array[i]);

				result_count++;
				break;
			}
		}
	}

	return 0;
}

int read_inst_data(connection *conn)
{
	printf("===================================== Start reading Instantaneous Profile ==============================\n");
	gxProfileGeneric pg;
	int ret;
	char *data = NULL;

	cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, "1.0.94.91.0.255");

	ret = com_read(conn, BASE(pg), 3);

	if (ret != DLMS_ERROR_CODE_OK)
	{

		return ret;
	}
	if (conn->trace > GX_TRACE_LEVEL_WARNING)
	{
		ret = obj_toString(&pg.base, &data);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			return ret;
		}
		if (data != NULL)
		{
			printf("%s", data);
			free(data);
			data = NULL;
		}
	}

	ret = com_read(conn, BASE(pg), 2);
	if (ret != DLMS_ERROR_CODE_OK)
	{

		return ret;
	}
	if (conn->trace > GX_TRACE_LEVEL_WARNING)
	{
		ret = obj_toString(&pg.base, &data);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			return ret;
		}
		if (data != NULL)
		{
			printf("%s", data);
			free(data);
			data = NULL;
		}
	}
	printf("======================================= End reading Instantaneous Profile ===============================\n");
	return 0;
}
/**
 * @brief Retrieves instantaneous data from a meter and processes it.
 *
 * This function performs several tasks:
 * - Retrieves the current date and time from the meter.
 * - Reads scalar values related to instantaneous measurements.
 * - Fetches instantaneous data using a specified OBIS code.
 * - Processes the fetched data and updates relevant structures.
 *
 * @param midx The index of the meter for which data is to be retrieved.
 * @return Returns RET_OK on success, or a negative error code on failure.
 */
int get_inst_data(int midx)
{
	static char fun_name[] = "get_inst_data()";
	// print_colored_message(YELLOW, 1, "%s 1111----- ser_port_data[%d].meter_cfg[%d].met_pw %s -----------\n", fun_name, g_pidx, midx, ser_port_data[g_pidx].meter_cfg[midx].met_pw);

	// rithika 16oct
	// gxProfileGeneric pg;
	gxByteBuffer ba;

	unsigned char loadStatus;

	char *data = NULL;
	char buffer[64];

	int ret = -1;
	int clk_ret = -1;

	struct timeval start, end;

	send_hanging_check();

	if (poll_cfg.poll_inst_data == 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : Inst Data Polling Disabled.\n", midx, fun_name);
		return 0;
	}

	if (get_curr_date_time(midx) < 0)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", midx, fun_name);
		return -1;
	}

	if (strcmp(feature_cfg.meter_inf_type, "PLCC") == 0)
	{
		int result = get_load_status(midx, &loadStatus);
		if (result == 0)
		{
			if (loadStatus)
			{
				inst_val_info.load_status = loadStatus;
				dbg_log(REPORT, "[Meter_%d]:[%-25s] : Load status [%d] -  Connected.\n", midx, fun_name, inst_val_info.load_status);
			}
			else
			{
				inst_val_info.load_status = loadStatus;
				dbg_log(REPORT, "[Meter_%d]:[%-25s] : Load status [%d] -  Disonnected.\n", midx, fun_name, inst_val_info.load_status);
			}
		}
	}

	// // rithika 06oct
	// cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, INST_OBIS);

	// if (inst_data_read[midx] == 0)
	// {
	// 	ret = read_scalar_values(midx, INST_SCALER);
	// 	if (ret != DLMS_ERROR_CODE_OK) // 03/09
	// 	{
	// 		dbg_log(WARNING, "[Meter_%d]:[%-25s] :  READ SCALER FAILED : RET : %d\n", fun_name, midx, ret);
	// 		return -4;
	// 	}

	// 	add_inst_scalar_info(g_pidx, midx, &inst_val_info);

	// 	// rithika 07oct2025
	// 	inst_data_read[midx] = 1;
	// }

	if (inst_data_read[midx] == 0)
	{
		int redis_ret = fetch_inst_scalar_info(midx, &inst_val_info);

		if (redis_ret == 0 && inst_val_info.num_scaler_vals > 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Inst Scaler info found in Redis — skipping read.\n",
					midx, fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No Inst scaler info in Redis — reading from meter.\n", midx,
					fun_name);

			ret = read_scalar_values(midx, INST_SCALER);
			if (ret != DLMS_ERROR_CODE_OK)
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] : READ SCALER FAILED : RET : %d\n",
						midx, fun_name, ret);
				return -4;
			}

			add_inst_scalar_info(g_pidx, midx, &inst_val_info);
		}

		inst_data_read[midx] = 1;
	}
	// init_dlms(0);
	// rithika 16oct
	// cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, INST_OBIS);

	printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ %d\n", inst_obis_read_enb[midx]);

	// rithika 27oct2025
	char serial_number[64] = {0};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		printf("Failed to get meter serial number\n");
	}

	if (inst_obis_read_enb[midx] == 1) // rithika oct16
	{
		// rithika 17oct

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Reading Inst Val OBIS inst_obis_read_enb[%d] : %d.\n", midx, fun_name, midx, inst_obis_read_enb[midx]);
		cosem_init(BASE(inst_pg[midx]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, INST_OBIS);

		gettimeofday(&start, NULL);
		ret = com_read(&con, BASE(inst_pg[midx]), OBIS_ATTR);
		if (ret != DLMS_ERROR_CODE_OK) // 03/09
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : INST VAL OBIS FAILED : RET : %d\n", midx, fun_name, ret);
			return -2;
		}
		else
		{

			dbg_log(INFORM, "[Meter_%d]:[%-25s] : INST VAL OBIS SUCCCESS : RET : %d\n", midx, fun_name, ret);

			ret = obj_toString(&inst_pg[midx], &data);
			if (ret != DLMS_ERROR_CODE_OK)
			{
				return ret;
			}
			if (data != NULL)
			{
				clk_ret = gettimeofday(&end, NULL);

				double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%.6f", duration);

				// char *serial_number = get_meter_2_ser_num(midx);

				// if (serial_number == NULL)
				// {
				// 	printf("Failed to get meter serial number\n");
				// 	// return -1;
				// }

				data_status_log("INST_OBIS:%s Total Time: %s S\n", serial_number, buffer);

				// Print duration
				print_colored_message(YELLOW, 1, "obis Time: %.6f seconds\n", duration);
#if 0
			printf("OBIS STRING %s\n", data);
#endif
				countOBISCodes(data, INSTDATA, midx);
				obis_parsing(data, INSTDATA, midx);
				free(data);
				data = NULL;
			}
		}
		inst_obis_read_enb[midx] = 0;
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Skipping Read Inst Val OBIS inst_obis_read_enb[%d] : %d.\n", midx, fun_name, midx, inst_obis_read_enb[midx]);
	}
	// strcpy(inst_val_info.make,"genus");
	// update_meter_inst_obis(&inst_val_info);
	char temp_manufacturer[64] = {0};
	if (get_manufacturer(midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Failed to get meter manufacturer.\n", midx, fun_name);
	}

	gettimeofday(&start, NULL);

	ret = com_read(&con, BASE(inst_pg[midx]), VAL_ATTR);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : INST VAL READING FAILED!!! RET : %d\n", midx, fun_name, ret);

		return -3;
	}
	else
	{
		clk_ret = gettimeofday(&end, NULL);

		double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%.6f", duration);

		// char *serial_number = get_meter_2_ser_num(midx);

		// if (serial_number == NULL)
		// {
		// 	printf("Failed to get meter serial number\n");
		// 	// return -1;
		// }

		data_status_log("INST_VAL:%s Total Time: %s S\n", serial_number, buffer);

		print_colored_message(YELLOW, 1, "data Time: %.6f seconds\n", duration);

		// printf("CPU cycles to execute float multiplication operation end : %d\n", start);
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : INST VAL SUCCCESS : RET : %d\n", midx, fun_name, ret);
		bb_init(&ba);
		obj_rowsToString(&ba, &inst_pg[midx].buffer);
		data = bb_toString(&ba);
		val_parse(data, INSTDATA, midx, -1, temp_manufacturer);
		free(data);
		bb_clear(&ba);
		// obj_clear(BASE(inst_pg[midx])); // rithika 05Sep
	}

	if (clk_ret != -1)
	{

		double elapsedTime;
		elapsedTime = (end.tv_sec - start.tv_sec);
		elapsedTime += (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^ Run %d took %.6f seconds ^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", inst_meter_metrics.cycleCount + 1, elapsedTime);
		inst_meter_metrics.currenttime = elapsedTime;
		inst_meter_metrics.totalAverages[inst_meter_metrics.cycleCount] = elapsedTime;

		strcpy(inst_meter_metrics.make, temp_manufacturer);

		inst_meter_metrics.cycleCount++;
		store_process_timing("inst_data_timing", &inst_meter_metrics);

		if (inst_meter_metrics.cycleCount >= 20)
		{
			inst_meter_metrics.cycleCount = 0;
		}
	}

	add_meter_inst_data(midx, 0, midx, &inst_val_info);

	if (ftp_enable == 1)
	{
		update_inst_det_ftp(midx);
	}

	// createDataTable("inst_data", g_pidx, dcu_ser_num, midx);
	return RET_OK;
}

/**
 * @brief Scales values based on corresponding scalar values for load survey data.
 *
 * This function iterates through load survey data values and their
 * corresponding scalar values. If an OBIS code matches, it calculates
 * the scaled value and updates the load survey data value.
 *
 * @return Returns 0 on success.
 */
int ls_scaler_val(int midx)
{
	char result_array[MAX_LS_VALS][32];
	int count1 = 4;
	int count2 = 4;
	int result_count = 0;

	// printf("Scaler values : %d num vals %d \n", load_survey_data_value.num_scaler_vals, load_survey_data_value.num_vals);

	for (int i = 0; i < load_survey_data_value.num_vals; i++)
	{
		for (int j = 0; j < load_survey_data_value.num_scaler_vals; j++)
		{
			// printf("i : %d j : %d [i].obis_code : %s [j].obis_code : %s value : %s\n", i, j, load_survey_data_value.values[i].obis_code, load_survey_data_value.scalar_values[j].obis_code, load_survey_data_value.values[i].val);

			// rithika 03nov2025
			//  if (strcmp(load_survey_data_value.values[i].obis_code, load_survey_data_value.scalar_values[j].obis_code) == 0)
			//  {
			if (strcmp(ls_val_obis[midx].obis_code[i], load_survey_data_value.scalar_values[j].obis_code) == 0)
			{
				// printf("String Raw Data : %s\t\t", load_survey_data_value.values[i].val);
				double value = string_to_double(load_survey_data_value.values[i].val);
				// printf("Raw Data : %lf multiplier : %f\t\t", value, load_survey_data_value.scalar_values[j].multiplier);
				double result = value * load_survey_data_value.scalar_values[j].multiplier;
				// printf("result Data : %lf\t\t", result);
				snprintf(load_survey_data_value.values[i].val, 30, "%lf", result);

				// printf("Matching OBIS: %s\n", load_survey_data_value.values[i].obis_code);
				// printf("Value: %s\n", load_survey_data_value.values[i].val);
				// printf("Scaling Factor: %f\t\t", load_survey_data_value.scalar_values[j].multiplier);
				// printf("Result: %s\n", load_survey_data_value.values[i].val);

				result_count++;
				break;
			}
		}
	}

	return 0;
}

int ls_scaler_val_2(int index)
{
	// char result_array[MAX_LS_VALS][32];
	int count1 = 4;
	int count2 = 4;
	// int result_count = 0;

	// printf("Scaler values : %d num vals %d \n", load_survey_data_value.num_scaler_vals, load_survey_data_value.num_vals);

	// for (int i = 0; i < load_survey_data_value.num_vals; i++)
	// {
	for (int j = 0; j < load_survey_data_value.num_scaler_vals; j++)
	{
		// printf("i : %d j : %d [i].obis_code : %s [j].obis_code : %s value : %s\n", i, j, load_survey_data_value.values[i].obis_code, load_survey_data_value.scalar_values[j].obis_code, load_survey_data_value.values[i].val);
		if (strcmp(load_survey_data_value.values[index].obis_code, load_survey_data_value.scalar_values[j].obis_code) == 0)
		{
			// printf("String Raw Data : %s\t\t", load_survey_data_value.values[i].val);
			double value = string_to_double(load_survey_data_value.values[index].val);
			printf("Raw Data : %lf\t\t", value);
			// dbg_log(INFORM, "[Meter_%d]:[%-25s] :  Matching obis %s multiplier %lf raw val %lf\n", "ls_scaler_val_2", load_survey_data_value.scalar_values[j].obis_code, load_survey_data_value.scalar_values[j].multiplier, value);
			double result = value * load_survey_data_value.scalar_values[j].multiplier;
			printf("val : %lf\t\t\n", result);
			snprintf(load_survey_data_value.values[index].val, 30, "%lf", result);

			// printf("Matching OBIS: %s\n", load_survey_data_value.values[i].obis_code);
			// printf("Value: %s\n", load_survey_data_value.values[i].val);
			// printf("Scaling Factor: %f\t\t", load_survey_data_value.scalar_values[j].multiplier);
			// printf("Result: %s\n", load_survey_data_value.values[i].val);

			// result_count++;
			break;
		}
	}
	// }

	return 0;
}

/**
 * @brief Retrieves load survey data for a specified day.
 *
 * This function checks if old load survey data is available, parses the date,
 * initializes connection parameters, and retrieves load survey data based on
 * the specified date range.
 *
 * @param midx The index of the meter for which data is to be retrieved.
 * @param date The date string in "DD-MM-YYYY" format.
 * @param num_days The number of days for which data is to be retrieved.
 * @return Returns 0 on success, or a negative error code on failure.
 */
int get_ls_data_day(int midx, char *date, char *num_days)
{
	static char fun_name[] = "get_ls_data_day()";
	Date start_date, end_date;

	print_colored_message(MAGENTA, 1, "================================ CURR MET IDX : %d =============================\n", midx);

	gxtime curr_ls_strt, curr_ls_end;
	// gxProfileGeneric pg;
	gxByteBuffer ba;

	char *data = NULL;

	int ret = -1;
	int temp_num_days = 0;
	int clk_ret = -1;
	char buffer[64];

	double duration;

	time_t now = time(NULL);
	struct tm *current_date = localtime(&now);

	struct timeval start, end;

	printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ old_ls_flag : %d ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", old_ls_flag);

	print_colored_message(RED, 1, "ls date : %s\n", date);

	send_hanging_check();

	if (poll_cfg.poll_ls_data == 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : LS Data Polling Disabled.\n", midx, fun_name);
		return 0;
	}

	char serial_number[64] = {0};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
	}

	char temp_manufacturer[64] = {0};
	if (get_manufacturer(midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Failed to get meter manufacturer.\n", midx, fun_name);
	}

	if (old_ls_flag == 1)
	{
		// char temp_met_sn[64] = {0};

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
		ret = old_data_check(g_old_ls_date_time, table_name, midx);

		// old_ls_flag = 0;
		if (ret == 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] :  LS Data already availble for this [%s] date. \n", midx, fun_name, date);
			return 0;
		}
	}

	temp_num_days = atoi(num_days);

	if (sscanf(date, "%2hhu-%2hhu-%4hu", &start_date.day, &start_date.month, &start_date.year) != 3)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] :  Error parsing the date-time string\n", midx, fun_name);
		return -4;
	}

	if (get_curr_date_time(midx) < 0)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] :  failed to get Meter curr date time\n", midx, fun_name);
		return -1;
	}

	time_now(&curr_ls_strt);
	time_now(&curr_ls_end);

	// memset(&curr_ls_strt.value, 0, sizeof(struct tm));
	// memset(&curr_ls_end.value, 0, sizeof(struct tm));

	// rithika 07oct2025
	//  if (ls_data_read_day[midx] == 0)
	//  {

	// 	ret = read_scalar_values(midx, LSDATA_SCALER);
	// 	if (ret != DLMS_ERROR_CODE_OK) // 03/09
	// 	{
	// 		dbg_log(WARNING, "[Meter_%d]:[%-25s] :  READ SCALER FAILED : RET : %d\n", fun_name, midx, ret);
	// 		return -4;
	// 	}

	// 	add_ls_scalar_info(g_pidx, midx, &load_survey_data_value);

	// 	ls_data_read_day[midx] = 1;
	// }

	if (ls_data_read_day[midx] == 0)
	{
		int redis_ret = fetch_ls_scalar_info(midx, &load_survey_data_value);

		if (redis_ret == 0 && load_survey_data_value.num_scaler_vals > 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s]:LS day Scaler info found in Redis — skipping read.\n",
					midx, fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  No LS day scaler info in Redis — reading from meter.\n",
					midx, fun_name);

			ret = read_scalar_values(midx, LSDATA_SCALER);
			if (ret != DLMS_ERROR_CODE_OK) // 03/09
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] :  READ SCALER FAILED : RET : %d\n", midx, fun_name, ret);
				return -4;
			}

			add_ls_scalar_info(g_pidx, midx, &load_survey_data_value);
		}
		ls_data_read_day[midx] = 1;
	}

	if (ls_block_obis_read_enb[midx] == 1 || strcmp(temp_manufacturer, "genus") == 0)
	{
		// gxProfileGeneric ls_blk_pg[MAX_NO_OF_METER_PER_PORT]; // rithika 05nov2025
		ret = cosem_init(BASE(ls_blk_pg[midx]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, LS_OBIS);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  LS DATE READING(cosem_init) FAILED!!! RET : %d\n", midx, fun_name, ret);
			return -1;
		}
		gettimeofday(&start, NULL);

		ret = com_read(&con, BASE(ls_blk_pg[midx]), OBIS_ATTR);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  LS DATE VAL OBIS IN DATA READING FAILED!!! RET : %d\n", midx, fun_name, ret);
			return -2;
		}
		else
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] :  LS DATE VAL OBIS SUCCCESS : RET : %d\n", midx, fun_name, ret);

			clk_ret = gettimeofday(&end, NULL);

			duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%.6f", duration);

			// char *serial_number = get_meter_2_ser_num(midx);

			// if (serial_number == NULL)
			// {
			// 	printf("Failed to get meter serial number\n");
			// 	// return -1;
			// }

			data_status_log("LS_OBIS:%s Total Time: %s S\n", serial_number, buffer);

			print_colored_message(YELLOW, 1, "obis Time: %.6f seconds\n", duration);

			ret = obj_toString(&ls_blk_pg[midx], &data);
			if (ret != DLMS_ERROR_CODE_OK)
			{
				return ret;
			}
			if (data != NULL)
			{
#if 0
			printf("OBIS STRING %s\n", data);
#endif
				countOBISCodes(data, LSDATA, midx);
				obis_parsing(data, LSDATA, midx);
				free(data);
				data = NULL;
			}
		}
		ls_block_obis_read_enb[midx] = 0;
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Skipping Read LS DAY VAL OBIS ls_block_obis_read_enb[%d] : %d.\n", midx, fun_name, midx, ls_block_obis_read_enb[midx]);
	}
#if DEBUG
	print_colored_message(RED, 1, "---------------------------------------------------------------------");
	printf("start_date.day : %d curr : %d\n", start_date.day, current_date->tm_mday);
	printf("start_date.month : %d curr : %d\n", start_date.month, current_date->tm_mon);
	printf("start_date.year : %d curr : %d\n", start_date.year, current_date->tm_year);
	print_colored_message(RED, 1, "---------------------------------------------------------------------");
#endif

	if (start_date.year == (current_date->tm_year + 1900) &&
		start_date.month == (current_date->tm_mon + 1) &&
		start_date.day == current_date->tm_mday)
	{
		print_colored_message(MAGENTA, 1, "--------------------- LS DATE IS TODAY --------------\n");
		// time_now(&curr_ls_strt);
		// time_now(&curr_ls_end);
		// curr_ls_end = curr_ls_strt;
		time_clearTime(&curr_ls_strt);

		curr_ls_end.value.tm_year = meter_date_time[midx].year - 1900;
		curr_ls_end.value.tm_mon = meter_date_time[midx].month - 1;
		curr_ls_end.value.tm_mday = meter_date_time[midx].day;
		curr_ls_end.value.tm_hour = meter_date_time[midx].hour;
		curr_ls_end.value.tm_min = meter_date_time[midx].minute;
	}
	else
	{
		memset(&curr_ls_strt.value, 0, sizeof(struct tm));

		// rithika 03Jan2026 added to poll secure meters 00:00th block for jan 1st, changed the start date to poll from 23:30 block of 31st december of the previous year
		if (strstr(date, "01-01"))
		{

			if (strcmp(temp_manufacturer, "secure") == 0)
			{
				curr_ls_strt.value.tm_year = start_date.year - 1901;
				curr_ls_strt.value.tm_mon = 11; // december
				curr_ls_strt.value.tm_mday = 31;
				curr_ls_strt.value.tm_hour = 23;
				curr_ls_strt.value.tm_min = 30;
			}
			else
			{
				print_colored_message(MAGENTA, 1, "--------------------- LS DATE IS NOT TODAY --------------\n");
				curr_ls_strt.value.tm_year = start_date.year - 1900;
				curr_ls_strt.value.tm_mon = start_date.month - 1;
				curr_ls_strt.value.tm_mday = start_date.day;
				curr_ls_strt.value.tm_hour = 0;
				curr_ls_strt.value.tm_min = 0;
			}
		}
		else
		{
			print_colored_message(MAGENTA, 1, "--------------------- LS DATE IS NOT TODAY --------------\n");
			curr_ls_strt.value.tm_year = start_date.year - 1900;
			curr_ls_strt.value.tm_mon = start_date.month - 1;
			curr_ls_strt.value.tm_mday = start_date.day;
			curr_ls_strt.value.tm_hour = 0;
			curr_ls_strt.value.tm_min = 0;
		}

		dbg_log(WARNING, "[%-25s] :temp_num_days : %d\n", fun_name, temp_num_days);

		temp_num_days = temp_num_days - 1;

		end_date = calculate_end_date(start_date, temp_num_days);

		curr_ls_end.value.tm_year = end_date.year - 1900;
		curr_ls_end.value.tm_mon = end_date.month - 1;
		curr_ls_end.value.tm_mday = end_date.day;
		curr_ls_end.value.tm_hour = 23;
		curr_ls_end.value.tm_min = 59;
	}

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : start Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, curr_ls_strt.value.tm_year + 1900, curr_ls_strt.value.tm_mon + 1, curr_ls_strt.value.tm_mday, curr_ls_strt.value.tm_hour, curr_ls_strt.value.tm_min);

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : end Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, curr_ls_end.value.tm_year + 1900, curr_ls_end.value.tm_mon + 1, curr_ls_end.value.tm_mday, curr_ls_end.value.tm_hour, curr_ls_end.value.tm_min);

	gettimeofday(&start, NULL);

	ret = com_readRowsByRange2(&con, &ls_blk_pg[midx], &curr_ls_strt, &curr_ls_end);

	if (ret != DLMS_ERROR_CODE_OK)
	{

		dbg_log(WARNING, "[Meter_%d]:[%-25s] : LS DATE VAL READING FAILED!!! RET : %d\n", midx, fun_name, ret);

		// rithika 26sep
		if (ret == DLMS_ERROR_CODE_DATA_BLOCK_UNAVAILABLE)
		{
			lsdata_not_avalb_log("meter_id %d %s\n", midx, date);
		}

		return -3;
	}
	else
	{
		clk_ret = gettimeofday(&end, NULL);

		duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%.6f", duration);

		// char *serial_number = get_meter_2_ser_num(midx);

		// if (serial_number == NULL)
		// {
		// 	printf("Failed to get meter serial number\n");
		// 	// return -1;
		// }
		// char serial_number[64] = {0};
		// if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
		// {
		// 	dbg_log(INFORM, "[Meter_%d]:[%-25s] :  Failed to get meter serial number \n", fun_name, midx);
		// }
		// else
		// {
		data_status_log("LS_VAL:%s NO_OF_DAYS : %s Total Time: %s S\n", serial_number, num_days, buffer);
		// }

		// Print duration
		print_colored_message(YELLOW, 1, "data Time: %.6f seconds\n", duration);

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : LS DATE VAL READING SUCCESS!!! RET : %d\n", midx, fun_name, ret);
		bb_init(&ba);
		obj_rowsToString(&ba, &ls_blk_pg[midx].buffer);
		data = bb_toString(&ba);
#if DEBUG
		printf("VAL STRING LS : %s\n", data);
#endif
		if (data != NULL)
		{
			char temp_buff[256];
			memset(temp_buff, 0, sizeof(temp_buff));
			sprintf(temp_buff, "%d:%d:%s:%s:%04d-%02d-%02d", midx + 1, g_pidx, serial_number, temp_manufacturer,
					curr_ls_strt.value.tm_year + 1900, curr_ls_strt.value.tm_mon + 1, curr_ls_strt.value.tm_mday);
			check_ls(temp_buff);

			parsing_strings_to_lines(data, LSDATA, midx, -1, temp_manufacturer);
			free(data);
		}
		else
		{
			return -16;
		}
		bb_clear(&ba);
		if (strcmp(temp_manufacturer, "genus") == 0)
		{
			obj_clear(BASE(ls_blk_pg[midx])); // rithika 05nov
		}
	}

	if (clk_ret != -1)
	{
		double elapsedTime;
		elapsedTime = (end.tv_sec - start.tv_sec);
		elapsedTime += (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^ Run %d took %.6f seconds ^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", ls_meter_metrics.cycleCount + 1, elapsedTime);
		ls_meter_metrics.currenttime = elapsedTime;
		ls_meter_metrics.totalAverages[ls_meter_metrics.cycleCount] = elapsedTime;
		strcpy(ls_meter_metrics.make, temp_manufacturer);

		ls_meter_metrics.cycleCount++;
		store_process_timing("ls_data_timing", &ls_meter_metrics);
		if (ls_meter_metrics.cycleCount >= 20)
		{
			ls_meter_metrics.cycleCount = 0;
		}
	}

	return 0;
}

/**
 * @brief Retrieves load survey data blocks based on a specified polling type.
 *
 * This function initializes the necessary parameters, reads the scalar values,
 * and fetches load survey data in blocks, either for a specified number of blocks
 * or for all available data based on the polling type.
 *
 * @param midx The index of the meter for which data is to be retrieved.
 * @param poll_type The type of polling to perform (e.g., "all").
 * @param num_blocks The number of blocks to retrieve.
 * @return Returns 0 on success, or a negative error code on failure.
 */
int get_ls_data_block(int midx, const char *poll_type, char *num_blocks)
{
	static char fun_name[] = "get_ls_data_block()";

	gxtime strt, nd;
	// gxProfileGeneric pg;
	gxByteBuffer ba;

	int clk_ret = -1;
	char buffer[64];

	double duration;
	struct timeval start, end;

	char temp_buff[256];

	char *data = NULL;
	int ret = -1;

	int temp_num_blocks = atoi(num_blocks);

	if (get_curr_date_time(midx) < 0)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", midx, fun_name);
		return -1;
	}

	send_hanging_check();

	if (poll_cfg.poll_ls_data == 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : LS Data Polling Disabled.\n", midx, fun_name);
		return 0;
	}

	char serial_number[64] = {0};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
	}

	// rithika 08oct2025
	if (ls_data_read_block[midx] == 0)
	{
		int redis_ret = fetch_ls_scalar_info(midx, &load_survey_data_value);

		if (redis_ret == 0 && load_survey_data_value.num_scaler_vals > 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : LS block Scaler info found in Redis — skipping read.\n",
					midx, fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No LS block scaler info in Redis — reading from meter.\n",
					midx, fun_name);

			ret = read_scalar_values(midx, LSDATA_SCALER);
			if (ret != DLMS_ERROR_CODE_OK) // 03/09
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] : READ SCALER FAILED : RET : %d\n", midx, fun_name, ret);
				return -4;
			}

			add_ls_scalar_info(g_pidx, midx, &load_survey_data_value);
		}
		ls_data_read_block[midx] = 1;
	}
	char temp_manufacturer[64] = {0};
	if (get_manufacturer(midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter manufacturer.\n", midx, fun_name);
	}

	if (ls_block_obis_read_enb[midx] == 1 || strcmp(temp_manufacturer, "genus") == 0)
	{
		// gxProfileGeneric ls_blk_pg[MAX_NO_OF_METER_PER_PORT]; // rithika 05nov2025
		ret = cosem_init(BASE(ls_blk_pg[midx]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, LS_OBIS);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  LS BLOCK READING(cosem_init) FAILED!!! RET : %d\n", midx, fun_name, ret);
			return -1;
		}
		gettimeofday(&start, NULL);

		ret = com_read(&con, BASE(ls_blk_pg[midx]), OBIS_ATTR);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  LS BLOCK VAL OBIS IN DATA READING FAILED!!! RET : %d\n", midx, fun_name, ret);
			return -2;
		}
		else
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : POLL TYPE : %s LS BLOCK VAL OBIS SUCCCESS : RET : %d\n", midx, fun_name, poll_type, ret);

			clk_ret = gettimeofday(&end, NULL);

			duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%.6f", duration);

			// char *serial_number = get_meter_2_ser_num(midx);

			// if (serial_number == NULL)
			// {
			// 	printf("Failed to get meter serial number\n");
			// 	// return -1;
			// }
			// else
			// {
			data_status_log("LS_OBIS:%s Total Time: %s S\n", serial_number, buffer);
			// }

			print_colored_message(YELLOW, 1, "obis Time: %.6f seconds\n", duration);

			ret = obj_toString(&ls_blk_pg[midx], &data);
			if (ret != DLMS_ERROR_CODE_OK)
			{
				return ret;
			}
			if (data != NULL)
			{
#if 0
			printf("OBIS STRING %s\n", data);
#endif
				countOBISCodes(data, LSDATA, midx);
				obis_parsing(data, LSDATA, midx);
				free(data);
				data = NULL;
			}
		}
		ls_block_obis_read_enb[midx] = 0;
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Skipping Read LS VAL OBIS ls_block_obis_read_enb[%d] : %d.\n", midx, fun_name, midx, ls_block_obis_read_enb[midx]);
	}

	load_survey_data_value.blk_period = get_blk_period();
	dbg_log(INFORM, "[Meter_%d]:[%-25s] : BLOCK INTERVAL PERIOD : %d\n", midx, fun_name, load_survey_data_value.blk_period);

	// to get data from present day 00:00 to now
	if (strcmp(poll_type, "all") == 0)
	{
		time_now(&strt);
		time_now(&nd);
		// nd = strt;

		nd.value.tm_year = meter_date_time[midx].year - 1900;
		nd.value.tm_mon = meter_date_time[midx].month - 1;
		nd.value.tm_mday = meter_date_time[midx].day;
		nd.value.tm_hour = meter_date_time[midx].hour;
		nd.value.tm_min = meter_date_time[midx].minute;

		time_clearTime(&strt);
	}
	else
	{
		time_now(&strt);
		time_now(&nd);
		// Date start_time = calculate_hours_minutes(temp_num_blocks, load_survey_data_value.blk_period);

		// strt.value.tm_year = start_time.year - 1900;
		// strt.value.tm_mon = start_time.month - 1;
		// strt.value.tm_mday = start_time.day;
		// strt.value.tm_hour = start_time.hour;
		// strt.value.tm_min = start_time.minute;

		nd.value.tm_year = meter_date_time[midx].year - 1900;
		nd.value.tm_mon = meter_date_time[midx].month - 1;
		nd.value.tm_mday = meter_date_time[midx].day;
		nd.value.tm_hour = meter_date_time[midx].hour;
		nd.value.tm_min = meter_date_time[midx].minute;

		// rithika 01Dec2025
		struct tm temp_strt_time = {0};
		temp_strt_time.tm_year = nd.value.tm_year;
		temp_strt_time.tm_mon = nd.value.tm_mon;
		temp_strt_time.tm_mday = nd.value.tm_mday;
		temp_strt_time.tm_hour = nd.value.tm_hour;
		temp_strt_time.tm_min = nd.value.tm_min;
		temp_strt_time.tm_sec = 0;

		time_t strt_ls_time = mktime(&temp_strt_time);

		strt_ls_time = strt_ls_time - (temp_num_blocks * load_survey_data_value.blk_period * 60);

		// printf("strt_ls_time %d  interval %d\n", strt_ls_time, (temp_num_blocks * load_survey_data_value.blk_period * 60));

		struct tm *strt_date = localtime(&strt_ls_time);

		strt.value.tm_year = strt_date->tm_year;
		strt.value.tm_mon = strt_date->tm_mon;
		strt.value.tm_mday = strt_date->tm_mday;
		strt.value.tm_hour = strt_date->tm_hour;
		strt.value.tm_min = strt_date->tm_min;

		// time_now(&nd);
	}

	memset(temp_buff, 0, sizeof(temp_buff));
	dbg_log(INFORM, "[Meter_%d]:[%-25s] :  start Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday, strt.value.tm_hour, strt.value.tm_min);

	dbg_log(INFORM, "[Meter_%d]:[%-25s] :  end Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, nd.value.tm_year + 1900, nd.value.tm_mon + 1, nd.value.tm_mday, nd.value.tm_hour, nd.value.tm_min);

	gettimeofday(&start, NULL);

	ret = com_readRowsByRange2(&con, &ls_blk_pg[midx], &strt, &nd);

	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] :  LS BLOCK VAL READING FAILED!!! RET : %d\n", midx, fun_name, ret);

		if (ret == DLMS_ERROR_CODE_DATA_BLOCK_UNAVAILABLE)
		{
			char date[16] = {0};
			sprintf(date, "%02d-%02d-%04d", strt.value.tm_mday, strt.value.tm_mon + 1, strt.value.tm_year + 1900);
			lsdata_not_avalb_log("meter_id %d %s\n", midx, date);
		}

		return -3;
	}
	else
	{
		// rithika 10oct2025
		time_t current_time = time(NULL);
		struct tm *tm_info = localtime(&current_time);
		char datetime_str[32];

		strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);
		// update_lst_ls_poll_time(midx, datetime_str);

		if (OD_CMD_RECEIVED == 0)
		{
			update_ls_poll_time(midx, datetime_str);
		}

		dbg_log(INFORM, "[Meter_%d]:[%-25s] :  LS BLOCK VAL READING SUCCESS!!! RET : %d\n", midx, fun_name, ret);

		clk_ret = gettimeofday(&end, NULL);

		duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%.6f", duration);

		// char *serial_number = get_meter_2_ser_num(midx);

		// if (serial_number == NULL)
		// {
		// 	printf("Failed to get meter serial number\n");
		// 	// return -1;
		// }
		// else
		// {
		data_status_log("LS_BLOCK_VAL:%s NUM_BLOCK : %d POLL_TYPE : %s Total Time: %s S\n", serial_number, temp_num_blocks, poll_type, buffer);
		// }

		// Print duration
		print_colored_message(YELLOW, 1, "data Time: %.6f seconds\n", duration);

		bb_init(&ba);
		obj_rowsToString(&ba, &ls_blk_pg[midx].buffer);
		print_colored_message(MAGENTA, 1, "%s\n", data);
		data = bb_toString(&ba);

		if (data != NULL)
		{
			memset(temp_buff, 0, sizeof(temp_buff));

			sprintf(temp_buff, "%d:%d:%s:%s:%04d-%02d-%02d", midx + 1, g_pidx, serial_number, temp_manufacturer,
					strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday);
			check_ls(temp_buff);
		}
		printf("VAL STRING LS : %s\n", data);
		parsing_strings_to_lines(data, LSDATA, midx, -1, temp_manufacturer);
		bb_clear(&ba);
		free(data);
		if (strcmp(temp_manufacturer, "genus") == 0)
		{
			obj_clear(BASE(ls_blk_pg[midx])); // rithika 05nov
		}
	}

	return 0;
}

/**
 * @brief Scales billing values based on corresponding scalar values.
 *
 * This function iterates through billing data values and their corresponding
 * scalar values. If an OBIS code matches, it calculates the scaled value
 * and updates the billing data value.
 *
 * @return Returns 0 on success.
 */
int billing_scaler_val(int midx)
{
	int count1 = 4;
	int count2 = 4;
	int result_count = 0;

	for (int i = 0; i < billing_data_value.num_vals; i++)
	{
		if (strcmp("0_0_0_1_2_255", billing_data_value.values[i].obis_code) == 0)
		{
			continue;
		}
		for (int j = 0; j < billing_data_value.num_scaler_vals; j++)
		{
			// printf("i : %d j : %d [i].obis_code : %s [j].obis_code : %s value : %s\n", i, j, billing_data_value.values[i].obis_code, billing_data_value.scalar_values[j].obis_code, billing_data_value.values[i].val);

			// rithika 03nov2025
			//  if (strcmp(billing_data_value.values[i].obis_code, billing_data_value.scalar_values[j].obis_code) == 0)
			//  {

			if (strcmp(billing_val_obis[midx].obis_code[i], billing_data_value.scalar_values[j].obis_code) == 0)
			{
				double value = string_to_double(billing_data_value.values[i].val);
				// printf("%lf\n", value);
				double result = value * billing_data_value.scalar_values[j].multiplier;
				// printf("-----------------%lf\n", result);
				snprintf(billing_data_value.values[i].val, 30, "%lf", result);

				// printf("Matching OBIS: %s\n", billing_data_value.values[i].obis_code);
				// printf("Value: %s\n", billing_data_value.values[i].val);
				// printf("Scaling Factor: %f\n", billing_data_value.scalar_values[j].multiplier);
				// printf("Result: %s\n\n", billing_data_value.values[i].val);

				break;
			}
		}
	}

	return 0;
}

/**
 * @brief Retrieves billing data for a specified meter.
 *
 * This function reads scalar values, fetches the billing object using its logical name,
 * and retrieves billing data based on the number of months specified.
 *
 * @param midx The index of the meter for which billing data is to be retrieved.
 * @param num_months The number of months of billing data to retrieve (or "current" for the latest).
 * @return Returns 0 on success, or a negative error code on failure.
 */
int get_billing_data(int midx, char *num_months) // current,all, mm/yy
{
	static char fun_name[] = "get_billing_value()";

	int ret = -1;
	int index = 0;
	int count = 0, num_entry, type = 0;
	int clk_ret = -1;

	// gxProfileGeneric pg;
	gxByteBuffer ba;
	gxtime strt, nd;

	char *data = NULL;
	char buffer[64];

	struct timeval start, end;

	time_now(&strt);
	time_now(&nd);

	if (get_curr_date_time(midx) < 0)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", midx, fun_name);
		return -1;
	}

	send_hanging_check();

	if (poll_cfg.poll_bill_data == 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : Billing Data Polling Disabled.\n", midx, fun_name);
		return 0;
	}

	char serial_number[64] = {0};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
	}

	char temp_manufacturer[64] = {0};
	if (get_manufacturer(midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter manufacturer.\n", midx, fun_name);
	}

	if (bill_data_read[midx] == 0)
	{
		int redis_ret = fetch_bill_scalar_info(midx, &billing_data_value);

		if (redis_ret == 0 && billing_data_value.num_scaler_vals > 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : Bill Scaler info found in Redis — skipping read.\n",
					midx, fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No Bill scaler info in Redis — reading from meter.\n",
					midx, fun_name);

			ret = read_scalar_values(midx, BILLING_SCALER);
			if (ret != DLMS_ERROR_CODE_OK) // 03/09
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] : READ SCALER FAILED : RET : %d\n", midx, fun_name, ret);
				return -4;
			}

			add_bill_scalar_info(g_pidx, midx, &billing_data_value);
		}
		bill_data_read[midx] = 1;
	}

	if (bill_obis_read_enb[midx] == 1)
	{
		gettimeofday(&start, NULL);
		cosem_init(BASE(bill_pg[midx]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, BILLING_OBIS);

		ret = com_read(&con, BASE(bill_pg[midx]), OBIS_ATTR);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : BILLING VAL OBIS FAILED : RET : %d\n", midx, fun_name, ret);
			return -2;
		}
		else
		{

			dbg_log(INFORM, "[Meter_%d]:[%-25s] : BILLING VAL OBIS SUCCCESS : RET : %d\n", midx, fun_name, ret);
			ret = obj_toString(&bill_pg[midx], &data);
			if (ret != DLMS_ERROR_CODE_OK)
			{
				return ret;
			}
			if (data != NULL)
			{
#if 0
			printf("OBIS STRING %s\n", data);
#endif
				clk_ret = gettimeofday(&end, NULL);

				double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

				memset(buffer, 0, sizeof(buffer));
				snprintf(buffer, sizeof(buffer), "%.6f", duration);

				// char *serial_number = get_meter_2_ser_num(midx);

				// if (serial_number == NULL)
				// {
				// 	printf("Failed to get meter serial number\n");
				// 	// return -1;
				// }
				// else
				// {
				data_status_log("BILLING_OBIS:%s Total Time: %s S\n", serial_number, buffer);
				// }
				countOBISCodes(data, BILLING, midx);
				obis_parsing(data, BILLING, midx);
				free(data);
				data = NULL;
			}
		}
		bill_obis_read_enb[midx] = 0;
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Skipping Read BILLING VAL OBIS bill_obis_read_enb[%d] : %d.\n", midx, fun_name, midx, bill_obis_read_enb[midx]);
	}

	ret = com_read(&con, BASE(bill_pg[midx]), 7);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : BILLING READING(ENTRY_IN_USE) FAILED!!! RET : %d\n", midx, fun_name, ret);
		return ret;
	}

	num_entry = bill_pg[midx].entriesInUse;

	dbg_log(INFORM, "[Meter_%d]:[%-25s] :No.of entries : %d\n", midx, fun_name, num_entry);

	// Read Event stored order
	ret = com_read(&con, BASE(bill_pg[midx]), 5);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : Failed to read(Event stored order) object attribute index %d\n", midx, fun_name, 8);
		return -6;
	}

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : %s - Bill stored order : %s\n", midx, fun_name, BILLING_OBIS, (bill_pg[midx].sortMethod == 1) ? "FIFO" : "LIFO");

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Polling Type : %s\n", midx, fun_name, num_months);

	type = bill_pg[midx].sortMethod;

	printf("===========================%d===================================\n", type);

	if (type == FI_FO)
	{
		if (strcmp(num_months, "current") == 0)
		{
			index = num_entry;
			count = 1;
		}
		else if (strstr(num_months, "_") != NULL)
		{
			printf("==============================================================\n");
			// char *token = strtok(num_months, "_");

			// if (token != NULL)
			// {

			// 	count = 1;

			// 	index = ((meter_date_time[midx].month - 1) - atoi(token));
			// 	if (count < 0)
			// 	{
			// 		count += 12;
			// 	}
			// }
			index = 1;
			count = num_entry;
			print_colored_message(MAGENTA, 1, "num_entry : %d count : %d\n", num_entry, count);
		}
		else
		{
			index = 1;
			// count = atoi(num_months);
			count = num_entry;
		}

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : FI_FO, StNumBill : %d, EndNumBill : %d\n", midx, fun_name, index, count);
	}
	else if (type == LI_FO)
	{
		if (strcmp(num_months, "current") == 0)
		{
			index = 1;
			count = 1;
		}
		else if (strstr(num_months, "_") != NULL)
		{
			// char *token = strtok(num_months, "_");

			// if (token != NULL)
			// {
			// 	index = 1;
			// 	count = (atoi(token) - meter_date_time[midx].month + 1);

			// 	if (count < 0)
			// 	{
			// 		count += 12;
			// 	}
			// }
			index = 1;
			count = num_entry;
			print_colored_message(MAGENTA, 1, "num_entry : %d count : %d\n", num_entry, count);
		}
		else
		{
			index = 1;
			// count = atoi(num_months);
			count = num_entry;
		}

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : LI_FO, StNumBill : %d, EndNumBill : %d\n", midx, fun_name, index, count);
	}

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Start idx : %d End idx : %d\n", midx, fun_name, index, count);

	gettimeofday(&start, NULL);

	ret = com_readRowsByEntry(&con, &bill_pg[midx], index, count);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : BILLING READING FAILED!!! RET : %d\n", midx, fun_name, ret);
		return -3;
	}
	else
	{
		clk_ret = gettimeofday(&end, NULL);

		double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%.6f", duration);

		// char *serial_number = get_meter_2_ser_num(midx);

		// if (serial_number == NULL)
		// {
		// 	printf("Failed to get meter serial number\n");
		// 	// return -1;
		// }
		// else
		// {
		data_status_log("BILLING_VAL:%s NUM_OF_ENTRY : %s Total Time: %s S\n", serial_number, num_months, buffer);
		// }
		// Print duration
		print_colored_message(YELLOW, 1, "data Time: %.6f seconds\n", duration);

		// rithika 10oct2025
		time_t current_time = time(NULL);
		struct tm *tm_info = localtime(&current_time);
		char datetime_str[32];

		strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);
		// update_lst_bill_poll_time(midx, datetime_str);
		if (OD_CMD_RECEIVED == 0)
		{
			update_bill_poll_time(midx, datetime_str);
		}

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : BILLING VAL READING SUCCESS. FOR %s MONTH .RET : %d\n", midx, fun_name, num_months, ret);
		bb_init(&ba);
		obj_rowsToString(&ba, &bill_pg[midx].buffer);
		print_colored_message(MAGENTA, 1, "==================== ba size %d pg.buffer.size %d =======", ba.size, bill_pg[midx].buffer.size);
		data = bb_toString(&ba);

		print_colored_message(MAGENTA, 1, "BILL DATA : %s\n", data);
		parsing_strings_to_lines(data, BILLING, midx, -1, temp_manufacturer);
		bb_clear(&ba);
		free(data);
		// obj_clear(BASE(bill_pg[midx])); // rithika 05Sep
	}

	if (clk_ret != -1)
	{
		double elapsedTime;
		elapsedTime = (end.tv_sec - start.tv_sec);
		elapsedTime += (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^ Run %d took %.6f seconds ^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", bill_meter_metrics.cycleCount + 1, elapsedTime);
		bill_meter_metrics.currenttime = elapsedTime;
		bill_meter_metrics.totalAverages[bill_meter_metrics.cycleCount] = elapsedTime;

		strcpy(bill_meter_metrics.make, temp_manufacturer);
		bill_meter_metrics.cycleCount++;
		store_process_timing("bill_data_timing", &bill_meter_metrics);
		if (bill_meter_metrics.cycleCount >= 20)
		{
			bill_meter_metrics.cycleCount = 0;
		}
	}

	return 0;
}

/**
 * @brief Scales the values in the daily profile data based on corresponding scalar values.
 *
 * This function iterates through the daily profile data values, checking each value against its
 * corresponding scalar value. If a match is found, the value is scaled by the scalar multiplier
 * and updated in the data structure.
 *
 * @return Returns 0 on success.
 */
int midnight_scaler_val(int midx)
{
	char result_array[MAX_MN_VALS][32];
	int count1 = 4;
	int count2 = 4;
	int result_count = 0;

	for (int i = 0; i < daily_profile_data_value.num_vals; i++)
	{
		for (int j = 0; j < daily_profile_data_value.num_scaler_vals; j++)
		{
			// printf("i : %d j : %d [i].obis_code : %s [j].obis_code : %s value : %s\n", i, j, daily_profile_data_value.values[i].obis_code, daily_profile_data_value.scalar_values[j].obis_code, daily_profile_data_value.values[i].val);

			// rithika 03nov2025
			//  if (strcmp(daily_profile_data_value.values[i].obis_code, daily_profile_data_value.scalar_values[j].obis_code) == 0)
			//  {
			if (strcmp(daily_profile_val_obis[midx].obis_code[i], daily_profile_data_value.scalar_values[j].obis_code) == 0)
			{
				//  printf("raw val: %s\n", daily_profile_data_value.values[i].val);
				double value = string_to_double(daily_profile_data_value.values[i].val);
				// printf("float : %lf\n", value);
				double result = value * daily_profile_data_value.scalar_values[j].multiplier;
				// printf("-----------------%lf\n", result);
				snprintf(daily_profile_data_value.values[i].val, 30, "%lf", result);

				// printf("Matching OBIS: %s\n", daily_profile_data_value.values[i].obis_code);
				// printf("Value: %s\n", daily_profile_data_value.values[i].val);
				// printf("Scaling Factor: %f\n", daily_profile_data_value.scalar_values[j].multiplier);
				// printf("Result: %s\n\n", result_array[i]);

				result_count++;
				break;
			}
		}
	}

	return 0;
}

/**
 * @brief Retrieves midnight data for a specified number of days.
 *
 * This function initializes the necessary structures and retrieves
 * data from the meter based on the specified number of days. It
 * reads the data for either "today" or a specified number of days
 * back from the current date.
 *
 * @param midx Index of the meter.
 * @param num_days Number of days to retrieve data for; "today" for today's data.
 * @return Returns 0 on success, or a negative error code on failure.
 */
int get_midnight_data(int midx, char *num_days)
{
	static char fun_name[] = "get_midnight_data()";

	// gxProfileGeneric pg;
	gxByteBuffer ba;

	char *data = NULL;
	char buffer[64];

	int ret = -1;
	int temp_num_days;
	int clk_ret = -1;
	int num_entry;

	double duration;

	gxtime strt, nd;
	time_t curr_time = 0, st_time_in_sec = 0, end_time_in_sec = 0;

	struct timeval start, end;

	send_hanging_check();

	if (get_curr_date_time(midx) < 0)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : failed to get Meter curr date time\n", midx, fun_name);
		return -1;
	}

	if (poll_cfg.poll_mn_data == 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] : Midnight Data Polling Disabled.\n", midx, fun_name);
		return 0;
	}

	char serial_number[64] = {0};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
	}

	if (mn_data_read[midx] == 0)
	{
		int redis_ret = fetch_mn_scalar_info(midx, &daily_profile_data_value);

		if (redis_ret == 0 && daily_profile_data_value.num_scaler_vals > 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s]: Midnight Scaler info found in Redis — skipping read.\n", midx,
					fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : No Midnight scaler info in Redis — reading from meter.\n", midx,
					fun_name);

			ret = read_scalar_values(midx, DAILYPROFILE_SCALER);
			if (ret != DLMS_ERROR_CODE_OK) // 03/09
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] : READ SCALER FAILED : RET : %d\n", midx, fun_name, ret);
				return -4;
			}

			add_mn_scalar_info(g_pidx, midx, &daily_profile_data_value);
		}
		mn_data_read[midx] = 1;
	}

	char temp_manufacturer[64] = {0};
	if (get_manufacturer(midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter manufacturer.\n", midx, fun_name);
	}
	// rithika 29Nov2025
	if (mn_obis_read_enb[midx] == 1 || strcmp(temp_manufacturer, "genus") == 0)
	{
		ret = cosem_init(BASE(mn_pg[midx]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, MN_OBIS);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : MN READING(cosem_init) FAILED!!! RET : %d\n", midx, fun_name, ret);
			return -1;
		}

		gettimeofday(&start, NULL);

		ret = com_read(&con, BASE(mn_pg[midx]), OBIS_ATTR);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : MN VAL OBIS IN DATA READING FAILED!!! RET : %d\n", midx, fun_name, ret);
			return -2;
		}
		else
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : MN VAL OBIS SUCCCESS : RET : %d\n", midx, fun_name, ret);

			clk_ret = gettimeofday(&end, NULL);

			duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%.6f", duration);

			// char *serial_number = get_meter_2_ser_num(midx);

			// if (serial_number == NULL)
			// {
			// 	printf("Failed to get meter serial number\n");
			// 	// return -1;
			// }
			// else
			// {
			data_status_log("MID_OBIS:%s Total Time: %s S\n", serial_number, buffer);
			// }
			print_colored_message(YELLOW, 1, "obis Time: %.6f seconds\n", duration);

			ret = obj_toString(&mn_pg[midx], &data);
			if (ret != DLMS_ERROR_CODE_OK)
			{
				return ret;
			}
			if (data != NULL)
			{
#if 0
			printf("OBIS STRING %s\n", data);
#endif
				countOBISCodes(data, DAILYPROFILE, midx);
				obis_parsing(data, DAILYPROFILE, midx);
				free(data);
				data = NULL;
			}
		}
		mn_obis_read_enb[midx] = 0;
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Skipping Read MIDNIGHT VAL OBIS mn_obis_read_enb[%d] : %d.\n", midx, fun_name, midx, mn_obis_read_enb[midx]);
	}
	// Read entries.

	ret = com_read(&con, BASE(mn_pg[midx]), 7);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : Failed to read(Read entries) object attribute index %d\n", midx, fun_name, 7);
		return -5;
	}

	num_entry = mn_pg[midx].entriesInUse;

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : No.of entries : %d\n", midx, fun_name, mn_pg[midx].entriesInUse);

	dbg_log(INFORM, "[Meter_%d]:[%-25s] : Polling Type : %s\n", midx, fun_name, num_days);

	if (strcmp(num_days, "today") == 0)
	{
		curr_time = time(NULL);
		st_time_in_sec = curr_time - (1 * 24 * 60 * 60);
		end_time_in_sec = curr_time;

		struct tm *timeinfo = localtime(&st_time_in_sec);
		strt.value = *timeinfo;

		struct tm *end_timeinfo = localtime(&end_time_in_sec);
		nd.value = *end_timeinfo;

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : start Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday, strt.value.tm_hour, strt.value.tm_min);

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : end Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, nd.value.tm_year + 1900, nd.value.tm_mon + 1, nd.value.tm_mday, nd.value.tm_hour, nd.value.tm_min);
	}
	else if (strcmp(num_days, "all") == 0)
	{
		// start_idx = 1;
		// end_idx = num_entry;

		// dbg_log(INFORM, "[%-25s] : Start idx : %d End idx : %d\n", fun_name, start_idx, end_idx);

		curr_time = time(NULL);

		// rithika 15nov2025
		if (strcmp(dev_model, "TCS_DCU") == 0)
		{

			st_time_in_sec = curr_time - (NUM_MIDNIGHT_DAYS_TCS * 24 * 60 * 60);
		}
		else
		{
			st_time_in_sec = curr_time - (NUM_MIDNIGHT_DAYS * 24 * 60 * 60);
		}

		end_time_in_sec = curr_time;

		struct tm *timeinfo = localtime(&st_time_in_sec);
		strt.value = *timeinfo;

		struct tm *end_timeinfo = localtime(&end_time_in_sec);
		nd.value = *end_timeinfo;

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : start Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday, strt.value.tm_hour, strt.value.tm_min);

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : end Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, nd.value.tm_year + 1900, nd.value.tm_mon + 1, nd.value.tm_mday, nd.value.tm_hour, nd.value.tm_min);
	}
	else if (strstr(num_days, "_") != NULL)
	{
		calculate_start_and_end_date(num_days, &strt.value, &nd.value, midx);

		dbg_log(INFORM, "[Meter_%d]:[%-25s] :start Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday, strt.value.tm_hour, strt.value.tm_min);

		dbg_log(INFORM, "[Meter_%d]:[%-25s] :end Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, nd.value.tm_year + 1900, nd.value.tm_mon + 1, nd.value.tm_mday, nd.value.tm_hour, nd.value.tm_min);
	}
	else
	{
		temp_num_days = atoi(num_days);
		curr_time = time(NULL);
		st_time_in_sec = curr_time - (temp_num_days * 24 * 60 * 60);
		end_time_in_sec = curr_time;

		struct tm *timeinfo = localtime(&st_time_in_sec);
		strt.value = *timeinfo;

		struct tm *end_timeinfo = localtime(&end_time_in_sec);
		nd.value = *end_timeinfo;

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : start Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, strt.value.tm_year + 1900, strt.value.tm_mon + 1, strt.value.tm_mday, strt.value.tm_hour, strt.value.tm_min);

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : end Year: %d, Month: %d, Day: %d hour :%d min : %d\n", midx, fun_name, nd.value.tm_year + 1900, nd.value.tm_mon + 1, nd.value.tm_mday, nd.value.tm_hour, nd.value.tm_min);
	}

	gettimeofday(&start, NULL);

	// if (strcmp(num_days, "all") != 0)
	// {
	ret = com_readRowsByRange(&con, &mn_pg[midx], &strt, &nd);
	// }
	// else
	// {
	// 	ret = com_readRowsByEntry(&con, &pg, start_idx, end_idx);
	// }

	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : MN VAL READING FAILED!!! RET : %d\n", midx, fun_name, ret);
		return -3;
	}
	else
	{
		clk_ret = gettimeofday(&end, NULL);

		double duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%.6f", duration);

		// char *serial_number = get_meter_2_ser_num(midx);
		// if (serial_number == NULL)
		// {
		// 	printf("Failed to get meter serial number\n");
		// 	// return -1;
		// }
		// else
		// {
		data_status_log("MID_VAL:%s NUM_OF_ENTRY : %s  Total Time: %s S\n", serial_number, num_days, buffer);
		// }
		// Print duration
		print_colored_message(YELLOW, 1, "data Time: %.6f seconds\n", duration);

		// rithika 10oct2025
		time_t current_time = time(NULL);
		struct tm *tm_info = localtime(&current_time);
		char datetime_str[32];

		strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);
		// update_lst_mn_poll_time(midx, datetime_str);
		if (OD_CMD_RECEIVED == 0)
		{
			update_mn_poll_time(midx, datetime_str);
		}

		dbg_log(INFORM, "[Meter_%d]:[%-25s] : MN VAL READING SUCCESS!!! RET : %d\n", midx, fun_name, ret);
		bb_init(&ba);
		obj_rowsToString(&ba, &mn_pg[midx].buffer);
		data = bb_toString(&ba);

		printf("%s\n", data);

		parsing_strings_to_lines(data, DAILYPROFILE, midx, -1, temp_manufacturer);
		bb_clear(&ba);
		free(data);
		// obj_clear(BASE(mn_pg[midx])); // rithika 05Sep
	}

	if (clk_ret != -1)
	{
		double elapsedTime;
		elapsedTime = (end.tv_sec - start.tv_sec);
		elapsedTime += (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^ Run %d took %.6f seconds ^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", mn_meter_metrics.cycleCount + 1, elapsedTime);
		mn_meter_metrics.currenttime = elapsedTime;
		mn_meter_metrics.totalAverages[mn_meter_metrics.cycleCount] = elapsedTime;

		strcpy(mn_meter_metrics.make, temp_manufacturer);

		mn_meter_metrics.cycleCount++;
		store_process_timing("mn_data_timing", &mn_meter_metrics);
		if (mn_meter_metrics.cycleCount >= 20)
		{
			mn_meter_metrics.cycleCount = 0;
		}
	}

	return 0;
}

/**
 * @brief Scales event data values based on corresponding scaling factors.
 *
 * This function iterates over the event data values and matches them
 * with their corresponding scaling factors. When a match is found,
 * it multiplies the event value by the scaling factor and updates the
 * event value accordingly.
 *
 * @return Returns 0 on success.
 */
int event_scaler_val(int midx)
{
	int count1 = 4;
	int count2 = 4;

	// for (int i = 0; i < events_data_value.num_event; i++)
	// {

	printf("events_data_value.num_event %d events_data_value.num_scaler_vals %d\n", events_data_value.num_event, events_data_value.num_scaler_vals);
	for (int i = 2; i < events_data_value.num_event; i++)
	{
		for (int j = 0; j < events_data_value.num_scaler_vals; j++)
		{
			// printf("i : %d j : %d val.obis_code : %s scalar.obis_code : %s value : %s\n", i, j, events_data_value.values[i].obis_code, events_data_value.scalar_values[j].obis_code, events_data_value.values[i].val);
			if (strcmp(event_val_obis[midx][g_event_idx].obis_code[i], events_data_value.scalar_values[j].obis_code) == 0)
			{
				double value = string_to_double(events_data_value.values[i].val);
				// printf("%lf\n", value);
				double result = value * events_data_value.scalar_values[j].multiplier;
				// printf("-----------------%lf\n", result);
				snprintf(events_data_value.values[i].val, 30, "%lf", result);

				// printf("Matching OBIS: %s\n", events_data_value.values[i].obis_code);
				// printf("Value: %s\n", events_data_value.values[i].val);
				// printf("Scaling Factor: %f\n", events_data_value.scalar_values[j].multiplier);
				// printf("Result: %s\n\n", events_data_value.values[i].val);

				break;
			}
		}
	}

	return 0;
}

/**
 * @brief Retrieves event data based on the specified event type and number of events.
 *
 * This function initializes communication with the specified event data object,
 * reads the available entries, and fetches event data according to the provided parameters.
 *
 * @param midx Index for the meter.
 * @param event_type The type of event data to retrieve.
 * @param num_events The number of events to retrieve.
 * @return Returns 0 on success, or an error code on failure.
 */
int get_event_data(int midx, char *event_type, char *num_events)
{
	static char fun_name[] = "get_event_data()";

	if (poll_cfg.poll_event_data == 0)
	{
		dbg_log(REPORT, "[Meter_%d]:[%-25s] :Event Data Polling Disabled.\n", midx, fun_name);
		return 0;
	}

	print_colored_message(BLUE, 1, "------- event_type : %s num_events : %s -----------\n", event_type, num_events);

	// gxProfileGeneric pg;
	gxByteBuffer ba;

	char *data = NULL;
	char obis[18];

	char buffer[64];

	int ret = -1;
	int attr = 3;
	int count = 0;
	int temp_num_events = 0;
	int clk_ret = -1, event_idx;
	uint32_t st_event_num = 0, end_event_num = 0;
	double duration;

	struct timeval start, end;

	temp_num_events = atoi(num_events);

	memset(obis, 0, sizeof(obis));

	// if ((strcmp(num_events, "all")) != 0)
	// {
	dbg_log(REPORT, "[Meter_%d]:[%-25s] : Event_type : %s Poll_type : %s\n", midx, fun_name, event_type, num_events);

	if (strcmp(event_type, "event_data_cat1") == 0)
	{
		strcpy(obis, EVENT_01_OBIS);
		event_idx = 1;
	}
	else if (strcmp(event_type, "event_data_cat2") == 0)
	{
		strcpy(obis, EVENT_02_OBIS);
		event_idx = 2;
	}
	else if (strcmp(event_type, "event_data_cat3") == 0)
	{
		strcpy(obis, EVENT_03_OBIS);
		event_idx = 3;
	}
	else if (strcmp(event_type, "event_data_cat4") == 0)
	{
		strcpy(obis, EVENT_04_OBIS);
		event_idx = 4;
	}
	else if (strcmp(event_type, "event_data_cat5") == 0)
	{
		strcpy(obis, EVENT_05_OBIS);
		event_idx = 5;
	}
	else if (strcmp(event_type, "event_data_cat6") == 0)
	{
		strcpy(obis, EVENT_06_OBIS);
		event_idx = 6;
	}
	else
	{
		strcpy(obis, EVENT_07_OBIS);
		event_idx = 7;
	}
	// }

	// rithika 30oct2025
	g_event_idx = event_idx - 1;

	send_hanging_check();

	char serial_number[64] = {0};
	if (get_meter_2_ser_num(midx, serial_number, sizeof(serial_number)) != 0)
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s] : Failed to get meter serial number \n", midx, fun_name);
	}

	char temp_manufacturer[64] = {0};
	if (get_manufacturer(midx, serial_number, temp_manufacturer, sizeof(temp_manufacturer)) != 0)
	{
		// handle error
	}

	if (event_obis_read_enb[midx][event_idx - 1] == 1)
	{
		cosem_init(BASE(event_pg[midx][event_idx - 1]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, obis);

		// if (obj == NULL)
		// {
		// 	dbg_log(WARNING, "[%-25s] :Object '%s' not found from the association view.\n", fun_name, obis);
		// 	return -1;
		// }
		gettimeofday(&start, NULL);

		ret = com_read(&con, BASE(event_pg[midx][event_idx - 1]), OBIS_ATTR);
		if (ret != DLMS_ERROR_CODE_OK) // 03/09
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] : EVENT VAL OBIS FAILED : RET : %d\n", midx, fun_name, ret);
			return -2;
		}
		else
		{

			clk_ret = gettimeofday(&end, NULL);
			duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
			print_colored_message(YELLOW, 1, "obis Time: %.6f seconds\n", duration);

			memset(buffer, 0, sizeof(buffer));
			snprintf(buffer, sizeof(buffer), "%.6f", duration);

			// char *serial_number = get_meter_2_ser_num(midx);

			// if (serial_number == NULL)
			// {
			// 	printf("Failed to get meter serial number\n");
			// 	// return -1;
			// }
			// else
			// {
			data_status_log("EVENT_OBIS:%s Total Time: %s S\n", serial_number, buffer);
			// }
			dbg_log(INFORM, "[Meter_%d]:[%-25s] : EVENT VAL OBIS SUCCCESS : RET : %d\n", midx, fun_name, ret);

			ret = obj_toString(&event_pg[midx][event_idx - 1], &data);
			if (ret != DLMS_ERROR_CODE_OK)
			{
				return ret;
			}
			if (data != NULL)
			{
#if 0
			printf("OBIS STRING %s\n", data);
#endif

				countOBISCodes(data, EVENTDATA, midx);
				obis_parsing(data, EVENTDATA, midx);
				free(data);
				data = NULL;
			}
		}
		event_obis_read_enb[midx][event_idx - 1] = 0;
	}
	else
	{
		dbg_log(INFORM, "[Meter_%d]:[%-25s]: Skipping Read EVENT VAL OBIS event_obis_read_enb[%d][event_idx: %d]  : %d.\n", midx, fun_name, midx, event_idx, event_obis_read_enb[midx][event_idx - 1]);
	}

	// Read Event stored order
	ret = com_read(&con, BASE(event_pg[midx][event_idx - 1]), 5);
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] : Failed to read(Event stored order) object attribute index %d\n", midx, fun_name, 8);
		return -6;
	}

	dbg_log(INFORM, "[Meter_%d]:[%-25s] :  %s - Event stored order : %s\n", midx, fun_name, obis, (event_pg[midx][event_idx - 1].sortMethod == 1) ? "FIFO" : "LIFO");

	// Read entries.
	ret = com_read(&con, BASE(event_pg[midx][event_idx - 1]), 7);

	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] :  Failed to read(Read entries) object attribute index %d\n", midx, fun_name, 7);
		return ret;
	}

	dbg_log(INFORM, "[Meter_%d]:[%-25s] :  %s - No.of entries : %d\n", midx, fun_name, obis, event_pg[midx][event_idx - 1].entriesInUse);

	if (event_pg[midx][event_idx - 1].entriesInUse == 0)
	{
		return 0;
	}
	// rithika 14oct2025
	else if ((num_event_entries[midx][event_idx - 1] == event_pg[midx][event_idx - 1].entriesInUse) && OD_CMD_RECEIVED == 0 && strcmp(num_events, "all") != 0)
	{
		// rithika 30oct2025
		uint32_t latest_idx;
		gxByteBuffer bytearr;
		char *temp_data = NULL;
		// Read Event stored order

		if (event_pg[midx][event_idx - 1].sortMethod == FI_FO)
		{
			latest_idx = event_pg[midx][event_idx - 1].entriesInUse;
		}
		else
		{
			latest_idx = 1;
		}

		ret = com_readRowsByEntry(&con, &event_pg[midx][event_idx - 1], latest_idx, LATEST_EVE_CNT);
		if (ret != DLMS_ERROR_CODE_OK)
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  LATEST EVENT READING FAILED!!! RET : %d\n", midx, fun_name, ret);
			return ret;
		}
		else
		{
			bb_init(&bytearr);
			obj_rowsToString(&bytearr, &event_pg[midx][event_idx - 1].buffer);
			temp_data = bb_toString(&bytearr);
			bb_clear(&bytearr);

			// dbg_log(WARNING, "[Meter_%d]:[%-25s] :  ****************************before evnt indx %d latest_row_exist %d\n", fun_name, midx, event_idx, latest_row_exist);

			latest_row_exist = 0;

			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  *******************temp_data %s\n", midx, fun_name, temp_data);

			parsing_strings_to_lines(temp_data, EVENTDATA, midx, event_idx, temp_manufacturer);

			free(temp_data);
			bb_clear(&bytearr);

			if (latest_row_exist == 1)
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] :  No new event entry is added for the category %d latest_row_exist %d\n", midx, fun_name, event_idx, latest_row_exist);

				return 0;
			}
			else
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] :  New entry is added continue polling event data for the category %d latest_row_exist %d\n", midx, fun_name, event_idx, latest_row_exist);
			}
		}
	}

	// rithika 28oct2025
	if (OD_CMD_RECEIVED == 0)
	{
		num_event_entries[midx][event_idx - 1] = event_pg[midx][event_idx - 1].entriesInUse;
	}

	if (event_data_read[midx] == 0)
	{
		int redis_ret = fetch_event_scalar_info(midx, &events_data_value);

		if (redis_ret == 0 && events_data_value.num_scaler_vals > 0)
		{
			dbg_log(INFORM, "[Meter_%d]:[%-25s]: Event Scaler info found in Redis — skipping read.\n",
					midx, fun_name);
		}
		else
		{
			dbg_log(WARNING, "[Meter_%d]:[%-25s] :  No Event scaler info in Redis — reading from meter.\n",
					midx, fun_name);

			ret = read_scalar_values(midx, EVENTDATA_SCALER);
			if (ret != DLMS_ERROR_CODE_OK) // 03/09
			{
				dbg_log(WARNING, "[Meter_%d]:[%-25s] :  READ SCALER FAILED : RET : %d\n", midx, fun_name, ret);
				return -4;
			}

			add_event_scalar_info(g_pidx, midx, &events_data_value);
		}
		event_data_read[midx] = 1;
	}

	if ((strcmp(num_events, "all")) == 0)
	{
		st_event_num = 0; // changed from 1 to 0//rithika 4Sep
		end_event_num = event_pg[midx][event_idx - 1].entriesInUse;
	}
	else
	{
		if (event_pg[midx][event_idx - 1].sortMethod == LI_FO)
		{
			if (event_pg[midx][event_idx - 1].entriesInUse > temp_num_events)
			{
				st_event_num = event_pg[midx][event_idx - 1].entriesInUse - temp_num_events;
				memset(&end_event_num, 0, 4);
				end_event_num = event_pg[midx][event_idx - 1].entriesInUse;
			}
			else if (event_pg[midx][event_idx - 1].entriesInUse < 1)
			{
				st_event_num = 0;
				end_event_num = 0;
			}
			else
			{
				st_event_num = 0;
				end_event_num = event_pg[midx][event_idx - 1].entriesInUse;
			}
			dbg_log(INFORM, "[Meter_%d]:[%-25s] :  LI_FO, StNumEvent : %d, EndNumEvent : %d\n", midx, fun_name, st_event_num, end_event_num);
		}
		else if (event_pg[midx][event_idx - 1].sortMethod == FI_FO)
		{
			printf("pg.entriesInUse : %d\n", event_pg[midx][event_idx - 1].entriesInUse);
			if (event_pg[midx][event_idx - 1].entriesInUse == 0)
				st_event_num = 0;
			else
				st_event_num = event_pg[midx][event_idx - 1].entriesInUse - temp_num_events;
			;

			if (event_pg[midx][event_idx - 1].entriesInUse > temp_num_events)
			{
				memset(&end_event_num, 0, 4);
				end_event_num = event_pg[midx][event_idx - 1].entriesInUse;
			}
			else
			{
				st_event_num = 0;
				end_event_num = event_pg[midx][event_idx - 1].entriesInUse;
			}

			dbg_log(INFORM, "[Meter_%d]:[%-25s] :  FI_FO, StNumEvent : %d, EndNumEvent : %d\n", midx, fun_name, st_event_num, end_event_num);
		}
	}

	gettimeofday(&start, NULL);

	// ret = com_readRowsByEntry(&con, &pg, st_event_num, end_event_num); //rithika 04/09
	ret = com_readRowsByEntry(&con, &event_pg[midx][event_idx - 1], st_event_num + 1, (end_event_num - st_event_num)); // here need to pass the last parameter as count of entries to read
	if (ret != DLMS_ERROR_CODE_OK)
	{
		dbg_log(WARNING, "[Meter_%d]:[%-25s] :  EVENT READING FAILED!!! RET : %d\n", midx, fun_name, ret);
		return -3;
	}
	else
	{
		clk_ret = gettimeofday(&end, NULL);
		// Compute duration in seconds (with microsecond precision)
		duration = (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

		memset(buffer, 0, sizeof(buffer));
		snprintf(buffer, sizeof(buffer), "%.6f", duration);

		// char *serial_number = get_meter_2_ser_num(midx);

		// if (serial_number == NULL)
		// {
		// 	printf("Failed to get meter serial number\n");
		// 	// return -1;
		// }
		// else
		// {
		data_status_log("EVENT_VAL:%s EVENT_TYPE :%s StNumEvent : %d, EndNumEvent : %d NUM_OF_ENTRY : %s Total Time: %s S\n", serial_number, event_type, st_event_num, end_event_num, num_events, buffer);
		// }
		// Print duration
		print_colored_message(YELLOW, 1, "data Time: %.6f seconds\n", duration);

		// rithika 10oct2025
		time_t current_time = time(NULL);
		struct tm *tm_info = localtime(&current_time);
		char datetime_str[32];

		strftime(datetime_str, sizeof(datetime_str), "%Y-%m-%d %H:%M:%S", tm_info);
		// update_lst_event_poll_time(midx, datetime_str);
		if (OD_CMD_RECEIVED == 0)
		{
			update_event_poll_time(midx, datetime_str);
		}

		dbg_log(INFORM, "[Meter_%d]:[%-25s] :  EVENT READING SUCCESS!!! RET : %d\n", midx, fun_name, ret);
		bb_init(&ba);
		obj_rowsToString(&ba, &event_pg[midx][event_idx - 1].buffer);
		data = bb_toString(&ba);
		printf("VAL STRING %s\n", data);
		parsing_strings_to_lines(data, EVENTDATA, midx, event_idx, temp_manufacturer);
		free(data);
		bb_clear(&ba);
		// obj_clear(BASE(event_pg[midx][event_idx - 1])); // rithika 05Sep
	}

	if (clk_ret != -1)
	{
		double elapsedTime;
		elapsedTime = (end.tv_sec - start.tv_sec);
		elapsedTime += (end.tv_usec - start.tv_usec) / 1000000.0;
		printf("^^^^^^^^^^^^^^^^^^^^^^^^^^^ Run %d took %.6f seconds ^^^^^^^^^^^^^^^^^^^^^^^^^^^\n", event_meter_metrics.cycleCount + 1, elapsedTime);
		event_meter_metrics.currenttime = elapsedTime;
		event_meter_metrics.totalAverages[event_meter_metrics.cycleCount] = elapsedTime;

		strcpy(event_meter_metrics.make, temp_manufacturer);

		event_meter_metrics.cycleCount++;
		store_process_timing("event_data_timing", &event_meter_metrics);
		if (event_meter_metrics.cycleCount >= 20)
		{
			event_meter_metrics.cycleCount = 0;
		}
	}

	return RET_OK;
}

// int test_inst_data()
// {

// 	int ret;
// 	// If you already know captured objects layout, no need to read attr#3 again
// 	// Optionally uncomment next line for first-time mapping
// 	// gxProfileGeneric pg;
// 	// cosem_init(BASE(pg), DLMS_OBJECT_TYPE_PROFILE_GENERIC, "1.0.94.91.0.255");

// 	int cnt = 0;
// 	gxProfileGeneric pg[2];
// 	cosem_init(BASE(pg[0]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, "1.0.94.91.0.255");

// 	cosem_init(BASE(pg[1]), DLMS_OBJECT_TYPE_PROFILE_GENERIC, "1.0.94.91.0.255");

// 	while (1)
// 	{

// 		for (int i = 0; i < 2; i++)
// 		{

// 			init_dlms(i);
// 			if (cnt == 0)
// 			{
// 				// read obis code
// 				ret = com_read(&con, BASE(pg[i]), 3);
// 			}
// 			// cnt++;
// 			// Read buffer (attribute 2)
// 			printf("------------------------Reading instantaneous data buffer...\n");
// 			// read value
// 			ret = com_read(&con, BASE(pg[i]), 2);
// 			if (ret != 0)
// 			{
// 				printf("Error reading buffer: %d\n", ret);
// 			}
// 			else
// 			{
// 				printf("Buffer read successful.\n");
// 				gxArray rows = pg[i].buffer;
// 				for (unsigned short r = 0; r < rows.size; ++r)
// 				{
// 					gxArray *row = (gxArray *)rows.data[r];
// 					printf("Row %d: ", r + 1);
// 					// for (unsigned short c = 0; c < row->size; ++c)
// 					// {
// 					//     gxValue *val = (gxValue *)row->data[c];
// 					//     printf("%s | ", var_toString(val));
// 					// }
// 					printf("\n");
// 				}
// 			}
// 			printf("************* read success for meter %d *************\n", i);
// 			// get_inst_data(i);
// 			// get_name_plate(i);
// 			// get_ls_data_block(i, "now", "8");

// 			// get_ls_data_block(1,"now","8");

// 			// init_dlms(0);
// 			if (cnt == 0)
// 			{
// 				obis_read_enb = 1;
// 			}
// 			else
// 			{
// 				obis_read_enb = 0;
// 			}

// 			sleep(5);
// 			com_disconnect(&con);
// 		}
// 		cnt++;
// 	}

// 	/////////////
// }