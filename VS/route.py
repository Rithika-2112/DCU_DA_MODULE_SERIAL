import json, os, sys
import re
import gc
import jwt
import tarfile, hashlib
import time
import smtplib
import configparser
import requests
import socket
import calendar
import random
import time
import json
import jsonschema
from prettytable import PrettyTable
import ast
from flask import Response, request
from email.mime.multipart import MIMEMultipart
from email.mime.text import MIMEText
from email.mime.image  import MIMEImage
from cryptography.fernet import Fernet
from MySQL import  Session
from debug_logger import *
from schema import *
from ondemand import *
from datetime import datetime, timedelta, date
from jsonschema import validate
from dateutil.relativedelta import relativedelta

from mysql.connector import Error
import time
import MySQLdb
import mysql.connector
from mysql.connector import Error
from typing import List, Dict
secret_key= b'u8_LA_v7ADJviOsx8UrQXl_hgt1nuhaDQXnaeWoy1x0='
cipher_suite = Fernet(secret_key)

PRODUCTION = 0


fm_upload_folder = None 
upload_folder = None 
download_folder = None
mail_server= None
mail_port= None
sender= None
mail_passwd= None
support_mail= None
chance_superadmin=None
chance_admin=None
chance_operator=None
download_file = "mdmCfg.cfg"
energy_sf= 1 # (1 - Wh & VARh) & (1000 - KWh & KVARh) & (1000000 - MWh & MVARh)
sec_code= "$DfG2<jy^&@G?:Mc3ilC3@"
refresh_code= "+$IfG<^>jyN^&@:|c3&lC3@"
mail_code = "Re$et*G.k@l+2=C03)$"
db_conn = None
MAX_METERS= 5
MAX_METER_EVENTS= 50
MAX_MODEM_EVENTS= 50
MAX_DEV_EVENTS= 50
obis_cache = {}
if PRODUCTION == 1:
    log_folder= r"C:\CMS\CDCS_MDAS\Web\log\\"
    ConfigFile = r"C:\CMS\CDCS_MDAS\Web\config\config.ini"
    local_download_folder = r"C:\CMS\CDCS_MDAS\Web\config\download\\"

elif PRODUCTION == 2:
    # added by inzamam
    # log_folder= r"/home/bitnami/RMS_WEB/log/"
    # ConfigFile = r"/home/bitnami/RMS_WEB/config/config.ini"
    # local_download_folder = r"/home/bitnami/RMS_WEB/config/download/"
    #added by priya
    log_folder= r"/home/bitnami/RMS_TATA_PW_DEMO/log/"
    ConfigFile = r"/home/bitnami/RMS_TATA_PW_DEMO/config/config.ini"
    local_download_folder = r"/home/bitnami/RMS_TATA_PW_DEMO/config/download/"
 
else:
    local_download_folder = r"./config/download/"
    log_folder= r"./log/"
    ConfigFile = r"./config/config.ini"



def create_file():
    fn= "create_file()"
    global fm_upload_folder, ser_err, local_download_folder, upload_folder, log_folder
    try:

        if not os. path. isdir(local_download_folder):
            os. makedirs(local_download_folder)

        if not os. path. isdir(fm_upload_folder):
            os. makedirs(fm_upload_folder) 
            
        if not os. path. isdir(upload_folder):
            os. makedirs(upload_folder) 

        if not os. path. isdir(log_folder):
            os. makedirs(log_folder) 
            
        setup_logger(log_folder + "pybackend_log")

        return 0

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        print(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")

def init_connection():
    global ConfigFile, db_host, db_user, db_pass, db_db, db_port, fm_upload_folder, upload_folder, PRODUCTION, download_folder, mail_server, mail_port, sender, mail_passwd, support_mail,chance_superadmin,chance_admin,chance_operator
    fn = "init_connection()"
    try:
      
     
        config = configparser.RawConfigParser(allow_no_value=True)  
        config.read(ConfigFile)
        db_host = config.get("Database", "Host")    
        db_user = config.get("Database", "User")    
        db_pass = config.get("Database", "Password")    
        db_db  = config.get("Database",  "Db")    
        db_port  = int(config.get("Database", "Port"))   
        
        mail_server = config.get("MAIL_SERVER", "Host")    
        mail_port = config.get("MAIL_SERVER", "port")    
        sender = config.get("MAIL_SERVER", "sender")    
        mail_passwd  = config.get("MAIL_SERVER",  "pwd")    
        support_mail  = config.get("MAIL_SERVER", "support_mail") 
        chance_superadmin=config.get("Config","SuperAdmin")
        chance_admin=config.get("Config","Admin")
        chance_operator=config.get("Config","operator")

        if PRODUCTION:
            fm_upload_folder= r"{}".format(config.get("Config", "firmware"))
            upload_folder = r"{}".format(config.get("Config", "upload"))
            download_folder = r"{}".format(config.get("Config", "download"))
        else:
            download_folder = r"./config/download/"
            fm_upload_folder= r"./config/firmware/"
            upload_folder = r"./config/upload/"
       
       
        return [db_host, db_user, db_pass, db_db, db_port]
    
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        print(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return []

def init_db_connection(mysql, port):
    """Establish connection with Dtabase"""
    global db_conn, PORT
    fn = "init_db_connection()"
    try:
        db_conn = mysql
        PORT = port
        # empty_db()
        return True
        
    except Exception as e:
        exc_info = sys.exc_info()
        print ( '%s error on line number : %s : %s' % (fn, exc_info[2].tb_lineno, e) )
        return False

def create_download_folders():
    fn = "create_download_folders()"
    global db_conn, fm_upload_folder, upload_folder, local_download_folder
    try:
        
        db_dev_details = execute(2, "SELECT dev_id FROM dev_tbl ORDER BY loc_id") #dev_type, dev_model
        
        for dev_id in db_dev_details:
            fm_folder = fm_upload_folder + "{}/".format(dev_id[0])
            up_folder = upload_folder 
            folder = local_download_folder + "{}/".format(dev_id[0])

            if not os. path. isdir(fm_folder):
                os. makedirs(fm_folder)
                
            if not os. path. isdir(up_folder):
                os. makedirs(up_folder)
                
            if not os. path. isdir(folder):
                os. makedirs(folder)
            
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")        






Month= {"Jan": '01', "Feb": '02', "Mar":  '03', "Apr": '04', "May": '05', "Jun": '06', "Jul": '07', "Aug": '08', "Sep": '09', "Oct": '10', "Nov": '11', "Dec": '12'}
Str_month= { 1: "Jan", 2: "Feb", 3:"Mar", 4:"Apr", 5:"May", 6:"Jun", 7:"Jul", 8:"Aug", 9:"Sep", 10:"Oct", 11:"Nov", 12:"Dec"}
Month = {"Jan": '01', "Feb": '02', "Mar":  '03', "Apr": '04', "May": '05', "Jun": '06',
         "Jul": '07', "Aug": '08', "Sep": '09', "Oct": '10', "Nov": '11', "Dec": '12'}

day_count = {"Jan": '31', "Feb": '28', "Mar":  '31', "Apr": '30', "May": '31', "Jun": '30',
             "Jul": '31', "Aug": '31', "Sep": '30', "Oct": '31', "Nov": '30', "Dec": '31'}

month_fullname = {"Jan": 'January', "Feb": 'February', "Mar":  'March', "Apr": 'April', "May": 'May', "Jun": 'June',
             "Jul": 'July', "Aug": 'August', "Sep": 'September', "Oct": 'October', "Nov": 'November', "Dec": 'December'}

ser_err= json.dumps({"status":  "server error"})

# def empty_db():
#     fn="empty_db"
#     try:
#         for i in range(0,len(list_table_name)):
#             print(check_table(list_table_name[i]),list_table_name[i])
#             if not check_table(list_table_name[i]):
#                 execute(1,list_query[i])
        
        
#     except Exception as e:
#         print("---------------------------------")
#         exc_type, exc_obj, exc_tb = sys.exc_info()
#         print(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
#         print("---------------------------------")

        


def get_key_value_pairs(qry):
    fn = "get_key_value_pairs"
    global db_conn
    try:
       
        cursor = db_conn.connection.cursor()
        cursor.execute(qry)

        results = cursor.fetchall()

        if results == None or results == () or results == [] or results == ((None,),):
            return None
        columns = [desc[0] for desc in cursor.description]

        key_value_pairs = []
        for row in results:
            row_dict = {}
            for i in range(len(columns)):
                if type(row[i]) == datetime:

                    row_dict[columns[i]] = row[i].strftime('%Y-%m-%d %H:%M:%S')
                else:
                    row_dict[columns[i]] = row[i]
            key_value_pairs.append(row_dict)
               
        return key_value_pairs

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        init_connection()
        print("--------------connection initializing-------------------")
        return []

def execute(cnt, qry):
    fn= "exe"
    global db_conn
    try:
        
        cursor = db_conn.connection.cursor()
        cursor.execute(qry)
        
        if cnt == 1:
            results = cursor.fetchone()
        else:
            results = cursor.fetchall()
        
        return results
        
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return None
        
def update_tbl(qry):
    fn= "update_tbl()"
    global db_conn
    try:
        
        cursor = db_conn.connection.cursor()
        cursor.execute(qry)
        
        db_conn.connection.commit()
        return True
        
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return False
    
def create_meter_map_table(table_name):
    fn = "create_meter_map_table()"
    global db_conn
    try:
        cursor = db_conn.connection.cursor()

        create_table_query = f"""
        CREATE TABLE IF NOT EXISTS `{table_name}` (
          id INT NOT NULL AUTO_INCREMENT,
          met_sn VARCHAR(45) DEFAULT NULL,
          met_loc VARCHAR(45) DEFAULT NULL,
          PRIMARY KEY (id),
          UNIQUE KEY met_sn_UNIQUE (met_sn)
        ) ENGINE=InnoDB DEFAULT CHARSET=latin1;
        """
        

        cursor.execute(create_table_query)
        db_conn.connection.commit()
        return True

    except Exception as e:
        import sys
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        return False
   
def check_table(table_name):
    fn = "check_table()"
    global db_conn
    try:
        qry = "show tables like '%s'" %(table_name)
        result = execute(2, qry)
        if len(result) == 0:
            return False

        return True
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        print ('%s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return False
    
def init_mail_server():
    fn= "init_mail_server()"
    global mail_server, mail_port, sender, mail_passwd, support_mail
    try:
        mail = execute(1, "SELECT mail_ip, mail_port, mail_email, mail_password, spport_mail_id FROM site_genupstream_details")
        
        if mail == None or mail == ():
            logwarn("Mail server table/data is not available!")
            return
        
        mail_server= mail[0]
        mail_port= mail[1]
        sender= mail[2]
        mail_passwd= mail[3]
        support_mail= mail[4]
        
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr ('%s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
    
def daterange(start_date, end_date):
    fn= "daterange()"
    try:
        for n in range(int((end_date - start_date).days)):
            yield start_date + timedelta(n)
    except Exception as e:
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))

def modify_folder(action, dev_id):
    fn = "modify_folder()"
    global fm_upload_folder, upload_folder, download_folder, local_download_folder
    try:
        
        fm_folder = fm_upload_folder + "{}/".format(dev_id)
        up_folder = upload_folder 
        folder2 = local_download_folder + "{}/".format(dev_id)

        
        if action == 'add':
            if not os. path. isdir(fm_folder):
                os. makedirs(fm_folder)
            if not os. path. isdir(up_folder):
                os. makedirs(up_folder)
            if not os. path. isdir(folder2):
                # os. makedirs(folder)
                os. makedirs(folder2)
                

        else:

            exe = (folder2.replace('.', '')).replace('/', '', 1)
            print(exe)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")        
def next_pre(table_name,gen_id):
    fn='next_pre'
    try:
       
        next="select* from {} where ui_id={}".format(table_name,gen_id)
        next_data_res=execute(2,next)
        if next_data_res==None or next_data_res==():
            next_data="false"
        else:
            next_data="true"
        return next_data
    except Exception as e:
        exc_info = sys.exc_info()
        print("%s error on line number : %s : %s" %
              (fn, exc_info[2].tb_lineno, e))
        return False      

def check_schema(request_json, schema):
  
    try:
        validate(instance=request_json, schema=schema)
     
    except jsonschema.exceptions.ValidationError as e:
      
        return f"JSON schema is invalid. Error: {e.message}"
    return True

def daterange(start_date, end_date):
    fn = "daterange()"
    try:
        for n in range(int((end_date - start_date).days)):
            yield start_date + timedelta(n)
    except Exception as e:
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))

def meter_count(tuple_of_tuple):
    try:
        fn='meter_count'
        meters_count=0
        for idx in tuple_of_tuple:
            for dev_id in idx:
                query = "select num_meters from dev_tbl where dev_id='{}'".format(
                dev_id)
                
                num_meters_list = execute(1,query)
                
                if num_meters_list == [] or num_meters_list == None:
                    num_meters = 0
                        
                else:
                    num_meters = int(num_meters_list[0])
                meters_count=meters_count+num_meters
        return meters_count

    except Exception as e:
        exc_info = sys.exc_info()
        print("%s error on line number : %s : %s" %
              (fn, exc_info[2].tb_lineno, e))
        return False
    

def get_obis_name(obis_code, type):
    fn = "get_obis_name()"
    # global db_conn
    try:
      
        name = execute( 
            1, "SELECT param_name FROM obis_param_map WHERE obis_code = '{}' and type='{}'".format(obis_code, type))

        if name != None:
            return name[0]
        else:
            return obis_code

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return ""
 
def get_device_serial_config_by_id_extra(dev_id):
    fn = "get_device_serial_config_by_id()"
    global ser_err, sec_code, db_conn

    try:
        main_dict= {}
        keys= ("id", "dev_id", "meter_enable","meter_type","met_id", "meter_ip", "meter_port","meter_pass", "meter_location" ,"meter_unique_id")
        keys_1= ("id", "dcu_id", "enable","meter_type","met_address", "met_loc", "meter_unique_id", "meter_type_name", "met_pass" , "ser_port" )
        keys_2 = ("comm_mode","ser_met_add_size", "ser_baud_rate", "ser_stop_bits","ser_parity", "ser_data" )

       
        results =  execute(2, "SELECT id, dev_id,  met_enable, meter_id, met_addr, comm_port, met_pass,met_loc, unique_id, ser_port_id FROM dcu_dlms_meter_tbl WHERE dev_id = '{}'".format(dev_id))
        no_rows= len(results)
        users= []

        met_merged_ser_data_1= []
        ser_met_data_2 =[]
        ser_met_data_3 =[]
        ser_met_data_4 =[]

        results_ser_port1 =  execute(2, "SELECT port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_ser_port2 =  execute(2, "SELECT port_type2,  ser_met_add_size2, ser_baud2, ser_stop_bit2, ser_parity2, ser_data_bits2 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_ser_port3 =  execute(2, "SELECT port_type3,  ser_met_add_size3, ser_baud3, ser_stop_bit3, ser_parity3, ser_data_bits3 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_ser_port4 =  execute(2, "SELECT port_type4,  ser_met_add_size4, ser_baud4, ser_stop_bit4, ser_parity4, ser_data_bits4 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))

        results_serial =  execute(2, "SELECT id, dev_id,  met_enable,met_type, met_addr, met_name, unique_id, meter_type_name, met_pass FROM dcu_dlms_meter_tbl WHERE dev_id = '{}'".format(dev_id))

        for idx in range(0, len(results_ser_port1)):
            ser_serial_1_1 = dict(zip(keys_2, results_ser_port1[idx]))
            

            for idx in range(0, len(results_serial)):
                if idx<5:
                    met_merged_data_1 = dict(zip(keys_1, results_serial[idx]))
                    
                    new_data = {**met_merged_data_1, **ser_serial_1_1}

                    met_merged_ser_data_1.append(new_data)

        
        for idx in range(0, len(results_ser_port2)):
            ser_serial_1_2 = dict(zip(keys_2, results_ser_port2[idx]))

            for idx in range(0, len(results_serial)):
                if idx >=5 and idx<10:
                    met_merged_data_2 = dict(zip(keys_1, results_serial[idx]))

                    new_data = {**met_merged_data_2, **ser_serial_1_2}  
                    ser_met_data_2.append(new_data)

        for idx in range(0, len(results_ser_port3)):
            ser_serial_1_3 = dict(zip(keys_2, results_ser_port3[idx]))

            for idx in range(0, len(results_serial)):
                if idx >=10 and idx<15:
                    met_merged_data_3 = dict(zip(keys_1, results_serial[idx]))

                    new_data = {**met_merged_data_3, **ser_serial_1_3}  
                    ser_met_data_3.append(new_data)

        for idx in range(0, len(results_ser_port4)):
            ser_serial_1_4 = dict(zip(keys_2, results_ser_port4[idx]))

            for idx in range(0, len(results_serial)):
                if idx >=15 and idx<20:
                    met_merged_data_4 = dict(zip(keys_1, results_serial[idx]))

                    new_data = {**met_merged_data_4, **ser_serial_1_4} 
                    ser_met_data_4.append(new_data)


       

        for idx in range(len(results)):
            merged_data = dict(zip(keys, results[idx]))
         
            users.append(merged_data)

        # user_data ={}
        if len(results)<=5:
            no_serial =1
        
        if len(results)>5 and len(results)<=10 :
            no_serial =2
        
        if len(results)>10 and len(results)<=15 :
            no_serial =3

        if len(results)>15 and len(results)<=20 :
            no_serial =4
        # else:
        #     no_serial=0

        if users != []:
            main_dict["status"] = "found"
            main_dict["no_serial"]= no_serial
            main_dict["serial_count"]= len(results)
            main_dict["serial_details"]= {}
            main_dict["serial_details"]["serPor1Cfg"]= met_merged_ser_data_1
            main_dict["serial_details"]["serPor2Cfg"]= ser_met_data_2
            main_dict["serial_details"]["serPor3Cfg"]= ser_met_data_3
            main_dict["serial_details"]["serPor4Cfg"]= ser_met_data_4

        else:
            main_dict["status"] = "not found"

     
        return main_dict
     
    # except jwt.ExpiredSignatureError as e:
    #     return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def get_device_general_config_by_id_extra(dev_id):
    fn = "get_device_general_config_by_id()"
    global ser_err, sec_code, db_conn
    try:
        

        main_dict= {}
        keys= ("id", "dev_id", "dbglog_enable","dbglog_ip","net_type","eth_ip1", "eth_new_ip1",  "eth_new_ip2", "eth_subnet1","eth_gateway1", "eth_ip2" ,"eth_subnet2", "eth_gateway2", "eth_ip3","eth_subnet3", "eth_gateway3", "eth_ip4",  "eth_subnet4","eth_gateway4","ntp_enable1", "ntp_enable2","ntp_ipadd1","ntp_port1", "ntp_ipadd2", "ntp_port2","ntp_interval" , "iec_enable","ftp_enable","mqtt_enable")
      
        keys_1=("enable_allow_master","master1_enable",  "master1_ip", "master2_enable", "master2_ip", "master3_enable", "master3_ip", "master4_enable","master4_ip")
        
        keys_2=("dcu_id", "dcu_location","num_eth_port")
      
        
        keys_3= ("ftp_enable", "ftp_ip","ftp_port", "ftp_user", "ftp_pass","ftp_dir", "ftp_period") 
        keys_4 =("iec_addr",  "iec_cyc_int", "iec_port", "iec_ioa","iec_cot", "iec_met_start_ioa")
        keys_5 =("mqtt_broker_ip",  "mqtt_broker_port", "mqtt_username", "mqtt_password","mqtt_periodicity")
        results =  execute(2, "SELECT id, dev_id,  dbglog_enable, dbglog_ip, net_type,eth_ip1, eth_ip1, eth_ip2, eth_subnet1,eth_gateway1, eth_ip2 ,eth_subnet2, eth_gateway2, eth_ip3, eth_subnet3, eth_gateway3, eth_ip4, eth_subnet4,eth_gateway4,ntp_enable1, ntp_enable2,ntp_ipadd1,ntp_port1, ntp_ipadd2, ntp_port2,ntp_interval ,iec_enable, ftp_enable,mqtt_enable  FROM dcu_gen_config_tbl WHERE dev_id = '{}'".format(dev_id))
        no_rows= len(results)
        users= []


        results_1 =  execute(2, "SELECT enable_allow_master, enable_ip1,  master1_ip, enable_ip2, master2_ip, enable_ip3, master3_ip, enable_ip4,master4_ip  FROM dcu_iec104_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        no_rows_1= len(results_1)

        results_2 =  execute(2, "SELECT dev_id, loc_name,  num_eth_port  FROM dev_tbl WHERE dev_id = '{}'".format(dev_id))
        no_rows_2= len(results_2)
     


        results_3 =  execute(2, "SELECT  ip,  port, user_name, pwd, directory, periodicity  FROM dcu_ftp_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_4 =  execute(2, "SELECT asdu, cyc_int,  port, iot_size, cot_size, start_ioa FROM dcu_iec104_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_5 =  execute(2, "SELECT mqtt_broker  , mqtt_broker_port ,  user_name , pwd , hc_int FROM dcu_mqtt_cfg_tbl WHERE dev_id = '{}'".format(dev_id))

    
        data_length = min(len(results), len(results_1), len(results_2), len(results_3), len(results_4))


        for idx in range(data_length):
            merged_data = dict(zip(keys, results[idx]))
            merged_data.update(dict(zip(keys_1, results_1[idx])))
            merged_data.update(dict(zip(keys_2, results_2[idx])))
            merged_data.update(dict(zip(keys_3, ([results[0][-1]] + list(results_3[idx])))))
            merged_data.update(dict(zip(keys_4, results_4[idx])))
            merged_data.update(dict(zip(keys_5, results_5[idx])))

            users.append(merged_data)

        if users != []:
            return users 
           
        else:
            main_dict["status"] = "not found"
            return main_dict
     
    
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
    
    
def is_future_date(day, month, year):
    current_date = datetime.now()
    given_date = datetime(year, month, day)
    return given_date > current_date

def type_dataview(table_name, from_datetime_str, column_name, functype):
    print(functype,"===========================================",table_name)
    fn = 'type_dataview'
    try:       
        if functype == "Load Survey" :
            query = "select * from {} where DATE({}) ='{}' and {} != '{} 00:00:00' or {} = '{} 00:00:00' order by `{}`".format(
                table_name, column_name, from_datetime_str[0], column_name, from_datetime_str[0], column_name, from_datetime_str[1], column_name)
            print(query)
        
        if functype == "Event":
          
            query = "select * from {}  order by ui_id  DESC limit {} ".format(
                table_name, column_name)
            
          

        if functype == "Daily Profile":
            query = "SELECT *FROM {} WHERE DATE_FORMAT({}, '%Y-%m') = '{}' order by `{}`".format(
                table_name, column_name, from_datetime_str, column_name)
            
           
        if  functype == "Billing":
             query = " SELECT * FROM {} WHERE DATE_FORMAT({}, '%Y') = '{}'  order by  `{}` DESC".format(
                table_name, column_name, from_datetime_str, column_name)
      

        result = get_key_value_pairs(query)

        if result == [] or result == None:
            return []
        else:
            final_dict = []

            
        # for row in result:
        #     row_dict = {}
        #     for key, value in row.items():
        #         # print(key)
        #         obis_val = get_obis_name(key,functype)
        #         # print(key,obis_val)
        #         if value == None or isinstance(value, str) or isinstance(value, int):
        #             row_dict[obis_val] = value
        #         else:
        #             row_dict[obis_val] = value.strftime('%Y-%m-%d %H:%M:%S')
        #     final_dict.append(row_dict)
        # print("result : ", result)
        for row in result:
            row_dict = {}
            for key, value in row.items():
                obis_val = get_obis_name_optimized(key, functype)
                
                if obis_val != 'DATE_TIME':
                    if value == None or value == "" or value == "NA" or  (value == "0.0" and obis_val=="EV_CODE"):
                        continue
                
                # For other types (string, int, or datetime), process as before
                if isinstance(value, (str, int)):
                    row_dict[obis_val] = value
                else:
                    row_dict[obis_val] = value.strftime('%Y-%m-%d %H:%M:%S')
                    
            final_dict.append(row_dict)
        print("final_dict : ", final_dict)
        return final_dict
    
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def preload_obis_cache():
    """Preload all OBIS mappings into the cache."""
    global obis_cache
    obis_cache = {}  # Initialize or clear cache

    # Fetch all mappings from the database
    results = execute(2, "SELECT obis_code, type, param_name FROM obis_param_map")

    # Populate the cache
    for obis_code, obis_type, param_name in results:
        obis_cache[(obis_code, obis_type)] = param_name


def get_obis_name_optimized(obis_code, obis_type):
    """Fetch OBIS name from cache, preloading if needed."""
    global obis_cache

    # Preload cache if it's empty
    if not obis_cache:
        preload_obis_cache()

    # Return from cache if available, else fallback to OBIS code
    return obis_cache.get((obis_code, obis_type), obis_code)


def dcufullname(dev_id):
    fn = 'dcufullname'
    try:
      
        query = "select loc_id,dev_name from dev_tbl where dev_id='{}'".format(
            dev_id)
        loc_id_dev_name = execute(1, query)
        if loc_id_dev_name == None:
            return "not found"
        else:
            loc_query = "select category_1,category_2,category_3,category_4,category_5,category_6,loc_name from loc_mgmt where id={}".format(
                loc_id_dev_name[0])
            loc_details = execute(1, loc_query)

            dcu_list = list(loc_details)

            filtered_list = [
                element for element in dcu_list if element.strip()]

            result_fin = '/'.join(filtered_list)
            
            dcu_name = result_fin+"/"+loc_id_dev_name[1]
            return dcu_name

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def total_device_online_offline(column_name):
    fn = "total_device_online_offline"
    try:
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")
        query = "select {} from dcu_overall_connectivity_dashboard".format(
            column_name)
        result = execute(4, query)
        total = 0
        for inner_tuple in result:
            total += sum(inner_tuple)
        return total
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def site_total(column, site_type):
    try:
        fn = "site_total"
        query = "select {} from dcu_overall_connectivity_dashboard where site_type='{}' ".format(
            column, site_type)
        result = execute(1, query)
        return result[0]
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def per_cal(value, totalvalue):
    try:
        fn = 'per_cal'

        if value == 0 or totalvalue == 0:
            return 0
        value = ((value/totalvalue)*100)

        calculate = round(value, 2)
        return int(calculate)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")


def update_details_db(dev_id, ret):  
    fn = "update_details_db()"  
    global db_conn  
    try: 
        header = request.headers.get('Authorization')
    
        jwt.decode(header, sec_code, algorithms="HS256")
        
        dev_cfg_keys= ("DEV_TOKEN", "LOC", "FW_VER", "SN", "DEV_SN", "TS")
        met_cfg_keys= ("METER_ENABLE", "METER_ADDR", "METER_PASSWORD", "METER_LOCATION")
        
     
        dev_config= dict(Session.set_config.copy())  
        
        config_dict= dev_config["DATA"]["CONFIG_DATA"]
        
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        db_dev_details = execute(1, "SELECT dev_token, loc_name, fw_ver, met_comm_type FROM dev_tbl WHERE dev_id = '{}'".format(dev_id))
        
        if len(db_dev_details) < 4:   
            logwarn(" {} : No valid device id: {} info available!".format(fn, dev_id))   
            return False
        
        loc_name= db_dev_details[1] 
        # meter_type= db_dev_details[3] 
        
        if db_dev_details[3] == 'RS-232':
            meter_type= '0'
        else:
            meter_type= '1'
            
        
        dev_config["NP"]= dict(zip(dev_cfg_keys, list(db_dev_details)[:3] + [dev_id, dev_id, now])) 

        
        met_details= []     
        for met_idx in range(1, MAX_METERS+1): 
            dev_met_details = execute(1, "SELECT met_enable, met_addr, met_pass, met_name FROM slave_dev_tbl WHERE unique_id = '{}_{}'".format(dev_id, met_idx))
            # print(met_idx)
            
            if dev_met_details == None or len(dev_met_details) == 0: 
                met_list= [0, None, None, None]
            else:
                met_list = list(dev_met_details) #+ [loc_name]
                
                if met_list[0] == 'Enable':
                    met_list[0] = "1"
                else:
                    met_list[0] = "0"

            # print(met_list[0])
            met_details.append(dict(zip(met_cfg_keys, met_list)))

            
        config_dict["DEV_LOC"] = loc_name
        config_dict["METER_COMM_TYPE"] = meter_type
        config_dict["DEV_SER_NUM"] = dev_id
        config_dict["METER_CONFIG"] = met_details
        
        dev_config["DATA"]["CONFIG_DATA"]= config_dict
        
        res = execute(1, "SELECT seq_num FROM command_tbl ORDER BY seq_num desc")
        if res != None:
            if len(res) != 0:
                seq_no = int(res[0]) + 1
            else:
                seq_no = 1000
        else:
            seq_no = 1000
            
        dev_config['SEQ_NUM'] = str(seq_no)
        send_json = json.dumps(dev_config)
        update_tbl("INSERT INTO command_tbl (COMMAND, seq_num, DEV_ID, ISSUE_TIME, COMMAND_STATUS, send_data, retry) VALUES ('SET_CONFIG', {}, {}, '{}', 'issued', '{}', 0)".format(seq_no, dev_id, now, send_json))
            
        return True
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return False

def graph_interval(data, st_date , time_stamp):
    
        data = [{'x': datetime.strptime(item['x'], '%Y-%m-%d %H:%M:%S'), 'y': item['y']} for item in data]

        start_time = datetime.strptime('{} 00:00:00'.format(st_date), '%Y-%m-%d %H:%M:%S')
        end_time = datetime.strptime('{} 23:45:00'.format(st_date), '%Y-%m-%d %H:%M:%S')

        expected_intervals = set(start_time + timedelta(minutes=5 * i) for i in range(time_stamp))
        
        actual_intervals = set(item['x'] for item in data)
        
        missing_intervals = expected_intervals - actual_intervals
        
        for interval in missing_intervals:
            data.append({'x': interval, 'y': ""})

        data.sort(key=lambda item: item['x'])
        
        for item in data:
            item['x'] = item['x'].strftime('%Y-%m-%d %H:%M:%S')

        return data

def get_event_dcu_info(dev_id):
    fn = 'get_event_dcu_info()'
    try:
        dev_info_qry = "SELECT dev_name, dev_sn, dev_model, loc_name, dev_ip_addr, dev_common_status from dev_tbl where dev_id = '" + dev_id + "'"
        dev_info = execute(2, dev_info_qry)
        dev_name, dev_sn, dev_model, loc_name, dev_ip_add, dev_common_status = dev_info[0]
        loc_info_qry = "SELECT site_type, gen_type from loc_mgmt where loc_name = '" + loc_name + "'"
        loc_info = execute(2, loc_info_qry)
        site_type, gen_type = loc_info[0]
        modem_link_sts_qry = "select modem_link_sts ,router_id from modem_link_tbl where dcu_id={}".format(dev_id)
        modem_link_sts_info = execute(2, modem_link_sts_qry)
        if modem_link_sts_info == ():
            modem_status = "NA"
            router_id = "NA"
        else:
            modem_status = modem_link_sts_info[0][0]
            router_id = modem_link_sts_info[0][1]
        if modem_status == "Internal":
            table_name = "dcu_modem_dig_info_" + dev_id
        elif modem_status == "External":
            table_name = "router_dig_info_" + router_id
        else:
            table_name = "no_tbl"
            nw_type = "VSAT"

        if not check_table(table_name):
            signal_strength = "-"
            if modem_status == "VSAT":
                nw_type = "VSAT"
            else:
                nw_type = "-"
        else:
            signal_con_info_qry = "select sig_str as signal_strength,nw_type as connectivity from `{}`  ORDER BY id DESC LIMIT 1".format(
                table_name)
            signal_con_info = get_key_value_pairs(signal_con_info_qry)
            if signal_con_info is None or signal_con_info == [] or dev_common_status == "Not Communicating":
                signal_strength = "-"
                nw_type = "-"
            else:
                signal_strength = signal_con_info[0]["signal_strength"]
                nw_type = signal_con_info[0]["connectivity"]
        dcu_info_data_list = [dev_name, dev_sn, dev_model, loc_name, nw_type, dev_ip_add, site_type, gen_type,
                              signal_strength]
        
        return dcu_info_data_list
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def NamePlate_dataview(dev_id,met_id,ser_port_id):
    fn="nameplate_dataview"
    try:
        qry="SHOW COLUMNS FROM dcu_dlms_meter_tbl LIKE 'load_status'"
        result_check=get_key_value_pairs(qry)
        # print(result_check)
        
        if result_check!=None or result_check!=[]:

            query= "select dev_name as dcu_name,met_name,met_sn,met_manf_name,met_version,block_interval,int_ct_ratio,int_pt_ratio,comn_status as status,load_status as load_status from  dcu_dlms_meter_tbl where dev_id='{}' and meter_id='{}' and ser_port_id='{}'".format(dev_id,met_id,ser_port_id)
            result=get_key_value_pairs(query)
        else:
            query= "select dev_name as dcu_name,met_name,met_sn,met_manf_name,met_version,block_interval,int_ct_ratio,int_pt_ratio,comn_status as status from  dcu_dlms_meter_tbl where dev_id='{}' and meter_id='{}' and ser_port_id='{}'".format(dev_id,met_id,ser_port_id)
            result=get_key_value_pairs(query)
            result[0]["load_status"] ="test data"
        
        dcu_name = dcufullname(dev_id)
       
        
        
        if result == () or result == None or result == []:
            return []
        else:
            result[0]["dcu_fullname"]=dcu_name  
            print(result)
            return result
            
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")



def get_old_data(record_id,table_name):
   
    if table_name=="user_mgmt":
        query = "SELECT id,role,user_name,user_full_name,mail_id as email, ph_no  FROM {} WHERE id = '{}'".format(table_name,record_id)  
    elif table_name=="loc_mgmt":
        query = "SELECT id ,loc_name ,site_type,gen_type,category_1,category_2,category_3,category_4,category_5,category_6 FROM {} WHERE id = '{}'".format(table_name,record_id) 
    elif table_name=="obis_param_map":
        query = "SELECT id ,obis_code as obis ,param_name as def,type FROM {} WHERE id = '{}'".format(table_name,record_id)  
    elif table_name=="dev_tbl":
        query = "SELECT id , loc_name as location_name ,dev_name as device_name,dev_ip_addr as ip_address,dev_id as device_id FROM {} WHERE id = '{}'".format(table_name,record_id)
  
    else:
        query = "SELECT * FROM {} WHERE id = '{}'".format(table_name,record_id)
    old_data =get_key_value_pairs(query)

    if old_data == None or old_data == []:
        return []
   
    return old_data


def generate_audit_message(new_data, old_data):
    changes = []
   
    for key in old_data:
        if key!="loc_id" and key!="user_id" and  key!= 'active_user' and key!='active_role' and key!="id":
          
            if old_data[key] != new_data[key] :
                if key=="location_name" or key== "loc_name" or key=="dcu_location":
                    key1="location name"
                elif key=="ph_no" or key== "mobile_num" or key=="phone_num" or key=="mobile_num":
                    key1="phone number"
                elif key=="user_name":
                    key1="user name"
                elif key=="email_id" or key=="email":
                    key1="E-mail Address"
                elif key=="user_full_name":
                    key1="user full name"
                elif key=="meter_type_name":
                    key1="meter type name"
                elif key=="category_1":
                    key1="category 1"
                elif key=="category_2":
                    key1="category 2"
                elif key=="category_3":
                    key1="category 3"
                elif key=="category_4":
                    key1="category 4"
                elif key=="category_5":
                    key1="category 5"
                elif key=="category_6":
                    key1="category 6"
                elif key=="category_type":
                    key1="category type"
                elif key=="ph_mail":
                    key1="SMS / MAIL"
                elif key=="obis":
                    key1="obis code"
                elif key=="def":
                    key1="obis name"
                elif key=="device_name":
                    key1="device name"
                elif key=="ip_address":
                    key1="ip address"
                elif key=="ftp_enable":
                    key1="FTP enable"
                elif key=="mqtt_enable":
                    key1="MQTT enable"
                elif key=="eth_new_ip1":
                    key1="ETHERNET 1 new ip address"
                elif key=="eth_new_ip2":
                    key1="ETHERNET 2 new ip address"
                elif key=="eth_subnet1":
                    key1="ETHERNET 1 subnet mask"
                elif key=="eth_subnet2":
                    key1="ETHERNET 2 subnet mask"
                elif key=="eth_gateway2":
                    key1="ETHERNET 2 Gateway"
                elif key=="eth_gateway1":
                    key1="ETHERNET 1 Gateway"
                elif key=="ntp_enable1":
                    key1="server 1 : NTP enable"
                elif key=="ntp_enable2":
                    key1="server 2 : NTP enable"
                elif key=="ntp_ipadd1":
                    key1="server 1 : ip address"
                elif key=="ntp_ipadd2":
                    key1="server 2 :ip address"
                elif key=="ntp_port1":
                    key1="server 1 :port"
                elif key=="ntp_port2":
                    key1="server 2 :port"
                elif key=="ntp_interval":
                    key1="update interval"
                elif key=="mqtt_broker_ip":
                    key1="MQTT ip address"
                elif key=="mqtt_broker_port":
                    key1="MQTT port"
                elif key=="mqtt_username":
                    key1="MQTT username"
                elif key=="ftp_ip":
                    key1="FTP ip address"
                elif key=="ftp_user":
                    key1="FTP username"
                elif key=="ftp_dir":
                    key1="FTP directory"
                elif key=="ftp_period ":
                    key1="FTP time period"
                elif key=="iec_enable":
                    key1="IEC enable"
                elif key=="iec_addr":
                    key1="IEC station address"
                elif key=="iec_cyc_int":
                    key1="IEC cycle int"
                elif key=="iec_port":
                    key1="IEC port number"
                elif key=="iec_ioa":
                    key1="IEC ioa"
                elif key=="iec_cot":
                    key1="IEC cot"
                elif key=="iec_met_start_ioa":
                    key1="IEC start ioa Address"
                elif key=="enable_allow_master":
                    key1="allow master enable"
                elif key=="master1_enable":
                    key1="master 1 enable"
                elif key=="master1_ip":
                    key1="master 1 ip address"
                elif key=="master2_enable":
                    key1="master 2 enable"
                elif key=="master3_enable":
                    key1="master 3 enable"
                elif key=="master3_ip":
                    key1="master 3 ip address"
                elif key=="master4_enable":
                    key1="master 4 enable"
                elif key=="master4_ip":
                    key1="master 4 ip address"
                elif key=="master2_ip":
                    key1="master 2 ip address"
                elif key=="ntp_port1 ":
                    key1="IEC start ioa Address"
                    
                   
                else:
                    key1=key  
                
                changes.append(f" {key1} updated from ({old_data[key]}) to ({new_data[key]})")
    if changes!=[]:

        audit_message = " , ".join(changes)
        return audit_message

    else:
        return "No Updation Made"


def dcu_summary_info_dataview(dev_id):
    fn = 'dev_summary_info'
    try:
      
        request_data = json.loads(request.data)
    
        
        query = "select dev_name,dev_ip_addr,loc_name,dev_model,dev_type,num_meters,dev_common_status,last_msg_recv_time as last_date_time,conct_up_time ,dev_reg_status ,dev_sn from dev_tbl where dev_id='{}'".format(
        dev_id)
        result = get_key_value_pairs(query)
        
        if result==None or result==[]:
            return Response(json.dumps({"status": "not found"}), mimetype='application/json', status=200)
     
        if result[0]["dev_reg_status"]=="Not registered" or result[0]["dev_reg_status"]==None:
                result[0]["dev_common_status"]="Not registered"
        
        
        if result[0]["dev_common_status"]=="Not Communicating":
            result[0]["conct_up_time"]="-"
      

        if result[0]["last_date_time"] == None:
            result[0]["last_date_time"] = '00:00:00'
            
            
        query4 = "select modem_link_sts ,router_id from modem_link_tbl where dcu_id='{}'".format(
            request_data["dev_id"])
        modem_link_sts = execute(2, query4)
       
        if modem_link_sts==():
            modem_status= "NA"
            router_id="NA"
        else:
            modem_status=modem_link_sts [0][0]
            router_id=modem_link_sts[0][1]
      
            
        if modem_status== "Internal":
            table_name = "dcu_modem_dig_info_" + request_data["dev_id"]

        else:
            table_name = "router_dig_info_" + router_id
            
      
       
            
        if not check_table(table_name) :
                signal_strength="-"
                nw_type="-"
        else:
            query1 = "select sig_str as signal_strength, nw_type as connectivity from `{}`  ORDER BY id DESC LIMIT 1".format(
            table_name)
            result1 = get_key_value_pairs(query1)

            if result1 == None or result1==[]:
                signal_strength = "-"
                nw_type = "-"
            else:
                signal_strength= result1[0]["signal_strength"]
                nw_type =result1[0]["connectivity"]
            
            

        query2 = "select * from `dcu_dlms_meter_tbl` where comn_status = 'Communicating' and dev_id='{}' and met_enable = 'Yes' AND ser_port_id != '' and met_sn != ''".format(
        request_data["dev_id"])
        online_meters = execute(2, query2)
     

        if online_meters == None or result[0]["dev_common_status"]=="Not Communicating":
            len_online_meters= 0
        else:
            len_online_meters=len(online_meters)


        query_conf = "select * from `dcu_dlms_meter_tbl` where met_enable = 'Yes' and dev_id='{}' and comn_status !='Not Configured' AND ser_port_id != '' and met_sn != ''".format(
        request_data["dev_id"]) 
        configured_meters = execute(2, query_conf)
      
        
        # query_metsn = "select met_sn from `dcu_dlms_meter_tbl` where  dev_id='{}' and ser_port_id='{}' and meter_id='{}'".format(request_data["dev_id"], request_data["ser_port_id"],  request_data["met_id"])
        # print("---------metsn-----------------",query_metsn)
        # metsn = execute(1, query_metsn)
        # print(">>>>>>>>query_metsn>>>>>>>>>>>>>>", metsn)
        # if metsn==None:
        #     met_serial="-"
        # else:
        #     met_serial=metsn[0]


        if online_meters == None:
            online_meters = 0

        dev_summary = result[0]
        dev_summary["modem_type"] = modem_status
        dev_summary["signal_strength"] = signal_strength
        dev_summary["connectivity"] = nw_type
        dev_summary["online_meters"] = len_online_meters
        dev_summary["configured"] = len(configured_meters)
       
     

        # dev_query = "select site_type from loc_mgmt where id ='{}' ".format(
        # loc_id)
        # dev_type = execute(1, dev_query)
     

        # if dev_type == "IPPS":
        #     dev_summary["category"] = dev_type[0] + \
        #     "("+ipps_type+")"
        # else:
        #     dev_summary["category"] = dev_type[0]

        dcu_name = dcufullname(request_data["dev_id"])

        dict_final= {"dcu_fullname": dcu_name,
         "dcu_name": result[0]["dev_name"],
           "dev_id": request_data["dev_id"], 
           "details": dev_summary
           }
        return dict_final
    # except jwt.ExpiredSignatureError as e:
    #         return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def dcu_alert_all(start_date,end_date,input,final_result):
    fn="dcu_alert_all"
    try:
        table_name="dcu_event_{}_tbl".format(input["dev_id"])
      
        if final_result==None:
            final_result=[]
        else:
            final_result=final_result
            
        if not check_table("dev_tbl"):
            return None
        if not check_table("dcu_dlms_meter_tbl"):
            return None
        
   
        
        if "dcu_name" not in input:
            return final_result
     
    
        dev_id=get_key_value_pairs("select dev_id from dev_tbl where dev_name='{}'".format(input["dcu_name"]))
    
        if input["meter_name"] =="All":
            # input["type"]=="All"
                query="select unique_id ,dev_name , met_name from dcu_dlms_meter_tbl where dev_id='{}' ".format(input["dev_id"])
          
        else:
            query="select unique_id ,dev_name , met_name from dcu_dlms_meter_tbl where dev_id='{}' and  unique_id = '{}'".format(input["dev_id"],input["unique_id"] )
          
        meter_ids=get_key_value_pairs(query)
    
        if meter_ids!=None or meter_ids!=[]:
    
            for m_id in meter_ids:
                table_name="dcu_event_data_{}".format(m_id["unique_id"])
                
              
              
                
                if check_table(table_name)==True:
                    
                    if input["event_code"]=="All":
                        query="select update_time as event_time,EVENTS as events  from {} where DATE(update_time) BETWEEN '{}' and '{}'  order by id asc ".format(table_name,start_date,end_date)
                    elif  input["event_code"]!="All"  :
                        query="select update_time as event_time,EVENTS as events  from {} where DATE(update_time) BETWEEN '{}' and '{}' and EVENTS='{}' order by id asc ".format(table_name,start_date,end_date,input["event_code"])
                    result=get_key_value_pairs(query)
                   
                    print(query)
                    if result!=None or result!=[]:
                        for i in range (0,int(len(result))):
                            
                            result[i]["dcu_name"]=m_id["dev_name"]
                            result[i]["meter_name"]=m_id["met_name"]
                            result[i]["type"]="Meter Events"
                            final_result.append(result[i])
                            
        
        return final_result
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
                (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")


    
#Login

def feedback_form():
    
    fn = 'feedback_form'
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        if mail_server == None or mail_port == None or sender == None or mail_passwd == None or support_mail == None:
            err = json.dumps({"status": "Invalid Mail details!"})
            return Response(err, mimetype='application/json')

        post_dict = json.loads(request.headers.get('From'))

        username = post_dict["user_name"]

        if 0:
            post_dict = {}
            post_dict['attach_sts'] = request.form.get('attach_sts')
            username = request.form['user_name']
            post_dict['role'] = request.form.get('role')
            post_dict['category'] = request.form.get('category')
            post_dict['msg'] = request.form.get('msg')
            post_dict['feedback_sts'] = request.form.get('feedback_sts')

        if post_dict['attach_sts'] == 'true':
            files = request.files['config_file']
        else:
            files = []
        user = execute(
            1, "SELECT mail_id FROM user_mgmt WHERE user_name = '{}'".format(username))

        if user == None or user == ():
            err = json.dumps({"status": "Invalid User!"})
            return Response(err, mimetype='application/json')

        user_mail = user[0]

        message = MIMEMultipart()
        message['Subject'] = "EV USER SUPPORT : {}".format(
            post_dict['category'])
        message['From'] = sender
        message['To'] = support_mail

        feed_back = '''
            <html>
                <body>
                    <p>
                        <h3>User {},</h3>
                        <h4>Role: {}</h4>
                        <h4>Message category: {}</h4>
                        <h4>Given feedback / Asking support is:</h4>
                        <h3>&nbsp;&nbsp; {}</h3>
                        <h4>Overall Rating: {}</h4>
                        <h3>User Contact Info: {}</h3>
                    </p>
                </body>
            </html>
            '''.format(username, post_dict['role'], post_dict['category'], post_dict['msg'], post_dict['feedback_sts'], user_mail)

        message.attach(MIMEText(feed_back, "html"))

        if files != []:
            filename = files.filename
            files.save(filename)

            with open(filename, "rb") as attachment:
                part = MIMEImage(attachment.read(),
                                 name=os.path.basename(filename))

            os.remove(filename)
            message.attach(part)

        smpt = smtplib.SMTP(host=mail_server, port=mail_port, timeout=30)
        smpt.starttls()
        smpt.login(sender, mail_passwd)
        smpt.sendmail(sender, support_mail, message.as_string())
        smpt.quit()
     
        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                        post_dict['role'],  username, "Support", "feedback given in the category of ({}).".format(post_dict['category']))
        update_tbl(sql)
        return Response(json.dumps({"status": "success"}), mimetype='application/json')

    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)


    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def change_password():
    fn = "change_password()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
 
        login_dict = json.loads(request.data)
        results =execute(2, "SELECT user_name, `password` FROM user_mgmt WHERE user_name='{}'".format(login_dict["user_name"]))
        if len(results)==0:
           return Response(json.dumps({"status": "Invalid Username!"}), mimetype='application/json')

        if (login_dict["newpassword"])==(login_dict["oldpassword"]):
           return Response(json.dumps({"status": "Old and New Passwords should not be same!"}), mimetype='application/json')
        no_rows = len(results)

        for id in range(0, no_rows):
            user_name = results[id][0]
            pass_wd = results[id][1]
            decrypted_password = cipher_suite.decrypt(pass_wd.encode('utf-8'))
            passwd=decrypted_password.decode('utf-8')
            
            new_pass_wd = login_dict["newpassword"]
            
            special_chars_pattern = r'[!@#$%^&*()_+{}|:"<>?`\-=[\];\',./]'  #if not re.search(special_chars_pattern, password):
            
         

            if(user_name == login_dict["user_name"]):
                if(passwd == login_dict["oldpassword"]) :

                    if (len(new_pass_wd)) < 8:
                        return Response(json.dumps({"status": "Password contains atlest 8 charecters"}), mimetype='application/json')
                    
                    if (len(new_pass_wd)) >32:
                        return Response(json.dumps({"status": "Password contain greaterthan 32 charecters"}), mimetype='application/json')
                    
                    if not re.search(special_chars_pattern, new_pass_wd):
                        return Response(json.dumps({"status": "Password contain atleast 1 special charecters"}), mimetype='application/json')
                    
                    
                    encrypted_password = cipher_suite.encrypt(new_pass_wd.encode('utf-8'))
                    
                    update_tbl("UPDATE user_mgmt SET `password`= '{}' WHERE user_name= '{}'".format(encrypted_password.decode('utf-8'), user_name))
                    
                    sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                    login_dict["active_role"],  user_name, "Login / Password", "login password changed")
                    update_tbl(sql)
  
                    return Response(json.dumps({"status": "success"}), mimetype='application/json')
                else:
                    return Response(json.dumps({"status": "Incorrect Password"}), mimetype='application/json')
                
            else:
              return Response(json.dumps({"status": "Incorrect Username"}), mimetype='application/json')  
        else:
            return Response(json.dumps({"status": "Incorrect Password"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def login():
    fn = "login()"
    global sec_code, db_conn, refresh_code
    try:
        start=datetime.now()
        input = json.loads(request.data)
        
        # schema_result=check_schema(input,login_schema)
        # if schema_result!=True:
        #     return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        
        for key, value in input.items():
            if value =="":
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

        resultss = execute(2, "SELECT  user_name, `password` , max_num_users, num_users FROM user_mgmt where user_name='{}'".format(input["user_name"]))
       
        if resultss==():
            now = datetime.now()
            curtime= now + timedelta(hours=24)
            sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( now,
            "-",  "-", "Login / Logout", "Login failed: Invalid username entered")
            sql=update_tbl(sql)
            return Response(json.dumps({"status": "Invalid Username"}), mimetype='application/json')
        print(resultss)
        if  resultss[0][3]==None:
            resultss = (resultss[0][:3] + (0,),)

        decrypted_password = cipher_suite.decrypt(resultss[0][1].encode('utf-8'))
        db_passwd = decrypted_password.decode('utf-8')
    
        if resultss[0][3]>=resultss[0][2]:
            if input["password"]==db_passwd:
                return Response(json.dumps({"status": "session limit reached!"}), mimetype='application/json')
            else:
                return Response(json.dumps({"status": "Invalid Password"}), mimetype='application/json')
        else:
            num_users=int(resultss[0][3])+1
     
            
        main_dict={}
        if input["user_name"]==resultss[0][0]:
            if input["password"]==db_passwd:
                    user_info = execute(1,"SELECT id, `role`, user_full_name, mail_id, ph_no FROM user_mgmt WHERE user_name= '{}'".format(input["user_name"]))   
                    now = datetime.now()
                    session_id = random.randint(0, 10000)
                    if user_info[-2] == session_id:
                        session_id += 105 
                    main_dict["user_details"] ={
                        "id": user_info[0],
                        "role": user_info[1],
                        "user_name": input["user_name"],
                        "user_full_name": user_info[2],
                        "email": user_info[3],
                        "ph_no": user_info[4]
                    }
                    now = datetime.now()
                    curtime= now + timedelta(hours=24)
                    resp_dict  = {
                    "payload": main_dict,
                    "exp": time.mktime(curtime.timetuple()) #minutes=15
                    }
                    encoded_jwt = jwt.encode(resp_dict, sec_code, algorithm= "HS256")
                    
                    

                    main_dict["user_details"]["access_token"] = encoded_jwt
                    main_dict["user_details"]["session_id"] = session_id
                    main_dict["user_details"]["password"] = input["password"]
                    main_dict["status"] = "success"
                    
                    log_query="UPDATE `user_mgmt` SET  num_users='{}' WHERE user_name= '{}' AND `password`= '{}'".format(num_users, resultss[0][0], resultss[0][1])
                    log_query=update_tbl(log_query)
               
                    session_query="INSERT INTO session_mgmt(date_time,user_name, session_id,  login_status) VALUES ('{}', '{}', '{}', '{}')".format( now,
                    resultss[0][0],  session_id, "loged-in")
                    session_query=update_tbl(session_query)
                
                    sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( now,
                    user_info[1],  resultss[0][0], "Login / Logout", "logged in successfully")
                    sql=update_tbl(sql)
                 
            else:
                    now = datetime.now()
                    curtime= now + timedelta(hours=24)
                    user_info = execute(1,"SELECT id, `role`, user_full_name, mail_id, ph_no FROM user_mgmt WHERE user_name= '{}'".format(input["user_name"]))
                    main_dict["status"] = "Invalid password"
                    sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( now,
                    user_info[1],  resultss[0][0], "Login / Logout", "Login failed: Invalid password entered")
                    sql=update_tbl(sql)
            end=datetime.now()
            difference_time=difference(start,end,fn)
            print(main_dict)
            # print(" {}  Time difference: {}".format(fn,difference_time) )    
            return Response(json.dumps(main_dict), mimetype='application/json')
        else:
            return Response(json.dumps({"status": "Invalid Username"}), mimetype='application/json')
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("-------jj--------------------------")
        return Response(ser_err, mimetype='application/json')
  
def ping_user():
    fn = "...ping user..."
    global ser_err, sec_code, db_conn, refresh_code
    try:
        start=datetime.now()
        # header = request.headers.get('Authorization')
        # header = header.encode()
        # jwt.decode(header, sec_code, algorithms= "HS256")

        main_dict = {}
        input = json.loads(request.data)
        user_name = input['user_name']
        pass_wd = input['password']
    
        results = execute(1, "SELECT login_status ,session_id  FROM session_mgmt WHERE user_name = '{}' and session_id='{}'".format(user_name, input['session_id'])) 
        if results == None :
            return Response(json.dumps({"status": "terminate"}), mimetype='application/json')
        
        else:
            query="select password from user_mgmt where user_name='{}'".format(input['user_name']) 
            passwd=get_key_value_pairs( query)
            decrypted_password = cipher_suite.decrypt(passwd[0]["password"].encode('utf-8'))
            passwd=decrypted_password.decode('utf-8')
            
            if pass_wd==passwd:
                main_dict['status'] = results[0]
                main_dict['session_id'] = results[1]
        end=datetime.now()
        difference_time=difference(start,end,fn)
        # print(" {}  Time difference: {}".format(fn,difference_time) )
        return Response(json.dumps(main_dict), mimetype='application/json')
        
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)


    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')


def logout():
    fn = "logout()"
    global ser_err, sec_code, db_conn, refresh_code
    try:
        start=datetime.now()
        input = json.loads(request.data)

        # schema_result=check_schema(input,logout_schema)
        # if schema_result!=True:
        #     return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        for key, value in input.items():
            if value =="":
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
       
        results_session = execute(1,"SELECT login_status FROM session_mgmt WHERE user_name = '{}' and session_id='{}'".format(input['user_name'], input['session_id']))
       
        if results_session==None:
            return Response(json.dumps({"status": "success"}), mimetype='application/json')
        if results_session[0] == 'terminate':
            return Response(json.dumps({"status": "user session is inactive"}), mimetype='application/json')
        
        results_user = execute(1,"SELECT `role`, password , num_users ,user_name FROM user_mgmt WHERE user_name = '{}'".format(input['user_name']))
      
        if results_user == None or len(results_user) == 0:
            return Response(json.dumps({"status": "Invalid Username "}), mimetype='application/json')
       
        db_decrypted_password = cipher_suite.decrypt(results_user[1] .encode('utf-8'))
        db_pass_wd=db_decrypted_password.decode('utf-8')
        
        if input['password']==db_pass_wd and results_user[3]==input['user_name']:
            if results_session[0]=='loged-in':
                update_tbl("UPDATE `user_mgmt` SET  num_users='{}' WHERE user_name= '{}'".format(int(results_user[2])-1,input['user_name']))
                update_tbl("DELETE  FROM  `session_mgmt` WHERE session_id='{}' AND  user_name= '{}'".format(input['session_id'],input['user_name']))
                update_tbl("INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                input['active_role'], input['active_user'], "Login / Logout", "logged out successfully"))
             
        end=datetime.now()
        difference_time=difference(start,end,fn)
        # print(" {}  Time difference: {}".format(fn,difference_time) ) 
        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    # except jwt.ExpiredSignatureError as e:
    #             return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)       
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
    
    
def send_mail():
    fn= "send_mail()"
    global ser_err, sec_code, db_conn, mail_server, mail_port, sender, mail_passwd, PORT
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        post_dict = json.loads(request.data)
        
        username= post_dict['user_name'] 
        
        user =execute(1, "SELECT mail_id FROM user_mgmt WHERE user_name = '{}'".format(username))
        
        if user == None or user == ():
            err= json.dumps({"status": "Invalid User!"})
            return Response(err, mimetype='application/json')

        recever= user[0]
        now = datetime.now()
        
        curtime= now + timedelta(minutes=2)
        resp_dict  = {
        "payload": {"reason" : mail_code},
        "exp": time.mktime(curtime.timetuple())
        }
        token = jwt.encode(resp_dict, sec_code, algorithm="HS256")
        
        if PRODUCTION != 2:
            hostname = socket.gethostname()
            IPAddr = socket.gethostbyname(hostname)
        else:
            metadata_url = "http://169.254.169.254/latest/meta-data/public-ipv4"
            IPAddr      = requests.get(metadata_url).text
 
        
        subject = "RMS MONITOR : RESET PASSWORD"
        # URL = "https://cmscloud.in/#/reset/{}".format(username)# /{}   token    http://localhost:8082/#/
        URL = "http://{}:{}/#/reset/{}/{}".format(IPAddr, PORT, username, token)    
        
        if 1:

            body = '''<h3>Dear {},</h3>
            <h2>We received a request to reset your password. Click the below link to reset the password.</h2>

            <a href={}>{}</a><br><br>

            <b>Contact Us:</b><br>
                  &nbsp; Mail Us: <a href=support@cmsgp.com>support@cmsgp.com</a><br>

                  &nbsp; Call Us: +91-9741966060<br>

                  &nbsp; Address: No. 300-B, 5th Main, 4th Phase, Peenya Industrial Area, Bangalore, Karnataka 560058.<br><br>

            @{} Creative Micro Systems <a href=https://www.cmsgp.com>https://www.cmsgp.com</a><br>
            '''.format(username, URL, URL, now.year)
            
            
            html_body = '''
            <html>
                <body>
                    <img src='cid:myimageid' width="80">
                    <p>{}</p>
                </body>
            </html>
            '''.format(body)
        
        if 0:
            with open("resetemailtemplate.html", "r") as f:
                body = f.read()
                
            html_body = body.replace("$$year$$", "{}".format(now.year))
            html_body = html_body.replace("$$resetpasswordUrl$$", "{}".format(URL))
            html_body = html_body.replace('$$Username$$', username)

        message = MIMEMultipart()
        message['Subject'] = subject
        message['From'] = sender
        message['To'] = recever
        
        message.attach(MIMEText(html_body, "html"))
    
        smpt= smtplib.SMTP(host = mail_server, port= mail_port, timeout= 5 )
        smpt.starttls()
        smpt.login(sender, mail_passwd)
        smpt.sendmail(sender, recever, message.as_string())
        smpt.quit()
            
        resp ={
            "status": "success",
       
        }
            
        return Response(json.dumps(resp), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)   
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        print(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def kick_out_user():
    fn = "kick_out_user()"
    global ser_err, sec_code, db_conn, refresh_code
    try:
        start=datetime.now()
        main_dict = {}
        input = json.loads(request.data)
        
        results=execute(1,"select role ,num_users from user_mgmt where user_name='{}'".format(input['user_name'])  )
       
        if results[0]=="Operator":
            chance=chance_operator
        elif results[0]=="Super Admin":
            chance=chance_superadmin
        else:
            chance=chance_admin
        
        status_results = execute(2,"SELECT `login_status` FROM session_mgmt WHERE user_name = '{}' ".format(input['user_name']))
        if status_results == None :
            return Response(json.dumps({"status": "Invalid Username or Password"}), mimetype='application/json')
        
        # print(results[1],chance)
        # print(type(results[1]),type(chance))
        
        if int(results[1])>=int(chance):
            update_tbl("UPDATE `user_mgmt` SET `num_users` = '0' WHERE user_name= '{}'".format(input['user_name']))
            update_tbl("DELETE  FROM  `session_mgmt` WHERE   user_name= '{}'".format(input['user_name']))
            main_dict["status"] = "success"
        
        else:
            main_dict["status"] = "Invalid Entry"
        
            update_tbl( "INSERT INTO audit_table (date_time, active_role, active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        results[0],  input['user_name'], "Login / Logout", "user logged out forcefully".format(input['user_name'])))
       
        end=datetime.now()
        difference_time=difference(start,end,fn)
        # print(" {}  Time difference: {}".format(fn,difference_time) )
        return Response(json.dumps(main_dict), mimetype='application/json')
        
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

   
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')



def validate_token():
    fn = "validate_token()"
    global sec_code, db_conn, mail_code
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        err= json.dumps({"status": "Invalid", "msg": "Request expired"})
     
        
        message = jwt.decode(header, sec_code, algorithms="HS256")
        if message['payload']["reason"] ==  mail_code:
            return Response(json.dumps({"status": "Valid"}), mimetype='application/json', status=200)
        else:
            err["msg"] = "Invalid message"
            return Response(err, mimetype='application/json', status=200)
    
  
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(err, mimetype='application/json')

def reset_password():
    fn = "reset_password()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        post_dict = json.loads(request.data)
        
        host_name= request.environ['REMOTE_ADDR']
        port = request.environ['REMOTE_PORT']
        
        URL = "{}:{}/".format(host_name, port)
      
        results =execute(2, "SELECT user_name, `role` FROM user_mgmt" )
        no_rows = len(results)

        for id in range(0, no_rows):
            user_name = results[id][0]
            if(user_name == post_dict["user_name"]):
                decrypted_password = cipher_suite.encrypt(post_dict["password"].encode('utf-8'))
                passwd=decrypted_password.decode('utf-8')
            
                update_tbl("UPDATE user_mgmt SET `password`= '{}' WHERE user_name= '{}'".format(passwd, user_name))
           
                user =execute(1, "SELECT role FROM user_mgmt WHERE user_name = '{}'".format(user_name))
                role= user[0]

                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                role,  user_name, "Login / Password", "login password reseted")
                update_tbl(sql)
                
                return Response(json.dumps({"status": "success", "url": URL}), mimetype='application/json')
        else:
            return Response(json.dumps({"status": "Invalid Username", "msg": "Invalid Username"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

#main dash
def get_install_details():
    fn = 'get_install_details'
    try:
        header = request.headers.get('Authorization')
        
        header = header.encode()
    
        jwt.decode(header, sec_code, algorithms="HS256")
        
        if not check_table("dcu_overall_connectivity_dashboard"):
            return Response(json.dumps({"status": "found", "total_install": 0, "online_count": 0, "offline_count": 0}), mimetype = 'application/json', status = 200)

        result = execute(1, "SELECT SUM(total_devices) AS total_device, SUM(online) AS online, SUM(offline) AS offline FROM  dcu_overall_connectivity_dashboard")
        if result == None:
            return Response(json.dumps({"status": "found", "total_install": 0, "online_count": 0, "offline_count": 0}), mimetype = 'application/json', status = 200)

        return Response(json.dumps({"status": "found", "total_install": int(result[0]), "online_count": int(result[1]), "offline_count": int(result[2])}), mimetype = 'application/json', status = 200)
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status = 200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    

def get_division_details():
    fn = 'get_division_details'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        if not check_table("loc_mgmt"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)

        division_result = execute(2, "SELECT DISTINCT category_1 FROM loc_mgmt")
        
        if division_result == None or division_result==(('',),) or division_result==():
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        
        division_list = [{"div_name": division[0]} for division in division_result if division[0]!='' ]

        return Response(json.dumps({"status": "found", "details": division_list}), mimetype = 'application/json', status = 200)
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def get_connv_dataavl_monthprofile_details():
    fn = 'get_connv_dataavl_monthprofile_details'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        connv_dataavil_input = json.loads(request.data)
        month, year = connv_dataavil_input['start_month'].split(' ')
        month_numeric_value = Month[month]

        if connv_dataavil_input['type'] == 'Connectivity':
            tbl_name = 'dcu_monthly_connectivity_tbl'
            col_name = 'overall_connect_per'
        else:
            tbl_name = 'dcu_monthly_data_avail_info'
            col_name = 'overall_data_avail_per'
            
        if not check_table("dcu_monthly_data_avail_info"):
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")
        connv_avail_query = "SELECT {}, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM {} WHERE YEAR(block_date) = '{}' AND MONTH(block_date) = '{}' ORDER BY block_date".format(col_name, tbl_name,
                                                                                                                                                            year, month_numeric_value)

        connv_avail_result = execute(2, connv_avail_query)
        if connv_avail_result == () or connv_avail_result == None:
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")
        
        num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]

        data_list = []
        tot_vals = len(connv_avail_result)
        tuple_idx = 0
        for idx in range(0, num_days):
            day_value = str(idx + 1).zfill(2)
            day = f"{year}-{month_numeric_value}-{day_value}"

            if tot_vals > tuple_idx:
                data = connv_avail_result[tuple_idx]
            else:
                local_dict = {"x": day, "y": 0}
                data_list.append(local_dict)
                continue

            if day == str(data[1]):
                tuple_idx += 1
                if data[0] == None:
                    data = list(data)
                    data[0] = 0

                local_dict = {"x": data[1], "y": float(data[0])}
                data_list.append(local_dict)

            else:
                local_dict = {"x": day, "y": 0}
                data_list.append(local_dict)

        avail_data = {"name": connv_dataavil_input["type"], "data": data_list}
   
    
        return Response(json.dumps({"status":"found", "details": [avail_data]}), mimetype = 'application/json', status = 200)
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
   
def get_connectivity_device_monthly_graph():

    fn = "get_connectivity_device_monthly_graph"
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        request_data = json.loads(request.data)

        schema_result=check_schema(request_data,connectivity_monthly_graph)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        keys_to_check = ["active_role", "active_user", "start_month","site_type"]
        for key in keys_to_check:
            if request_data[key] == "":
                return Response(json.dumps({"status": "Empty value found for key '{}'".format(key)}), mimetype="application/json")
             
        if request_data['site_type'] == 'IPPS' and request_data["gen_type"] == "":
            return Response(json.dumps({"status": "Empty value found for key gen_type"}), mimetype="application/json")
        start_month = request_data["start_month"]
        month_name, year = start_month.split()

        month_numeric_value = Month[month_name]
        num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]

        if request_data["gen_type"] == "All" or request_data["gen_type"] == "":
            site_type_req = request_data["site_type"].upper()
            site_type = site_type_req+"_Per"
        else:
            site_type_req = request_data["site_type"].upper()
            gen_type = request_data["gen_type"]
         
            site_type = site_type_req+"_"+gen_type+"_"+"Per"
        if not check_table("dcu_monthly_connectivity_tbl"):
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")

        query = "SELECT {}, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM dcu_monthly_connectivity_tbl WHERE YEAR(block_date) = '{}' AND MONTH(block_date) = '{}' ORDER BY block_date".format(
            site_type, year, month_numeric_value)
        result = execute(2, query)

        if result == () or result == None:
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")

        final_dict = []
        data_list = []
        data_list1 = []

        tot_vals = len(result)
        tuple_idx = 0
        for idx in range(0, num_days):

            day_value = str(idx+1).zfill(2)
            day = f"{year}-{month_numeric_value}-{day_value}"

            if tot_vals > tuple_idx:
                data = result[tuple_idx]
            else:
                local_dict = {
                    "x": day,
                    "y": "0"
                }
                local_dict1 = {
                    "x": day,
                    "y": "0"

                }

                data_list.append(local_dict)
                data_list1.append(local_dict1)
                continue

            if day == str(data[1]):
                tuple_idx += 1

                if data[0] == None or data[0] == 0:
                    data = list(data)
                    data[0] = "0"

                local_dict = {
                    "x": str(data[1]),
                    "y": str(data[0])
                }
                local_dict1 = {
                    "x": str(data[1]),
                    "y": "{:.2f}".format(100-float(data[0]))

                }
                data_list.append(local_dict)
                data_list1.append(local_dict1)

            else:

                local_dict = {
                    "x": day,
                    "y": '0'
                }
                local_dict1 = {
                    "x": day,
                    "y": '0'

                }

                data_list.append(local_dict)
                data_list1.append(local_dict1)

        connected_data = {
            "name": "Connected",
            "data": data_list
        }
        final_dict.append(connected_data)
        disconnected_data = {
            "name": "DisConnected",
            "data": data_list1

        }
        final_dict.append(disconnected_data)

        return Response(json.dumps({"status": "found", "details": final_dict}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def get_subdivision_details():
    fn = 'get_subdivision_details'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        subdivision_input = json.loads(request.data)
        
        if not check_table("loc_mgmt"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        subdivision_result = execute(2, "SELECT DISTINCT category_2 FROM loc_mgmt WHERE category_1 = '{}'".format(subdivision_input['div_name']))
    

        if subdivision_result == None or subdivision_result == (('',),) or subdivision_result == ():
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        
        subdivision_list = [{"div_name": subdivision_input['div_name'], "subdiv_name": subdivision[0]} for subdivision in subdivision_result]

        return Response(json.dumps({"status": "found", "details": subdivision_list}), mimetype = 'application/json', status = 200)
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    

def get_substation_details():
    fn = 'get_substation_details'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        substation_input = json.loads(request.data)
        if not check_table("loc_mgmt"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        
        substation_result = execute(2, "SELECT DISTINCT category_3, loc_name, id FROM loc_mgmt WHERE category_1 = '{}' AND category_2 = '{}'".format(substation_input['div_name'],
                                     substation_input['subdiv_name']))

        if substation_result == None or substation_result == (('',),):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        
        
        if not check_table("dev_tbl"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        
        
        substation_list = []
        for substation in substation_result:
            substation_dict = {}
            dev_query_result = execute(2, "SELECT dev_id, dev_token, dev_name, dev_common_status FROM dev_tbl WHERE dev_type = 'DCU' and loc_id = {}".format(substation[2]))
            substation_dict["loc_id"] = str(substation[2])
            substation_dict["loc_name"] = substation[1]
            substation_dict["div_name"] =  substation_input['div_name']
            substation_dict["subdiv_name"] = substation_input['subdiv_name']
            substation_dict["ss_name"] = substation[0]
            if dev_query_result:
                substation_dict["dev_id"] = dev_query_result[0][0]
                substation_dict["dev_name"] = dev_query_result[0][2]
                substation_dict["dev_token"] = dev_query_result[0][1]
                substation_dict["dev_comm_status"] = dev_query_result[0][3]
                substation_dict["status"] =  "Configured"
                meter_query_result = get_key_value_pairs("SELECT meter_id AS met_id, met_name, ser_port_id AS ser_port_id, comn_status AS met_comm_status, met_sn FROM dcu_dlms_meter_tbl WHERE dev_id = {} AND met_enable= 'Yes' and comn_status != 'Not Configured' ".format(dev_query_result[0][0]))
                
                if meter_query_result:
                    meter_list = []
                    for meter_info in meter_query_result:
                        meter_list.append(meter_info)
                    substation_dict['met_details'] = meter_list
                else:
                    substation_dict['met_details'] = []
            else:
                substation_dict["status"] =  "Not Configured"
            substation_list.append(substation_dict)
        print(substation_list)
        return Response(json.dumps({"status": "found", "details": substation_list}), mimetype = 'application/json', status = 200)
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    

def get_location_details():
    fn = 'get_location_details'
    global db_conn
    try:

        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms = "HS256")

        location_input = json.loads(request.data)
        if not check_table("loc_mgmt"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        if location_input['type'] != 'All': 
            if location_input["div_name"]=='' or location_input["substation_name"] :
                return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)

        if location_input['type'] == 'All':
            # loc_query = "SELECT id, loc_name, location_gps_coordinates, category_1 AS div_name, category_2 AS subdiv_name, category_4, category_5, category_6,\
            # gen_type, site_type FROM loc_mgmt"
            loc_query="SELECT id, loc_name, location_gps_coordinates, category_1 AS div_name, category_2 AS subdiv_name, category_4, category_5, category_6, gen_type, site_type FROM loc_mgmt WHERE NULLIF(loc_name, '') IS NOT NULL AND  NULLIF(category_1, '') IS NOT NULL AND NULLIF(category_2, '') IS NOT NULL order by id asc"
            loc_result = get_key_value_pairs(loc_query)
        elif location_input['type'] == "Division":
            loc_query = "SELECT id, loc_name, location_gps_coordinates, category_1 AS div_name, category_2 AS subdiv_name, category_4, category_5, category_6,\
            gen_type, site_type FROM loc_mgmt WHERE category_1 = '{}' order by id asc".format(location_input['div_name'])
            loc_result = get_key_value_pairs(loc_query)

        elif location_input['type'] == "Sub Division":
            loc_query = "SELECT id, loc_name, location_gps_coordinates, category_1 AS div_name, category_2 AS subdiv_name, category_4, category_5, category_6,\
            gen_type, site_type FROM loc_mgmt WHERE category_1 = '{}' AND category_2 = '{}' order by id asc".format(location_input['div_name'], location_input['subdiv_name'])
            loc_result = get_key_value_pairs(loc_query)

        elif location_input['type'] == "Substation":
            loc_query = "SELECT id, loc_name, location_gps_coordinates, category_1 AS div_name, category_2 AS subdiv_name, category_3 AS ss_name, category_4, category_5, category_6,\
            gen_type, site_type FROM loc_mgmt WHERE category_1 = '{}' AND category_2 = '{}' AND loc_name = '{}' order by id asc".format(location_input['div_name'], location_input['subdiv_name'], location_input['substation_name'])
            loc_result = get_key_value_pairs(loc_query)
    
        if loc_result is None or loc_result == []:
          
            return Response(json.dumps({"status":"No Records found", "details" : []}), mimetype = 'application/json', status = 200)
        
        
        if not check_table("dev_tbl"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        
      
        
        return_loc_result = []
        for loc_info in loc_result:
           
            dev_query = "SELECT dev_id AS dev_id, dev_name AS dev_name, dev_type AS dev_type, dev_model AS dev_model, dev_sn AS dev_sn, imei AS IMEI_number, fw_ver AS fw_ver, dev_phone_num AS dev_phone_num, dev_common_status AS commn_status, conct_up_time AS\
            up_time, last_msg_recv_time AS last_msg_recv_time, dev_token AS dev_token, dev_reg_status AS dev_reg_status, dev_ip_addr AS dev_ip_addr, num_ser_port AS num_ser_port, num_eth_port AS num_eth_port, num_meters AS num_meters, seq_num AS seq_num , lat , lng  FROM dev_tbl WHERE\
            dev_type = 'DCU' and loc_id = {} order by id asc".format(loc_info['id'])
          
            dev_result = get_key_value_pairs(dev_query)
            # print("----------------------dddd--------------",dev_result)
            if dev_result!=None:
                for dev_info in dev_result:

                    if dev_info['commn_status'] == 'Communicating':
                        dev_info['commn_status'] = 'Online'
                        dev_info['icon'] = "static/img/images/greenmarker.png"
                    elif dev_info['commn_status'] == 'Not Communicating':
                        dev_info['commn_status'] = 'Offline'
                        dev_info['icon'] = "static/img/images/redmarker.png"

                    dev_info['id'] = loc_info['id']
                    dev_info['loc_name'] = loc_info['loc_name']
                    dev_info['location_gps_coordinates'] = loc_info['location_gps_coordinates']
                    dev_info['div_name'] = loc_info['div_name']
                    dev_info['subdiv_name'] = loc_info['subdiv_name']
                    dev_info['category_4'] = loc_info['category_4']
                    dev_info['category_5'] = loc_info['category_5']
                    dev_info['category_6'] = loc_info['category_6']
                    dev_info['gen_type'] = loc_info['gen_type']
                    dev_info['site_type'] = loc_info['site_type']
                    return_loc_result.append(dev_info)
                   
            # else:
            #     loc_info["commn_status"] = "Not Configured"
            #     loc_info['icon'] = "static/img/images/graymarker.png"
            #     return_loc_result.append(loc_info)
      
        if return_loc_result==[]:
            return Response(json.dumps({"status":"not found", "details" : return_loc_result}), mimetype = 'application/json', status = 200)
        # print(return_loc_result)
        return Response(json.dumps({"status":"found", "details" : return_loc_result}), mimetype = 'application/json', status = 200)
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    

def get_location_device_details():
    fn = 'get_location_device_details'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms = "HS256")
        
        return_device_list = []
        get_device_input = json.loads(request.data)
        print("===========================>>>>>>>>>>>>>",get_device_input)
        if not check_table("dev_tbl"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)
        if not check_table("dev_tbl"):
            return Response(json.dumps({"status": "not found", "details": []}), mimetype = 'application/json', status = 200)

        dev_query_result = get_key_value_pairs("SELECT dev_id, dev_token, dev_name, dev_common_status FROM dev_tbl WHERE loc_id = {}".format(get_device_input['loc_id']))
        
        print("==========",dev_query_result)
        if dev_query_result:
            for dev_info in dev_query_result:
                meter_query_result = get_key_value_pairs("SELECT meter_id AS met_id, met_name, ser_port_id AS ser_port_id, comn_status AS met_comm_status, met_sn FROM dcu_dlms_meter_tbl WHERE dev_id = {} AND met_enable= 'Yes'AND comn_status !='Not Configured' and ser_port_id != '' and met_sn != ''".format(dev_info['dev_id']))
                plcc = execute(2, "SELECT dev_model FROM dcu_serialnum_details WHERE serial_num = '{}'".format(dev_info['dev_id']))
                print(meter_query_result)
                
                if meter_query_result:
                    meter_list = []
                    for meter_info in meter_query_result:
                        meter_list.append(meter_info)
                    dev_info['met_details'] = meter_list
                    dev_info['dev_model'] = plcc[0][0]                   
                else:
                    dev_info['met_details'] = []
                    dev_info['dev_model'] = plcc[0][0]
                return_device_list.append(dev_info)
            
        else:
            return Response(json.dumps({"status":"Not found", "details" : []}), mimetype = 'application/json', status = 200)
        print("testing",return_device_list)
        return Response(json.dumps({"status":"found", "details" : return_device_list}), mimetype = 'application/json', status = 200)
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
def get_meter_name(result)   :
    for idx in result:
        met_name=get_key_value_pairs("select met_name from dcu_dlms_meter_tbl where unique_id='{}'".format(idx["unique_id"]))
        if met_name==None:
            meter_name="-"
        else:
            meter_name=met_name[0]["met_name"]
        idx["meter_name"]=meter_name
    return result
    

# def Alerts():
#     fn = "alerts"
#     try:

#         header = request.headers.get('Authorization')
#         jwt.decode(header, sec_code, algorithms="HS256")
        
#         input=json.loads(request.data)
       
#         s_day,s_month_name, s_year = input["start_date"].split()
#         s_month = Month[s_month_name]
#         start_date = f"{s_year}-{s_month}-{s_day}"
       
#         e_day,e_month_name, e_year = input["end_date"].split()
#         e_month = Month[e_month_name]
#         end_date = f"{e_year}-{e_month}-{e_day}"
      
#         table_name="dcu_event_{}_tbl".format(input["dev_id"])
#         if not check_table(table_name):
#           final_result=[]
    
#         if input["meter_name"] =="All":
#             if  input["type"] =="All":
#                 if not check_table(table_name):
#                     final_result=None
#                 else:
#                     query="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' order by event_time asc ".format(input["dcu_name"],table_name,start_date,end_date)
#                     final_result=get_key_value_pairs(query)
                
          
#                 if final_result!=None:
#                     final_result=get_meter_name(final_result)
#                 result=dcu_alert_all(start_date,end_date,input,final_result)
                
#             else:
#                 if input["type"] !="Meter Events":
#                     if not check_table(table_name):
#                         final_result=None
#                     else:
#                         query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' order by event_time asc ".format(input["dcu_name"],table_name,start_date,end_date,input["type"])
#                         final_result=get_key_value_pairs(query1)
#                     if final_result!=None:
#                         result=get_meter_name(final_result)
#                     result=final_result
                   
#                 else:
#                     if not check_table(table_name):
#                         final_result=None
#                     else:
#                         if input["event_code"]=="All":
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' order by event_time asc".format(input["dcu_name"],table_name,start_date,end_date,input["type"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                    
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
#                         else:
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' and event_desc ='{}' order by event_time asc".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["event_code"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                    
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
                        
                    
        
#         if input["meter_name"] !="All":
#             if  input["type"] =="All":
#                 if not check_table(table_name):
#                     final_result=None
#                 else:
#                     query1="select event_time, event_type as type,event_desc as events ,'{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and unique_id='{}' order by event_time asc".format(input["dcu_name"],table_name,start_date,end_date,input["unique_id"])
#                     final_result=get_key_value_pairs(query1)
                  
#                 if final_result!=None:
#                         final_result=get_meter_name(final_result)
#                 result=dcu_alert_all(start_date,end_date,input,final_result)
             
                
#             else:
#                 if input["type"] !="Meter Events":
#                     if not check_table(table_name):
#                         final_result=None
#                     else:
#                         query1="select event_time, event_type as type,event_desc as events ,'{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' and unique_id='{}' order by event_time asc".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["unique_id"])
#                         final_result=get_key_value_pairs(query1)
#                     if final_result!=None:
#                         result=get_meter_name(final_result)
#                     result=final_result
                   
#                 else:
#                     if not check_table(table_name):
#                         final_result=None
#                     else:
#                         if input["event_code"]=="All":
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}'  and unique_id='{}'order by event_time asc ".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["unique_id"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                    
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
#                         else:
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}'  and unique_id='{}' and event_desc='{}' order by event_time asc ".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["unique_id"],input["event_code"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                    
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
                          
   
#         print(result)
#         result = sort_by_meter_name(result)
#         if result==[] or result==None:
#             return Response(json.dumps(  {"status": "not found", "alerts_count": 0, "details": result}),content_type="application/json")
     
#         else:
          
#             return Response(json.dumps(  {"status": "found", "alerts_count": len(result), "details": result}),content_type="application/json")

#     except jwt.ExpiredSignatureError as e:
#         return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
#     except Exception as e:
#         print("---------------------------------")
#         exc_type, exc_obj, exc_tb = sys.exc_info()
#         logerr(" %s Error on line number : %s : %s" %
#                (fn, exc_tb.tb_lineno, e))
#         print("---------------------------------")
#         return Response(ser_err, mimetype="application/json")
    
def Alerts():
 
    fn="Alerts"
    try:
        print("Alerts")
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        input=json.loads(request.data)
        if "/get/alert/details" in request.url:
            offset = (1-1) * 20
        else:
            offset = (input["page_size"]-1) * 20
      
        limit = 20
        print(input)
        event_list = []
        if input["event_code"]!="All":
            if input["event_code"]=="VOLT":
                event_list= [1,2,3,4,5,6,7,8,9,10,11,12]
            elif input["event_code"]=="POWER":
                event_list= [101,102]
            elif input["event_code"]=="CURRENT":
                event_list= [51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68]
            elif input["event_code"]=="TRANSACTION":
                event_list= [1,2,3,4,5,6,7,8,9,10,11,12]
            elif input["event_code"]=="OTHERS":
                event_list= [201, 202, 203, 204, 205, 206]
            elif input["event_code"]=="NON_ROLL_OVER":
                event_list= [251]
            elif input["event_code"]=="TAMPER":
                event_list= [251]
 
        s_day,s_month_name, s_year = input["start_date"].split()
        s_month = Month[s_month_name]
        start_date = f"{s_year}-{s_month}-{s_day}"
       
        e_day,e_month_name, e_year = input["end_date"].split()
        e_month = Month[e_month_name]
        end_date = f"{e_year}-{e_month}-{e_day}"
        if input["meter_name"] !="All" and "$$" in  input["meter_name"]:
            port,meter,meter_name=input["meter_name"].split("$$")
            input["meter_name"]=meter_name
        else:
            meter_name= input["meter_name"]
        result=[]
        for key, value in input.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
           
            if input["type"] =="Meter Events":
                if input["meter_name"] !="All":
                    unique_id = execute(1, "SELECT unique_id FROM dcu_dlms_meter_tbl WHERE met_name = '{}' and dev_id = '{}'".format(meter_name,input["dev_id"]))
                    table_name = f"dcu_event_data_{unique_id[0]}"
                    print(table_name)
                    if check_table(table_name)==True:
                        if input["event_code"]=="All":
                            query1="select 0_0_1_0_0_255 as event_time, '{}' as type,EVENTS as events ,'{}' as dcu_name  from {} where DATE(0_0_1_0_0_255) BETWEEN '{}' and '{}' ORDER BY 0_0_1_0_0_255 desc".format(input["type"],input["dcu_name"],table_name,start_date,end_date)                            
                            print(query1)
                            result=get_key_value_pairs(query1)    
                        else:
                            event_list_str = ", ".join(map(str, event_list))
                            print(event_list)
                            query = "SELECT 0_0_1_0_0_255 AS event_time, '{}' AS type, EVENTS AS events, '{}' AS dcu_name FROM {} WHERE 0_0_96_11_0_255 IN ({}) AND DATE(0_0_1_0_0_255) BETWEEN '{}' AND '{}' ORDER BY 0_0_1_0_0_255 DESC".format(
                            input["type"],
                            input["dcu_name"],
                            table_name,
                            event_list_str,
                            start_date,
                            end_date
                        )
                            print(query)
                            result = get_key_value_pairs(query)
                    else:
                        print("Table not found")        
                else:
                    unique_id = execute(2, "SELECT unique_id,met_name FROM dcu_dlms_meter_tbl WHERE dev_id = '{}'".format(input["dev_id"]))
                    for key in unique_id:
                        table_name = f"dcu_event_data_{key[0]}"
                        met_name = key[1]
                   
                        if check_table(table_name)==True:
                            if input["event_code"]=="All":
                                query1="select '{}' as meter_name,0_0_1_0_0_255 as event_time, '{}' as type,EVENTS as events ,'{}' as dcu_name from {} where DATE(0_0_1_0_0_255) BETWEEN '{}' and '{}' ORDER BY 0_0_1_0_0_255 DESC".format(met_name,input["type"],input["dcu_name"],table_name,start_date,end_date)
                                final_result=get_key_value_pairs(query1)
                                print(query1)
                            else:
                                query = "SELECT '{}' AS meter_name, 0_0_1_0_0_255 AS event_time, '{}' AS type, EVENTS AS events, '{}' AS dcu_name FROM {} WHERE 0_0_96_11_0_255 IN ({}) AND DATE(0_0_1_0_0_255) BETWEEN '{}' AND '{}' ORDER BY 0_0_1_0_0_255 DESC".format(
                                    met_name,
                                    input["type"],  
                                    input["dcu_name"],
                                    table_name,
                                    ", ".join(map(str, event_list)),  
                                    start_date,
                                    end_date
                                )
                                print(query)
                                
                                final_result = get_key_value_pairs(query)
                            if final_result:  # More Pythonic than `!= []`
                                result.extend(final_result)
                           
            else:
                print("====================>>>>>>>>>>>>>>>",input["type"])
                if input["type"]!="Modem Events":
 
                    DCU,event_type=input["type"].split(" - ")
                    event_type=event_type
                else:
                    event_type=input["type"]
                    
                print("=============event_type=======>>>>>>>>>>>>>>>",event_type)
                table_name="dcu_event_{}_tbl".format(input["dev_id"])
                if check_table(table_name)==True:
                   
                    query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name  from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' ".format(input["dcu_name"],table_name,start_date,end_date,event_type)
                    result=get_key_value_pairs(query1)
                    print(query1)
                else:
                    print("Table not found")
            if result != None:
                total_count = len(result)       
                print(total_count)
                result = result[offset:offset + limit]
                if total_count != 0:
                        offset = offset+1
                        for i, item in enumerate(result, start=offset):
                            item['row_index'] = i
                print(result)
            else:
                total_count = 0
            if result!=None and result!=[]:
                return Response(json.dumps({"status": "found", "alerts_count": total_count, "details": result,"total_count":total_count}),content_type="application/json")
            else:
                return Response(json.dumps(  {"status": "not found", "alerts_count": 0, "details": result,"total_count":total_count}),content_type="application/json")
        else:
            return Response(json.dumps(  {"status": "not found", "alerts_count": 0, "details": []}),content_type="application/json")    
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
                (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
def total_alert_count():
    fn = "alerts"
    try:   
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        if not check_table("Alerts"):
              return Response(json.dumps({"status": "found" ,"count":"0"}))
        result=get_key_value_pairs("select * from Alerts order by event_time desc")
        if result==None:
            return Response(json.dumps({"status": "found" ,"count":"0"}))
        else:
            return Response(json.dumps({"status": "found" ,"count":len(result)}))
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)   
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")


def alert_initial_data():
    fn="alert_initial_data"
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
         
        if not check_table("Alerts"):
             return Response(json.dumps({"status": "not found",  "details": []}))
        result = get_key_value_pairs("select  event_time, unique_id , event_type as type , event_desc as events from Alerts order by event_time asc limit 50")
        if result==None:
            return Response(json.dumps({"status": "not found",  "details": []}))
  
        
        for idx in result:
            met_and_dev_name=get_key_value_pairs("select met_name,dev_name from dcu_dlms_meter_tbl where unique_id='{}'".format(idx["unique_id"]))
            if met_and_dev_name==None:
                meter_name="-"
                dev_name="-"
            else:
                meter_name=met_and_dev_name[0]["met_name"]
                dev_name=met_and_dev_name[0]["dev_name"]
            idx["meter_name"]=meter_name
            idx["dcu_name"]=dev_name
        else:
            return Response(json.dumps({"status": "found",  "details":result})) 
        
        
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")  
def dcu_and_meter_list():
    fn="dcu_and_meter_list"
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        if not check_table("dev_tbl"):
              return Response(json.dumps({"status": "not found",  "details": []}))
        
        dev_result=get_key_value_pairs("select dev_name,dev_id from dev_tbl")  
        if dev_result==None:
            return Response(json.dumps({"status": "not found",  "details": []}))
        final_list=[]
        for i in range (0,int(len(dev_result))):
            result=get_key_value_pairs("select met_name,unique_id from dcu_dlms_meter_tbl where dev_name='{}' AND met_name != '' AND met_name IS NOT NULL AND met_enable='Yes' AND ser_port_id != '' and met_sn != ''".format(dev_result[i]["dev_name"]))  
            if result!=None:
                
                final_dict={ "dev_name":dev_result[i]["dev_name"],
                            "dev_id":dev_result[i]["dev_id"],
                            "meter_details":result
                    
                }
            else:
                final_dict={ "dev_name":dev_result[i]["dev_name"],
                            "dev_id":dev_result[i]["dev_id"],
                            "meter_details":result
                    
                } 
          
            final_list.append(final_dict)
    
        return Response(json.dumps({"status": "found",  "details": final_list}))
        
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)      
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
   
#dcu view 

def dcu_summary_info():
    fn = 'dev_summary_info'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        request_data = json.loads(request.data)
        
        schema_result=check_schema(request_data,DCU_summmary_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
     
        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
            
        if not check_table("dev_tbl"):
             return Response(json.dumps({"status": "not found"}), mimetype='application/json', status=200)
        
        query = "select dev_name,dev_ip_addr,loc_name,dev_model,dev_type,num_meters,dev_common_status,last_msg_recv_time as last_date_time,conct_up_time ,dev_reg_status,dev_sn  from dev_tbl where dev_id='{}'".format(
            request_data["dev_id"])
       
        result = get_key_value_pairs(query)
        
        if result==None:
            return Response(json.dumps({"status": "not found"}), mimetype='application/json', status=200)
        if result[0]["dev_reg_status"]=="Not registered" or result[0]["dev_reg_status"]==None:
                result[0]["dev_common_status"]="Not registered"

        if result != [] or result != None:
            result[0]["conct_up_time"] = convert_to_days_hms(str(result[0]["conct_up_time"]))
        
        if result[0]["dev_common_status"]=="Not Communicating":
            result[0]["conct_up_time"]="-"

        if result[0]["last_date_time"] == None:
            result[0]["last_date_time"] = '00:00:00'
      
        query4 = "select modem_link_sts ,router_id from modem_link_tbl where dcu_id='{}'".format(
            request_data["dev_id"])
      
        modem_link_sts = execute(2, query4)
        if modem_link_sts==() or modem_link_sts==None:
            modem_status= "NA"
            router_id="NA"
        else:
            modem_status=modem_link_sts [0][0]
            router_id=modem_link_sts[0][1]
       
        if modem_status== "Internal":
            table_name = "dcu_modem_dig_info_" + request_data["dev_id"]

        elif modem_status== "External":
            table_name = "router_dig_info_" + router_id
        else:
            table_name="no_tbl"
            nw_type="VSAT"
            
        if not check_table(table_name) :
                signal_strength="-"
                if modem_status=="VSAT":
                    nw_type="VSAT"
                else: 
                    nw_type="-"
        else:
            query1 = "select sig_str as signal_strength,nw_type as connectivity from `{}`  ORDER BY id DESC LIMIT 1".format(
            table_name)
        
            result1 = get_key_value_pairs(query1)

            if result1 == None or result1==[] or result[0]["dev_common_status"]=="Not Communicating":
                signal_strength = "-"
                nw_type = "-"
            else:
                signal_strength= result1[0]["signal_strength"]
                nw_type =result1[0]["connectivity"]
                
        query2 = "select * from `dcu_dlms_meter_tbl` where comn_status = 'Communicating' and dev_id='{}' and met_enable = 'Yes' AND ser_port_id != '' and met_sn != ''".format(
        request_data["dev_id"])
      
        online_meters = execute(2, query2)
       

        if online_meters == None or result[0]["dev_common_status"]=="Not Communicating":
            len_online_meters= 0
        else:
            len_online_meters=len(online_meters)
        print(len_online_meters)
        #
        query_conf = "select * from `dcu_dlms_meter_tbl` where met_enable = 'Yes' and dev_id='{}' and comn_status !='Not Configured' AND ser_port_id != '' and met_sn != ''".format(
        request_data["dev_id"]) 
        
        configured_meters = execute(2, query_conf)

        if configured_meters==None:
            configured_meters=0
        else:
             configured_meters=len(configured_meters)
        print(configured_meters)
        if online_meters == None:
            online_meters = 0

        plcc = execute(2, "SELECT dev_model FROM dcu_serialnum_details WHERE serial_num = '{}'".format(request_data["dev_id"]))
        #norows= len(plcc)
        print(plcc[0][0])

        dev_summary = result[0]
        dev_summary["modem_type"] = modem_status
        dev_summary["signal_strength"] = signal_strength
        dev_summary["connectivity"] = nw_type
        dev_summary["online_meters"] = len_online_meters
        dev_summary["configured"] = configured_meters
        dev_summary["dev_model"] = plcc[0][0]
     
        if 'project_depend_view' in request_data:
            if request_data['project_depend_view'] == 2:
                dev_summary["category"] = "NA"
        else:
            dev_query = "select site_type from loc_mgmt where id ={}".format(request_data["loc_id"])
           
            dev_type = execute(1, dev_query)
       
            if dev_type == "IPPS":
                dev_summary["category"] = dev_type[0] + \
                                        "(" + request_data["ipps_type"] + ")"
            else:
                dev_summary["category"] = dev_type[0]

        dcu_name = dcufullname(request_data["dev_id"])
        print("test",dev_summary)
        return Response(json.dumps({"status": "found", "dcu_fullname": dcu_name, "dcu_name": result[0]["dev_name"], "dev_id": request_data["dev_id"], "details": dev_summary}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
    
def dcu_con_monthly_graph():
    fn = 'dcu_con_monthly_graph'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        request_data = json.loads(request.data)
        schema_result=check_schema(request_data,dcu_con_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
   
        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
        start_month = request_data["start_month"]
        month_name, year = start_month.split()
        month_numeric_value = Month[month_name]
        date_format = f"{year}-{month_numeric_value}"
        num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]

        now = datetime.now()

        month_numeric_value = Month[month_name]
        query = "select conct_per,DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date from dcu_daily_connectivity_tbl where dev_id={} and DATE_FORMAT(block_date, '%Y-%m')='{}' order by block_date".format(
            request_data["dev_id"], date_format)
        result = execute(2, query)
        
        if result == () or result == None:
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")

        final_list = []
        data_list = []
        data_list1 = []

        tot_vals = len(result)
        tuple_idx = 0
        for idx in range(0, num_days):

            day_value = str(idx+1).zfill(2)
            day = f"{year}-{month_numeric_value}-{day_value}"

            if tot_vals > tuple_idx:
                data = result[tuple_idx]
            else:
                local_dict = {
                    "x": day,
                    "y": 0
                }
                local_dict1 = {
                    "x": day,
                    "y": 0
                }

                data_list.append(local_dict)
                data_list1.append(local_dict1)

                continue

            if day == str(data[1]):
                tuple_idx += 1

                if data[0] == None or data[0] == "0":
                    data = list(data)
                    data[0] = 0

                local_dict = {
                    "x": data[1],
                    "y": data[0]
                }
                local_dict1 = {
                    "x": data[1],
                    "y": "{:.2f}".format(100-float(data[0]))
                }

                data_list.append(local_dict)
                data_list1.append(local_dict1)

            else:

                local_dict = {
                    "x": day,
                    "y": 0
                }
                local_dict1 = {
                    "x": day,
                    "y": 0
                }

                data_list.append(local_dict)
                data_list1.append(local_dict1)

        dcu_con_ondata = {
            "name": "Connected",
            "data": data_list
        }
        dcu_con_offdata = {
            "name": "Disconnected",
            "data": data_list1
        }

        final_list.append(dcu_con_ondata)
        final_list.append(dcu_con_offdata)

        return Response(json.dumps({"status": "found", "details": final_list}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
    
def dcu_dataavail_monthly_graph():
    fn = 'dcu_dataavail_monthly_graph'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        request_data = json.loads(request.data)
        schema_result=check_schema(request_data,dcu_avail_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
     
        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
     
        start_month = request_data["start_month"]

        start_month = request_data["start_month"]
        month_name, year = start_month.split()

        month_numeric_value = Month[month_name]
        date_format = f"{year}-{month_numeric_value}"
        num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]

        query = "SELECT data_avail_per, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM dcu_daily_data_avail_info  where dev_id= {} and DATE_FORMAT(block_date, '%Y-%m')='{}' order by block_date".format(
            request_data["dev_id"], date_format)
        result = execute(2, query)


        if result == () or result == None:
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")

        final_list = []
        data_list = []

        tot_vals = len(result)
        tuple_idx = 0
        for idx in range(0, num_days):

            day_value = str(idx+1).zfill(2)
            day = f"{year}-{month_numeric_value}-{day_value}"

            if tot_vals > tuple_idx:
                data = result[tuple_idx]
            else:
                local_dict = {
                    "x": day,
                    "y": 0
                }

                data_list.append(local_dict)

                continue

            if day == str(data[1]):
                tuple_idx += 1

                if data[0] == None or data[0] == "0":
                    data = list(data)
                    data[0] = 0

                local_dict = {
                    "x": data[1],
                    "y": float(data[0])
                }

                data_list.append(local_dict)

            else:

                local_dict = {
                    "x": day,
                    "y": 0
                }

                data_list.append(local_dict)

        avail_data = {
            "name": "Data Availability",
            "data": data_list
        }
        final_list.append(avail_data)

        return Response(json.dumps({"status": "found", "details": final_list}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
    
def dcu_signal_strength_graph():
    fn = 'dcu_signal_strength_graph'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
      
        request_data = json.loads(request.data)
    
        schema_result=check_schema(request_data,dcu_signal_strength_schema)

        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        
        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
            
        if request_data["signal_type"] == "Month":
        
            start_month = request_data["start_month"]
            month_name, year = start_month.split()

            month_numeric_value = Month[month_name]
            date_format = f"{year}-{month_numeric_value}"
            num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]

            query4 = "select modem_link_sts,router_id from modem_link_tbl where dcu_id={}".format(
                request_data["dev_id"])
        
            modem_status = execute(2, query4)
            if modem_status==() or modem_status [0][0]=="VSAT" :
                return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")
            else:
                modem_sts=modem_status [0][0]
                router_id=modem_status[0][1]
         
            if modem_sts== "Internal":
                query = "SELECT ss, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM dcu_daily_signal_strength_tbl  where dev_id= {} and DATE_FORMAT(block_date, '%Y-%m')='{}' order by block_date".format(
                    request_data["dev_id"], date_format)

            if modem_sts== "External":
                query = "SELECT ss, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM router_daily_signal_strength_tbl  where dev_id= '{}' and DATE_FORMAT(block_date, '%Y-%m')='{}'  order by block_date".format(
                   router_id, date_format)
     
            result1 = execute(2, query)
       
            if result1 == () or result1 == None:
                return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")
     
            seen_dates = {}
            result = []

            for value, date in result1:
                if date not in seen_dates:
                    seen_dates[date] = True
                    result.append((value, date))
            

            final_list = []
            data_list = []

            tot_vals = len(result)
            tuple_idx = 0
            for idx in range(0, num_days):

                day_value = str(idx+1).zfill(2)
                day = f"{year}-{month_numeric_value}-{day_value}"

                if tot_vals > tuple_idx:
                    data = result[tuple_idx]
                else:
                    local_dict = {
                    "x": day,
                    "y": None
                }

                    data_list.append(local_dict)
                    continue

                if day == str(data[1]):
                    tuple_idx += 1

                    if data[0] == None or data[0] == "0":
                        data = list(data)
                        data[0] = 0
                    local_dict = {
                        "x": data[1],
                        "y": float(data[0])
                    }
                    data_list.append(local_dict)
                else:
                    local_dict = {
                    "x": day,
                    "y": None
                }
                    data_list.append(local_dict)
            avail_data = {
            "name": "Signal Strength",
            "data": data_list
                }
            final_list.append(avail_data)
            return Response(json.dumps({"status": "found", "details": final_list}), content_type="application/json")
        
        if request_data["signal_type"]=="Daily":            
            
            date, month_name,  year = request_data["start_date"].split()
            month = Month[month_name]
            st_date = f"{year}-{month}-{date}"
 
            query_status="select modem_link_sts,router_id from modem_link_tbl where dcu_id='{}'".format(request_data["dev_id"])
            link_status=execute(1,query_status)
         
            if link_status==None:
                return Response(json.dumps({"status": "not found"}), mimetype='application/json', status=200)
            else:
                if link_status[0]=='Internal':
                  table_name= "dcu_daily_diag_info_"+request_data["dev_id"]
               
                elif link_status[0]=='External':
                    
                    table_name= "router_daily_diag_info_"+link_status[1]

                elif link_status[0]=='VSAT':
                    return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")
           
                if check_table(table_name)==False:
                    return Response(json.dumps({"status": "table not found", "details": []}), content_type="application/json")

                query="select sig_str as y , update_time as x  from `{}` where DATE_FORMAT(update_time, '%Y-%m-%d')='{}' order by update_time asc".format(table_name,st_date)
              
                result=get_key_value_pairs(query)

                if result == () or result == None:
                    return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")
              
                y_values = [float(item['y']) for item in result]

                max_y_value = max(y_values)
                result=graph_interval(result,st_date,288)

                final_list=[]
                daily_data = {
                    "name": "Signal Strength",
                    "data": result
                    }
                final_list.append(daily_data)
                print("max_y_value",max_y_value)
                return Response(json.dumps({"status": "found", "details":final_list,"max_axis": max_y_value ,"date":st_date}), content_type="application/json")
 
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

  # on demand
def meter_Comm_status():
    fn = 'meter_Comm_status'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        dev_id = request.args.get('dev_id')

        query = "select dev_id, dev_name, num_meters, num_ser_port,dev_common_status from dev_tbl where dev_id= {}".format(
            dev_id)
        result_dev = get_key_value_pairs(query)
        
  
        print("--------------",result_dev)
        if result_dev == () or result_dev == None or result_dev == []:
            return Response(json.dumps({"status": "not found", "details": []}), content_type="application/json")
        
        if result_dev[0]["num_ser_port"]=='' or result_dev[0]["num_ser_port"]==None :
            num_port=0
        else:
            num_port = int(result_dev[0]["num_ser_port"])+1
      
        final_dict = {}
        for x in range(1, num_port):
            query_port = "select * from dcu_dlms_meter_tbl where ser_port_id= {} and dev_id= {}".format(x, dev_id)
            logerr("--------->>>>-----{}".format(query_port))
            result = get_key_value_pairs(query_port)
            if result_dev[0]["dev_common_status"]=="Not Communicating":
               if result[0]["comn_status"]=="Communicating":
                   result[0]["comn_status"]="Not Communicating"
            final_dict["port" + str(x)] = result
        summary = {
            "status": "found",
            "dev_id": dev_id,
            "dev_name": result_dev[0]["dev_name"],
            "num_meters": result_dev[0]["num_meters"],
            "num_serial": result_dev[0]["num_ser_port"],
            "met_details": final_dict

        }
   

        return Response(json.dumps(summary), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
    
def dev_connections():
    fn='dev_connections'
    global ser_err, sec_code, db_conn, refresh_code
    try:
        
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data) 
     
        s_day,s_month_name, s_year = request_data["start_date"].split()
        s_month = Month[s_month_name]
        start_date = f"{s_year}-{s_month}-{s_day}"
        e_day,e_month_name, e_year = request_data["end_date"].split()
        e_month = Month[e_month_name]
        end_date = f"{e_year}-{e_month}-{e_day}"
        
        if request_data["key"]=="DCU_Wise":
           
            table_name="dcu_dev_connectivity_"+request_data["dev_id"]
            meter_name="metname"
            table_exist=check_table(table_name)
            print("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++")
            if table_exist:
                query="SELECT id,update_time ,duration ,comn_sts, '{}' as met_name FROM {} WHERE DATE(update_time) BETWEEN '{}' AND '{}' order by update_time".format(meter_name,table_name,start_date,end_date)
                
                result=get_key_value_pairs(query)
                print(result)
                if result==None:
                       
                       return Response(json.dumps({"status": "not found", "details":[]}), content_type="application/json")
                
                for idx in result:
                    # print("--------------------------->>>>>>>>>>>>>>",idx["comn_sts"])
                    if idx["duration"]!=None:
                        
                        # print("--------------------------->>>>>>>>>>>>>>",idx["comn_sts"],idx["duration"])
                        hours, minutes, seconds = map(int, idx["duration"].split(':'))
                        time_to_add = timedelta(hours=hours, minutes=minutes, seconds=seconds)
                        endtime=datetime.strptime(idx["update_time"], "%Y-%m-%d %H:%M:%S")+time_to_add
                        idx["end_time"]=endtime.strftime("%Y-%m-%d %H:%M:%S")
                        idx["start_time"]=idx["update_time"]
                        if idx["comn_sts"]=="Communicating":
                            idx["comn_sts"]="Online"
                        elif idx["comn_sts"]=="Not Communicating":
                           idx["comn_sts"]="Offline"
                    else:
                        
                        # print("--------------------------->>>>>>>>>>>>>>",idx["comn_sts"])
                        idx["start_time"]=idx["update_time"]
                        idx["end_time"]=str(date.today())+" 23:59:59"
                        
                        timestamp1 = datetime.strptime(str(idx["end_time"]), "%Y-%m-%d %H:%M:%S")
                        timestamp2 = datetime.strptime(str(idx["start_time"]), "%Y-%m-%d %H:%M:%S")
                        difference = timestamp1 - timestamp2
                        idx["duration"]=str(difference)
                        if idx["comn_sts"]=="Communicating":
                            idx["comn_sts"]="Online"
                        elif idx["comn_sts"]=="Not Communicating":
                           idx["comn_sts"]="Offline"
                # print(result)
                if result!=None:
                    return Response(json.dumps({"status": "found", "details":result}), content_type="application/json")
            else:
                return Response(json.dumps({"status": "not found", "details":[]}), content_type="application/json")
                    
        elif request_data["key"]=="Meter_Wise":
           
            if request_data["meter_name"]=="All":
                final_list=[]
                for i in range (0, len(request_data["meter_info"])):
                    port_id=request_data["meter_info"][i]["ser_port_id"]
                    met_id=request_data["meter_info"][i]["met_id"]
                    unique_id="{}_{}_{}".format(request_data["dev_id"],port_id,met_id)
                    table_name="met_connectivity_{}".format(unique_id)
                    if check_table(table_name):
                        query="SELECT id ,start_time ,end_time ,duration ,comn_sts, '{}' as met_name FROM {} WHERE DATE(start_time) BETWEEN '{}' AND '{}' order by start_time".format(request_data["meter_info"][i]["met_name"],table_name,start_date,end_date)
                        result=get_key_value_pairs(query)
                        print(result)
                        if result!=None:
                            for idx in result:
                                if idx["end_time"]==None:
                                    print()
                                    # idx["end_time"]=str(date.today())+" 23:59:59"
                                    idx["end_time"]=datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                                    timestamp1 = datetime.strptime(str(idx["end_time"]), "%Y-%m-%d %H:%M:%S")
                                    timestamp2 = datetime.strptime(str(idx["start_time"]), "%Y-%m-%d %H:%M:%S")
                                    difference = timestamp1 - timestamp2
                                    idx["duration"]=str(difference)
                                    if idx["comn_sts"]=="Communicating":
                                        idx["comn_sts"]="Online"
                                    elif idx["comn_sts"]=="Not Communicating":
                                        idx["comn_sts"]="Offline"
                                else:
                                    if idx["comn_sts"]=="Communicating":
                                        idx["comn_sts"]="Online"
                                    elif idx["comn_sts"]=="Not Communicating":
                                        idx["comn_sts"]="Offline"
                            final_list.extend(result)
                    else:
                        return Response(json.dumps({"status": "not found", "details":[]}), content_type="application/json")
               
                if final_list==[] or final_list==None:
                    return Response(json.dumps({"status": "not found", "details":final_list}), content_type="application/json")
                else:
                    return Response(json.dumps({"status": "found", "details":final_list}), content_type="application/json")
            else:    
                port_id,met_id,meter_name=request_data["meter_name"].split("$$")
                unique_id="{}_{}_{}".format(request_data["dev_id"],port_id,met_id)
                table_name="met_connectivity_{}".format(unique_id)
                if check_table(table_name):
                    query="SELECT id ,start_time ,end_time ,duration ,comn_sts, '{}' as met_name FROM {} WHERE DATE(start_time) BETWEEN '{}' AND '{}' order by start_time".format(meter_name,table_name,start_date,end_date)
                    result=get_key_value_pairs(query)
                    print(result)
                    print(query)
                    if result!=None:
                        for idx in result:
                            if idx["end_time"]==None:
                                idx["end_time"]=datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                                timestamp1 = datetime.strptime(str(idx["end_time"]), "%Y-%m-%d %H:%M:%S")
                                timestamp2 = datetime.strptime(str(idx["start_time"]), "%Y-%m-%d %H:%M:%S")
                                difference = timestamp1 - timestamp2
                                idx["duration"]=str(difference)
             
                    if result==[] or result==None:
                        return Response(json.dumps({"status": "not found", "details":result}), content_type="application/json")
                    else:
                        return Response(json.dumps({"status": "found", "details":result}), content_type="application/json")
                    
        else:
            return Response(json.dumps({"status": "not found", "details":[]}), content_type="application/json")       
     
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
      
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')  

# User_Management
def get_users():
    fn = "get_users()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        post_dict = json.loads(request.data)
        if post_dict["active_role"]=="Super Admin":
            query = "SELECT * FROM user_mgmt"
        elif post_dict["active_role"]=="Operator":
            query = "SELECT * FROM user_mgmt where role !='Super Admin' and role !='Admin'"
        else:
            query = "SELECT * FROM user_mgmt where role !='Super Admin'"
     
        results=get_key_value_pairs(query)
        if results==None:
            return Response(json.dumps({"status": "not found","details":[]}), mimetype='application/json', status=200)
        final_dict=[]
        for row in results:
            row_dict={}
            for key,value in row.items():
                if key=="password":
                    decrypted_password = cipher_suite.decrypt(value.encode('utf-8'))
                 
                    row_dict[key] =decrypted_password.decode('utf-8')
                else:
                    row_dict[key]=value
            final_dict.append(row_dict)
     
        return Response(json.dumps({"status":"found","user_details":final_dict}), mimetype='application/json')
    
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def add_user():
    fn = "add_user()"
    global ser_err1, sec_code, db_conn
    try:
        fn = "add_user()"
        global ser_err, sec_code, db_conn,chance_SuperAdmin
    
        start=datetime.now()
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms= "HS256")
        input = json.loads(request.data)
        
    
        # schema_result=check_schema(input, add_user1)
        # if schema_result!=True:
        #     return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        
        for key, value in input.items():
            if not value:
                return Response(json.dumps({"status": "Empty value found for key ({})".format(key)}), mimetype="application/json")
        
        results= get_key_value_pairs("SELECT * FROM user_mgmt where user_name='{}'".format(input["user_name"]))
        if results!=None:
            return Response(json.dumps({"status": "User Already exist!"}), mimetype='application/json')
        
        

        if input["role"]=="Admin":
            chance=chance_admin
        if input["role"]=="Super Admin":  
            chance=chance_superadmin
         
        if input["role"]=="Operator":  
            chance=chance_operator
            
        mobile_number_pattern = r'^\(?(\d{3})\)?[-.\s]?(\d{3})[-.\s]?(\d{4})$'
        mail_pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'

        if not re.search(mobile_number_pattern, input["ph_no"]):
            return Response(json.dumps({"status": "Invalid Mobile number!"}), mimetype='application/json')

        if not re.search(mail_pattern, input["email"]):
            return Response(json.dumps({"status": "Invalid Mail address!"}), mimetype='application/json')

        encrypted_password = cipher_suite.encrypt(input["password"].encode('utf-8'))

        update_tbl("INSERT INTO user_mgmt (`role`, user_full_name,  user_name, `password`, mail_id, ph_no,max_num_users,num_users) VALUES{}".format(
        (input["role"], input["user_full_name"], input["user_name"], encrypted_password.decode('utf-8'), input["email"], input["ph_no"],chance,0)))
        
        update_tbl("INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
        input["active_role"],  input["active_user"], "User Management", "user name ({}) added ".format(input["user_name"])))
        end=datetime.now()
        difference_time=difference(start,end,fn)
        # print(" {}  Time difference: {}".format(fn,difference_time) )
        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
     
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def update_user():
    fn = "update_user()"
    global ser_err1, sec_code, db_conn
    try:
        start=datetime.now()
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms= "HS256")
        input = json.loads(request.data)
        # schema_result=check_schema(input, update_user1)
        # if schema_result!=True:
        #     return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        
        for key, value in input.items():
            if not value:
                return Response(json.dumps({"status": "Empty value found for key ({})".format(key)}), mimetype="application/json")
 
        mobile_number_pattern = r'^\(?(\d{3})\)?[-.\s]?(\d{3})[-.\s]?(\d{4})$'
        mail_pattern = r'^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$'
 
        for key, value in input.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
        
        old_data=get_old_data(input["id"],"user_mgmt")
        audit_msg=generate_audit_message(input,old_data[0])
        
 
        if input["role"]=="Admin":
            chance=chance_admin
        if input["role"]=="Super Admin":  
            chance=chance_superadmin
        if input["role"]=="Operator":  
            chance=chance_operator
           
        if not re.search(mobile_number_pattern, input["ph_no"]):
            return Response(json.dumps({"status": "Invalid Mobile number!"}), mimetype='application/json')
   
        if not re.search(mail_pattern, input["email"]):
            return Response(json.dumps({"status": "Invalid Mail address!"}), mimetype='application/json')
       
        encrypted_password = cipher_suite.encrypt(input["password"].encode('utf-8'))
   
        update_tbl("UPDATE user_mgmt SET role = '{}',user_full_name = '{}', user_name = '{}', `password` = '{}', mail_id = '{}', ph_no = '{}' ,max_num_users='{}',num_users='{}' WHERE id = '{}'".format(input["role"],
        input["user_full_name"], input["user_name"], encrypted_password.decode('utf-8'), input["email"], input["ph_no"] , chance,0,input["id"]))
      
 
        update_tbl("INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
        input["active_role"],  input["active_user"], "User Management", audit_msg))
        end=datetime.now()
        difference_time=difference(start,end,fn)
        # print(" {}  Time difference: {}".format(fn,difference_time) )
        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)    
   
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def delete_user():
    fn = "delete_user()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
 
        input =json.loads(request.data)
       

        for key, value in input.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
            
        
    
        user_name = execute(1,"SELECT user_name FROM user_mgmt WHERE id = '{}'".format(input["id"]))[0]
        
        update_tbl("DELETE FROM user_mgmt WHERE id = '{}'".format(input["id"]))

        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        input["active_role"],  input["active_user"], "User Management", "username ({}) deleted successfully".format(user_name))
        update_tbl(sql)

        return Response(json.dumps({"status": "success"}), mimetype='application/json')
      
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
   
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')


#Meter_type_password_configuration_management
def get_met_pass_cfg():                                                   
    fn = "get_met_pass_cfg()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        main_dict= {}
        results =  get_key_value_pairs("SELECT * FROM meter_type_password_tbl ORDER BY id ASC")

        if results != None:
            main_dict["details"]= results 
            main_dict["status"] = "found"
        else:
            main_dict["status"] = "not found"

        return Response(json.dumps(main_dict), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def add_met_pass_cfg():
    fn = "add_met_pass_cfg()"            
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        login_dict = json.loads(request.data)

        for key, value in login_dict.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
        
    
        results= get_key_value_pairs("SELECT meter_type_name FROM meter_type_password_tbl where meter_type_name='{}'".format(login_dict["meter_type_name"]))
        if results!=None:
            return Response(json.dumps({"status": "Meter type name already exists!"}), mimetype='application/json')

        sql = "INSERT INTO meter_type_password_tbl (`meter_type_name`, password) VALUES ('{}','{}')".format(login_dict["meter_type_name"], login_dict["password"])
        update_tbl(sql)

        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
        login_dict["active_role"],  login_dict["active_user"], "Meter Type Password Configuration", "meter type name ({}) added successfully".format(login_dict["meter_type_name"]))
        update_tbl(sql)

        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def update_met_pass_cfg():
    fn = "update_met_pass_cfg()"     
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        login_dict = json.loads(request.data)
        for key, value in login_dict.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

        results= get_key_value_pairs("SELECT meter_type_name FROM meter_type_password_tbl WHERE id != '{}' and meter_type_name='{}'".format(login_dict["meter_type_name"],login_dict["id"]))
        if results!=None:
            return Response(json.dumps({"status": "Meter type name already exists!"}), mimetype='application/json')
        
        old_data=get_old_data(login_dict["id"],"meter_type_password_tbl")[0]
        audit_msg=generate_audit_message(login_dict,old_data)
       
        sql = "UPDATE meter_type_password_tbl SET meter_type_name = '{}',password = '{}' WHERE id = '{}'".format(login_dict["meter_type_name"], 
        login_dict["password"],login_dict["id"])
        update_tbl(sql)

        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
        login_dict["active_role"],  login_dict["active_user"], "Meter Type Password Configuration", audit_msg)
        update_tbl(sql)

        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def delete_met_pass_cfg():
    fn = "delete_met_pass_cfg()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
 
        input =json.loads(request.data)

        for key, value in input.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
        
        user_name = execute(1,"SELECT meter_type_name FROM meter_type_password_tbl WHERE id = '{}'".format(input["id"]))[0]
        
        update_tbl("DELETE FROM meter_type_password_tbl WHERE id = '{}'".format(input["id"]))

        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
       input["active_role"] ,  input["active_user"], "Meter Type Password Configuration", "meter type name ({}) deleted successfully".format(user_name))
        update_tbl(sql)

        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')



#Device Management
def get_devices_list():
    fn = "get_devices_list()"
    global ser_err, sec_code, db_conn
    try:
            # header = request.headers.get('Authorization')
            # header = header.encode()
            # jwt.decode(header, sec_code, algorithms="HS256")
            
            
            args = request.args.get('modem_type')
            print("======args=========",args)
            offset = request.args.get('page_size')
          
            offset = (int(offset) - 1) * 20
            
            limit=20
        
            modem_type_value = args

            if modem_type_value == 'Internal':
                results_modem = execute(2, "SELECT dcu_id FROM modem_link_tbl  WHERE modem_link_sts='{}' order by id asc ".format(modem_type_value))
                

            elif modem_type_value == 'External':
                results_modem = execute(2, "SELECT dcu_id FROM modem_link_tbl  WHERE modem_link_sts='{}' order by id asc".format(modem_type_value))
                # length = len(results_modem)

            elif modem_type_value == 'VSAT':
                results_modem = execute(2, "SELECT dcu_id FROM modem_link_tbl  WHERE modem_link_sts='{}' order by id asc".format(modem_type_value))
                # length = len(results_modem)
                
            else:
                results_modem = execute(2, "SELECT dcu_id FROM modem_link_tbl order by id asc")
                # length = len(results_modem)
    
            if results_modem==() or  results_modem==None:
                  return Response(json.dumps({"status": "not found"}), mimetype='application/json')
            else:
                length = len(results_modem) 
       
         
            if length > 0:
                users = []

                for i in range(length):

                    main_dict = {}
                    # print(main_dict)
                    keys = ("id", "location_id", "location_name", "device_id", "device_name", "ip_address", "port_no", "num_meters","fw_ver", "status","seq_num","lat","lng")

                    query = "SELECT id, loc_id, loc_name, dev_id, dev_name, dev_ip_addr, num_ser_port, num_meters,fw_ver, dev_common_status , seq_num ,lat,lng FROM dev_tbl  WHERE   dev_id='{}' order by id".format( results_modem[i][0])
                    results=execute(2,query)
                    no_rows = len(results)

                  
                    for idx in range(0, no_rows):
                        results_1 = execute(2, "SELECT met_enable FROM dcu_dlms_meter_tbl WHERE dev_id = '{}' AND met_enable='Yes'".format(results_modem[i][0]))
                        if len(results_1) >= 1:
                            num_of_meters = len(results_1)
                        else:
                            num_of_meters = 0

                        device_data = dict(zip(keys, results[idx]))
                        device_data["num_of_met_en"] = str(num_of_meters)
                        users.append(device_data)
                        
                total_count = len(users)
                users = users[offset:offset + limit]
                if total_count != 0:
                    offset = offset+1
                    for i, item in enumerate(users, start=offset):
                        item['row_index'] = i
                
                if users!=[]:

                    main_dict["status"] = "found"
                    main_dict["num_devices"] = str(length)
                    main_dict["device_details"] = users
                    main_dict["total_count"] = total_count
                        
                else:
                    main_dict["status"] = "not found"
                
                
                # sorted_data = sorted(users, key=lambda x: x['id'])
                print("================main=====>>>>>>>>>>>>>>",main_dict)
                return Response(json.dumps(main_dict), mimetype='application/json')
                
            else:
                return Response(json.dumps({"status": "not found"}), mimetype='application/json')
  
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def add_device():
    fn = "add_device()"
    global ser_err1, sec_code, db_conn
    try:
        
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        login_dict = json.loads(request.data)
        # UPDATE dev_tbl SET `lat` = '112', `lng` = '123' WHERE `dev_id` = '64308';

        results= execute(2,"SELECT dev_id, dev_name,dev_ip_addr FROM dev_tbl")
        id = len(results)
        
      
     
        results_port= execute(1,"SELECT dev_model FROM dcu_serialnum_details WHERE serial_num ='{}'".format(login_dict["device_id"]))
     
 
        if "single" in results_port[0] :
            login_dict["port_no"] = "1"
       
 
        elif "2" in results_port[0] :
            login_dict["port_no"] = "2"
          
 
 
        elif "4" in results_port[0] :
            login_dict["port_no"] = "4"
            
        dev_type_re= execute(2,"SELECT dev_type FROM dcu_serialnum_details WHERE serial_num = '{}'".format(login_dict["device_id"]))
        dev_type_result = dev_type_re[0][0]
       

        for indx in range(0, id):
           
            if results[indx][1] == login_dict["device_name"]:
                return Response(json.dumps({"status": "Device name already exists!"}), mimetype='application/json')
        
            if results[indx][2] == login_dict["ip_address"]:
                return Response(json.dumps({"status": "Device IP already exists!"}), mimetype='application/json')
            
     
        sql= "UPDATE dcu_serialnum_details SET flag='true' WHERE serial_num = '{}'".format(login_dict["device_id"])
        update_tbl(sql)
        
        sql = "INSERT IGNORE INTO meter_type_password_tbl (`meter_type_name`, `password`) VALUES ('{}','{}')".format("L&T", "Admin@123")
        update_tbl(sql)

        if login_dict["port_no"] == "1":

            #dcu_ser_port_cfg_tbl
            sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1) VALUES ('{}','{}', '{}','{}', '{}','{}','{}')".format(
            login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8")
            update_tbl(sql)

            if dev_type_result =="DCU":
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"DCU","1_PORT",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","5", datetime.now(), "FW_1234")
                update_tbl(sql)
            
            else:
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"-","-",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","5", datetime.now(), "FW_1234")
                update_tbl(sql)

            #dcu_dlms_meter_tbl
            for meter in range(0, 5):
                if meter <2:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1", login_dict["device_name"],"123_{}".format(meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql) 
                
                if meter >=2 and meter<5:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1", login_dict["device_name"],"123_{}".format(meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql)

        
        if login_dict["port_no"] == "2":

            #dcu_ser_port_cfg_tbl
            sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1, port_type2,  ser_met_add_size2, ser_baud2, ser_stop_bit2, ser_parity2, ser_data_bits2) VALUES ('{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
            login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8", "RS485_MODE", "2", "9600","1", "Even","8")
            update_tbl(sql)

            if dev_type_result =="DCU":
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"DCU","2_PORT",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","10", datetime.now(), "FW_1234")
                update_tbl(sql)
            
            else:
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"-","-",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","10", datetime.now(), "FW_1234")
                update_tbl(sql)

            for meter in range(0, 10):
                if meter <2:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"123_{}".format(meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql)
                
                if meter >=2 and meter<5:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"123_{}".format(meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql)

                if meter >=5 and meter<7:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter-4, "No", "Incomer", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4),"2",login_dict["device_name"],"123_{}".format(meter),"met_2_{}".format(meter-4) , "Not Configured", "L&T")
                    update_tbl(sql)
                if meter >=7 and meter<10:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter-4, "No", "Live Outgoing", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4),"2",login_dict["device_name"],"123_{}".format(meter),"met_2_{}".format(meter-4), "Not Configured", "L&T" )
                    update_tbl(sql)
 
        if login_dict["port_no"] == "4":
            #dcu_ser_port_cfg_tbl
            sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1, port_type2,  ser_met_add_size2, ser_baud2, ser_stop_bit2, ser_parity2, ser_data_bits2, port_type3,  ser_met_add_size3, ser_baud3, ser_stop_bit3, ser_parity3, ser_data_bits3, port_type4,  ser_met_add_size4, ser_baud4, ser_stop_bit4, ser_parity4, ser_data_bits4) VALUES ('{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
            login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8", "RS232_MODE", "2", "9600","1", "Even","8", "RS485_MODE", "2", "9600","1", "Even","8", "RS485_MODE", "2", "9600","1", "Even","8")
            update_tbl(sql)

            if dev_type_result =="DCU":
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"DCU","4_PORT",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","20", datetime.now(), "FW_1234")
                update_tbl(sql)
            
            else:
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"-","-",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","20", datetime.now(), "FW_1234")
                update_tbl(sql)

            #dcu_dlms_meter_tbl
            for meter in range(0, 20):
                if meter <2:
                    sql_111 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name,comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"123_{}".format(meter),"met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql_111)

                if meter >=2 and meter<5:
                    sql_111 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"123_{}".format(meter),"met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql_111)

                if meter >=5 and meter<7:
                    sql_112 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                    login_dict["device_id"], meter-4, "No", "Incomer", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4), "2",login_dict["device_name"],"123_{}".format(meter),"met_2_{}".format(meter-4), "Not Configured", "L&T")
                    update_tbl(sql_112)
                
                if meter >=7 and meter<10:
                    sql_112 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                    login_dict["device_id"], meter-4, "No", "Live Outgoing", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4), "2",login_dict["device_name"],"123_{}".format(meter),"met_2_{}".format(meter-4), "Not Configured", "L&T")
                    update_tbl(sql_112)

                
                if meter >=10 and meter<12:
                    sql_113 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                    login_dict["device_id"], meter-9, "No", "Incomer", meter-9, "123", "123","meter_2_{}".format(meter), "{}_3_{}".format(login_dict["device_id"],meter-9),"3",login_dict["device_name"],"123_{}".format(meter),"met_3_{}".format(meter-9), "Not Configured", "L&T" )
                    update_tbl(sql_113)
                if meter >=12 and meter<15:
                    sql_113 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                    login_dict["device_id"], meter-9, "No", "Live Outgoing", meter-9, "123", "123","meter_2_{}".format(meter), "{}_3_{}".format(login_dict["device_id"],meter-9),"3",login_dict["device_name"],"123_{}".format(meter),"met_3_{}".format(meter-9), "Not Configured", "L&T" )
                    update_tbl(sql_113)

                if meter >=15 and meter<17:
                    sql_114 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                    login_dict["device_id"], meter-14, "No", "Incomer", meter-14, "123", "123","meter_3_{}".format(meter), "{}_4_{}".format(login_dict["device_id"],meter-14),"4",login_dict["device_name"],"123_{}".format(meter),"met_4_{}".format(meter-14), "Not Configured" , "L&T")
                    update_tbl(sql_114)
                
                if meter >=17 and meter<20:
                    sql_114 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                    login_dict["device_id"], meter-14, "No", "Live Outgoing", meter-14, "123", "123","meter_3_{}".format(meter), "{}_4_{}".format(login_dict["device_id"],meter-14),"4",login_dict["device_name"],"123_{}".format(meter),"met_4_{}".format(meter-14), "Not Configured" , "L&T")
                    update_tbl(sql_114)

        #dcu_gen_config_tbl
        sql_1 = "INSERT INTO dcu_gen_config_tbl(dev_id, dbglog_enable,dbglog_ip, net_type, eth_ip1,eth_subnet1,  eth_gateway1,  eth_ip2,eth_subnet2,  eth_gateway2, eth_ip3,eth_subnet3,  eth_gateway3, eth_ip4,eth_subnet4, eth_gateway4, ntp_enable1, ntp_enable2, ntp_ipadd1, ntp_port1,ntp_ipadd2, ntp_port2, ntp_interval, iec_enable,ftp_enable, modtcp_enable, vpn_enable,restapi_enable, gprs_enable, mqtt_enable) VALUES ('{}', '{}','{}', '{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}','{}', '{}')".format(
        login_dict["device_id"] ,"No","192.168.10.105","na","192.168.10.58","255.255.255.0","192.168.10.1","192.168.11.58","255.255.255.0","192.168.11.1","192.168.10.107","255.255.255.0","192.168.10.118",'192.168.10.109',"255.255.255.0","192.168.10.128","No","No","192.168.10.141","123","192.168.10.142","1234","10","No","No","No","No","No","No", "Yes")       #, eth_new_ip1, eth_new_ip2, , login_dict["ip_address"], login_dict["ip_address"]     , '{}'
        update_tbl(sql_1)
       
        #dcu_iec104_cfg_tbl
        sql_12 = "INSERT INTO dcu_iec104_cfg_tbl(dev_id, asdu, port, t1, t2, t3,iot_size, cot_size, cyc_int,enable_allow_master,enable_ip1,master1_ip,enable_ip2,master2_ip,enable_ip3,master3_ip,enable_ip4,master4_ip, start_ioa) VALUES ('{}', '{}', '{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}','{}', '{}', '{}')".format(
        login_dict["device_id"] ,"192.168.10.105","1234","12","13","10","3","2","30","No","No","192.168.10.160",'No',"192.168.10.81","No","192.168.10.82","No","192.168.10.83", "2000")
        update_tbl(sql_12)

        #dcu_ftp_cfg_tbl
        sql_13 = "INSERT INTO dcu_ftp_cfg_tbl(dev_id, ip, port, user_name, pwd, directory, periodicity) VALUES ('{}', '{}', '{}','{}', '{}', '{}', '{}')".format(
        login_dict["device_id"] ,"192.168.10.1","21","internet","internet","local/files/", "60")
        update_tbl(sql_13)

        #dcu_modtcp_cfg_tbl 
        sql_14 = "INSERT INTO dcu_modtcp_cfg_tbl(dev_id, ip, port, slave_id, resp_to_all, hold_reg_st_addr, input_reg_st_addr, read_coil_st_addr, read_discrete_st_addr) VALUES ('{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}')".format(
        login_dict["device_id"] ,"192.168.10.1","21","1","1","1000", "1000", "3000", "5000")
        update_tbl(sql_14)

        #dcu_vpn_cfg_tbl
        # sql_15 = "INSERT INTO dcu_vpn_cfg_tbl(dev_id, instance, gen_details, phase1_details, phase2_details, vpmppt_details) VALUES ('{}', '{}', '{}','{}', '{}', '{}')".format(
        # login_dict["device_id"] ,"1","NA","NA","NA","NA")
        # update_tbl(sql_15)

        #dcu_gprs_cfg_tbl
        sql_16 = "INSERT INTO dcu_gprs_cfg_tbl(dev_id, user_name, pwd, phone_num, apn, ping_ip1, ping_ip2, `interval`, num_attempts, ping_enable, connection_type) VALUES ('{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
        login_dict["device_id"], "internet", "internet", "*99#", "uninet", "0.0.0.0", "0.0.0.0", "0", "2", "No", "NA")
        update_tbl(sql_16)
       
        query="select dev_type, dev_model from dcu_serialnum_details where serial_num= '{}'".format(  login_dict["device_id"])
        ser=execute(2,query)
     
        if ser[0][0]=="DCU" or "Internal" in  ser[0][1]:
            
            query_modem="insert into  modem_link_tbl (dcu_id,modem_link_sts,router_id,modem_enable) values ('{}','{}','{}','{}')".format( login_dict["device_id"],"Internal","NA","NA")
            update_tbl(query_modem)
            query_vpn= "INSERT INTO dcu_vpn_cfg_tbl(dev_id,instance,gen_details,phase1_details,phase2_details,vpmppt_details) values ('{}','{}','{}','{}','{}','{}')".format( login_dict["device_id"],"NA","{}","{}","{}","{}")
            update_tbl(query_vpn)
        
            # query_router_vpn= "INSERT INTO router_vpn_cfg_tbl(dev_id,instance,gen_details,phase1_details,phase2_details,vpmppt_details) values ('{}','{}','{}','{}','{}','{}')".format( login_dict["device_id"],"NA","NA","NA","NA","NA")
            # update_tbl(query_router_vpn)

        else:
            query_modem="insert into  modem_link_tbl (dcu_id,modem_link_sts,router_id,modem_enable) values ('{}','{}','{}','{}')".format( login_dict["device_id"],"VSAT","NA","NA")
            update_tbl(query_modem)
      

        #dcu_mqtt_cfg_tbl
        query_DCU_MQTT="INSERT INTO dcu_mqtt_cfg_tbl(dev_id, mqtt_broker, mqtt_broker_port, user_name, pwd, client_id, hc_int, publish_topic, subscribe_topic, dcu_da_mod_topic, dcu_eve_mod_topic, dcu_diag_mod_topic) values ('{}','{}','{}','{}','{}','{}', '{}','{}','{}','{}','{}','{}')".format(login_dict["device_id"],login_dict["ip_address"],login_dict["port_no"], login_dict["active_user"], "softel", "NA",60,"NA","NA","NA","NA","NA")
        update_tbl(query_DCU_MQTT)

        #dcu_restapi_cfg_tbl
        query_restapi="INSERT INTO dcu_restapi_cfg_tbl(dev_id, user_name, pwd, url, periodicity) values ('{}','{}','{}','{}','{}')".format( login_dict["device_id"],login_dict["active_user"],"NA","NA","NA")
        update_tbl(query_restapi)
        
        query=update_tbl("UPDATE dev_tbl SET `lat`='{}', lng='{}'  where dev_id='{}'".format(login_dict["lat"],login_dict["lng"],login_dict["device_id"]))
        #audit_table 
        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        login_dict["active_role"],  login_dict["active_user"], "Device Management", "device ({}) added successfully".format(login_dict["device_name"]))
        update_tbl(sql)
        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()

        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype ='application/json')

def update_device():
    fn = "update_device()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()

        jwt.decode(header, sec_code, algorithms="HS256")

        login_dict = json.loads(request.data)
        old_data=get_old_data(login_dict["id"],"dev_tbl")[0]
        audit_msg=generate_audit_message(login_dict,old_data)
   
      
        results= execute(2,"SELECT dev_id, dev_name,dev_ip_addr FROM dev_tbl WHERE id  <> '{}'".format(login_dict["id"]))
      
        id = len(results)

        for indx in range(0, id):
            if results[indx][1] == login_dict["device_name"]:
                return Response(json.dumps({"status": "Device name already exists!"}), mimetype='application/json')
        
            if results[indx][2] == login_dict["ip_address"]:
                return Response(json.dumps({"status": "Device IP already exists!"}), mimetype='application/json')
            

        sql = "UPDATE dev_tbl SET loc_id = '{}',loc_name = '{}',dev_id='{}', dev_name = '{}', `dev_ip_addr` = '{}', num_ser_port = '{}',lat='{}',lng='{}' WHERE id = '{}'".format(login_dict["location_id"],
        login_dict["location_name"],login_dict["device_id"], login_dict["device_name"], login_dict["ip_address"], login_dict["port_no"],login_dict["lat"],login_dict["lng"], login_dict["id"])
        update_tbl(sql)

        sql = "UPDATE dcu_dlms_meter_tbl SET dev_name = '{}' WHERE dev_id = '{}'".format(login_dict["device_name"], login_dict["device_id"])
        update_tbl(sql)

        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        login_dict["active_role"],  login_dict["active_user"], "Device Management", audit_msg)
        update_tbl(sql)

        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def delete_device():
    fn = "delete_device()"

    global ser_err1, sec_code, db_conn
    try:
        # header = request.headers.get('Authorization')
        # header = header.encode()
        # jwt.decode(header, sec_code, algorithms="HS256")
 
        args = json.loads(request.data)

        for key, value in args.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
            
        serial_results= execute(2,"SELECT serial_num FROM dcu_serialnum_details")
        ser_id = len(serial_results)

        for index in range(0, ser_id):
            if serial_results[index][0] == args['dev_id']:
                sql= "UPDATE dcu_serialnum_details SET flag='false' WHERE serial_num = '{}'".format(args['dev_id'])
                update_tbl(sql)
        dev_name=get_key_value_pairs("select dev_name from dev_tbl where dev_id ='{}'".format(args['dev_id']))
        dev_name=[{"dev_name":"MIM"}]
        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        args["active_role"],  args["active_user"], "Device Management", "device ({}) deleted successfully".format(dev_name[0]["dev_name"]))
        update_tbl(sql)
    
        update_tbl("DELETE FROM dev_tbl WHERE dev_id = '{}'".format(args['dev_id']))

        update_tbl("DELETE FROM dcu_gen_config_tbl WHERE dev_id = '{}'".format(args['dev_id']))
        update_tbl("DELETE FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(args['dev_id']))
        update_tbl("DELETE FROM dcu_dlms_meter_tbl WHERE dev_id = '{}'".format(args['dev_id']))
        update_tbl("DELETE FROM dcu_iec104_cfg_tbl WHERE dev_id = '{}'".format(args['dev_id']))
        update_tbl("DELETE FROM dcu_ftp_cfg_tbl WHERE dev_id = '{}'".format(args['dev_id']))   

        update_tbl("DELETE FROM dcu_modtcp_cfg_tbl WHERE dev_id = '{}'".format(args['dev_id']))
     
        update_tbl("DELETE FROM dcu_gprs_cfg_tbl WHERE dev_id = '{}'".format(args['dev_id']))
        update_tbl("DELETE FROM router_vpn_cfg_tbl  WHERE dev_id = '{}'".format(args['dev_id']))
        router_id=execute(1,"select router_id from modem_link_tbl WHERE dcu_id = '{}'".format(args['dev_id']))

        if router_id != None or router_id == ():
            sql= "UPDATE dcu_serialnum_details SET flag='false' WHERE serial_num = '{}'".format(router_id[0])
            update_tbl(sql)
            update_tbl("DELETE FROM dev_tbl WHERE dev_id = '{}'".format(router_id[0]))
            
        else:
            print("Unable to delete dcu's {} -- router id".format(args['dev_id']))

        update_tbl("DELETE FROM modem_link_tbl  WHERE dcu_id = '{}'".format(args['dev_id']))

        update_tbl("DELETE FROM dcu_vpn_cfg_tbl  WHERE dev_id = '{}'".format(args['dev_id']))
        update_tbl("DELETE FROM dcu_mqtt_cfg_tbl  WHERE dev_id = '{}'".format(args['dev_id']))
        update_tbl("DELETE FROM dcu_restapi_cfg_tbl  WHERE dev_id = '{}'".format(args['dev_id']))

        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def get_serial_num_list():
    fn = "get_serial_num_list()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        main_dict= {}
        keys= ("id", "serial_num", "flag","dev_model","dev_name","dev_type")
        
        args= request.args
     
        results =  execute(2, "select * from dcu_serialnum_details where flag='false' and dev_type like 'DCU'")
        results1 =  execute(2, "select * from dcu_serialnum_details where flag='false' and dev_type like 'Modem' and dev_model like 'MIM_1PORT'")
        print(results1)
        no_rows= len(results)
        no_rows_1 = len(results1)
        users= []
        for idx in range(0, no_rows):
            users.append(dict(zip(keys, results[idx])))
        print(no_rows,"=========================================",no_rows_1)
        for idx in range(no_rows, no_rows+no_rows_1):
            
            users.append(dict(zip(keys, results1[idx-no_rows])))
        
        if users != []:
            main_dict["status"] = "found"

            main_dict["serial_num_details"]= users
        else:
            main_dict["status"] = "not found"
        print("+++++++++++++++++++++++++")
        print(users)
        return Response(json.dumps(main_dict), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def get_device_general_config_by_id():
    fn = "get_device_general_config_by_id()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        args = request.args

        if args != {}:
            if args['dev_id'] != "":
                dev_id = args.get('dev_id')
            else:
                return Response(json.dumps({"status": "invalid  dev_id argument"}), mimetype='application/json')
        
        args= request.args
      
        final_list=[]
        main_dict= {}
      
        results =  get_key_value_pairs("SELECT id, dev_id,  dbglog_enable, dbglog_ip, net_type,eth_ip1, eth_ip1, eth_ip2, eth_subnet1,eth_gateway1, eth_ip2 ,eth_subnet2, eth_gateway2, eth_ip3, eth_subnet3, eth_gateway3, eth_ip4, eth_subnet4,eth_gateway4,ntp_enable1, ntp_enable2,ntp_ipadd1,ntp_port1, ntp_ipadd2, ntp_port2,ntp_interval ,iec_enable, ftp_enable,mqtt_enable  FROM dcu_gen_config_tbl WHERE dev_id = '{}'".format(dev_id))
        results_1 =  get_key_value_pairs("SELECT enable_allow_master, enable_ip1 as master1_enable,  master1_ip, enable_ip2 as master2_enable , master2_ip, enable_ip3 as master3_enable , master3_ip, enable_ip4 as master4_enable ,master4_ip  FROM dcu_iec104_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_2 =  get_key_value_pairs( "SELECT dev_id , loc_name as dcu_location,  num_eth_port,dev_model  FROM dev_tbl WHERE dev_id = '{}'".format(dev_id))
        results_3 = get_key_value_pairs("SELECT  ip as ftp_ip,  port as ftp_port, user_name as ftp_user, pwd as ftp_pass, directory as ftp_dir, periodicity as ftp_period  FROM dcu_ftp_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_4 =  get_key_value_pairs( "SELECT asdu as iec_addr, cyc_int as iec_cyc_int,  port as iec_port, iot_size as iec_ioa, cot_size as iec_cot, start_ioa as iec_met_start_ioa FROM dcu_iec104_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_5 =  get_key_value_pairs("SELECT mqtt_broker as mqtt_broker_ip , mqtt_broker_port as mqtt_broker_port ,  user_name as mqtt_username , pwd as mqtt_password , hc_int as mqtt_periodicity FROM dcu_mqtt_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        
        results[0]["eth_new_ip1"]= results[0]["eth_ip1"]
        results[0]["eth_new_ip2"]= results[0]["eth_ip2"]
        combined_dict = {}
        for d in results_1 + results_2 + results_3 + results_4 + results_5+results:
            combined_dict.update(d)

        final_list.append(combined_dict)

        if combined_dict != []:
            main_dict["status"] = "found"
            main_dict["count"] = len(results)
            main_dict["details"]= final_list
           
        else:
            main_dict["status"] = "not found"
        print(main_dict)
        return Response(json.dumps(main_dict), mimetype='application/json')
    
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def get_device_serial_config_by_id():
    fn = "get_device_serial_config_by_id()"
    global ser_err, sec_code, db_conn

    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        args = request.args
        if args != {}:
            if args['dev_id'] != "":
                dev_id = args.get('dev_id')
            else:
                return Response(json.dumps({"status": "invalid dev_id argument"}), mimetype='application/json')

        plcc = execute(2, "SELECT dev_model FROM dcu_serialnum_details WHERE serial_num = '{}'".format(dev_id))
        #norows= len(plcc)
        print(plcc[0][0])
        main_dict= {}
        keys= ("id", "dev_id", "meter_enable","meter_type","met_id", "meter_ip", "meter_port","meter_pass", "meter_location" ,"meter_unique_id")
        keys_1= ("id", "dcu_id", "enable","meter_type","met_address", "met_loc", "meter_unique_id", "meter_type_name", "met_pass" , "ser_port" )
        keys_3 = {"enable","met_address", "met_loc"}
        keys_2 = ("comm_mode","ser_met_add_size", "ser_baud_rate", "ser_stop_bits","ser_parity", "ser_data" )

        results =  execute(2, "SELECT id, dev_id,  met_enable, meter_id, met_addr, comm_port, met_pass,met_loc, unique_id, ser_port_id FROM dcu_dlms_meter_tbl WHERE dev_id = '{}'".format(dev_id))
        no_rows= len(results)
        users= []

        met_merged_ser_data_1= []
        ser_met_data_2 =[]
        ser_met_data_3 =[]
        ser_met_data_4 =[]

        results_ser_port1 =  execute(2, "SELECT port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_ser_port2 =  execute(2, "SELECT port_type2,  ser_met_add_size2, ser_baud2, ser_stop_bit2, ser_parity2, ser_data_bits2 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_ser_port3 =  execute(2, "SELECT port_type3,  ser_met_add_size3, ser_baud3, ser_stop_bit3, ser_parity3, ser_data_bits3 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))
        results_ser_port4 =  execute(2, "SELECT port_type4,  ser_met_add_size4, ser_baud4, ser_stop_bit4, ser_parity4, ser_data_bits4 FROM dcu_ser_port_cfg_tbl WHERE dev_id = '{}'".format(dev_id))

        results_serial =  execute(2, "SELECT id, dev_id,  met_enable,met_type, met_addr, met_name, unique_id, meter_type_name, met_pass FROM dcu_dlms_meter_tbl WHERE dev_id = '{}'".format(dev_id))

       
        if plcc[0][0] == "DCU_PLCC":
            table_name = f"dcu_{dev_id}_meter_map"
            table_exist = check_table(table_name)
            if table_exist:
                results = execute(2, "SELECT met_sn, met_loc FROM {} order by id".format(table_name))
                ser_serial_1_1 = dict(zip(keys_2, results_ser_port1[0]))        
                
                if results != []:
                    for idx in range(len(results)):
                        met_merged_data_1 = {
                            "met_address": results[idx][0],
                            "enable": "Yes",
                            "met_loc": results[idx][1]
                        }

                        new_data = {**met_merged_data_1, **ser_serial_1_1}
                        met_merged_ser_data_1.append(new_data)
                else:
                    met_merged_ser_data_1 = []
        else:
            for idx in range(0, len(results_ser_port1)):
                ser_serial_1_1 = dict(zip(keys_2, results_ser_port1[idx]))
            

                for idx in range(0, len(results_serial)):
                    if idx<5:
                        met_merged_data_1 = dict(zip(keys_1, results_serial[idx]))
                        
                        new_data = {**met_merged_data_1, **ser_serial_1_1} 

                        met_merged_ser_data_1.append(new_data)

            for idx in range(0, len(results_ser_port2)):
                ser_serial_1_2 = dict(zip(keys_2, results_ser_port2[idx]))

                for idx in range(0, len(results_serial)):
                    if idx >=5 and idx<10:
                        met_merged_data_2 = dict(zip(keys_1, results_serial[idx]))

                        new_data = {**met_merged_data_2, **ser_serial_1_2}  
                        ser_met_data_2.append(new_data)

            for idx in range(0, len(results_ser_port3)):
                ser_serial_1_3 = dict(zip(keys_2, results_ser_port3[idx]))

                for idx in range(0, len(results_serial)):
                    if idx >=10 and idx<15:
                        met_merged_data_3 = dict(zip(keys_1, results_serial[idx]))

                        new_data = {**met_merged_data_3, **ser_serial_1_3}  
                        ser_met_data_3.append(new_data)

            for idx in range(0, len(results_ser_port4)):
                ser_serial_1_4 = dict(zip(keys_2, results_ser_port4[idx]))

                for idx in range(0, len(results_serial)):
                    if idx >=15 and idx<20:
                        met_merged_data_4 = dict(zip(keys_1, results_serial[idx]))

                        new_data = {**met_merged_data_4, **ser_serial_1_4}
                        ser_met_data_4.append(new_data)

        for idx in range(len(results)):
            merged_data = dict(zip(keys, results[idx]))
            users.append(merged_data)


       
        if plcc[0][0] == "DCU_PLCC":
            print("P")
            no_serial = 1
        else:
            if len(results)<=5:
                no_serial =1
            
            if len(results)>5 and len(results)<=10 :
                no_serial =2
            
            if len(results)>10 and len(results)<=15 :
                no_serial =3

            if len(results)>15 and len(results)<=20 :
                no_serial =4

        if users != [] and plcc[0][0] != "DCU_PLCC":
            main_dict["status"] = "found"
            main_dict["no_serial"]= no_serial
            main_dict["serial_count"]= len(results)
            main_dict["dev_model"]= plcc[0][0]
            main_dict["serial_details"]= {}
            main_dict["serial_details"]["serPor1Cfg"]= met_merged_ser_data_1
            main_dict["serial_details"]["serPor2Cfg"]= ser_met_data_2
            main_dict["serial_details"]["serPor3Cfg"]= ser_met_data_3
            main_dict["serial_details"]["serPor4Cfg"]= ser_met_data_4
        elif plcc[0][0] == "DCU_PLCC":
            main_dict["status"] = "found"
            main_dict["no_serial"]= no_serial
            main_dict["serial_count"]= len(results)
            main_dict["dev_model"]= plcc[0][0]
            main_dict["serial_details"]= {}
            main_dict["serial_details"]["serPor1Cfg"]= met_merged_ser_data_1
            # main_dict["serial_details"]["serPor2Cfg"]= ser_met_data_2
            # main_dict["serial_details"]["serPor3Cfg"]= ser_met_data_3
            # main_dict["serial_details"]["serPor4Cfg"]= ser_met_data_4
        else:
            main_dict["status"] = "not found"
      
        return Response(json.dumps(main_dict), mimetype='application/json')

     
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def device_general_config_save():
    fn = "device_general_config_save()"
    global ser_err1, sec_code, db_conn
    try:
        
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        login_dict = json.loads(request.data)
     
        old_data= get_device_general_config_by_id_extra(login_dict["dev_id"])
      
        audit_msg=generate_audit_message(login_dict,old_data[0])

        sql_1 = "UPDATE dev_tbl SET dev_name='{}', loc_name='{}', num_eth_port='{}' WHERE dev_id='{}'".format(
        login_dict["device_name"], login_dict["dcu_location"], login_dict["num_eth_port"], login_dict["dev_id"])
        update_tbl(sql_1)

        sql_2 = "UPDATE dcu_gen_config_tbl SET dbglog_enable='{}', dbglog_ip='{}', net_type='{}', eth_ip1='{}', eth_subnet1='{}', eth_gateway1='{}', eth_ip2='{}', eth_subnet2='{}', eth_gateway2='{}', eth_ip3='{}', eth_subnet3='{}', eth_gateway3='{}', eth_ip4='{}', eth_subnet4='{}', eth_gateway4='{}',  ntp_ipadd1='{}', ntp_port1='{}', ntp_ipadd2='{}', ntp_port2='{}', ntp_interval='{}' , ntp_enable1='{}',ntp_enable2='{}', iec_enable ='{}', ftp_enable = '{}' ,mqtt_enable='{}' WHERE dev_id='{}'".format(
        login_dict["dbglog_enable"], login_dict["dbglog_ip"], login_dict["net_type"], login_dict["eth_new_ip1"], login_dict["eth_subnet1"], login_dict["eth_gateway1"], login_dict["eth_new_ip2"], login_dict["eth_subnet2"], login_dict["eth_gateway2"], login_dict["eth_ip3"], login_dict["eth_subnet3"], login_dict["eth_gateway3"], login_dict["eth_ip4"], login_dict["eth_subnet4"], login_dict["eth_gateway4"],  login_dict["ntp_ipadd1"], login_dict["ntp_port1"], login_dict["ntp_ipadd2"], login_dict["ntp_port2"], login_dict["ntp_interval"], login_dict["ntp_enable1"], login_dict["ntp_enable2"],login_dict["iec_enable"], login_dict["ftp_enable"],login_dict["mqtt_enable"], login_dict["dev_id"])
        update_tbl(sql_2)
      
            
        sql_3 = "UPDATE dcu_iec104_cfg_tbl SET start_ioa= '{}', asdu='{}', cyc_int='{}', port='{}', iot_size='{}', cot_size='{}', enable_allow_master='{}', enable_ip1='{}', master1_ip='{}', enable_ip2='{}', master2_ip='{}', enable_ip3='{}', master3_ip='{}', enable_ip4='{}', master4_ip='{}' WHERE dev_id='{}'".format(
        login_dict["iec_met_start_ioa"], login_dict["iec_addr"], login_dict["iec_cyc_int"], login_dict["iec_port"], login_dict["iec_ioa"], login_dict["iec_cot"], login_dict["enable_allow_master"], login_dict["master1_enable"], login_dict["master1_ip"], login_dict["master2_enable"], login_dict["master2_ip"], login_dict["master3_enable"], login_dict["master3_ip"], login_dict["master4_enable"], login_dict["master4_ip"], login_dict["dev_id"])
        update_tbl(sql_3)

        sql_4 = "UPDATE dcu_ftp_cfg_tbl SET ip='{}', port='{}', user_name='{}', pwd='{}', directory='{}', periodicity='{}' WHERE dev_id='{}'".format(
         login_dict["ftp_ip"], login_dict["ftp_port"], login_dict["ftp_user"], login_dict["ftp_pass"], login_dict["ftp_dir"], login_dict["ftp_period"], login_dict["dev_id"])
        update_tbl(sql_4)
        
        sql_5 = "UPDATE dcu_mqtt_cfg_tbl SET mqtt_broker='{}', mqtt_broker_port='{}', user_name='{}', pwd='{}', hc_int='{}' WHERE dev_id='{}'".format(
        login_dict["mqtt_broker_ip"], login_dict["mqtt_broker_port"], login_dict["mqtt_username"], login_dict["mqtt_password"], login_dict["mqtt_periodicity"],login_dict["dev_id"])
        update_tbl(sql_5)

        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
        login_dict["active_role"],  login_dict["active_user"], "Device Management", "GEN CONFIG - device name [{}] - {}".format( login_dict["device_name"],audit_msg))
        update_tbl(sql)

        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def delete_unmatched_meters(met_sn_list, met_id_list, table_name):
    # Identify items to delete
    to_delete = [sn for sn in met_sn_list if sn.strip() not in [mid.strip() for mid in met_id_list]]

    for met_sn in to_delete:
        delete_query = f"DELETE FROM {table_name} WHERE met_sn = '{met_sn.strip()}'"
        update_tbl(delete_query)


def device_serial_config_save():
    fn = "device_serial_config_save()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        login_dict = json.loads(request.data)
        #old_data=get_device_serial_config_by_id_extra(login_dict["device_id"])
   
        if login_dict['serial_details']['serPor1Cfg'] != []:
            met_address_list = [entry["met_address"] for entry in login_dict["serial_details"]["serPor1Cfg"]]
            duplicates = set([x for x in met_address_list if met_address_list.count(x) > 1])

            if duplicates:
                return Response(json.dumps({"status": f"Duplicate met_address values found:{duplicates}"}), mimetype='application/json')
            
            dev_id = login_dict["device_id"]
            dev_model = get_key_value_pairs("SELECT dev_model FROM dev_tbl WHERE dev_id = '{}'".format(dev_id))
          
            if dev_model[0]["dev_model"] == "DCU_PLCC":
                table_name = f"dcu_{dev_id}_meter_map"
                table_exist = check_table(table_name)
                if table_exist:
                    query = f"SELECT met_sn FROM {table_name}"
                    results = execute(2, query)
                    met_sn_list = [row[0] for row in results]
                    
                    for item in login_dict['serial_details']['serPor1Cfg']:
                        met_sn = item['met_address'].strip()  
                        met_loc = item['met_loc']
                        
                        result = get_key_value_pairs("SELECT id FROM {} WHERE met_sn = '{}'".format(table_name,met_sn))
                        result_loc = get_key_value_pairs("SELECT id,met_sn FROM {} WHERE met_loc = '{}'".format(table_name,met_loc))                    
                        if result_loc != None:
                         
                            current_sn = result_loc[0]["met_sn"]
    
                            if current_sn != met_sn and met_sn in met_sn_list:
                                # Safe swap using temporary value
                                temp_value = "__TEMP__SN__"

                                # Step 1: Free the target SN
                                query1 = f"UPDATE {table_name} SET met_sn='{temp_value}' WHERE met_sn='{current_sn}'"
                                update_tbl(query1)

                                # Step 2: Assign new SN to current location
                                query2 = f"UPDATE {table_name} SET met_sn='{current_sn}' WHERE met_sn='{met_sn}'"
                                update_tbl(query2)

                                # Step 3: Replace temp with new SN
                                query3 = f"UPDATE {table_name} SET met_sn='{met_sn}' WHERE met_sn='{temp_value}'"
                                update_tbl(query3)

                                met_id = result_loc[0]["id"]
                                update_event = f"dcu_event_{dev_id}_tbl"

                                if check_table(update_event):     
                                    current_time = datetime.now()
                                   
                                    oldsernum = result_loc[0]["met_sn"]
                                    event_description = f"Meter serial number has been swapped between {current_sn} and {met_sn} at location {met_loc}"
                                    meter_events = 'Meter Events'                        
                                    update_event_query = "INSERT INTO {} (event_time, update_time,event_type,event_desc)VALUES ('{}','{}', '{}','{}' )".format(update_event,current_time,current_time,meter_events,event_description)
                                    update_tbl(update_event_query)

                            elif result_loc[0]["met_sn"] != met_sn:
                                    
                                    update_query = "UPDATE {} SET met_sn='{}'  WHERE met_loc='{}' ".format(table_name,met_sn,met_loc)
                                    update_tbl(update_query)

                                    met_id = result_loc[0]["id"]
                                    update_event = f"dcu_event_{dev_id}_tbl"
                                  
                                    if check_table(update_event):
                                        
                                        current_time = datetime.now()
                                        
                                        oldsernum = result_loc[0]["met_sn"]
                                        event_description = f"meter serial num changed from  {oldsernum}  to {met_sn}  in  location  {met_loc}"
                                        meter_events = 'Meter Events'                        
                                        update_event_query = "INSERT INTO {} (event_time, update_time,event_type,event_desc)VALUES ('{}','{}', '{}','{}' )".format(update_event,current_time,current_time,meter_events,event_description)
                                        update_tbl(update_event_query)
            
                        if result == None and result_loc == None:
                            
                            insert_query = "INSERT INTO {} (met_sn, met_loc)VALUES ('{}', '{}')".format(table_name,met_sn, met_loc)
                            update_tbl(insert_query)
                            
                        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
                        login_dict["active_role"],  login_dict["active_user"], "Device Management", "SERIAL CONFIG - device name [{}] - updated successfully".format( login_dict["device_name"]))
                        update_tbl(sql)                           
                    delete_unmatched_meters(met_sn_list, met_address_list, table_name)
                    return Response(json.dumps({"status": "success"}), mimetype='application/json')
                else:
                        table_creation = create_meter_map_table(table_name)
                        if table_creation:
                            for item in login_dict['serial_details']['serPor1Cfg']:
                                met_sn = item['met_address'].strip()  # Remove extra spaces
                                met_loc = item['met_loc']
                                
                                check_query = "SELECT id FROM dcu_1607009091_meter_map WHERE met_sn = %s"
                                result = get_key_value_pairs("SELECT id FROM {} WHERE met_sn = '{}'".format(table_name,met_sn))
                         
                                if result == None:
                                    insert_query = "INSERT INTO {} (met_sn, met_loc)VALUES ('{}', '{}')".format(table_name,met_sn, met_loc)
                                    update_tbl(insert_query)
                                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
                                login_dict["active_role"],  login_dict["active_user"], "Device Management", "SERIAL CONFIG - device name [{}] - updated successfully".format( login_dict["device_name"]))
                                update_tbl(sql)
                                    
                            return Response(json.dumps({"status": "success"}), mimetype='application/json')
                        
                
                return Response(json.dumps({"status": "meter serial already exist"}), mimetype='application/json')
            else:
                length= len(login_dict["serial_details"])
                length_1= len(login_dict["serial_details"]["serPor1Cfg"])
                length_2= len(login_dict["serial_details"]["serPor2Cfg"])
                length_3= len(login_dict["serial_details"]["serPor3Cfg"])
                length_4= len(login_dict["serial_details"]["serPor4Cfg"])

                for indx in range (length):
                    for idx in range (length_1):
                        print("12")
                        if idx <5:
                            print("true")
                            sql_1 = "UPDATE dcu_dlms_meter_tbl SET met_enable='{}', met_type='{}', met_addr='{}', met_pass='{}', met_name='{}', unique_id='{}', comm_port='{}', meter_type_name='{}' WHERE dev_id='{}' and unique_id ='{}'".format(
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["enable"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["meter_type"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["met_address"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["met_pass"].strip(),
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["met_loc"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["meter_unique_id"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["ser_port"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["meter_type_name"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["dcu_id"],
                                    login_dict["serial_details"]["serPor1Cfg"][idx]["meter_unique_id"]
                                )
                            test = update_tbl(sql_1)
                            print(sql_1)

                            sql_2 = "UPDATE dcu_ser_port_cfg_tbl " \
                                    "SET port_type1='{}', ser_met_add_size1='{}', ser_baud1='{}', ser_stop_bit1='{}', ser_parity1='{}', ser_data_bits1='{}' " \
                                    "WHERE dev_id='{}'".format(
                                        login_dict["serial_details"]["serPor1Cfg"][idx]["comm_mode"],
                                        login_dict["serial_details"]["serPor1Cfg"][idx]["ser_met_add_size"],
                                        login_dict["serial_details"]["serPor1Cfg"][idx]["ser_baud_rate"],
                                        login_dict["serial_details"]["serPor1Cfg"][idx]["ser_stop_bits"],
                                        login_dict["serial_details"]["serPor1Cfg"][idx]["ser_parity"],
                                        login_dict["serial_details"]["serPor1Cfg"][idx]["ser_data"],
                                        login_dict["serial_details"]["serPor1Cfg"][idx]["dcu_id"]
                                    )

                        

                            update_tbl(sql_2)
                        
                    for idx in range (length_2):
                    
                        if idx <5:
                            
                            sql_1 = "UPDATE dcu_dlms_meter_tbl SET met_enable='{}', met_type='{}', met_addr='{}', met_pass='{}', met_name='{}', unique_id='{}', comm_port='{}', meter_type_name='{}' WHERE dev_id='{}'and unique_id ='{}'".format(
                                login_dict["serial_details"]["serPor2Cfg"][idx]["enable"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["meter_type"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["met_address"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["met_pass"].strip(),
                                login_dict["serial_details"]["serPor2Cfg"][idx]["met_loc"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["meter_unique_id"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["ser_port"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["meter_type_name"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["dcu_id"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["meter_unique_id"]

                            )
                            update_tbl(sql_1)

                            sql_2 = "UPDATE dcu_ser_port_cfg_tbl SET port_type2='{}', ser_met_add_size2='{}', ser_baud2='{}', ser_stop_bit2='{}', ser_parity2='{}', ser_data_bits2='{}' WHERE dev_id='{}'".format(
                                login_dict["serial_details"]["serPor2Cfg"][idx]["comm_mode"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["ser_met_add_size"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["ser_baud_rate"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["ser_stop_bits"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["ser_parity"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["ser_data"],
                                login_dict["serial_details"]["serPor2Cfg"][idx]["dcu_id"]
                            )
                            update_tbl(sql_2)

                    for idx in range (length_3):
                        
                        if idx <5:
                        
                            sql_1 = "UPDATE dcu_dlms_meter_tbl SET met_enable='{}', met_type='{}', met_addr='{}', met_pass='{}', met_name='{}', unique_id='{}', comm_port='{}', meter_type_name='{}' WHERE dev_id='{}'and unique_id ='{}'".format(
                                login_dict["serial_details"]["serPor3Cfg"][idx]["enable"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["meter_type"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["met_address"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["met_pass"].strip(),
                                login_dict["serial_details"]["serPor3Cfg"][idx]["met_loc"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["meter_unique_id"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["ser_port"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["meter_type_name"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["dcu_id"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["meter_unique_id"]

                            )

                            update_tbl(sql_1)

                            sql_2 = "UPDATE dcu_ser_port_cfg_tbl SET port_type3='{}', ser_met_add_size3='{}', ser_baud3='{}', ser_stop_bit3='{}', ser_parity3='{}', ser_data_bits3='{}' WHERE dev_id='{}'".format(
                                login_dict["serial_details"]["serPor3Cfg"][idx]["comm_mode"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["ser_met_add_size"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["ser_baud_rate"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["ser_stop_bits"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["ser_parity"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["ser_data"],
                                login_dict["serial_details"]["serPor3Cfg"][idx]["dcu_id"]
                            )

                            update_tbl(sql_2)

                    for idx in range (length_4):
                        
                        if idx <5:
                            
                            sql_1 = "UPDATE dcu_dlms_meter_tbl SET met_enable='{}', met_type='{}', met_addr='{}', met_pass='{}', met_name='{}', unique_id='{}', comm_port='{}', meter_type_name='{}' WHERE dev_id='{}'and unique_id ='{}'".format(
                                login_dict["serial_details"]["serPor4Cfg"][idx]["enable"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["meter_type"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["met_address"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["met_pass"].strip(),
                                login_dict["serial_details"]["serPor4Cfg"][idx]["met_loc"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["meter_unique_id"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["ser_port"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["meter_type_name"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["dcu_id"],
                                login_dict["serial_details"]["serPor4Cfg"][idx]["meter_unique_id"]
                            )
                            update_tbl(sql_1)

                            sql_2 = "UPDATE dcu_ser_port_cfg_tbl SET port_type4='{}', ser_met_add_size4='{}', ser_baud4='{}', ser_stop_bit4='{}', ser_parity4='{}', ser_data_bits4='{}' WHERE dev_id='{}'".format(
                                    login_dict["serial_details"]["serPor4Cfg"][idx]["comm_mode"],
                                    login_dict["serial_details"]["serPor4Cfg"][idx]["ser_met_add_size"],
                                    login_dict["serial_details"]["serPor4Cfg"][idx]["ser_baud_rate"],
                                    login_dict["serial_details"]["serPor4Cfg"][idx]["ser_stop_bits"],
                                    login_dict["serial_details"]["serPor4Cfg"][idx]["ser_parity"],
                                    login_dict["serial_details"]["serPor4Cfg"][idx]["ser_data"],
                                    login_dict["serial_details"]["serPor4Cfg"][idx]["dcu_id"]
                                )
                            update_tbl(sql_2)

                
                    
                    sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format( datetime.now(),
                    login_dict["active_role"],  login_dict["active_user"], "Device Management", "SERIAL CONFIG - device name [{}] - updated successfully".format( login_dict["device_name"]))
                    update_tbl(sql)

                    return Response(json.dumps({"status": "success"}), mimetype='application/json')

            return Response(json.dumps({"status": "Not found"}), mimetype='application/json')
        else:
            print("No serial details found")
            dev_id = login_dict["device_id"]
            table_name = f"dcu_{dev_id}_meter_map"
            table_exist = check_table(table_name)
            if table_exist:
                print("table exist")
                truncate_query = f"TRUNCATE TABLE {table_name};"
                update_tbl(truncate_query)
                return Response(json.dumps({"status": "success"}), mimetype='application/json')
            else:
                return Response(json.dumps({"status": "No serial details found"}), mimetype='application/json')
                
            return Response(json.dumps({"status": "No serial details found"}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')
    
def overall_comn_status():
    fn = "overall_comn_status()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")
        
        main_dict= {}

        dev_len= 0
        gw_comn= 0
        met_len= 0
        met_comn= 0
        
        db_dev_details = execute(2, "SELECT dev_id, dev_name, dev_common_status FROM dev_tbl ORDER BY loc_id") #dev_type, dev_model
        dev_len= len(db_dev_details)
        
        
        if dev_len == 0:
            return Response(json.dumps({"status": "No devices available!"}), mimetype='application/json')

        for dev_idx in range(0, dev_len):
            if db_dev_details[dev_idx] != None :
                if len(db_dev_details[dev_idx]) == 3 :
                    if db_dev_details[dev_idx][2] == 'Communicating':
                        gw_comn += 1
        
        met_details = execute(2, "SELECT dev_id, met_name, communication_status FROM slave_dev_tbl") 
        met_len= len(met_details)
     
        if met_len == 0:
            return Response(json.dumps({"status": "No devices available!"}), mimetype='application/json')

        for met_idx in range(0, met_len):
            if met_details[met_idx] != None and met_details[met_idx] != () :
                if met_details[met_idx][2] == 'Communicating':
                    met_comn += 1

        
        main_dict["total_gw"] = dev_len
        main_dict["comn_gw"] = gw_comn
        main_dict["total_met"] = met_len
        main_dict["comn_met"] = met_comn
        
        main_dict["status"] = "found"

        return Response(json.dumps(main_dict), mimetype='application/json')
     
   
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
def update_modem():
    fn = 'update_modem'
    global sec_code, ser_err

    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
    
        request_data = json.loads(request.data) 
        modem_sts=execute(2,"select modem_link_sts,router_id   from  modem_link_tbl where  dcu_id='{}'".format(request_data["dev_id"]))
        
     
        modem=request_data["modem_details"]
        vpn=request_data["vpn_details"]
      
        if request_data["modem_link_sts"]!=modem_sts[0][0]:
        
            if modem_sts[0][0]=="Internal":
            
      
                dcu_gprs = "UPDATE dcu_gprs_cfg_tbl SET  user_name='{}', pwd='{}', phone_num='{}', apn='{}', ping_ip1='{}', ping_ip2='{}', `interval`='{}', num_attempts='{}', ping_enable='{}', connection_type='{}' where dev_id='{}'".format(
                modem["user_name"],modem["pwd"],modem["phone_num"],modem["apn"],modem["ping_ip1"], modem["ping_ip2"], modem["interval"],modem["num_attempts"], modem["ping_enable"],  modem["connection_type"], request_data["dev_id"])
                update_tbl(dcu_gprs)
                update_tbl("UPDATE dcu_gen_config_tbl SET vpn_enable= 'No', gprs_enable='No' WHERE dev_id = '{}'".format(request_data['dev_id']))
                query_vpn= "UPDATE  dcu_vpn_cfg_tbl SET instance='{}',gen_details='{}',phase1_details='{}',phase2_details='{}',vpmppt_details='{}' where dev_id='{}'".format( vpn["instance"],  json.dumps(vpn["gen_details"]), json.dumps(vpn["phase1_details"]), json.dumps(vpn["phase2_details"]), json.dumps(vpn["vpmppt_details"]),request_data['dev_id'])
                update_tbl(query_vpn)
              
            if modem_sts[0][0]=="External":
                update_tbl("DELETE FROM dev_tbl WHERE dev_id = '{}'".format(modem_sts[0][1]))
                ser_num_query="UPDATE  dcu_serialnum_details SET flag='false' where serial_num='{}'".format(modem_sts[0][1])
                update_tbl(ser_num_query) 
          
              
            modem_sts_query="UPDATE modem_link_tbl SET modem_link_sts='{}' WHERE dcu_id = '{}'".format(request_data["modem_link_sts"],request_data['dev_id'])    
            update_tbl(modem_sts_query)
      
        if  request_data["modem_link_sts"]=="External":
         
            if modem_sts[0][0]==request_data["modem_link_sts"]:

                ser_num_query1="UPDATE  dcu_serialnum_details SET flag='false' where serial_num='{}'".format(modem_sts[0][1])
                update_tbl(ser_num_query1) 

                update_tbl("DELETE FROM dev_tbl WHERE dev_id = '{}'".format(modem_sts[0][1]))


            query="UPDATE  modem_link_tbl SET  modem_link_sts='{}', router_id='{}' where dcu_id ='{}'".format(request_data["modem_link_sts"],request_data["router_id"],request_data["dev_id"])
            update_tbl(query)  

            ser_num_query="UPDATE  dcu_serialnum_details SET flag='true' where serial_num='{}'".format(request_data["router_id"])
            update_tbl(ser_num_query)  
         
            dev_query="INSERT INTO `dev_tbl` (`loc_id`, `dev_id`, `loc_name`, `dev_type`, `dev_model`, `dev_sn`, `dev_common_status`) VALUES('{}','{}','{}','{}','{}','{}','{}')".format(request_data["loc_id"] ,request_data["router_id"] ,request_data["loc_name"], 'ROUTER', 'CS-IoT-RTR', request_data["router_id"], 'Not communicating')
            update_tbl(dev_query)
     
        if request_data["modem_link_sts"]=="Internal":
            query="update  `dcu_gprs_cfg_tbl` SET  user_name='{}', pwd='{}', phone_num='{}', apn='{}', ping_ip1='{}', ping_ip2='{}', `interval`='{}', num_attempts='{}', ping_enable='{}', connection_type='{}' where dev_id ='{}'".format(
                 request_data["modem_details"]["user_name"],
                request_data["modem_details"]["pwd"],
                request_data["modem_details"]["phone_num"],
                request_data["modem_details"]["apn"],
                request_data["modem_details"]["ping_ip1"],
                request_data["modem_details"]["ping_ip2"],
                request_data["modem_details"]["interval"], 
                request_data["modem_details"]["num_attempts"],
                request_data["modem_details"]["ping_enable"],
               
                request_data["modem_details"]["connection_type"],
                request_data["dev_id"]
            )
            update_tbl(query)   
            

            modem_enable_query= "UPDATE  dcu_gen_config_tbl SET gprs_enable='{}', vpn_enable='{}' where dev_id='{}'".format( request_data["modem_details"]["modem_enable"],request_data["vpn_details"]["gen_details"]["ENABLE"],request_data["dev_id"])
            update_tbl(modem_enable_query) 
           
            query_vpn="UPDATE  dcu_vpn_cfg_tbl SET  instance='{}', gen_details='{}', phase1_details='{}', phase2_details='{}' where dev_id ='{}'".format( #,vpmppt_details='{}'
            request_data["vpn_details"]["instance"],
            json.dumps(request_data["vpn_details"]["gen_details"]),
            json.dumps(request_data["vpn_details"]["phase1_details"]),
            json.dumps(request_data["vpn_details"]["phase2_details"]),
            request_data["dev_id"])
            update_tbl(query_vpn)
        
        if request_data["modem_link_sts"]=="VSAT":   
            
            query="UPDATE  modem_link_tbl SET modem_link_sts='{}' where dcu_id='{}'".format(request_data["modem_link_sts"],request_data["dev_id"])
            update_tbl(query)
        
        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        request_data["active_role"],  request_data["active_user"], "Device Management", "MODEM MAPPING- device name [{}] - updated successfully".format(request_data["dev_name"]))
        update_tbl(sql)
        return Response(json.dumps({"status": "success"}), content_type="application/json")

    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
   
def getlist_modem_config():
    fn='getlist_modem_config'
    try: 
        
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data) 
        # print(request_data)
        query_gprs_details="select * from dcu_gprs_cfg_tbl where dev_id='{}'".format(request_data["dev_id"])
        # query_vpn_details="select * from router_vpn_cfg_tbl where dev_id='{}'".format(request_data["dev_id"])
        # print(query_vpn_details)
        query_vpn_details="select * from dcu_vpn_cfg_tbl where dev_id='{}'".format(request_data["dev_id"])
   
        query_modem_details="select * from modem_link_tbl where dcu_id='{}'".format(request_data["dev_id"])
      
        modem_enable_query= "select gprs_enable  from  dcu_gen_config_tbl where dev_id='{}'".format(request_data["dev_id"])
        modem_enable_res=execute(1,modem_enable_query)
        if modem_enable_res==None:
            modem_enable="-"
        else:
            modem_enable=modem_enable_res[0]
        
        gprs=get_key_value_pairs(query_gprs_details)
      
        if gprs==None:
            modem_details={
		"connection_type":"",
		"user_name":"",
		"pwd":"",
		"phone_num":"",
		"ping_enable":"",
        "modem_enable":"",
		"apn":"",
		"ping_ip1":"",
		"ping_ip2":"",
		"interval":"",
		"num_attempts":""

	}
        else:
            gprs[0]["modem_enable"]=modem_enable
            modem_details=gprs[0]
            
        vpn=get_key_value_pairs(query_vpn_details)
        
        print("----------------------------",vpn)
      
      
        if vpn==None:
            vpn_details={
		"instance":"",
		"gen_details":{
			"ENABLE":"",
			"vpn_type":"",
			"KEY_TYPE":"",
			"tun_name":"",
			"KEY_LIFETime":"",
			"LOCAL_IP":"",
			"LOCAL_SUBNET":"",
			"LOCAL_ID":"",
			"REMOTE_IP":"",
			"REMOTE_SUBNET":"",
			"REMOTE_ID":"",
			"PRESHARED_KEY":"",
			"AGG_MODE":"",
			"keying_mode":"",
			"nat_traves_mode":"",
			"Reserved":"",
			"PFS":"",
			"STATUS":""
		    },
		"phase1_details":{
			"DH_GROUP1":"",
			"ENCRPTION1":"",
			"AUTH1":""
		    },
		"phase2_details":{
			"DH_GROUP2":"",
			"ENCRPTION2":"",
			"AUTH2":""
		    },
		"vpmppt_details":{
			"vpnpptpConn_name":"",
			"vpnpptpLoc_username":"",
			"vpnpptpLoc_password":"",
			"vpnpptpremoteIpadd":"",
			"vpnpptprequirwMoney":""
		    }
	        }
           
        else:
            vpn[0]["gen_details"] = json.loads(vpn[0].get("gen_details")) if vpn[0].get("gen_details") not in [None, "NA"] else "-"
            vpn[0]["phase1_details"] = json.loads(vpn[0].get("phase1_details")) if vpn[0].get("phase1_details") not in [None, "NA"] else "-"
            vpn[0]["phase2_details"] = json.loads(vpn[0].get("phase2_details")) if vpn[0].get("phase2_details") not in [None, "NA"] else "-"
            vpn_details=vpn[0]
        
        modem=get_key_value_pairs(query_modem_details)
      
        if modem==None:
            modem_link={
	            "modem_link_sts":"",
	            "dev_id":"",
	         
	            "router_id":""}
        else:
            modem_link=modem[0]
      
        # vpn_details['gen_details'] = json.loads(vpn_details['gen_details'])
        # vpn_details['phase1_details'] = json.loads(vpn_details['phase1_details'])
        # vpn_details['phase2_details'] = json.loads(vpn_details['phase2_details'])
        modem_link["modem_details"]=modem_details
        modem_link["vpn_details"]=vpn_details
        print("---------------gret---------------------",modem_link)
        return Response(json.dumps({"status": "found","details":modem_link}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")  
    


#location
def location_get_list():
    fn = 'get_list'
    global sec_code, ser_err
    try:
       
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        args1 = request.args.get('page_size')
        print(type(args1))
        if args1 != "$$page_size$$":
            args1 = int(args1) - 1
            
            offset = args1 * 20
            query = "select * from loc_mgmt order by id asc limit 20 offset {}".format(offset)     
            result = get_key_value_pairs(query)
            query1 = "select count(*) AS total_count from loc_mgmt order by id asc"      
            count = get_key_value_pairs(query1)
            total_count = count[0].get('total_count')
            if total_count != 0:
                offset = offset+1
                for i, item in enumerate(result, start=offset):
                    item['row_index'] = i
        else:
            query = "select * from loc_mgmt order by id asc "   
            result = get_key_value_pairs(query)
            query1 = "select count(*) AS total_count from loc_mgmt order by id asc"      
            count = get_key_value_pairs(query1)
            total_count = count[0].get('total_count')
            

        if result == () or result == None:
          
            return Response(json.dumps({"status": "not found"}), content_type="application/json")
        else:
           
            return Response(json.dumps({"status": "found", "details": result,"total_count":total_count}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def location_nonconfig_get_list():
    fn = 'get_list'
    global sec_code, ser_err
    try:
       
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        query = execute(2,"select loc_name from dev_tbl ")
        print(query)
        if query==():
            query1 = f"SELECT * FROM loc_mgmt  ORDER BY id ASC " 
            
        else:
      
            unique_loc_names = set(row[0] for row in query)
            loc_names_tuple = tuple(unique_loc_names)
            if len(loc_names_tuple) == 1:
                loc_names_tuple = f"('{loc_names_tuple[0]}')"  # SQL requires parentheses like ('kovai')
            else:
                loc_names_tuple = str(loc_names_tuple)
            query1 = f"SELECT * FROM loc_mgmt WHERE loc_name NOT IN {loc_names_tuple} ORDER BY id ASC " 
            
        print(query1)
        result = get_key_value_pairs(query1)
        if result == () or result == None:
          
            return Response(json.dumps({"status": "not found"}), content_type="application/json")
        else:
           
            return Response(json.dumps({"status": "found", "details": result}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")




def add_location():
    fn = 'add_location'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)
        keys_to_check = ["active_role", "active_user", "loc_name"]
        for key in keys_to_check:
            if request_data[key] == "":
                return Response(json.dumps({"status": "Empty value found for key '{}'".format(key)}), mimetype="application/json")

        if request_data['site_type'] == 'IPPS' and request_data["gen_type"] == "":
            return Response(json.dumps({"status": "Empty value found for key gen_type"}), mimetype="application/json")

        query = "SELECT loc_name FROM loc_mgmt WHERE loc_name = '{}'".format( request_data["loc_name"] )
        result = execute(1, query)

        if result == None:
            insert_query = "INSERT INTO loc_mgmt ( loc_name, category_1, category_2, category_3, category_4, category_5, category_6,gen_type,site_type) VALUES{}".format(
                (
                    request_data["loc_name"],
                    request_data["category_1"],
                    request_data["category_2"],
                    request_data["category_3"],
                    request_data["category_4"],
                    request_data["category_5"],
                    request_data["category_6"],
                    request_data["gen_type"],
                    request_data["site_type"]
                )

            )
            update_tbl(insert_query)

            audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
                request_data["active_user"],
                request_data["active_role"],
                datetime.now(),
                "Location Management",
                "location ({}) added successfully".format(request_data["loc_name"]),
            )
            update_tbl(audit_query)

            return Response(json.dumps({"status": "success"}), mimetype="application/json")
        else:
            return Response(json.dumps({"status": "location name already exist"}), mimetype="application/json")
        
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def update_location():
    fn = 'update_location'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)

        if request_data['site_type'] == 'IPPS' and request_data["gen_type"] == "":
            return Response(json.dumps({"status": "Empty value found for key gen_type"}), mimetype="application/json")
        
        query = "SELECT loc_name FROM loc_mgmt WHERE loc_name = '{}' and id!={}".format(
            request_data["loc_name"], request_data["id"]
        )
        result = execute(1, query)
        if result != None:
            return Response(json.dumps({"status": "location name already exist"}), mimetype="application/json")
        else:
            old_data=get_old_data(request_data["id"],"loc_mgmt")[0]
            audit_msg=generate_audit_message(request_data,old_data)
            insert_query = "UPDATE loc_mgmt SET loc_name='{}', location_gps_coordinates='{}', category_1='{}', category_2='{}', category_3='{}', category_4='{}', category_5='{}', category_6='{}',gen_type='{}',site_type='{}' WHERE id='{}'".format(

                request_data["loc_name"],
                request_data["loc_name"],
                request_data["category_1"],
                request_data["category_2"],
                request_data["category_3"],
                request_data["category_4"],
                request_data["category_5"],
                request_data["category_6"],
                request_data["gen_type"],
                request_data["site_type"],
                request_data["id"]
            )
        
            dev_update="UPDATE dev_tbl SET loc_name ='{}' where loc_id ='{}'".format(  request_data["loc_name"],request_data["id"])
            update_tbl(dev_update)

        update_tbl(insert_query)
        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
            request_data["active_user"],
            request_data["active_role"],
            datetime.now(),
            "Location Management",
          audit_msg)
        
        update_tbl(audit_query)

        return Response(json.dumps({"status": "success"}), mimetype="application/json")
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
    
def delete_location():
    fn = 'delete_location'

    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        request_data = json.loads(request.data)
        
        schema_result=check_schema(request_data,delete_location_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        for key, value in request_data.items():
            if not value:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

        query = "SELECT loc_id,loc_name,dev_name FROM dev_tbl WHERE loc_id = '{}'".format(
            request_data["id"])
        dev_details = execute(1, query)

        if dev_details == None or dev_details == ():

            query = "SELECT loc_name FROM loc_mgmt WHERE id = '{}'".format(
                request_data["id"])
            loc_name = execute(1, query)
            if loc_name == None or loc_name == ():
                return Response(json.dumps({"status": "id doesnt exist"}), content_type="application/json")
            else:
                delete_query = "DELETE FROM loc_mgmt WHERE id='{}'".format(
                    request_data["id"])
                update_tbl(delete_query)

                audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
                    request_data["active_user"],
                    request_data["active_role"],
                    datetime.now(),
                    "location_management",
                    "location ({}) deleted successfully".format(loc_name[0]),
                )

            update_tbl(audit_query)
            return Response(json.dumps({"status": "success"}), content_type="application/json")
        else:
            return Response(json.dumps({"status": "{} location configured with {}, so can't delete".format(dev_details[1], dev_details[2])}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")


# notification management
def get_notification_list():
    fn = get_notification_list
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        query = "select * from notification_config_table"
        result = get_key_value_pairs(query)
        if result == None:
            return Response(json.dumps({"status": "not found!"}), content_type="application/json")
        else:
            return Response(json.dumps({"status": "found", "details": result}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def add_notification():
    fn = add_notification
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        request_data = json.loads(request.data)
        schema_result=check_schema(request_data,add_notification_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        for key, value in request_data.items():
            if not value:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

        query1 = "select * from notification_config_table where user_name='{}' and mobile_num='{}' and email_id='{}' and  category_type='{}' and loc_id='{}' and loc_name='{}' and ph_mail='{}' and  type='{}'  and  user_id={}".format(request_data["user_name"],
                                                                                                                                                                                                                                        request_data["mobile_num"], request_data["email_id"],  request_data["category_type"],   request_data["loc_id"],     request_data["loc_name"],    request_data["ph_mail"], request_data["type"], request_data["user_id"])

        result = execute(4, query1)

        if result == None or result == [] or result == ():
            insert_query = "INSERT INTO notification_config_table (user_name,mobile_num,email_id,category_type,loc_id,loc_name,ph_mail,type,user_id) VALUES ('{}', '{}', '{}', '{}','{}', '{}', '{}', '{}','{}')".format(
                request_data["user_name"],
                request_data["mobile_num"],
                request_data["email_id"],
                request_data["category_type"],
                request_data["loc_id"],
                request_data["loc_name"],

                request_data["ph_mail"],
                request_data["type"],
                request_data["user_id"]


            )

            update_tbl(insert_query)
            audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
                request_data["active_user"],
                request_data["active_role"],
                datetime.now(),
                "Notification Management",
                "location ({}) added in category ({})  mapped with user ({})".format(  request_data["loc_name"],   request_data["category_type"],
                    request_data["user_name"]),
            )

            update_tbl(audit_query)
            return Response(json.dumps({"status": "success"}), content_type="application/json")
        else:
            return Response(json.dumps({"status": "Given Data Already Exist!"}), mimetype="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def update_notification():
    fn = update_notification
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)
        schema_result=check_schema(request_data,update_notification_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        keys_to_check = ["active_role", "active_user", "id"]
        for key in keys_to_check:
            if request_data[key] == "":
                return Response(json.dumps({"status": "Empty value found for key '{}'".format(key)}), mimetype="application/json")

        query1 = "select * from notification_config_table where user_name='{}' and mobile_num='{}' and email_id='{}' and  category_type='{}' and loc_id='{}' and loc_name='{}' and ph_mail='{}' and  type='{}'  and  user_id !={}".format(request_data["user_name"],
                                                                                                                                                                                                                                        request_data["mobile_num"], request_data["email_id"],  request_data["category_type"],   request_data["loc_id"],     request_data["loc_name"],    request_data["ph_mail"], request_data["type"], request_data["user_id"])
      
        result_check = execute(4, query1)
        if result_check == () or result_check == None or result_check == []:
            query = "SELECT * FROM notification_config_table WHERE id = '{}'".format(
                request_data["id"])
            result = execute(1, query)

            if result == None or result == ():
                return Response(json.dumps({"status": "id doesnot exist"}), content_type="application/json")
            else:
                old_data=get_old_data(  request_data["id"],"notification_config_table")[0]
                audit_msg=generate_audit_message(request_data,old_data)
                insert_query = "UPDATE notification_config_table SET user_name='{}', mobile_num='{}', category_type='{}', email_id='{}',loc_id='{}',loc_name='{}',ph_mail='{}',type='{}',user_id='{}' WHERE id='{}'".format(
                    request_data["user_name"],
                    request_data["mobile_num"],
                    request_data["category_type"],
                    request_data["email_id"],
                    request_data["loc_id"],
                    request_data["loc_name"],

                    request_data["ph_mail"],
                    request_data["type"],
                    request_data["user_id"],
                    request_data["id"]


                )
          
            update_tbl(insert_query)
            audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
                request_data["active_user"],
                request_data["active_role"],
                datetime.now(),
                "Notification Management",
                audit_msg)
            
            update_tbl(audit_query)
            return Response(json.dumps({"status": "success"}), content_type="application/json")
        else:
            return Response(json.dumps({"status": "Given Data Already Exist!"}), mimetype="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def delete_notification():
    fn = 'delete_notification'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        request_data = json.loads(request.data)
     
        schema_result=check_schema(request_data,delete_notification_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        for key, value in request_data.items():
            if not value:
       
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
            
            
        query = "SELECT user_name , loc_name ,category_type  FROM notification_config_table WHERE id = '{}'".format(
            request_data["id"])
        result = execute(2, query)
      
        if result == None or result == ():
            return Response(json.dumps({"status": "id doesnt exist"}), content_type="application/json")
        else:

            delete_query = "DELETE FROM notification_config_table WHERE id='{}'".format(
                request_data["id"])
            update_tbl(delete_query)

            audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
                request_data["active_user"],
                request_data["active_role"],
                datetime.now(),
                "Notification Management",
                "location ({}) deleted in category ({})  mapped with user ({})".format(  result[0][1],  result[0][2],
                    result[0][0]),
            )

            update_tbl(audit_query)
            data = {"status": "success"}
            json_data = json.dumps(data)
            return Response(json_data, content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def add_field_person():
    fn = 'add_field_person'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)
        schema_result=check_schema(request_data,add_field_person_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        for key, value in request_data.items():
            if not value:
                return Response(json.dumps({"status": "Empty value found for key ({})".format(key)}), mimetype="application/json")

        query1 = "select * from field_person_table where name='{}'".format(
            request_data["name"])
        query2 = "select * from field_person_table where  phone_num='{}'".format(
            request_data["phone_num"])
        query3 = "select * from field_person_table where  mail_id='{}'".format(
            request_data["mail_id"])

        result1 = execute(2, query1)
        result2 = execute(2, query2)
        result3 = execute(2, query3)

        if len(result1) != 0 or result2 == None:
            return Response(json.dumps({"status": "name Already Exist"}), mimetype="application/json")
        if len(result2) != 0 or result2 == None:
            return Response(json.dumps({"status": "phone_num Already Exist"}), mimetype="application/json")
        if len(result3) != 0 or result2 == None:
            return Response(json.dumps({"status": "mail_id Already Exist"}), mimetype="application/json")

        insert_query = "INSERT INTO field_person_table (name,phone_num,mail_id) VALUES ( '{}', '{}', '{}')".format(

            request_data["name"],
            request_data["phone_num"],
            request_data["mail_id"],
        )

        update_tbl(insert_query)

        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
            request_data["active_user"],
            request_data["active_role"],
            datetime.now(),
            "notification_management",
            "field person ({}) added successfully".format(request_data["name"]),
        )

        update_tbl(audit_query)
        return Response(json.dumps({"status": "success"}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def field_persons_list():
    fn = "field_persons_list"
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        query = "select * from field_person_table"
        result = execute(2, query)

        if len(result) > 0:
            notification_list = []
            for row in result:
                noti_data = {
                    "id": row[0],
                    "name": row[1],
                    "phone_num": row[2],
                    "mail_id": row[3],


                }
                notification_list.append(noti_data)

            return Response(json.dumps({"status": "found", "details": notification_list}), content_type="application/json")
        else:

            return Response(json.dumps({"status": "not found"}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def get_site_type_config():
    fn = 'get_site_type_config'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        if not check_table("site_type_config_table"):
                return Response(json.dumps({"status": "not found!"}), content_type="application/json")
        query = "select * from site_type_config_table"
        result = get_key_value_pairs(query)

        if result == None or result == ():
            return Response(json.dumps({"status": "not found!"}), content_type="application/json")
        else:
            return Response(json.dumps({"status": "found", "details": result}), content_type="application/json")
        
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")


#serial
def get_serial_list():
    fn = 'get_serial_list'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        query = "select id,serial_num as ser_num, dev_model, dev_type, flag as status from dcu_serialnum_details"
        result = get_key_value_pairs(query)
      
        if result is None:
            return Response(json.dumps({"status": "not found", "details": []}), mimetype='application/json')

        final_details = []
        for idx in range(0, len(result)):
            row_info = result[idx]
            ser_num=result[idx]["ser_num"]
            
            if result[idx]["dev_type"]=="Modem":
                modem = get_key_value_pairs("select dcu_id from modem_link_tbl where router_id='{}'".format(ser_num))
                if modem==None:
                    ser_num="-"
                else:
                    ser_num=modem[0]["dcu_id"]
            
           
            query = "select dev_name from dev_tbl where dev_id='{}'".format(ser_num)
            dev_name_1 = execute(1, query)

            if dev_name_1 is not None:
                row_info["dev_name"] = dev_name_1[0]
            else:
                row_info["dev_name"] = ""

            if result[idx]["status"] == "true":
                row_info["status"] = "Configured"

            else:
                row_info["status"] = "Not Configured"
            final_details.append(row_info)
            
      

        return Response(json.dumps({"status": "found", "details": final_details}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def add_serial():
    fn = 'add_serial'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        request_data = json.loads(request.data)
        print(request_data)
        if request_data["dev_model"] == "MIM":
            request_data["dev_model"] = "MIM_1PORT"
        check_query="select * from dcu_serialnum_details where dev_type='{}'and serial_num='{}'".format(  request_data["dev_type"],  request_data["ser_num"])
        result=get_key_value_pairs(check_query)
        if result  not in [None,[]]  :
              return Response(json.dumps({"status": "Serial Number and DCU Type already exist !"}), mimetype='application/json')

        query = "INSERT INTO dcu_serialnum_details (serial_num, dev_model, dev_type) VALUES ('{}', '{}', '{}')".format(
            request_data["ser_num"],
            request_data["dev_model"],
            request_data["dev_type"]

        )
        update_tbl(query)

        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
            request_data["active_user"],
            request_data["active_role"],
            datetime.now(),
            "Serial & Obis Management",
            " serial number ({}) added successfully".format(
                request_data["ser_num"])
        )

        update_tbl(audit_query)
        return Response(json.dumps({"status": "success"}), mimetype='application/json')
    
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def delete_serial():
    fn = 'delete_serial'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)

        query = "SELECT * FROM dcu_serialnum_details WHERE id = '{}'".format(
            request_data["id"])
        result = execute(2, query)

        if result == None or result == ():
            return Response(json.dumps({"status": "id doesnt exist"}), content_type="application/json")

        delete_query = "DELETE FROM dcu_serialnum_details WHERE id='{}'".format(
            request_data["id"])
        
        update_tbl(delete_query)

        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
            request_data["active_user"],
            request_data["active_role"],
            datetime.now(),
            "Serial & Obis Management",
            " serial number ({}) deleted successfully".format(request_data["ser_num"]))

        update_tbl(audit_query)
        return Response(json.dumps({"status": "success"}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
#obis
def obis_getlist():
    fn = 'obis_getlist'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        query = "select id,obis_code as obis, param_name as def, type from obis_param_map"
        result = get_key_value_pairs(query)
        if result == None:
            return Response(json.dumps({"status": "not found", "details": []}), mimetype='application/json')

        for row in result:
            row["obis"] = row["obis"].replace("_", ".")

        return Response(json.dumps({"status": "found", "details": result}), content_type="application/json")
      
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def add_obis():
    fn = 'add_obis'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        request_data = json.loads(request.data)

        request_data["obis"] = request_data["obis"].replace(".", "_")

        query = "INSERT INTO obis_param_map (obis_code, param_name, type) VALUES ('{}', '{}', '{}')".format(
            request_data["obis"],
            request_data["def"],
            request_data["type"]

        )
        update_tbl(query)

        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
            request_data["active_user"],
            request_data["active_role"],
            datetime.now(),
            "Serial & Obis Management",
            "obis code ({}) added in ({}) successfully".format(
                request_data["obis"],  request_data["type"])
        )

        update_tbl(audit_query)
        return Response(json.dumps({"status": "success"}), mimetype='application/json')
      
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

def update_obis():
    fn = 'update_obis'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)
        request_data["obis"] = request_data["obis"].replace(".", "_")
        
        query = "SELECT * FROM obis_param_map WHERE id = '{}'".format(
                request_data["id"])

        result = execute(1, query)

        if result == None or result == ():
            return Response(json.dumps({"status": "id doesnot exist"}), content_type="application/json")
        else:
            
            old_data=get_old_data( request_data["id"],"obis_param_map")[0]
            audit_msg=generate_audit_message(request_data,old_data)

            request_data["obis"] = request_data["obis"].replace(".", "_")
            insert_query = "UPDATE obis_param_map SET obis_code='{}', param_name='{}',  type='{}' where id = {}".format(
                request_data["obis"],
                request_data["def"],
                request_data["type"],
                request_data["id"])
     
            update_tbl(insert_query)
            audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
                request_data["active_user"],
                request_data["active_role"],
                datetime.now(),
                "Serial & Obis Management",
               audit_msg)
            
            update_tbl(audit_query)
            return Response(json.dumps({"status": "success"}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def delete_obis():
    fn = 'delete_obis'

    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)
        request_data["obis"] = request_data["obis"].replace(".", "_")
        
        query = "SELECT * FROM obis_param_map WHERE id = '{}'".format(
            request_data["id"])
        result =get_key_value_pairs(query)

        if result == None or result == ():
            return Response(json.dumps({"status": "id doesnt exist"}), content_type="application/json")

        delete_query = "DELETE FROM obis_param_map WHERE id='{}'".format(
            request_data["id"])
       
        update_tbl(delete_query)

        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
            request_data["active_user"],
            request_data["active_role"],
            datetime.now(),
            "Serial & Obis Management",
            "obis code ({}) deleted in ({}) successfully".format(request_data["obis"], result[0]["type"]))

        update_tbl(audit_query)
        return Response(json.dumps({"status": "success"}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    

# Data view
def data_view_device_details():
    fn = "data_view_device_details()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")

        login_dict = json.loads(request.data)    
     
        main_dict= {}
        keys= ("loc_id", "loc_name", "dev_id", "dev_name","dev_comm_status", "dev_token")
        keys_2= ("met_id","met_name","ser_port_id","met_comm_status","met_sn")

        dev_details= []
        dev_result =  execute(2, "SELECT loc_id, loc_name, dev_id, dev_name, dev_common_status, dev_token  FROM dev_tbl WHERE loc_id='{}' AND loc_name ='{}' and dev_type ='DCU'".format(login_dict["loc_id"],login_dict["loc_name"] ))

        for dev in dev_result:
            dev_result
            device_details= dict(zip(keys, dev))
            met_details= []
            met_result =  execute(2, "SELECT meter_id, met_loc, ser_port_id, comn_status ,met_sn FROM dcu_dlms_meter_tbl WHERE dev_id='{}' and met_enable='Yes' order by meter_id".format(dev[2]))
            plcc = execute(2, "SELECT dev_model FROM dcu_serialnum_details WHERE serial_num = '{}'".format(dev[2]))
            
            for met_res in met_result:
                met_details.append(dict(zip(keys_2, met_res)))
       
            device_details["site_type"] = login_dict["site_type"]
            device_details["gen_type"] = login_dict["gen_type"]
            device_details["num_meters"] = len(met_result)
            device_details["met_details"] = met_details
            device_details["dev_model"] = plcc[0][0]
            dev_details.append(device_details)
  
        if dev_details == []:
            main_dict["status"] = "not found"
            main_dict["details"] = []
        else:
            main_dict["status"] = "found"
            main_dict["dev_details"] = dev_details
      
        return Response(json.dumps(main_dict), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
    
def dataview():
    fn = 'dataview'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
     
        request_data = json.loads(request.data)
        # print(request_data)
    
        schema_result=check_schema(request_data,dataview_schema)
        if schema_result!=True:
          
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        for key, value in request_data.items():
            if not value:
             
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
            
            
            
        dcu_name = dcufullname(request_data["dev_id"])
        dcu_summary=dcu_summary_info_dataview(request_data["dev_id"])
        # dcu_summary=dcu_summary_info_dataview(request_data["dev_id"])
      
        met_info=NamePlate_dataview(request_data["dev_id"],request_data["met_id"], request_data["ser_port_id"])
        fw_query = "select fw_ver from dev_tbl where dev_id= {}".format(request_data["dev_id"])
        fw_ver_name = execute(1, fw_query)
     
        if fw_ver_name == None:
            fw_ver = "-"
        else:
            fw_ver = fw_ver_name[0]
      
        if request_data["type"] == "Instantaneous":
          
            dev_common_status_query = "select dev_common_status from dev_tbl where dev_id= {}".format(request_data["dev_id"])
            dev_common_status=execute(1,dev_common_status_query)
      
            dcu_query = "select * from dcu_dlms_meter_tbl where dev_id= '{}' and met_enable= 'Yes' and comn_status !='Not Configured' and ser_port_id != '' and met_sn != ''".format(
                request_data["dev_id"])
            
            dcu_details = get_key_value_pairs(dcu_query)
            if dcu_details == None:
                return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
            print(dcu_details)
            last_dict = []
            for x in range(len(dcu_details)):
                if dcu_details[x]['comn_status']=="Not Communicating":
                    print("__________________________________________________________")
                    dcu_details[x]['comn_status']='Not Communicating'
                    dcu_details[x]['int_ct_ratio']= "-"
                    dcu_details[x]['int_pt_ratio']= "-"
                    row_dict = {}
                    row_dict["meter_details"] = dcu_details[x]
                    dcu_details[x]['comn_status']='Not Communicating'
                    last_dict.append(row_dict)
                  
                else:

                    table_name = "dcu_inst_data_{}_{}_{}".format(
                        request_data["dev_id"], dcu_details[x]['ser_port_id'], dcu_details[x]['meter_id'])


 
            
                    if check_table(table_name) == True:
                        latest_data_query = "SELECT * FROM {} ORDER BY id DESC LIMIT 1".format(
                            table_name)
                    
                        latest_data = get_key_value_pairs(latest_data_query)
                        print("__________________________________________________________",latest_data)
                        if latest_data == [] or latest_data == None:
                            
                            row_dict = {}
                            if   dcu_details[x]["int_ct_ratio"] !=None:
                                dcu_details[x]["int_ct_ratio"] = int(float(dcu_details[x]["int_ct_ratio"]))
                            if   dcu_details[x]["int_pt_ratio"] !=None:    
                                dcu_details[x]["int_pt_ratio"] = int(float(dcu_details[x]["int_pt_ratio"]))
                            row_dict["meter_details"] = dcu_details[x]

                            last_dict.append(row_dict)
                        
                        else:
                            
                            
                            data_dict = latest_data[0]
                            row_dict = {}
                            for key, value in data_dict.items():
                                if value == None or value == "":
                                    continue
                                    value = "-"
                                else:

                                    if isinstance(value, float):
                                        value = "{:.2f}".format(value)
                                    elif isinstance(value, str):
                                        split_value = value.split('.')
                                        if len(split_value) == 1:
                                            value = split_value[0]
                                        else:

                                            value = split_value[0] + \
                                                '.' + split_value[1][:2]
                                
                                obis_result = get_obis_name_optimized(
                                    key, "Instantaneous")

                                row_dict[obis_result] = value

                            row_dict["id"] = x+1
                            #int(float(num_str))
                            if   dcu_details[x]["int_ct_ratio"] !=None:
                                dcu_details[x]["int_ct_ratio"] = int(float(dcu_details[x]["int_ct_ratio"]))
                            if   dcu_details[x]["int_pt_ratio"] !=None:
                                dcu_details[x]["int_pt_ratio"] = int(float(dcu_details[x]["int_pt_ratio"]))
                           
                            row_dict["meter_details"] = dcu_details[x]
                            last_dict.append(row_dict)

                    else:
                        print("PRDIN")
                        row_dict = {}
                        dcu_details[x]["int_ct_ratio"] = "-"
                        dcu_details[x]["int_pt_ratio"] = "-"
                        row_dict["meter_details"] = dcu_details[x]
                        dcu_details[x]['comn_status']='Not Communicating'
                        last_dict.append(row_dict)
          
            return Response(json.dumps({"status": "found", "details": last_dict, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")

        elif request_data["type"] == "Load Survey":
            
            dcu_query = "select met_sn from dcu_dlms_meter_tbl where unique_id= '{}_{}_{}'".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
            met_sn = execute(1, dcu_query)
      
            if met_sn == None or  met_sn[0]=="" :
               met_sn="-"
            else:
                met_sn = met_sn[0]
            
            day, month_name, year = request_data["from_date"].split()
            to_date_str = f"{year}-{Month[month_name]}-{day}"
      
            table_name = "{}{}{}{}{}{}".format(
                "dcu_ls_data_", request_data["dev_id"], "_", request_data["ser_port_id"], "_", request_data["met_id"])
   
            query_block_intervel="select block_interval from dcu_dlms_meter_tbl where dev_id='{}' and ser_port_id='{}' and  meter_id='{}'".format(request_data["dev_id"],request_data["ser_port_id"],request_data["met_id"])
            result_blockinnterval= execute(1,query_block_intervel)
   
            if result_blockinnterval==None or result_blockinnterval[0]=="" :
                block_int=0
            else:
                block_int= int(result_blockinnterval[0])

            total_num_of_blocks = 0
            if block_int != 0:
                # if str(datetime.now().date())== to_date_str:
                #     current_time = datetime.now().time()

                #     minutes_since_midnight = current_time.hour * 60 + current_time.minute
                #     total_num_of_blocks = minutes_since_midnight // block_int
                # else:
                    total_num_of_blocks = (24*60)//block_int

            table_exist = check_table(table_name)
            if table_exist == True:
                next_day = (datetime.strptime(to_date_str, '%Y-%m-%d') + timedelta(days=1)).strftime('%Y-%m-%d')
                column_name = "0_0_1_0_0_255"
                type = "Load Survey"
     
                result = type_dataview(table_name, [to_date_str, next_day], column_name, type)
                
                if result == () or result == None or result == []:
                    return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"total_num_of_blocks":total_num_of_blocks,"num_of_blocks_available":0,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
               
            else:
                return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"total_num_of_blocks":total_num_of_blocks,"num_of_blocks_available":0,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
            # print(total_num_of_blocks,len(result)-3)
            # if total_num_of_blocks < (len(result)-3):
            #     # if block_int != 0:
                #     start_date = datetime(int(year),int(Month[month_name]),  int(day), 0, block_int, 0)
                #     date_list=[]
                #     for i in range(int(total_num_of_blocks)):
                #         interval_date = start_date + timedelta(minutes=i * block_int)
                #         formatted_date = interval_date.strftime('%Y-%m-%d %H:%M:%S')
                #         date_list.append(formatted_date)
                #     final_list=[]  
                #     for i in range (len(date_list) ):   
                #         missing_data = [item for item in result if item["DATE_TIME"]==date_list[i]]   
                #         if missing_data==[]:
                            
                #             data= {
                #                          "id":"",
                #                           "seq_num":"",
                #                           "update_time":"",
                #                           "block_time":"",
                #                           "DATE_TIME":date_list[i],
                #                           "EN_ACT_IMP":"",
                #                           "EN_ACT_EXP":"",
                #                           "1_0_10_29_0_255":"",
                #                           "EN_KVARH_Q1":"",
                #                           "EN_KVARH_Q2":"",
                #                           "EN_KVARH_Q3":"",
                #                           "EN_KVARH_Q4":"",
                #                           "CURR_1":"",
                #                           "CURR_2":"",
                #                           "CURR_3":"",
                #                           "VOLT_RN":"",
                #                           "VOLT_YN":"",
                #                           "VOLT_BN":"",
                #                           "FREQ":"",
                #                           "EN_NET_ACT":"",
                #                           "1_0_94_91_1_255":"",
                #                           "1_0_94_91_2_255":"",
                #                           "missing":"True"
                #                           }
                #             final_list.append(data)
                    
                #         else:
                #             missing_data[0]['DATE_TIME'] = date_list[i]
                #             missing_data[0]["missing"]="False"
                #             final_list.append(missing_data[0])
                
                #     # logger(json.dumps({"status": "found", "details": final_list, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"total_num_of_blocks":total_num_of_blocks,"num_of_blocks_available":len(result),"dcu_summary":dcu_summary,"met_info":met_info}))
                #     return Response(json.dumps({"status": "found", "details": final_list, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"total_num_of_blocks":total_num_of_blocks,"num_of_blocks_available":len(result),"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
            # else:
                # logger(json.dumps({"status": "found", "details": result, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"total_num_of_blocks":total_num_of_blocks,"num_of_blocks_available":len(result),"dcu_summary":dcu_summary,"met_info":met_info}))
            
            return Response(json.dumps({"status": "found", "details": result, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"total_num_of_blocks":total_num_of_blocks,"num_of_blocks_available":len(result),"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")

        elif request_data["type"] == "Event":
          
            # print(request_data)
            total_event=0
            dcu_query = "select met_sn from dcu_dlms_meter_tbl where unique_id= '{}_{}_{}'".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
            met_sn = execute(1, dcu_query)

            if met_sn == None or  met_sn[0]=="" :
               met_sn="-"
            else:
                met_sn = met_sn[0]
            
            table_name = "dcu_event_data_{}_{}_{}".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])

            if check_table(table_name) == True:
                total_event =0
                query = "select count(*) as total_count  from {}  order by ui_id".format(table_name)
                total_event=get_key_value_pairs(query)
               
                if total_event==[]:
                    total_event=0
                else:
                    total_event=total_event[0]["total_count"]
           
                day, month_name, year = request_data["from_date"].split()
                to_date_str = f"{year}-{Month[month_name]}-{day}"
                month = request_data["from_date"]
                column_name = "0_0_1_0_0_255"
                type = "Event"
      
                result = type_dataview(table_name, to_date_str,request_data["event_count"] , type)

                if result == () or result == None or result == []:
                    return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"next_data":"-","prev_data":"-","dcu_summary":dcu_summary,"met_info":met_info,"total_event":total_event}), content_type="application/json")
   
                
                if len(result)< int(request_data["event_count"]):
                        
                        next_data="false"
                        prev_data="false"
                        length_result=len(result)
                        previous_id=result[length_result-1]["ui_id"]
                        next_id=result[0]["ui_id"]
                else:
                        # gen_id_next=len(result)+1
                        # gen_id_prev=len(result)-1
                    
                        length_result=len(result)
                        previous_id=result[length_result-1]["ui_id"]
                        
                        print(previous_id)
                        next_id=result[0]["ui_id"]
                        next_data=next_pre(table_name,int(next_id)+1)
                        prev_data=next_pre(table_name,int(previous_id)-1)
                        
                        print("------ event pre next-----",previous_id,next_id)
                        print("------ event pre next-----",prev_data,next_data)
 

                if result == () or result == None or result == []:
                    return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"next_data":next_data,"prev_data":prev_data,"dcu_summary":dcu_summary,"met_info":met_info,"total_event":total_event}), content_type="application/json")
                else:
                    result=sorted(result, key=lambda x: datetime.strptime(x["DATE_TIME"], "%Y-%m-%d %H:%M:%S"), reverse=True)
                    return Response(json.dumps({"status": "found", "details": result, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"next_data":next_data,"prev_data":prev_data,"dcu_summary":dcu_summary,"met_info":met_info,"total_event":total_event,"next_id":next_id,"previous_id":previous_id}), content_type="application/json")
            else:
                return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"next_data":"-","prev_data":"-","dcu_summary":dcu_summary,"met_info":met_info,"total_event":total_event}), content_type="application/json")

        elif request_data["type"] == "MidNight":
       
            dcu_query = "select met_sn from dcu_dlms_meter_tbl where unique_id= '{}_{}_{}'".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
            met_sn = execute(1, dcu_query)

            if met_sn == None or  met_sn[0]=="" :
               met_sn="-"
            else:
                met_sn = met_sn[0]
            
    
            table_name = "dcu_daily_profile_data_{}_{}_{}".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
     
            if  check_table(table_name) == True:
                from_key = request_data["month_date"]
                month_name, year = from_key.split()
                month = Month[month_name]
                date_format = f"{year}-{month}"

                column_name = "0_0_1_0_0_255"
                type = "Daily Profile"
                result = type_dataview(
                    table_name, date_format, column_name, type)
                 
                print(result)
                if result == () or result == None or result == []:
                    return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
                else:
                    return Response(json.dumps({"status": "found", "details": result, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
            else:
                return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
        
        elif request_data["type"] == "Billing":
            dcu_query = "select met_sn from dcu_dlms_meter_tbl where unique_id= '{}_{}_{}'".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
            met_sn = execute(1, dcu_query)
            if met_sn == None:
                met_sn="-"
            else:
                met_sn = met_sn[0]
            
            table_name = "dcu_bill_data_{}_{}_{}".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
            
            if  check_table(table_name) == True:
                from_key = request_data["year_date"]
                column_name = "0_0_0_1_2_255"
                type = "Billing"
                result = type_dataview(
                    table_name, from_key, column_name, type)
        
                if result == () or result == None or result == []:
                    return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
                else:
                    return Response(json.dumps({"status": "found", "details": result, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
            else:
                return Response(json.dumps({"status": "found",  "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
            
        elif request_data["type"] == "NamePlate":
            qry="SHOW COLUMNS FROM dcu_dlms_meter_tbl LIKE 'load_status'"
            result_check=get_key_value_pairs(qry)
       
        
            if result_check!=None:
          
                query= "select dev_name as dcu_name,met_name,met_sn,met_manf_name,met_version,block_interval,int_ct_ratio,int_pt_ratio,comn_status as status,load_status as load_status from  dcu_dlms_meter_tbl where dev_id='{}' and meter_id='{}' and ser_port_id='{}'".format(request_data["dev_id"],request_data["met_id"],request_data["ser_port_id"])
                result=get_key_value_pairs(query)
            else:
             
                query= "select dev_name as dcu_name,met_name,met_sn,met_manf_name,met_version,block_interval,int_ct_ratio,int_pt_ratio,comn_status as status from  dcu_dlms_meter_tbl where dev_id='{}' and meter_id='{}' and ser_port_id='{}'".format(request_data["dev_id"],request_data["met_id"],request_data["ser_port_id"])
                result=get_key_value_pairs(query)
                result[0]["load_status"] ="test data"
                # query= "select dev_name as dcu_name,met_name,met_sn,met_manf_name,met_version,block_interval,int_ct_ratio,int_pt_ratio,comn_status as status from  dcu_dlms_meter_tbl where dev_id='{}' and meter_id='{}' and ser_port_id='{}'".format(request_data["dev_id"],request_data["met_id"],request_data["ser_port_id"])
                # result=get_key_value_pairs(query)
            print(result)
            if result !=None:    
                if result[0]["load_status"] == "1":
                    result[0]["load_status"] = "CONNECTED"
                else:
                    result[0]["load_status"] = "DISCONNECTED"   
               
            if result == () or result == None or result == []:
                    result=[{}]
                    result[0]["dcu_name"]=request_data["dev_name"]
                    return Response(json.dumps({"status": "found", "details": result[0], "info": request_data,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json")
            else:
                    result[0]["dcu_fullname"]=dcu_name  
                    return Response(json.dumps({"status": "found", "details": result[0], "info": request_data,"dcu_summary":dcu_summary,"met_info":met_info}), content_type="application/json") 
        else:
            return Response(json.dumps({"status": "no valid data type found!"}), content_type="application/json")
        
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")






def dataview_filter_location():
    fn = 'dataview_filter_location'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)
      
        schema_result=check_schema(request_data,dataview_filter_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        for key, value in request_data.items():
            if value =="":
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

        gen_length = len(request_data["gen_type"])

        if gen_length == 1:
            query = "SELECT id,loc_name,gen_type,site_type FROM loc_mgmt where gen_type='{}'".format(
                request_data["gen_type"][0])
        if gen_length == 2:
            query = "SELECT id,loc_name,gen_type,site_type FROM loc_mgmt where gen_type='{}' or gen_type='{}'".format(
                request_data["gen_type"][0], request_data["gen_type"][1])
           # print(query)
        if gen_length >= 3 or gen_length == 0:
            query = "select id,loc_name,gen_type,site_type from loc_mgmt where site_type='{}'".format(
                request_data["site_category"])
       # print(query)
        result = get_key_value_pairs(query)
       # print(result)

        if result == [] or result == None:
            return Response(json.dumps({"status": "not found"}), content_type="application/json")
        else:
            return Response(json.dumps({"status": "found", "details": result}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
    
def calender_view():
    fn="calender_view"
    try:
        request_data = json.loads(request.data) 
        month ,year=request_data["month"].split()
        current_date = datetime(int(year), int(Month[month]),1)
        
        target_date = datetime.strptime(request_data["month"], "%b %Y")


        resulting_months = []


        current_date = target_date


        for _ in range(request_data["count_month"]):
            resulting_months.append(current_date.strftime("%b %Y"))
    
            current_date -= relativedelta(months=1)

        
        table_name = "{}{}{}{}{}{}".format(
                "meter_data_avail_info_", request_data["dev_id"], "_", request_data["ser_port_id"], "_", request_data["met_id"])
        
        if check_table(table_name)==False:
            print("((((((((((((((((((((((((()))))))))))))))))))))))))")
            final_list=[]
            my_data=[]
        
            for i in range (len(resulting_months)):
          
                month_in_list,year_in_list=resulting_months[i].split()
                for m in range(1,int(day_count[month_in_list])+1):      
                        my_data.append(f"{year_in_list}-{Month[month_in_list].zfill(2)}-{str(m).zfill(2)}")
                        
                for j in range(1,int(day_count[month_in_list])+1): 
                    month_list=[]
                    for date in my_data:
                        current_date = datetime.now().date()
                        date_object = datetime.strptime(date, '%Y-%m-%d')
                        date_obj = datetime.strptime(date, "%Y-%m-%d").date()  # Convert string to date object
    
                        if date_obj > current_date:
                            future_date = "Yes"
                        else:
                            future_date = "No"
                        dict_1={
                            "date":str(date_object.day)+"_"+month_fullname[month_in_list]+"_"+year_in_list,
                            "data_avbl":"No",
                            "future_date":future_date,
                            "percent": 0
                        }
                        month_list.append(dict_1)
                    # print("wow",len(month_list))
                month_dict={
                            "month":month_fullname[month_in_list]+" "+year_in_list,
                            "details":month_list
                        }
                    # print(len(month_list))
                    
                final_list.append(month_dict)
                
                # print(len(final_list))
            return Response(json.dumps({"status": "found", "month_details":final_list}), content_type="application/json")
            #return Response(json.dumps({"status": "not found"}), content_type="application/json")
        
       
        final_list=[]
       
        for i in range (len(resulting_months)):
            month_in_list,year_in_list=resulting_months[i].split()
            query="SELECT * FROM `{}` WHERE DATE_FORMAT(`ls_date`, '%Y-%m') = '{}'".format(table_name,f"{year_in_list}-{Month[month_in_list].zfill(2)}")
            result=get_key_value_pairs(query)
           
            if result==None:
                print(resulting_months[i],"yes")
                month_data=[]
                for j in range(1,int(day_count[month_in_list])+1):
                    
                    
                    if is_future_date(int(j), int(Month[month_in_list]), int(year_in_list)):
                        future_date="Yes"
                    else:
                        future_date="No"
                    dict_1={
                            "date":str(j)+"_"+month_fullname[month_in_list]+"_"+year_in_list,
                            "data_avbl":"No",
                            "future_date":future_date ,
                            "percent": 0
                    }
                    month_data.append(dict_1)
                month_dict={
                        "month":month_fullname[month_in_list]+" "+year_in_list,
                        "details":month_data
                    }   
                final_list.append(month_dict)
                
            else:
              
                my_data=[]

                for i in range(1,int(day_count[month_in_list])+1):
                    my_data.append(f"{year_in_list}-{Month[month_in_list].zfill(2)}-{str(i).zfill(2)}")

                month_list=[]
                for date in my_data:
  
                    date_object = datetime.strptime(date, '%Y-%m-%d')
                    if is_future_date(int(date_object.day), int(Month[month_in_list]), int(year_in_list)):
                        future_date="Yes"
                    else:
                        future_date="No"
                        
                    found = False    
                    for entry in result:
                        if date in entry['ls_date']:                           
                            dict_1={
                            "date":str(date_object.day)+"_"+month_fullname[month_in_list]+"_"+year_in_list,
                            "data_avbl":"Yes",
                            "future_date":future_date ,
                            "percent": entry['day_per']}
                            found = True
                            break
                        
                        
                    if not found:   
                        dict_1={
                        "date":str(date_object.day)+"_"+month_fullname[month_in_list]+"_"+year_in_list,
                        "data_avbl":"No",
                        "future_date":future_date,
                        "percent": 0
                    }
                    month_list.append(dict_1)
                month_dict={
                        "month":month_fullname[month_in_list]+" "+year_in_list,
                        "details":month_list
                    }
                final_list.append(month_dict)
              
        return Response(json.dumps({"status": "found", "month_details":final_list}), content_type="application/json")
          
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")  
    
    
def event_previous_next():
    fn='event_previous_next'
    global ser_err, sec_code, db_conn, refresh_code
    try:
        request_data = json.loads(request.data)
        # print("-------------event pre next---------------------",request_data)
        fw_query = "select fw_ver from dev_tbl where dev_id= {}".format(request_data["dev_id"])
        fw_ver_name = execute(1, fw_query)
        dcu_summary=dcu_summary_info_dataview(request_data["dev_id"])
        met_info=NamePlate_dataview(request_data["dev_id"],request_data["met_id"], request_data["ser_port_id"])
        if fw_ver_name == None:
            fw_ver = ""
        else:
            fw_ver = fw_ver_name[0]
        
        dcu_name = dcufullname(request_data["dev_id"])
        
            
            
        dcu_query = "select met_sn from dcu_dlms_meter_tbl where unique_id= '{}_{}_{}'".format(
                request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
        met_sn = execute(1, dcu_query)
            
        if met_sn == None:
                return Response(json.dumps({"status": "found", "info": "meter serial number not found", "details": []}))
        met_sn = met_sn[0]
            
        table_name = "dcu_event_data_{}_{}_{}".format(request_data["dev_id"], request_data["ser_port_id"], request_data["met_id"])
        table_exist = check_table(table_name)

        if table_exist == True:
            query = "select count(*) as total_count  from {}  order by ui_id ".format(table_name)
            total_event=get_key_value_pairs(query)
            print(total_event)
            if total_event==None:
                total_event=0
            else:
                total_event=total_event[0]["total_count"]
                next_data     =0
                previous_data =0

            if  request_data["type_option"]=="Next":
                end_id=request_data["event_id"]+int(request_data["event_count"])
                gen_id=end_id+1
               
                query = "select * from {} where ui_id >'{}' AND ui_id <={} order by ui_id DESC limit {}".format(
                table_name, request_data["event_id"],end_id,request_data["event_count"])
                print("---------------next query---------------------------",query)
              
            if request_data["type_option"] =="Previous":
                start_id=request_data["event_id"]-int(request_data["event_count"])
                gen_id=start_id-1
              
                query = "select * from {} where ui_id >='{}' AND ui_id <{} order by ui_id DESC limit {}".format(
                    table_name,  start_id,request_data["event_id"],request_data["event_count"])
                
                print("----------------previous query---------------------------",query)
          
        
            result = get_key_value_pairs(query)

            
            next_id=result[0]["ui_id"]
            previous_id=result[len(result)-1]["ui_id"]
            next_data=next_pre(table_name,int(next_id)+1)
            previous_data=next_pre(table_name,int(previous_id)-1)
            


            if result == [] or result == None:
                    return Response(json.dumps({"status": "found", "details": [], "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"next_data":next_data,"prev_data":previous_data,"dcu_summary":dcu_summary,"met_info":met_info,"total_event":total_event}), content_type="application/json")
            else:
                    final_dict = []
            
                    for row in result:
                        row_dict = {}
                        for key, value in row.items():

                            obis_val = get_obis_name_optimized(key, "Event")
                
                            if obis_val != 'DATE_TIME':
                                if value == None or value == "" or value == "NA" or (value == "0.0" and obis_val=="EV_CODE"):
                                        continue
                                
                                # For other types (string, int, or datetime), process as before
                            if isinstance(value, (str, int)):
                                row_dict[obis_val] = value
                            else:
                                row_dict[obis_val] = value.strftime('%Y-%m-%d %H:%M:%S')
     
                        final_dict.append(row_dict)
            print("----pre next------",len(result))   
            print("prev",previous_id)
            print("next",next_id)
            final_dict=sorted(final_dict, key=lambda x: datetime.strptime(x["DATE_TIME"], "%Y-%m-%d %H:%M:%S"), reverse=True)
            return Response(json.dumps({"status": "found", "details": final_dict, "info": request_data, "fw": fw_ver, "dcu_fullname": dcu_name, "met_sn": met_sn,"next_data":next_data,"prev_data":previous_data,"dcu_summary":dcu_summary,"met_info":met_info,"total_event":total_event,"next_id":next_id,"previous_id":previous_id}), content_type="application/json")
    
         
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')   

# Commands

def on_demand_getlist():
    fn = 'on_demand_getlist'
    try:
        
        # header = request.headers.get('Authorization')
        # header = header.encode()
        # jwt.decode(header, sec_code, algorithms="HS256")
     
        request_data = json.loads(request.data)
        
        # print(request_data)
        
        
        # request_data["cmd_type"]="Meter Config Commands"
        # print("req data of getlist command ",request_data)
     
        schema_result=check_schema(request_data,get_list_command)

        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
        meter_commands = (
    'OD_TIMESYNC_MESSAGE',
    'OD_DEMAND_PERIOD_MESSAGE',
    'OD_PROF_CAP_PERIOD_MESSAGE',
    'OD_METERING_MODE_MESSAGE',
    'OD_PAYMENT_MODE_MESSAGE',
    'OD_LAST_TKN_RECH_AMT_MESSAGE',
    'OD_TOT_AMT_LAST_RECH_MESSAGE',
    'OD_CURR_BAL_AMT_MESSAGE',
    'OD_LOAD_LIMIT_EN_DIS_MESSAGE',
    'OD_SET_LOAD_MESSAGE',
    'OD_LOAD_CONN_DISCONN_MESSAGE',
    'OD_EN_DIS_RLY_OPER_MESSAGE',
    'OD_LAST_TKN_RECH_TIME_MESSAGE',
    'OD_CURR_BAL_TIME_MESSAGE',
    'OD_APN_MESSAGE',
    'OD_SNGL_ACT_SCHD_MESSAGE  ',
    'OD_PUSH_SETUP_MESSAGE',
    'OD_PUSH_ALERT_MESSAGE',
    'OD_ACT_CAL_MESSAGE',
    'OD_ESWF_MESSAGE',
	'OD_MD_RESET_MESSAGE'
)
        
        meter_commands = ",".join(f"'{cmd}'" for cmd in meter_commands)
        offset = (request_data['page_size'] - 1) * 15
        dev_comm_status="select dev_common_status from dev_tbl where dev_id='{}'".format(request_data["dev_id"])
        comm_status=execute(1,dev_comm_status)
        if comm_status==None:
            dev_status="-"
        else:
            dev_status=comm_status[0]
        
        if request_data["cmd_type"]=="Meter Config Commands":
            query = "SELECT * FROM dcu_command_tbl WHERE loc_id='{}' AND dev_id='{}' AND command IN ({}) ORDER BY ui_issue_time DESC LIMIT 20 OFFSET {}".format(
            request_data["loc_id"], request_data["dev_id"],meter_commands, offset)
            
            query1 = "select COUNT(*) AS total_count from dcu_command_tbl where loc_id='{}' and dev_id='{}' AND command IN ({})  ORDER BY ui_issue_time DESC".format(
            request_data["loc_id"], request_data["dev_id"],meter_commands)
            
        elif request_data["cmd_type"]=="Data Commands":
            
            query = "SELECT * FROM dcu_command_tbl WHERE loc_id='{}' AND dev_id='{}' AND command IN ('OD_INST_MESSAGE','OD_LS_MESSAGE','OD_MIDNIGHT_MESSAGE','OD_BILLING_MESSAGE','OD_EVENT_MESSAGE') ORDER BY ui_issue_time DESC LIMIT 20 OFFSET {}".format(
            request_data["loc_id"], request_data["dev_id"], offset)
            
            query1 = "select COUNT(*) AS total_count from dcu_command_tbl where loc_id='{}' and dev_id='{}' AND command IN ('OD_INST_MESSAGE','OD_LS_MESSAGE','OD_MIDNIGHT_MESSAGE','OD_BILLING_MESSAGE','OD_EVENT_MESSAGE') ORDER BY ui_issue_time DESC LIMIT 20 OFFSET {}".format(
            request_data["loc_id"], request_data["dev_id"],offset)
            
            
        elif request_data["cmd_type"]=="PLCC Commands":
            
            query = "SELECT * FROM dcu_command_tbl WHERE loc_id='{}' AND dev_id='{}' AND command IN ('OD_PLCC_NW_STATUS','OD_OVERALL_NET_SIZE','OD_OVERALL_DB_SIZE','LOAD_DISCONN','LOAD_CONN','NC_RESTART','DB_DELETE') ORDER BY ui_issue_time DESC LIMIT 20 OFFSET {}".format(
            request_data["loc_id"], request_data["dev_id"], offset)
            
            query1 = "select COUNT(*) AS total_count from dcu_command_tbl where loc_id='{}' and dev_id='{}' AND command IN ('OD_PLCC_NW_STATUS','OD_OVERALL_NET_SIZE','OD_OVERALL_DB_SIZE','LOAD_DISCONN','LOAD_CONN','NC_RESTART','DB_DELETE') ORDER BY ui_issue_time DESC LIMIT 20 OFFSET {}".format(
            request_data["loc_id"], request_data["dev_id"],offset)
            
            
        elif request_data["cmd_type"]=="General Commands":
            
            query = "SELECT * FROM dcu_command_tbl WHERE loc_id='{}' AND dev_id='{}' AND command IN ('DCU_RESTART') ORDER BY ui_issue_time DESC LIMIT 20 OFFSET {}".format(
            request_data["loc_id"], request_data["dev_id"], offset)
            
            query1 = "select COUNT(*) AS total_count from dcu_command_tbl where loc_id='{}' and dev_id='{}' AND command IN ('DCU_RESTART') ORDER BY ui_issue_time DESC LIMIT 20 OFFSET {}".format(
            request_data["loc_id"], request_data["dev_id"],offset)
        
        else:
            return Response(json.dumps({"status": "cant identify the type", "details": [],"dev_status":dev_status}), content_type="application/json") 
       
       
        # print(query)
        result = get_key_value_pairs(query)
        # print("===========getlist command===========",result)
        count = get_key_value_pairs(query1)
    
  
        total_count = count[0].get('total_count')
        if result!=None:
            
            for item in result:
                if item['time_taken_sec'] is not None:
                    item['time_taken_sec'] = to_mm_ss(float(item['time_taken_sec']))
                 
                item["issue_time"]=item["ui_issue_time"]
                
        else:
            return Response(json.dumps({"status": "not found", "details": [],"dev_status":dev_status}), content_type="application/json")

        if total_count != 0:
            offset = offset+1
            for i, item in enumerate(result, start=offset):
                item['row_index'] = i
                
        
      
        if result == () or result == None or result == []:
            return Response(json.dumps({"status": "not found", "details": [],"dev_status":dev_status}), content_type="application/json")
        else:     
            return Response(json.dumps({"status": "found", "details": result,"dev_status":dev_status,"total_count":total_count}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")



def add_demand():
    fn = 'add_demand'
    try:
        print("true")
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
 
        request_data = json.loads(request.data)
        print("========addd=============>>>>>>>>>>>>",request_data)
        if request_data['command_name']=="OD_EN_DIS_ON_OVERLOAD_OVERCURRENT_RLY_OPEARTION_MESSAGE":
            request_data['command_name']='OD_EN_DIS_RLY_OPER_MESSAGE'
        print(request_data)
        event_types = {
        0: "all",
        1: "voltage_related",
        2: "current_related",
        3: "power_related",
        4: "transaction_related",
        5: "other_tamper_related",
        6: "non_roll_over_events"
        }

        print(request_data["event_type"])
        event = event_types.get(int(request_data["event_type"]), "Invalid event number")
        set_time =request_data.get("time","")
        period=request_data.get("period","")
        mode=request_data.get("mode","0")
        amount=request_data.get("amount","")
        load_val=request_data.get("load_val","")
        limit_val=request_data.get("limit_val","")
        relay_val=request_data.get("relay_val","")
        apn_name=request_data.get("apn_name","")
        apn_bearer=request_data.get("apn_bearer","")
        val=request_data.get("val","")
 
        schema_result=check_schema(request_data,add_command_schema)
        if schema_result!=True:
 
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
       
       
        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
           
           
        if request_data["command_name"] not in ["DCU_RESTART", "NC_RESTART", "OD_OVERALL_NET_SIZE", "OD_OVERALL_DB_SIZE","OD_DELETE_DB",'OD_PLCC_NW_STATUS']:
           
            dev_status_query="select dev_common_status from dev_tbl where dev_id='{}'".format(request_data["dev_id"])
            dev_status=execute(1,dev_status_query)
            if dev_status==None:
                return Response(json.dumps({"status": "Can't Identify  Device Communicating Status!"}), mimetype='application/json', status=200)
            elif dev_status[0]!="Communicating":
                return Response(json.dumps({"status": "Selected Device name is Not Communicating . Please choose Communicating Device name!"}), mimetype='application/json', status=200)
               
 
            com_query="select  comn_status  from  dcu_dlms_meter_tbl where meter_id='{}' and dev_id='{}'and ser_port_id='{}'".format(request_data["met_id"],request_data["dev_id"],request_data["ser_port_id"])
       
            com_status=execute(1,com_query)  
            if com_status==None:
                return Response(json.dumps({"status": "Selected meter name is None type. Please choose Communicating meter name!"}), mimetype='application/json', status=200)
            if com_status[0]=="Not Communicating":
                return Response(json.dumps({"status": "Selected meter name is Not Communicating. Please choose Communicating meter name !"}), mimetype='application/json', status=200)
 
 
        qry = "SELECT `dev_token`, `fw_ver` FROM `dev_tbl` WHERE dev_id = '{}'".format(request_data["dev_id"])
        res_dev = execute(1, qry)
        res = execute(1, "SELECT seq_num FROM dcu_command_tbl ORDER BY seq_num desc")
 
        if res != None:
            if len(res) != 0:
                seq_no = int(res[0]) + 1
            else:
                seq_no = 1000
        else:
            seq_no = 1000

        return_query = "SELECT command_status,command,user_name,dev_name FROM dcu_command_tbl WHERE dev_id = '{}' and command_status  LIKE '%In Progress%' ".format(request_data["dev_id"])
        result_return_query = get_key_value_pairs(return_query)
        print(result_return_query)
        if result_return_query != None:
            if "In Progress" in result_return_query[0]["command_status"]:
                command_name = result_return_query[0]["command"]
                device_name = result_return_query[0]["dev_name"]
                if result_return_query[0]["user_name"] == "DCU CMD BACKEND":
                    return Response(json.dumps({"status": "AUTO OD COMMAND is in progress"}), content_type="application/json")
                else:
                    return Response(json.dumps({"status": " '{}' Command is currently in progress for the device ({}). Please wait until the operation completes.".format(command_name,device_name)}), content_type="application/json")
 

 
        if request_data["command_name"] == "OD_LS_MESSAGE" :
            date1, month_name,  year = request_data["from_date"].split()
            month = Month[month_name]
            from_date = f"{date1}_{month}_{year}"
        elif request_data["command_name"] == "OD_BILLING_MESSAGE" or request_data["command_name"] == "OD_MIDNIGHT_MESSAGE":
            if request_data["range_type"] == "ALL":
                from_date = "all"
            else:
                month_name,  year = request_data["from_month"].split()
                month = Month[month_name]
                from_date = f"{month}_{year}"
        elif request_data["command_name"] == "OD_EVENT_MESSAGE":
            if request_data["range_type"] == "ALL":
                from_date = "all"
            else:
                date1, month_name,  year = request_data["from_date"].split()
                month = Month[month_name]
                from_date = f"{date1}_{month}_{year}"
        else:
            from_date = "-"
 
        # if request_data["command_name"] == "OD_ACT_CAL_MESSAGE":
        #   val =json.dumps(val)
        send_data = {

            "TYPE": "OD_MESSAGE",
            "SEQ_NUM": str(seq_no),
            "CMD_TYPE": request_data["command_name"],
            "DATA": {"DEVTOKEN": request_data["dev_token"], "DCU_ID": request_data["dev_id"], "MET_ID": request_data["met_id"],  "LS_DATE": from_date,
                    "PORT_ID": "1","event_type": event,"MET_SERIAL_NUM": request_data["met_sn"],"SET_TIME":set_time,"PERIOD":period,"MODE":mode,"AMOUNT":amount,"LOAD_VAL":load_val,
                    "LIMIT_VAL":limit_val,"RLY_VAL":relay_val,"APN_NAME":apn_name,"VAL":val,"APN_BEARER":apn_bearer
                    }}#str(request_data["ser_port_id"]
        
            
        # else:
        #     send_data = {

        #         "TYPE": "OD_MESSAGE",
        #         "SEQ_NUM": str(seq_no),
        #         "CMD_TYPE": request_data["command_name"],
        #         "DATA": {"DEVTOKEN": request_data["dev_token"], "DCU_ID": request_data["dev_id"], "MET_ID": request_data["met_id"],  "LS_DATE": from_date,
        #                 "PORT_ID": "1","MET_SERIAL_NUM": request_data["met_sn"] }}#str(request_data["ser_port_id"]
        print(send_data)
        query = "INSERT INTO dcu_command_tbl(seq_num,command,loc_id,dev_id,met_id,issue_time,ui_issue_time,user_role,user_name,send_data,loc_name,dev_name,met_name,retry,command_status,ser_port_id,LS_DATE,met_sn)  VALUES('{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}','{}')".format(
                seq_no,  request_data["command_name"], request_data["loc_id"], request_data["dev_id"], request_data["met_id"], datetime.now().strftime('%Y-%m-%d %H:%M:%S'), datetime.now().strftime('%Y-%m-%d %H:%M:%S'),request_data["active_role"], request_data["active_user"], json.dumps(send_data), request_data["loc_name"], request_data["dev_name"], request_data["met_name"], 0, "Command Issued", request_data["ser_port_id"], request_data["from_date"],request_data["met_sn"])
        
        print("query======================",query)
        update_tbl(query)
 
        if request_data['command_name'] == "GET_OLD_DATA":
 
            today = datetime.today() + timedelta(days=1)
            st_date = datetime.today() - timedelta(days=9)
 
            for single_date in daterange(st_date, today):
         
                ct_date = single_date.strftime("%d_%m_%Y")
                send_dict = {
 
                    "TYPE": "OD_MESSAGE",
 
                    "SEQ_NUM": str(seq_no),
 
                    "DATA": {
 
                        "CMD_TYPE": "OD_LS_MESSAGE",
                        "MET_ID": request_data['met_id'],
                        "LS_DATE": ct_date
                    },
 
                    "NP": {
 
                        "DEV_TOKEN": res_dev[0],
 
                        "LOC": request_data['loc_name'],
 
                        "FW_VER": res_dev[1],
 
                        "SN": request_data["dev_id"],
 
                        "DEV_SN": request_data["dev_id"],
 
                        "TS": datetime.now().strftime('%Y-%m-%d %H:%M:%S')
 
                    }
                }
 
                qry = "INSERT INTO dcu_command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data, met_id) VALUES ('{}', {}, {}, '{}', 'issued', 0, '{}', '{}')".format(
 
                    request_data['command_name'], seq_no, request_data["dev_id"], datetime.now().strftime('%Y-%m-%d %H:%M:%S'), json.dumps(send_dict), request_data["met_id"])
 
 
                update_tbl(qry)
 
                seq_no += 1
 
        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
 
            request_data["active_user"],
 
            request_data["active_role"],
 
            datetime.now(),
 
            "On Demand Commands",
 
            "({}) issued to  device ({}) successfully".format(request_data["command_name"],request_data["dev_name"]),
 
        )
 
        update_tbl(audit_query)
 
        return Response(json.dumps({"status": "success"}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %  (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")


def getlist_setconfig():
    fn='set_config_dcu'
    try:
        request_data = json.loads(request.data) 
        
       
        res = "SELECT command_status FROM command_tbl where seq_num='{}' and dev_id='{}'".format(request_data["seq_num"],request_data["dev_id"])
        command_status=execute(1,res)
     
        if command_status==None:
                  return Response(json.dumps({"status": "found","cmd_status":""}), content_type="application/json")
                
      
        return Response(json.dumps({"status": "found","cmd_status":command_status[0]}), content_type="application/json")
        
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")  
    
def add_set_config():
    fn='add_set_config'
    try:
        request_data = json.loads(request.data) 

        res = execute(1, "SELECT seq_num FROM command_tbl ORDER BY seq_num desc")

        if res != None:
            if len(res) != 0:
                seq_no = int(res[0]) + 1
            else:
                seq_no = 1000
        else:
            seq_no = 1000
        
 

        # send_data = {
        #     "TYPE": "CYCLIC_MESSAGE",
        #     "SEQ_NUM": request_data["seq_num"],
        #     "DATATYPE": "",
        #     "NP": {
        #         "DEVTOKEN": np_value[0]["dev_token"],
        #         "SN": np_value[0]["dev_sn"],#dev_sn
        #         "IMEI": np_value[0]["imei"],
        #         "FW_VER": np_value[0]["fw_ver"],
        #         "TS": "2025-04-04 10:48:54"
        #     },
        #     "DATA": {
        #         "NUM_METERS": str(length_data),
        #         "PARAM_LIST": [
        #             "meter_id",
        #             "meter_ser",
        #             "meter_loc"
        #         ],
        #         "VAL_LIST": list_of_lists
        #     }
        # }
 
        query_command = "INSERT INTO command_tbl (seq_num,command,dev_id,issue_time,user_role,user_name,send_data,retry,command_status)  VALUES('{}','{}','{}','{}','{}','{}','{}','{}','{}')".format(
        seq_no, request_data["command_name"], request_data["dev_id"], datetime.now().strftime('%Y-%m-%d %H:%M:%S'), request_data["active_role"], request_data["active_user"], "-", 0, "issued")
            
        print("==============================command===================",query_command)
        update_tbl(query_command)
            
        query = "UPDATE dev_tbl SET seq_num = '{}' WHERE dev_id = '{}' ".format(seq_no,request_data["dev_id"])
        update_tbl(query)
    
        
        audit_query = "INSERT INTO audit_table (active_user, active_role, date_time, audit_type, audit_text) VALUES ('{}', '{}', '{}', '{}', '{}')".format(
        request_data["active_user"],
        request_data["active_role"],
        datetime.now(),
        "Device Management",
        "({}) added successfully".format(request_data["command_name"]),
        )

        update_tbl(audit_query)
        return Response(json.dumps({"status": "success","seq_num":seq_no}), content_type="application/json")
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")   

def view_response():
    fn = 'view_response'
    try:
        # header = request.headers.get('Authorization')
        # header = header.encode()
        # jwt.decode(header, sec_code, algorithms="HS256")
        request_data = json.loads(request.data)
        print("----------------response view --------------",request_data)
        schema_result=check_schema(request_data,view_resp_schema)
        if schema_result!=True:

            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        
        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

       
        request_data = json.loads(request.data)
        
        resp_keys_of_commands = {
        'OD_OVERALL_DB_SIZE': 'NC_SIZE',
        'OD_OVERALL_NET_SIZE': 'NW_SIZE',
        'OD_TIMESYNC_MESSAGE': 'SET_TIME',
        'OD_DEMAND_PERIOD_MESSAGE':'DEMAND_PERIOD',
        'OD_PROF_CAP_PERIOD_MESSAGE':'CAPTURE_PERIOD',
        'OD_METERING_MODE_MESSAGE':"METERING_MODE",
        'OD_PAYMENT_MODE_MESSAGE':'PAYMENT_MODE',
        'OD_LAST_TKN_RECH_AMT_MESSAGE':'LST_TKN_RECHR_AMT',
        'OD_TOT_AMT_LAST_RECH_MESSAGE':'TOT_AMT_LST_RECHR',
        'OD_CURR_BAL_AMT_MESSAGE':'CUR_BAL_AMT',
        'OD_LOAD_LIMIT_EN_DIS_MESSAGE':'LOAD_LMT_ENBL_DISAB',
        'OD_SET_LOAD_MESSAGE' : 'LIMIT_VAL',
        'OD_LOAD_CONN_DISCONN_MESSAGE' : 'LOAD_CONN_DISCON',
        'OD_EN_DIS_RLY_OPER_MESSAGE' : 'RELAY_OPER_VAL',
        "OD_LAST_TKN_RECH_TIME_MESSAGE": "LAST_TKN_RECH_TIME",
        "OD_CURR_BAL_TIME_MESSAGE": "CURR_BAL_TIME",
        "OD_APN_MESSAGE": "APN_NAME",
        "OD_SNGL_ACT_SCHD_MESSAGE": "SNGL_ACT_SCHD_BILL",
        "OD_PUSH_SETUP_MESSAGE": "PUSH_SETUP_DEST_ADDR",
        "OD_PUSH_ALERT_MESSAGE": "PUSH_ALERT_DEST_ADDR",
        "OD_ACT_CAL_MESSAGE": "ACTIVITY_CAL",
        'OD_MD_RESET_MESSAGE':'MD_RESET_CAL',
        'OD_ESWF_MESSAGE':'ESWF_CAL',
        'DCU_RESTART':"dcu"
    }
        command_list = [
    'OD_OVERALL_DB_SIZE',
    'OD_OVERALL_NET_SIZE',
    'OD_TIMESYNC_MESSAGE',
   'OD_DEMAND_PERIOD_MESSAGE',
    'OD_PROF_CAP_PERIOD_MESSAGE',
    'OD_METERING_MODE_MESSAGE',
    'OD_PAYMENT_MODE_MESSAGE',
    'OD_LAST_TKN_RECH_AMT_MESSAGE',
    'OD_TOT_AMT_LAST_RECH_MESSAGE',
    'OD_CURR_BAL_AMT_MESSAGE',
    'OD_LOAD_LIMIT_EN_DIS_MESSAGE',
    'OD_SET_LOAD_MESSAGE',
    'OD_LOAD_CONN_DISCONN_MESSAGE',
    'OD_EN_DIS_RLY_OPER_MESSAGE',
    'OD_LAST_TKN_RECH_TIME_MESSAGE',
    'OD_CURR_BAL_TIME_MESSAGE',
    'OD_APN_MESSAGE',
    'OD_SNGL_ACT_SCHD_MESSAGE',
    'OD_PUSH_SETUP_MESSAGE',
    'OD_PUSH_ALERT_MESSAGE',
    'OD_ACT_CAL_MESSAGE',
    'OD_ESWF_MESSAGE',
	'OD_MD_RESET_MESSAGE',
 'DCU_RESTART'
]



        query1 = "select exec_time  from dcu_command_tbl where seq_num='{}' and command='{}'".format(
            request_data["seq_num"],  request_data["command"])
        exec_time_list = get_key_value_pairs(query1)

        met_SN = "select met_sn from dcu_dlms_meter_tbl where dev_id={} and meter_id='{}'".format(
            request_data["dev_id"], request_data["met_id"])
        meter_sn_list = get_key_value_pairs(met_SN)

        if meter_sn_list == None or meter_sn_list[0]["met_sn"] == None:

            meter_sn = " - "
        else:
            meter_sn = meter_sn_list[0]["met_sn"]

        if exec_time_list == [] or exec_time_list == None:

            exec_time = "00:00:00"
        else:
            exec_time = exec_time_list[0]["exec_time"]

        if request_data["command"] == "OD_MIDNIGHT_MESSAGE":

            table_name = "dcu_daily_profile_od_tbl_" + \
                request_data["dev_id"]+"_" + (request_data["ser_port_id"]) + \
                "_" + (request_data["met_id"])

            com_type = "Daily Profile"

            column = OD_MIDNIGHT_MESSAGE

        elif request_data["command"] == "OD_BILLING_MESSAGE":

            table_name = "dcu_bill_od_tbl_" + \
                request_data["dev_id"]+"_" + request_data["ser_port_id"] + \
                "_" + request_data["met_id"]
            com_type = "Billing"
            column = OD_BILLING_MESSAGE

        elif request_data["command"] == "OD_EVENT_MESSAGE":

            table_name = "dcu_event_od_tbl_" + \
                request_data["dev_id"]+"_" + request_data["ser_port_id"] + \
                "_" + request_data["met_id"]
            com_type = "Event"

            column = OD_EVENT_MESSAGE

        elif request_data["command"] == "OD_INST_MESSAGE":

            table_name = "dcu_inst_od_tbl_" + \
                request_data["dev_id"]+"_" + request_data["ser_port_id"] + \
                "_" + request_data["met_id"]
            com_type = "Instantaneous"
            column = OD_INST_MESSAGE

        elif request_data["command"] == "OD_LS_MESSAGE":

            table_name = "dcu_ls_od_tbl_" + \
                request_data["dev_id"]+"_" + request_data["ser_port_id"] + \
                "_" + request_data["met_id"]
            com_type = "Load Survey"
            column = OD_LS_MESSAGE
           
        elif request_data["command"] in command_list:
         
            result=get_key_value_pairs("select exec_time,resp_data from dcu_command_tbl where seq_num='{}'".format( request_data["seq_num"]))
            print(type(result[0]["resp_data"]))
            print("----re-----",result[0]["resp_data"])
            resp_data=result[0]["resp_data"]
            if resp_data==None:
                  return Response(json.dumps({"status": "No Response Data Found !"}), content_type="application/json")
          
            parsed_json =  json.loads(resp_data)
            print("=======dddd====",parsed_json)
            command_response=parsed_json["DATA"].get(resp_keys_of_commands[request_data["command"]],"")
            print("==========res============>>>>>>>>>>>>>>>>>>",command_response)
             
                       
            command_dict = {

                        "status": "found",
                        "exec_time": result[0]["exec_time"],
                        "details":command_response
                    }
          
            print(resp_keys_of_commands[request_data["command"]])
            print("---resppppppppppppp",command_dict)
            return Response(json.dumps(command_dict), content_type="application/json")
        
     
         
        elif request_data["command"] ==  "LOAD_CONN" or  request_data["command"] == "LOAD_DISCONN" :
            print("load conn")
            result=get_key_value_pairs("select exec_time,resp_data,dev_id,ser_port_id,met_id from dcu_command_tbl where seq_num='{}'".format( request_data["seq_num"]))
           
            DEV_ID = result[0]["dev_id"]
            SER_PORT_ID = result[0]["ser_port_id"]
            MET_ID = result[0]["met_id"]
            
            load_status = get_key_value_pairs("select load_status from dcu_dlms_meter_tbl where dev_id='{}' and ser_port_id='{}' and meter_id='{}'".format(DEV_ID,SER_PORT_ID,MET_ID))
            
            
            if load_status == "1":
                load_status = "CONNECTED"
            else:
                load_status = "DISCONNECTED"
           
            command_dict = {

                    "status": "found",
                    "exec_time": result[0]["exec_time"],
                    "details":load_status
            }
            print("+++++++++++++++++++++++++++++++",command_dict)
            return Response(json.dumps(command_dict), content_type="application/json")  
         
        elif request_data["command"] == "OD_PLCC_NW_STATUS":
            
            if not check_table("dcu_dlms_meter_tbl"):
                meter_info=[{
             "online_count":"-",
             "offline_count":"-",
             "not_configured_count":"-",
             "total_count":"-"
             
             }]
            
            else:
                query = """
                SELECT 
                CAST(SUM(CASE WHEN comn_status = 'Communicating' THEN 1 ELSE 0 END) as UNSIGNED) AS online_count,
                CAST(SUM(CASE WHEN comn_status = 'Not Communicating' THEN 1 ELSE 0 END) as UNSIGNED)AS offline_count,
                CAST(SUM(CASE WHEN comn_status = 'Not Configured' THEN 1 ELSE 0 END) as UNSIGNED) AS not_configured_count,
                CAST(SUM(CASE WHEN comn_status = 'Configured' THEN 1 ELSE 0 END) as UNSIGNED) AS configured_count,
                COUNT(*) AS total_count
                FROM dcu_dlms_meter_tbl
                WHERE dev_id = '{}'
                """.format(request_data.get("dev_id"))
                meter_info=get_key_value_pairs(query)
                meter_info[0]["configured_count"]=int( meter_info[0]["total_count"]) - int(meter_info[0]["not_configured_count"] )
                # meter_info[0]["offline_count"]="-"
                
        
            if not check_table("plcc_met_table"):
                
                return {"status": "not found","count":"0","details":[],"info":meter_info[0]}
            
            
            query="select * from plcc_met_table where dev_id='{}'".format(request_data.get("dev_id"))
          
            total_count=get_key_value_pairs(query)
            if total_count==None:
                return {"status": "not found","count":"0","details":[],"info":meter_info[0]}
        
            
            # query="select id,nc_id ,dev_id,rs_node_id,rs_parent_id,rs_status,rs_sn,mapped_met_sn,met_id,signal_qualityfrom plcc_met_table where dev_id='{}' order by id asc limit 20 offset {}".format(input.get("dev_id"),start)
            query="SELECT id, nc_id, dev_id, rs_node_id, rs_parent_id, CASE WHEN rs_status = 1 THEN 'Online' WHEN rs_status = 0 THEN 'Offline' WHEN rs_status = 2 THEN 'Poor Online' ELSE 'unknown' END AS rs_status, rs_sn, mapped_met_sn, met_id, signal_quality FROM plcc_met_table WHERE dev_id = '{}' ORDER BY id ".format(request_data.get("dev_id"))

            result=get_key_value_pairs(query)
          

            
            qry="SHOW COLUMNS FROM plcc_nc_table LIKE 'updated_time'"
            check_column=get_key_value_pairs(qry)
            
            
            
            if check_column!=None:
                #COUNT(*) AS total_count
                query="select num_meters as total_count,db_size ,  nw_size,updated_time  from plcc_nc_table where dev_id='{}'".format(request_data.get("dev_id"))
                size=get_key_value_pairs(query)
            else:
                query="select num_meters as total_count,db_size ,  nw_size from plcc_nc_table where dev_id='{}'".format(request_data.get("dev_id"))
              
                size=get_key_value_pairs(query)
                # size[0]["updated_time"]="test time"
                

            if size!=None or size!=[]:
            
                meter_info[0]["db_size"]=size[0]["db_size"]
                meter_info[0]["net_size"]=size[0]["nw_size"]
                meter_info[0]["updated_time"]=size[0].get("updated_time","dummy time polled")
                meter_info[0]["configured_count"]=size[0]["total_count"]
            column = PLCC_ONDEMAND_MESSAGE
         
            command_dict = {

                    "status": "found",
                    "command":  request_data["command"],
                    "exec_time": exec_time,
                    "column_details": column,
                    "details": result,
                    "dev_sn": request_data["dev_id"]

                }
            print("command_dict",command_dict)
            if result==None:
                return {"status": "not found","details":result}
            
        
            return {

                    "status": "found",
                    "command":  request_data["command"],
                    "exec_time": exec_time,
                    "column_details": column,
                    "details": result,
                    "dev_sn": request_data["dev_id"]


                }

        
        
      
        if not check_table(table_name):
            return Response(json.dumps({"status": "table not found !"}), mimetype="application/json")
        
        
        if request_data["command"] == "OD_LS_MESSAGE":
            
            date_str = request_data["from_date"]
            date_obj = datetime.strptime(date_str, "%d %b %Y")
            next_date = date_obj + timedelta(days = 1)
            date_1 = date_obj.strftime("%Y-%m-%d") 
            
            query2 = "select * from {} where seq_num ='{}' AND DATE(0_0_1_0_0_255) = '{}' ORDER BY 0_0_1_0_0_255 ASC  ".format(
            table_name, request_data["seq_num"],date_1)
            print(query2 )
        elif request_data["command"]!= "OD_INST_MESSAGE" and request_data["command"]!= "OD_BILLING_MESSAGE":
            query2 = "select * from {} where seq_num='{}' ORDER BY 0_0_1_0_0_255 ASC".format(
            table_name, request_data["seq_num"])
            print(query2 )
        else:
            query2 = "select * from {} where seq_num='{}'".format(
            table_name, request_data["seq_num"])
        
       
        print("-----------yyyy-------------------------",query2)
        command_details = get_key_value_pairs(query2)
        
        
     

        if command_details == [] or command_details == None:
            command_dict = {

                "status": "not found",
                "command":  request_data["command"],
                "exec_time": exec_time,
                "ser_port_id": request_data["ser_port_id"],
                "column_details": column,
                "details": []
            }

            # print(command_dict)

            return Response(json.dumps(command_dict), content_type="application/json")

        else:
            view = []
            # final_dict = {}
            for row in command_details:
                row_dict = {}
                for key, value in row.items():
                 

                    obis_name = get_obis_name_optimized(key, com_type)
                    if com_type=="Load Survey" and obis_name=='DATE_TIME':
                          obis_name='BLOCK_TIME'

                    if isinstance(value, datetime):
                        row_dict[obis_name] = value.strftime(
                            '%Y-%m-%d %H:%M:%S')
                    
                    elif isinstance(value, date):
                        row_dict[obis_name] = value.strftime(
                            '%Y-%m-%d')
                    else:
                        
                        if com_type=="Load Survey" and   obis_name=='BLOCK_TIME':
                            parsed_time = datetime.strptime(value, "%Y-%m-%d %H:%M:%S")
                            time_part=parsed_time.strftime("%H:%M:%S")
                           
                            row_dict[obis_name]=time_part
                        else:
                            if value == None or value == "" or value == "NA" or (value == "0.0" and obis_name=="EV_CODE"):
                                continue
                            row_dict[obis_name]=value

                    row_dict = row_dict

                    row_dict["MET_SER_NUM"] = meter_sn
                    row_dict["SN"] = request_data["dev_id"]
             
                view.append(row_dict)

            if request_data["command"] == "OD_LS_MESSAGE":

                command_dict = {

                    "status": "found",
                    "command":  request_data["command"],
                    "exec_time": exec_time,

                    "column_details": column,
                    "details": view,
                    "date": request_data["from_date"]

                }

            else:

                command_dict = {

                    "status": "found",
                    "command":  request_data["command"],
                    "exec_time": exec_time,

                    "column_details": column,
                    "details": view

                }
        print("command_dict",command_dict)
        return Response(json.dumps(command_dict), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
def config_status():
    fn = "config_status()"
    global ser_err, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")
    
        post_dict = json.loads(request.data)

        schema_result=check_schema(post_dict,modem_diag_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        
        for key, value in post_dict.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

        if post_dict["details"]["cmd_type"] == 'Download Configuration':
            cmd_type = 'DOWNLOAD_CONFIG'
            tbl= "command_tbl"
            
        elif post_dict["details"]["cmd_type"] == 'Upload Configuration':
            cmd_type = 'UPLOAD_CONFIG'
            tbl= "command_tbl"
            
        elif post_dict["details"]["cmd_type"] == 'Upgrade Firmware':
            cmd_type = 'UPLOAD_FIRMWARE'
            tbl= "command_tbl"
            
        elif post_dict["details"]["cmd_type"] == 'RESET' or post_dict["details"]["cmd_type"] == 'RESTART':  # Restart Gateway
            cmd_type = 'RESET'
            tbl= "command_tbl"
        
        elif post_dict["details"]["cmd_type"] == 'ENABLE_FTP' or post_dict["details"]["cmd_type"] == 'DISABLE_FTP':
            
            tbl= "dcu_command_tbl"
            cmd_type = 'FTP_CFG'
        
        elif post_dict["details"]["cmd_type"] == 'ENABLE_LOGS' or post_dict["details"]["cmd_type"] == 'DISABLE_LOGS':
            cmd_type = 'SET_LOG_LEVEL'
            tbl= "dcu_command_tbl"
            
        else:
            cmd_type = post_dict["details"]["cmd_type"]
            tbl= "dcu_command_tbl"

        res = execute(1, "SELECT command_status, issue_time FROM `{}` where seq_num = '{}' and command = '{}'".format(tbl, post_dict['seq_num'], cmd_type))
   

        if res != None:
            sts = res[0]

            if sts == 'SUCCESS':
                post_dict['status'] = 'success'
            elif sts == 'command_issued':
                now = datetime.now() - \
                    datetime.strptime(str(res[1]), "%Y-%m-%d %H:%M:%S")
                if now.total_seconds() > 120:
                    post_dict['status'] = 'FAILED. (Command Not Processed By Gateway)'
                else:
                    post_dict['status'] = 'in_progress'
            else:
                post_dict['status'] = sts

        else:
            post_dict['status'] = 'Command not found'

        return Response(json.dumps(post_dict), mimetype='application/json')
   
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')

#diag
def modem_diagnostic():
    fn = 'modem_diagnostic'
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
    
        request_data = json.loads(request.data)
      
        schema_result=check_schema(request_data,modem_diag_schema)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        
        for key, value in request_data.items():
            if value is None:
                return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")

        query = "SELECT id AS sno, loc_name AS LOC_NAME, loc_id as LOC_ID ,dev_id as DCU_ID,dev_name as DEV_NAME,dev_common_status as DCU_STATUS,dev_type FROM dev_tbl where dev_id={}".format(
            request_data["dev_id"])
        dcu_info = get_key_value_pairs(query)
        if dcu_info == None:
            return Response(json.dumps({"status": "not found", "details": []}), content_type="application/json")
        
        
        query = "select  modem_link_sts ,router_id from modem_link_tbl where dcu_id={}".format(
            request_data["dev_id"])
        modem = execute(2, query)
      
        if modem == None or len(modem) == 0 :
            return Response(json.dumps({"status": "not found", "details": []}), content_type="application/json")


        modem_type = modem[0][0]
        dcu_info[0]["Modem_Type"]=modem_type
        

        if modem_type == "Internal":
            table_name = "dcu_modem_dig_info_" + request_data["dev_id"]
            vpn_table = "dcu_vpn_cfg_tbl"
         
            if dcu_info[0]["DCU_STATUS"]=="Communicating":
                dcu_info[0]["Modem_status"]= "online"
            else:
                dcu_info[0]["Modem_status"]= "offline"

        elif modem_type == "External":
            table_name = "router_dig_info_{}".format(modem[0][1])
            vpn_table = "router_vpn_cfg_tbl"
            query_modem_sts = "SELECT dev_common_status as modem_status from dev_tbl where dev_id='{}'".format(modem[0][1])
         
            modem_sts=execute(1,query_modem_sts)
        
            if modem_sts != None and modem_sts != ():
                if modem_sts[0]=="Communictating":
                    dcu_info[0]["Modem_status"]="online"
                else:
                    dcu_info[0]["Modem_status"]="offline" 
            else:
                return Response(json.dumps({"status": "not found"}), content_type="application/json")

        else:

            final_dict = {
            "status": "found",
            "dcu_info": dcu_info[0],
            "modem_info": "-"
            }
            return Response(json.dumps(final_dict), content_type="application/json")
        

        if check_table(table_name) == False:
        
            final_dict = {
            "status": "found",
            "Modem_Type":modem_type,
            "dcu_info": dcu_info[0],
            "modem_info": { "PPPIp":"-",
                        "GwIp":"-",
                            "Dns1Ip":"-",
                                "Dns2Ip":"-",
                                "GprsUpTime":"-",
                                "GprsStatus":"-",
                                "Eth1subnetmask":"-",
                                "Eth2subnetmask":"-",
                                "Eth1Ip":"-",
                                "Eth2Ip":"-",
                        "Eth1Gw":"-",
                        "Eth2Gw":"-",
                        "sig_stren":"-",
                        "nw_type":"-",
                        "nw_reg":"-",
                        "service_provider":"-",
                        "password":"-",
                        "phone_num":"-",
                        "apn":"-",
                        "act_sim":"-",
                        "username":"-",
                        "num_tunn":"-",
                        "sim_switch_over_time":"-",
                        "duration":"-",
                        
                        "ipsec_details":[{"id": "-",
                        "status": "-",
                        "ip": "-",
                        "tunnel": "-"}]
                
                        }
                        }
            return Response(json.dumps(final_dict), content_type="application/json")
      

       
        if  dcu_info[0]["Modem_status"] =="offline"  : #modem_type == "External" and 
            
            modem_info = [{}]

            modem_info[0]["PPPIp"]='-'
            modem_info[0]["GwIp"]='-'
            modem_info[0]["Dns2Ip"]='-'
            modem_info[0]["Dns1Ip"]='-'
            modem_info[0]["GprsUpTime"]='-'
            modem_info[0]["Eth1subnetmask"]='-'
            modem_info[0]["Eth2subnetmask"]='-'
            modem_info[0]["Eth1Ip"]='-'
            modem_info[0]["Eth2Ip"]='-'
            modem_info[0]["Eth1Gw"]='-'
            modem_info[0]["Eth2Gw"]='-'
            modem_info[0]["sig_stren"]='-'
            modem_info[0]["nw_type"]='-'
            modem_info[0]["nw_reg"]='-'
            modem_info[0]["service_provider"]='-'
            modem_info[0]["password"]='-'
            modem_info[0]["phone_num"]='-'
            modem_info[0]["apn"]='-'
            modem_info[0]["act_sim"]='-'
            modem_info[0]["num_tunn"]='-'
            modem_info[0]["username"]='-'
            modem_info[0]["sim_switch_over_time"]='-'
            modem_info[0]["duration"]='-'

            # query2 = "select lan1_mask as Eth1subnetmask,lan2_mask as Eth2subnetmask, lan1_ip as Eth1Ip,lan2_ip as Eth2Ip,lan1_gw as Eth1Gw,lan2_gw as Eth2Gw from `{}` ORDER BY update_time desc".format(
            #     table_name)
            
            # lan_info = get_key_value_pairs(query2)
        
            # modem_info[0] = lan_info[0].copy()

            # logger(modem_info)

        else:
            # query="SELECT 'sim_switch_over_time' AS column_name, IF(EXISTS (SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = 'mdas' AND TABLE_NAME = '{}' AND COLUMN_NAME = 'sim_switch_over_time'), 'Exists', 'Does Not Exist') AS column_status UNION ALL SELECT 'duration', IF(EXISTS (SELECT 1 FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = 'mdas' AND TABLE_NAME = '{}' AND COLUMN_NAME = 'duration'), 'Exists', 'Does Not Exist')".format(table_name,table_name)

            # result=get_key_value_pairs(query)
            
           
            # if result[0]["column_status"]=='Exists' and result[1]["column_status"]=='Exists':
            query2 = "select ppp_ip as PPPIp, ppp_gw as GwIp, ppp_dns1 as Dns1Ip ,ppp_dns2 as Dns2Ip,up_time as GprsUpTime,sim_status as GprsStatus,lan1_mask as Eth1subnetmask,lan2_mask as Eth2subnetmask, lan1_ip as Eth1Ip,lan2_ip as Eth2Ip,lan1_gw as Eth1Gw,lan2_gw as Eth2Gw,sig_str as sig_stren,nw_type, sim_reg as nw_reg,service_provider,password,ph_num as phone_num,act_sim , apn,user_name as username , sim_switch_over_time , duration  from `{}` ORDER BY update_time desc  limit 1".format(
                table_name)
            # elif result[0]["column_status"]=='Does Not Exist' and result[1]["column_status"]=='Does Not Exist':    
            #     query2 = "select ppp_ip as PPPIp, ppp_gw as GwIp, ppp_dns1 as Dns1Ip ,ppp_dns2 as Dns2Ip,up_time as GprsUpTime,sim_status as GprsStatus,lan1_mask as Eth1subnetmask,lan2_mask as Eth2subnetmask, lan1_ip as Eth1Ip,lan2_ip as Eth2Ip,lan1_gw as Eth1Gw,lan2_gw as Eth2Gw,sig_str as sig_stren,nw_type, sim_reg as nw_reg,service_provider,password,ph_num as phone_num,act_sim , apn,user_name as username , 'NA' as sim_switch_over_time , 'NA' as  duration from `{}` ORDER BY update_time desc  limit 1".format(
            #         table_name)
            # elif result[0]["column_status"]=='Exists' and result[1]["column_status"]=='Does Not Exist':    
            
            #     query2 = "select ppp_ip as PPPIp, ppp_gw as GwIp, ppp_dns1 as Dns1Ip ,ppp_dns2 as Dns2Ip,up_time as GprsUpTime,sim_status as GprsStatus,lan1_mask as Eth1subnetmask,lan2_mask as Eth2subnetmask, lan1_ip as Eth1Ip,lan2_ip as Eth2Ip,lan1_gw as Eth1Gw,lan2_gw as Eth2Gw,sig_str as sig_stren,nw_type, sim_reg as nw_reg,service_provider,password,ph_num as phone_num,act_sim , apn,user_name as username ,sim_switch_over_time, 'NA' as duration from `{}` ORDER BY update_time desc  limit 1".format(
            #         table_name)
            # elif result[0]["column_status"]=='Does Not Exist' and result[1]["column_status"]=='Exists':    
            #     query2 = "select ppp_ip as PPPIp, ppp_gw as GwIp, ppp_dns1 as Dns1Ip ,ppp_dns2 as Dns2Ip,up_time as GprsUpTime,sim_status as GprsStatus,lan1_mask as Eth1subnetmask,lan2_mask as Eth2subnetmask, lan1_ip as Eth1Ip,lan2_ip as Eth2Ip,lan1_gw as Eth1Gw,lan2_gw as Eth2Gw,sig_str as sig_stren,nw_type, sim_reg as nw_reg,service_provider,password,ph_num as phone_num,act_sim , apn,user_name as username ,'NA' as sim_switch_over_time,duration from `{}` ORDER BY update_time desc limit 1".format(
            #         table_name)
         
            modem_info = get_key_value_pairs(query2)

            if modem_info == None:
                modem_info = []
                num_tunnels = 0
                modem_info.append({})
            else:
                    query3 = "select instance, gen_details from {} where dev_id='{}'".format(
                        vpn_table, request_data["dev_id"])
                    result = get_key_value_pairs(query3)
                    if result == None or result==():
                        num_tunnels = 0
                        desired_fields = []
                    else:
                        num_tunnels = result[0]["instance"]
                        desired_fields = []

                        i = 1
                     
                     
                        if result[0]["gen_details"].startswith('['):
                            list_gen=ast.literal_eval(result[0]["gen_details"]) 
                            # gen_details=list_gen[0]
                          
                        else:
                            list_gen=[]
                            if result[0]["gen_details"]!="NA":
                                gen_details=json.loads(result[0]["gen_details"])
                                if type(gen_details)==dict:
                                    list_gen.append(gen_details)
                                else:
                                    list_gen=gen_details
                                print("-----------------------",list_gen)
                            else:
                                
                                hardcode={
                                    "id": "",
                                    "status": "",
                                    "ip": "",
                                    "tunnel": ""
                                }
                                list_gen.append(hardcode)
                                
                        end=len(list_gen)
                        for idx in range (0,end):
                            if list_gen[idx].get("STATUS","")=="":
                               list_gen[idx]["STATUS"]="-"
                            
                            if list_gen[idx].get("REMOTE_IP","")=="":
                               list_gen[idx]["REMOTE_IP"]="-"
                            
                            if  list_gen[idx].get("CONN_NAME","-")=="":
                                list_gen[idx]["CONN_NAME"]="-"
                                
                            row_dict = {
                                "id": i,
                                "status": list_gen[idx].get("STATUS","-"),
                                "ip": list_gen[idx].get("REMOTE_IP","-"),
                                "tunnel": list_gen[idx].get("CONN_NAME","-")
                            }
                            desired_fields.append(row_dict)
                            i += 1

            modem_info[0]["num_tunn"] = num_tunnels
            modem_info[0]["ipsec_details"] = desired_fields
        
        # if dcu_info[0]["Modem_status"]=="Not Communicating":
        #     dcu_info[0]["Modem_status"]="offline"
        # if dcu_info[0]["Modem_status"]=="Communicating":
        #     dcu_info[0]["Modem_status"]="online"
        
        final_dict = {
            "status": "found",
            "dcu_info": dcu_info[0],
            "modem_info": modem_info[0]
        }
        print("------------result----------------------",final_dict)
        return Response(json.dumps(final_dict), content_type="application/json")
    

    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
#report
def audit_getlist():
    fn="audit_getlist"
    global sec_code, ser_err
    try:
        request_data = json.loads(request.data) 
        day_st,month_st,year_st=request_data["start_date"].split()
        start_date= f"{year_st}-{Month[month_st]}-{day_st}"
        day_ed,month_ed,year_ed=request_data["end_date"].split()
        end_date= f"{year_ed}-{Month[month_ed]}-{day_ed}"

        if request_data["audit_type"]=="All":
            if request_data["active_role"]=="Super Admin":
                query="select id,date_time, active_user as user_name,active_role as  user_role , audit_type as  type, audit_text as descr   from  audit_table  where date (date_time) between '{}' AND '{}' order by  date_time desc limit 1000".format( start_date,end_date)
            else:
                query="select id,date_time, active_user as user_name,active_role as  user_role , audit_type as  type, audit_text as descr   from  audit_table  where date (date_time) between '{}' AND '{}' and active_role!='Super Admin' order by  date_time desc limit 1000".format( start_date,end_date)
        else:
            if request_data["active_role"]=="Super Admin":
                query="select id,date_time, active_user as user_name,active_role as  user_role , audit_type as  type , audit_text as descr from  audit_table where audit_type='{}' and date (date_time) between '{}' AND '{}' order by  date_time desc limit 1000".format( request_data["audit_type"],start_date,end_date)
            else:
                query="select id,date_time, active_user as user_name,active_role as  user_role , audit_type as  type , audit_text as descr from  audit_table where audit_type='{}' and date (date_time) between '{}' AND '{}' and active_role!='Super Admin' order by  date_time desc limit 1000".format( request_data["audit_type"],start_date,end_date)
        result=get_key_value_pairs(query)
        if result==None:
            return Response(json.dumps({"status": "not found", "details":[],"info":request_data}), content_type="application/json")
        return Response(json.dumps({"status": "found", "details":result,"info":request_data}), content_type="application/json")
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
# # def dcu_event_getist():
#     fn="dcu_events_getlist"
#     try:
#         input=json.loads(request.data)
#         print ("input :----------------------------------------- ",input)
#         s_day,s_month_name, s_year = input["start_date"].split()
#         s_month = Month[s_month_name]
#         start_date = f"{s_year}-{s_month}-{s_day}"
       
#         e_day,e_month_name, e_year = input["end_date"].split()
#         e_month = Month[e_month_name]
#         end_date = f"{e_year}-{e_month}-{e_day}"
      
#         table_name1="dcu_event_{}_tbl".format(input["dev_id"])
        

#         final_result=[]
#         port,met,met_name=input["meter_name"].split("$$")
        
#         if input["meter_name"] =="All":
#             query="select unique_id ,dev_name , met_name from dcu_dlms_meter_tbl where dev_id='{}'".format(input["dev_id"] )
#         else:
#             query="select unique_id ,dev_name , met_name from dcu_dlms_meter_tbl where dev_id='{}' and BINARY met_name = '{}'".format(input["dev_id"],met_name )
    
#         meter=get_key_value_pairs(query)
    
#         for met_id in meter:
#             unique_id=met_id["unique_id"]

#             table_name="dcu_event_data_{}".format(unique_id)
                
#             if check_table(table_name)==True:
#                 if input["event_type"]=="All":
#                     query="select update_time as event_time,EVENTS as events  from {} where DATE(update_time) BETWEEN '{}' and '{}'  order by id asc ".format(table_name,start_date,end_date)
#                 else:
#                     if input["event_type"]=="Meter Events":
                    
#                         if input["event_code"]=="All":
#                             query="select update_time as event_time,EVENTS as events  from {} where DATE(update_time) BETWEEN '{}' and '{}'  order by id asc ".format(table_name,start_date,end_date)
#                         elif  input["event_code"]!="All"  :
#                             query="select update_time as event_time,EVENTS as events  from {} where DATE(update_time) BETWEEN '{}' and '{}' and EVENTS='{}' order by id asc".format(table_name,start_date,end_date,input["event_code"])
#                     else:
#                         query="select event_time as update_time, msg_type as type,msg as events  from {} where   msg_type='{}' and DATE(event_time) BETWEEN '{}' and '{}' order by event_time ".format(table_name1,input["type"],start_date,end_date)
#                 result=get_key_value_pairs(query)
                    
#                 if result!=None:
#                     for i in range (0,int(len(result))):
#                             # print(">>>>>>>>>>>>>>>>>>>>>>>>>",result[i])
#                         # result[i]["dcu_name"]=meter[0]["dev_name"]
#                         result[i]["meter_name"]=meter[0]["met_name"]
#                         result[i]["type"]="Meter Events"
#                         # print(">>>>>>>>>>>>>>>>>>>>>>>>>",result[i])
#                         final_result.append(result[i])
        
                        
#             return final_result
#     except Exception as e:
#         print("---------------------------------")
#         exc_type, exc_obj, exc_tb = sys.exc_info()
#         logerr(" %s Error on line number : %s : %s" %
#                (fn, exc_tb.tb_lineno, e))
#         print("---------------------------------")
#         return Response(ser_err, mimetype="application/json")  
# # def dcu_alert_report_all(start_date,end_date,input):
# #     fn="dcu_alert_all"
# #     try:
       
# #     except Exception as e:
# #         print("---------------------------------")
# #         exc_type, exc_obj, exc_tb = sys.exc_info()
# #         logerr(" %s Error on line number : %s : %s" %
# #                (fn, exc_tb.tb_lineno, e))
# #         print("---------------------------------")
# #         return Response(ser_err, mimetype="application/json")
    
    
    
def difference(start,end,fn):
    var1 = datetime.strptime(str(start), "%Y-%m-%d %H:%M:%S.%f")
    var2 = datetime.strptime(str(end), "%Y-%m-%d %H:%M:%S.%f")
    time_difference = var2 - var1
    return time_difference
#plcc

def plcc_getlist():
    fn="plcc_getlist"
    try:
        input=json.loads(request.data)
        start=datetime.now()
        # input=json.loads(request.data)
        
    
        if not check_table("dcu_dlms_meter_tbl"):
            meter_info=[{
             "online_count":"-",
             "offline_count":"-",
             "not_configured_count":"-",
             "total_count":"-"
             
             }]
            
        else:
            query = """
            SELECT 
            CAST(SUM(CASE WHEN comn_status = 'Communicating' THEN 1 ELSE 0 END) as UNSIGNED) AS online_count,
            CAST(SUM(CASE WHEN comn_status = 'Not Communicating' THEN 1 ELSE 0 END) as UNSIGNED)AS offline_count,
            CAST(SUM(CASE WHEN comn_status = 'Not Configured' THEN 1 ELSE 0 END) as UNSIGNED) AS not_configured_count,
            CAST(SUM(CASE WHEN comn_status = 'Configured' THEN 1 ELSE 0 END) as UNSIGNED) AS configured_count,
            COUNT(*) AS total_count
            FROM dcu_dlms_meter_tbl
            WHERE dev_id = '{}'
            """.format(input.get("dev_id"))
            meter_info=get_key_value_pairs(query)
            
            meter_info[0]["configured_count"]=int( meter_info[0]["total_count"]) - int(meter_info[0]["not_configured_count"] )
            # meter_info[0]["offline_count"]="-"
            
       
        if not check_table("plcc_met_table"):
            print("====not found ")
            return {"status": "not found","count":"0","details":[],"info":meter_info[0]}
        
        
        query="select * from plcc_met_table where dev_id='{}'".format(input.get("dev_id"))
        # print(query)
        total_count=get_key_value_pairs(query)
        if total_count==None:
           
            return {"status": "not found","count":"0","details":[],"info":meter_info[0]}
      
        end = int(input.get("page"))*20
        start=end-20
        # query="select id,nc_id ,dev_id,rs_node_id,rs_parent_id,rs_status,rs_sn,mapped_met_sn,met_id,signal_qualityfrom plcc_met_table where dev_id='{}' order by id asc limit 20 offset {}".format(input.get("dev_id"),start)
        query="SELECT id, nc_id, dev_id, rs_node_id, rs_parent_id, CASE WHEN rs_status = 1 THEN 'Online' WHEN rs_status = 0 THEN 'Offline' WHEN rs_status = 2 THEN 'Poor Online' ELSE 'unknown' END AS rs_status, rs_sn, mapped_met_sn, met_id, signal_quality FROM plcc_met_table WHERE dev_id = '{}' ORDER BY id ASC LIMIT 20 OFFSET {}".format(input.get("dev_id"),start)

        result=get_key_value_pairs(query)
      
        # if result!=None:
        #     if result[0]["rs_status"]=="1":
        #         result[0]["rs_status"]="Online"
        #     elif result[0]["rs_status"]=="0":   
        #         result[0]["rs_status"]="Offline" 
        #     elif result[0]["rs_status"]=="2":   
        #         result[0]["rs_status"]="Poor Offline" 
        
        qry="SHOW COLUMNS FROM plcc_nc_table LIKE 'updated_time'"
        check_column=get_key_value_pairs(qry)
        
       
        
        if check_column!=None:
            #COUNT(*) AS total_count
            query="select num_meters as total_count,db_size ,  nw_size,updated_time  from plcc_nc_table where dev_id='{}'".format(input.get("dev_id"))
            size=get_key_value_pairs(query)
        else:
            query="select num_meters as total_count,db_size ,  nw_size from plcc_nc_table where dev_id='{}'".format(input.get("dev_id"))
            print(query)
            size=get_key_value_pairs(query)
            # size[0]["updated_time"]="test time"
            
      
   
        if size!=None or size!=[]:
        
            meter_info[0]["db_size"]=size[0]["db_size"]
            meter_info[0]["net_size"]=size[0]["nw_size"]
            meter_info[0]["updated_time"]=size[0].get("updated_time","dummy time polled")
            meter_info[0]["configured_count"]=size[0]["total_count"]
        
        print("-----------------eeee---------------------------",result)
        if result==None:
            return {"status": "not found","count":len(total_count),"details":result,"info":meter_info[0]}
        
    
        return {"status": "found","count":len(total_count),"details":result,"info":meter_info[0]}
        
       
        
        
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    

def filter_serial_num():
    fn="filter_serial"
    try:
        input=json.loads(request.data)
        
        if not check_table("plcc_met_table"):
            return Response(json.dumps({"status": "not found"}), content_type="application/json")
        
       
     
        query="select * from plcc_met_table where mapped_met_sn LIKE '%{}%'".format(input.get("serial"))
        total_count=get_key_value_pairs(query)
        
        if total_count==None:
            return Response(json.dumps({"status": "not found"}), content_type="application/json")
        end = int(input.get("page"))*20
        start=end-20
        # query="SELECT * FROM plcc_tbl WHERE meter_ser_num LIKE '%{}%' AND dev_id = '{}' AND id BETWEEN '{}' AND '{}' ORDER BY id ASC".format(input.get("serial"),input.get("dev_id"),start,end )

        query="SELECT id, nc_id, dev_id, rs_node_id, rs_parent_id, CASE WHEN rs_status = 1 THEN 'Online' WHEN rs_status = 0 THEN 'Offline' WHEN rs_status = 2 THEN 'Poor Online' ELSE 'unknown' END AS rs_status, rs_sn, mapped_met_sn, met_id, signal_quality FROM plcc_met_table WHERE mapped_met_sn LIKE '%{}%' and dev_id = '{}' order by id".format(input.get("serial"),input.get("dev_id"))
        result=get_key_value_pairs(query)
        
        if not check_table("dcu_dlms_meter_tbl"):
            meter_info=[{
             "online_count":"-",
             "offline_count":"-",
             "not_configured_count":"-",
             "total_count":"-"
             
             }]
        else:
       
            query = """
            SELECT 
            CAST(SUM(CASE WHEN comn_status = 'Communicating' THEN 1 ELSE 0 END) as UNSIGNED) AS online_count,
            CAST(SUM(CASE WHEN comn_status = 'Not Communicating' THEN 1 ELSE 0 END) as UNSIGNED)AS offline_count,
            CAST(SUM(CASE WHEN comn_status = 'Not Configured' THEN 1 ELSE 0 END) as UNSIGNED) AS not_configured_count           
            FROM dcu_dlms_meter_tbl
            WHERE dev_id = '{}'
            """.format(input.get("dev_id"))
            meter_info=get_key_value_pairs(query)
        
        query="select  num_meters AS total_count,db_size ,  nw_size  from plcc_nc_table where dev_id='{}'".format(input.get("dev_id"))
        size=get_key_value_pairs(query)
        
      
        if size!=None:
            meter_info[0]["db_size"]=size[0]["db_size"]
            meter_info[0]["net_size"]=size[0]["nw_size"]
            meter_info[0]["total_count"]=size[0]["total_count"]
        print("================================================================================================",meter_info[0])
        if result==None:
            return Response(json.dumps({"status": "not found","info":meter_info[0]}), content_type="application/json")
        else:  
                
            return Response(json.dumps({"status": "found","details":result,"count":len(total_count),"info":meter_info[0]}), content_type="application/json")

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

#config

def Download_Config():
    fn = "Download_Config()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
    
        jwt.decode(header, sec_code, algorithms="HS256")
  
        post_dict = json.loads(request.data)

        results_sn = execute(1, "SELECT dev_sn,dev_token  FROM dev_tbl WHERE dev_id='{}'".format(post_dict['dev_id']))
        dev_sn = results_sn[0]
        dev_token = results_sn[1]

        if post_dict != {}:
            if post_dict['dev_id'] != "":
                user_id = post_dict.get('dev_id')
            else:
                return Response(json.dumps({"status": "invalid  dev_id argument"}), mimetype='application/json')
            
            if post_dict['active_user'] != "":
                active_user = post_dict.get('active_user')
            else:
                return Response(json.dumps({"status": "Invalid active_user"}), mimetype='application/json')
            
            if post_dict['active_role'] != "":
                active_role = post_dict.get('active_role')
            else:
                return Response(json.dumps({"status": "Invalid active_role"}), mimetype='application/json')
            
        else:
            return Response(json.dumps({"status": "not found"}), mimetype='application/json')
        
        
        if 1:
            
            if post_dict['cmd_type'] == "Restart Gateway" or post_dict['cmd_type'] == "Restart Gateway":
                res = execute(1, "SELECT seq_num FROM `command_tbl` ORDER BY seq_num desc")
            else:
                res = execute(1, "SELECT seq_num FROM `dcu_command_tbl` ORDER BY seq_num desc")
                
                
            if res != None:
                if len(res) != 0:
                    seq_no = int(res[0] ) + 1
                else:
                    seq_no = 1000
            else:
                seq_no = 1000
                
            now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')

            if post_dict['cmd_type'] == "Download Configuration":

                send_data = {
                    "TYPE": "DOWNLOAD_CONFIG",
                    "SEQ_NUM": str(seq_no),
                }
    
                update_tbl("INSERT INTO command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data, user_name, user_role) VALUES ('DOWNLOAD_CONFIG', {}, {}, '{}', 'issued', 0, '{}', '{}', '{}')".format(seq_no, user_id, now, json.dumps(send_data),active_user, active_role))
            
                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                active_role,  active_user, "Config Management", "Download Config successfully")
                update_tbl(sql)
                
                cst_res = Download_Config1(post_dict["dev_id"])
                cst_res["seq_num"] = seq_no
                return Response(json.dumps(cst_res), mimetype='application/json')

        
            elif post_dict['cmd_type'] == "ENABLE_FTP":
  
                send_data = {
                    "TYPE": "OD_MESSAGE",
                    "CMD_TYPE": "FTP_CFG",
                    "SEQ_NUM": str(seq_no),
                    "DATA": {
                    "CFG_STS": "ENABLE",
                    "DEVTOKEN": dev_token,
                    "SN": dev_sn
                    }
                }           
                update_tbl("INSERT INTO dcu_command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data, user_name, user_role) VALUES ('FTP_CFG', {}, {}, '{}', 'issued', 0, '{}', '{}', '{}')".format(seq_no, user_id, now, json.dumps(send_data),active_user, active_role))
            
                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                active_role,  active_user, "Config Management", "FTP Enable successfully")
                update_tbl(sql)
            
            elif post_dict['cmd_type'] == "DISABLE_FTP":
                
                send_data = {
                    "TYPE": "OD_MESSAGE",
                    "CMD_TYPE": "FTP_CFG",
                    "SEQ_NUM": str(seq_no),
                    "DATA": {
                    "CFG_STS": "BLOCK",
                    "DEVTOKEN": dev_token,
                    "SN": dev_sn
                    }
                }           
                update_tbl("INSERT INTO dcu_command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data, user_name, user_role) VALUES ('FTP_CFG', {}, {}, '{}', 'issued', 0, '{}', '{}', '{}')".format(seq_no, user_id, now, json.dumps(send_data),active_user, active_role))
            
                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                active_role,  active_user, "Config Management", "FTP Disable successfully")
                update_tbl(sql)
            
            elif post_dict['cmd_type'] == "ENABLE_LOGS":

                send_data = {
                    "TYPE": "OD_MESSAGE",
                    "CMD_TYPE": "SET_LOG_LEVEL",
                    "SEQ_NUM": str(seq_no),
                    "DATA": {
                    "CFG_STS": "ENABLE",
                    "DEVTOKEN": dev_token,
                    "SN": dev_sn
                    }
                }           
                update_tbl("INSERT INTO dcu_command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data, user_name, user_role) VALUES ('SET_LOG_LEVEL', {}, {}, '{}', 'issued', 0, '{}', '{}', '{}')".format(seq_no, user_id, now, json.dumps(send_data),active_user, active_role))
            
                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                active_role,  active_user, "Config Management", "DB Log Enable successfully")
                update_tbl(sql)
            
            elif post_dict['cmd_type'] == "DISABLE_LOGS":
                
                send_data = {
                    "TYPE": "OD_MESSAGE",
                    "CMD_TYPE": "SET_LOG_LEVEL",
                    "SEQ_NUM": str(seq_no),
                    "DATA": {
                    "CFG_STS": "BLOCK",
                    "DEVTOKEN": dev_token,
                    "SN": dev_sn
                    }
                }           
                update_tbl("INSERT INTO dcu_command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data, user_name, user_role) VALUES ('SET_LOG_LEVEL', {}, {}, '{}', 'issued', 0, '{}', '{}', '{}')".format(seq_no, user_id, now, json.dumps(send_data),active_user, active_role))
            
                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                active_role,  active_user, "Config Management", "DB Log Disable successfully")
                update_tbl(sql)
            
            elif post_dict['cmd_type'] == "Restart Gateway":
                
                send_data = {
                    "TYPE": "OD_MESSAGE",
                    "CMD_TYPE": "RESTART",
                    "SEQ_NUM": str(seq_no),
                    "DATA": {
                    "DEVTOKEN": dev_token,
                    "SN": dev_sn
                    }
                }           
                update_tbl("INSERT INTO command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data, user_name, user_role) VALUES ('DOWNLOAD_CONFIG', {}, {}, '{}', 'issued', 0, '{}', '{}', '{}')".format(seq_no, user_id, now, json.dumps(send_data),active_user, active_role))
            
                sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
                active_role,  active_user, "Config Management", "Restart Gateway successfully")
                update_tbl(sql)


        
        
        return Response(json.dumps({"status": "success","seq_num":seq_no}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def Upload_Config():
    fn = "Upload_Config()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
     
        jwt.decode(header, sec_code, algorithms="HS256")
        
        args = request.args

        if args != {}:
            if args['dev_id'] != "":
                dev_id = args.get('dev_id')
            else:
                return Response(json.dumps({"status": "invalid dev_id argument"}), mimetype='application/json')
            
            if args['active_user'] != "":
                active_user = args.get('active_user')
            else:
                return Response(json.dumps({"status": "invalid active_user argument"}), mimetype='application/json')
            
            if args['active_role'] != "":
                active_role = args.get('active_role')
            else:
                return Response(json.dumps({"status": "invalid active_role argument"}), mimetype='application/json')
        
        
  
        res = execute(1, "SELECT seq_num FROM command_tbl ORDER BY seq_num desc")
        
        
        if res != None:
            if len(res) != 0:
                seq_no = int(res[0])+ 1
            else:
                seq_no = 1000
        else:
            seq_no = 1000
            
        send_data = {
            "TYPE": "UPLOAD_CONFIG",
            "SEQ_NUM": str(seq_no),
        }
            
      
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        update_tbl("INSERT INTO command_tbl (COMMAND, seq_num, DEV_ID, ISSUE_TIME, COMMAND_STATUS, retry, send_data) VALUES ('UPLOAD_CONFIG', {}, {}, '{}', 'issued', 0, '{}')".format(seq_no, dev_id, now, json.dumps(send_data)))
        
        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        args["active_role"],  args["active_user"], "Config Management", "Upload Config successfully")
        update_tbl(sql)
        
        return Response(json.dumps({"status": "success","seq_num":seq_no}), mimetype='application/json')#, "seq_num": seq_no
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def Upload_Firmware():
    fn = "Upload_Firmware()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
      
        jwt.decode(header, sec_code, algorithms="HS256")
        args = request.args

        if args != {}:
            if args['dev_id'] != "":
                dev_id = args.get('dev_id')
            else:
                return Response(json.dumps({"status": "invalid dev_id argument"}), mimetype='application/json')
            
            if args['active_user'] != "":
                active_user = args.get('active_user')
            else:
                return Response(json.dumps({"status": "invalid active_user argument"}), mimetype='application/json')
            
            if args['active_role'] != "":
                active_role = args.get('active_role')
            else:
                return Response(json.dumps({"status": "invalid active_role argument"}), mimetype='application/json')
            
        
        cst_res = Upload_Firmware1(dev_id)
        
        if cst_res["status"] != "success":
            print("no invalid json returning...", cst_res)
            return Response(cst_res, mimetype='application/json')
        
        # res = db_conn.execute("SELECT SEQ_NO FROM command_tbl").fetchall()
        res = execute(1, "SELECT seq_num FROM command_tbl ORDER BY seq_num desc")
        
        if res != None:
            if len(res) != 0:
                seq_no = int(res[0]) + 1
            else:
                seq_no = 1000
        else:
            seq_no = 1000
            
        send_data = {
            "TYPE": "UPLOAD_FIRMWARE",
            "SEQ_NUM": str(seq_no),
        }
            
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
      
        update_tbl("INSERT INTO command_tbl (COMMAND, seq_num, DEV_ID, ISSUE_TIME, COMMAND_STATUS, retry, send_data) VALUES ('UPLOAD_FIRMWARE', {}, {}, '{}', 'issued', 0, '{}')".format(seq_no, dev_id, now, json.dumps(send_data)))
        
        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        args["active_role"],  args["active_user"], "Config / Firmware Upload", "Gateway ({}) new config file uploaded.".format(dev_id))
        update_tbl(sql)
        
        return Response(json.dumps({"status": "success", "seq_num": seq_no}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')
  
def time_synch():
    fn = "time_synch()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
     
        jwt.decode(header, sec_code, algorithms="HS256")
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")
        
        post_dict = json.loads(request.data)
        
        res = execute(1, "SELECT seq_num FROM command_tbl ORDER BY seq_num desc")
        
        
        if res != None:
            if len(res) != 0:
                seq_no = int(res[0]) + 1
            else:
                seq_no = 1000
        else:
            seq_no = 1000
            
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        update_tbl("INSERT INTO command_tbl (COMMAND, SEQ_NO, DEV_ID, ISSUE_TIME, COMMAND_STATUS, retry) VALUES ('TIME_SYNC', {}, {}, '{}', 'issued', 0)".format(seq_no, post_dict["dev_id"], now))
        
        sql = "INSERT INTO audit_trail_report (date_time, user_role,  user_name, `type`, `description`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(now,
        post_dict["active_role"],  post_dict["active_user"], "Config / Firmware Upload", "For gateway ({}) Time synch command given.\nReason: {}".format(post_dict["dev_id"], post_dict["reason"]))
        update_tbl(sql)
        
        return Response(json.dumps({"status": "success","seq_num":seq_no}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def reset():
    fn = "reset()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
      
        jwt.decode(header, sec_code, algorithms="HS256")
        
        post_dict = json.loads(request.data)
        
        res = execute(1, "SELECT seq_num FROM command_tbl ORDER BY seq_num desc")
     
        if res != None:
            if len(res) != 0:
                seq_no = int(res[0]) + 1
            else:
                seq_no = 1000
        else:
            seq_no = 1000
            
       
        send_json = json.dumps({"TYPE":"RESET","SEQ_NUM": str(seq_no)}) #"".format(seq_no)
        now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        update_tbl("INSERT INTO command_tbl (command, seq_num, dev_id, issue_time, command_status, retry, send_data) VALUES ('RESET', {}, {}, '{}', 'issued', 0, '{}')".format(seq_no, post_dict["dev_id"], now, send_json))
        
        sql = "INSERT INTO audit_trail_report (date_time, user_role,  user_name, `type`, `description`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(now,
        post_dict["active_role"],  post_dict["active_user"], "Config / Firmware Upload", "For gateway ({}) reset command given.\nReason: {}".format(post_dict["dev_id"], post_dict["reason"]))
        update_tbl(sql)
        
        return Response(json.dumps({"status": "success", "seq_num": seq_no}), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=401)
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def config_status():
    fn = "config_status()"
    global ser_err1, sec_code, db_conn
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        
        post_dict = json.loads(request.data)
        print(post_dict)
        
        if post_dict['com_type'] == 'Download Configuration':
            cmd_type = 'DOWNLOAD_CONFIG'
        elif post_dict['com_type'] == 'Upload Configuration':
            cmd_type = 'UPLOAD_CONFIG'
        elif post_dict['com_type'] == 'Upgrade Firmware':
            cmd_type = 'UPLOAD_FIRMWARE'
        else: #Restart Gateway
            cmd_type = 'RESET'

        
        res = execute(1, "SELECT command_status, issue_time FROM command_tbl where seq_num = '{}' and command = '{}'".format(post_dict['seq_num'], cmd_type))
        if res != None:
            sts = res[0]
            
            if sts == 'SUCCESS':
                post_dict['status'] = 'success'
            elif sts == 'issued':
                now = datetime.now() - datetime.strptime(res[1], "%Y-%m-%d %H:%M:%S")
                if now.total_seconds() > 120:
                    post_dict['status'] = 'FAILED. (Command Not Processed By Gateway)'
                else:
                    post_dict['status'] = 'in_progress'
            else:
                post_dict['status'] = sts
                
        else:
            post_dict['status'] = 'Command not found'
            
        
        return Response(json.dumps(post_dict), mimetype='application/json')
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err1, mimetype ='application/json')

def  Download_Config1(dev_id):
    fn = "Download_Config1()"
    global ser_err, sec_code, download_folder, local_download_folder, PRODUCTION
    try:
        header = request.headers.get('Authorization')
  
        jwt.decode(header, sec_code, algorithms="HS256")
        main_dict = {}
        
        sh_file_path = download_folder + "{}.cfg".format(dev_id)

        main_dict["file_path"] = sh_file_path
                
        main_dict["status"] = "success"
        main_dict["file_name"] = "{}.cfg".format(dev_id)
    
        return main_dict
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        print(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return ser_err
        
def Config_Upload1(dev_id):
    '''Accepts config.tar file and validate!'''
    fn = "Config_Upload1()"
    global upload_folder, ser_err
    try:
        header = request.headers.get('Authorization')
    
        jwt.decode(header, sec_code, algorithms="HS256")
        
        resp = {"status" : "success"}
        
        if len(request.files) > 2:
            logwarn("File length not matched, more than one file received.")
            resp = {'status': 'Please select Valid config File!!!'}
            return resp
            # return Response(resp, mimetype='application/json', status=401)
            
        if 'config_file' not in request.files:
            logwarn("Config File not found : config.cfg!!!")
            resp = {'status': 'Config File not found : config.cfg!!!'}
            return resp
            # return Response(resp, mimetype='application/json', status=401)
            
        config_file = request.files['config_file']
        if config_file.filename == '':
            logwarn("No file selected for uploading")
            resp = {'status': 'No file selected for uploading'}
            return resp
         
        config_file_name =  "{}.cfg".format(dev_id)
        config_file_full_path = os.path.join(upload_folder, config_file_name)
        config_file.save(config_file_full_path)
        
        return {'status': 'success'}
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        print('%s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return ser_err
    
def Upload_Firmware1(dev_id):
    fn = "Upload_Firmware1()"
    global ser_err, fm_upload_folder
    try:
        header = request.headers.get('Authorization')
    
        jwt.decode(header, sec_code, algorithms="HS256")
        resp = {"status" : "success"}
     
        if len(request.files) != 1:
            logwarn("Multiple files were selected or Invalid config file selected!!!")
            return {'status': 'Multiple files were selected or Invalid config file selected!'}
        
        # If 'fm file' is not present in received data.
        if 'firmware_file' not in request.files:
            logwarn("Opps! Firmware File is not found!!!")
            return {'status': 'Opps! Firmware File is not found'}
        
        fm_file = request.files['firmware_file']  # config_file--------> fm_file
        
        if fm_file.filename == '':
            return {'status': 'No file selected for uploading'}
        
        fm_file_name =  "{}/mim_firmware".format(dev_id)
        fm_file_full_path = os.path.join(fm_upload_folder, fm_file_name)
        fm_file.save(fm_file_full_path)
        
   
        
        if 0:
            # EXTRACTING tar/bin FILE
            config_tar = tarfile.open(fm_file_full_path)
            config_tar.extractall(fm_upload_folder)
            config_tar.close()
            print("Going to Remove Files.")
            # Remove the file
            cmd = "rm %s" %(fm_file_full_path)
            os.system(cmd)
            fm_tar_path = os.path.join(fm_upload_folder, 'firmware.tar')
            checksum_file_path = os.path.join(fm_upload_folder, 'checksum')
            if os.path.exists(fm_tar_path) == False:
                logwarn("firmware.tar not found!!!")
                resp = {'status': 'firmware.tar not found!!!'}
                return resp
            if os.path.exists(checksum_file_path) == False:
                logwarn("checksum file not matched!!!")
                resp = {'status': 'checksum file not found!!!'}
                return resp
            calculated_md5 = hashlib.md5(
                open(fm_tar_path, 'rb').read()).hexdigest()
            received_md5 = open(checksum_file_path, 'rb').read()
            if received_md5.decode().strip("\n") == calculated_md5:
                # resp = {'status': 'Firmware File successfully uploaded and installed'}#, 'status_code': 200
                # Install Firmware
                if 0:
                    cmd = "/usr/cms/bin/install.sh -c 1 -f 1"
                    os.system(cmd)
                    #remove files in temp firmwar.tar & checksum
           
            else:
              
                resp = {'status': 'Checksum not matched!!!'}
                return resp
        
        return {'status': 'success'}
    except jwt.ExpiredSignatureError as e:
                return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200) 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        print(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return ser_err




def get_install_devices_info():
    fn = "get_install_devices_info"
    try:
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")
       
        query = "SELECT SUM(total_devices) AS total_sum FROM dcu_overall_connectivity_dashboard WHERE site_type IN ('IPPS_WIND', 'IPPS_SOLAR', 'IPPS_OTHERS')"
        result = execute(1, query)
       
        dev_query="select dev_id from dev_tbl where  dev_reg_status = 'registered' "
        reg_device=execute(2,dev_query)
       
        vsat_query="select dcu_id from modem_link_tbl where modem_link_sts='VSAT' "
        vsat=execute(2,vsat_query)
     
        gprs_query="select dcu_id from modem_link_tbl where modem_link_sts='Internal' or modem_link_sts='External' "
        gprs=execute(2,gprs_query)
     
        main_values = set(value[0] for value in reg_device)
        vsat_tuple_values = set(value[0] for value in vsat)
        gprs_tuple_values = set(value[0] for value in gprs)
       
        vsat_values = main_values.intersection(vsat_tuple_values)
        gprs_values = main_values.intersection(gprs_tuple_values)
       
        gprs_values.add(1)
        gprs_values.add(2)
       
        vsat_values.add(1)
        vsat_values.add(2)
   
        gprs_per_query="select conct_per from dcu_daily_connectivity_tbl where block_date='{}' and dev_id in {} ".format(datetime.now().date(),tuple(gprs_values))
        gprs_per_result=execute(2,gprs_per_query)
       
        vsat_per_query="select conct_per from dcu_daily_connectivity_tbl where block_date='{}' and dev_id in {} ".format(datetime.now().date(),tuple(vsat_values))
        vsat_per_result=execute(2,vsat_per_query)
       
       
        if gprs_per_result==():
            gprs_percentage=0
        else:
            gprs_per=0
            for outer_tuple in gprs_per_result:
                for idx in outer_tuple:
                    gprs_per=gprs_per+float(idx)
            gprs_percentage=round(gprs_per/len(gprs_per_result),2)
           
           
        if vsat_per_result==():
            vsat_percentage=0
        else:
            vsat_per=0
            for outer_tuple in vsat_per_result:
                for idx in outer_tuple:
                    vsat_per=vsat_per+float(idx)
            vsat_percentage=round(vsat_per/len(vsat_per_result),2)
           
        meter_total=execute(2,"select dev_id from dcu_dlms_meter_tbl where dev_id in {}".format(tuple(main_values)))
     
        meter_online=execute(2,"select unique_id from dcu_dlms_meter_tbl where dev_id in {} and comn_status='Communicating' or comn_status='Configured' ".format(tuple(main_values)))
     
        meter_offline=execute(2,"select unique_id from dcu_dlms_meter_tbl where dev_id in {} and comn_status='Not Communicating' or comn_status='Not Configured' ".format(tuple(main_values)))
      
       
     
        loc_data = {
 
 
            "status": "found",
            "total_device": total_device_online_offline("total_devices"),
            "total_online": total_device_online_offline("online"),
            "total_offline": total_device_online_offline("offline"),
            "dev_kptcl": site_total("total_devices", "KPTCL"),
            "dev_kpcl": site_total("total_devices", "KPCL"),
 
            "ipps_wind": site_total("total_devices", "IPPS_WIND"),
            "ipps_solar": site_total("total_devices", "IPPS_SOLAR"),
            "ipps_others": site_total("total_devices", "IPPS_OTHERS"),
            "dev_ipps": int(result[0]),
            "gprs_count":len(gprs_values),
            "vsat_count":len( vsat_values),
            "gprs_per":gprs_percentage,
            "vsat_per":vsat_percentage,
            "total_meters":len(meter_total),
            "total_online_meters":len(meter_online),
            "total_offline_meters":len(meter_offline),
            }
 
     
        return Response(json.dumps(loc_data), content_type="application/json")
   
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
       
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def get_connectivity_device_status():
    fn = "get_connectivity_device_status"
    try:
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")

        ipps_online = 0
        ipps_offline = 0
        ipps_totaldevices = 0
        query = "SELECT * FROM dcu_overall_connectivity_dashboard"
        resultdata = get_key_value_pairs(query)
        loc_data = {}
        loc_data["status"] = "found"
        loc_data["total_devices"] = total_device_online_offline(
            "total_devices")
       # print("===>", resultdata)
        for data in resultdata:

            site_type = data['site_type'].lower()

            if "ipps" in site_type:
                ipps_online = ipps_online + data['online']
                ipps_offline = ipps_offline + data['offline']
                ipps_totaldevices = ipps_totaldevices + data['total_devices']
            key = site_type+"_info"

            loc_data[key] = {


                "online": data['online'],
                "offline": data['offline'],
                "on_percent": per_cal(data['online'], data['total_devices']),
                "off_percent": per_cal(data['offline'], data['total_devices']),
            }
        loc_data["ipps_info"] = {
            "online": ipps_online,
            "offline": ipps_offline,
            "on_percent": per_cal(ipps_online, ipps_totaldevices),
            "off_percent": per_cal(ipps_offline, ipps_totaldevices),


        }

        return Response(json.dumps(loc_data), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")


def get_data_available_sitetype_info():
    fn = "get_data_available_sitetype_info"
    try:
        
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")

        query = "SELECT DATE_FORMAT(block_date, '%d-%m-%Y') as date, KPCL_Per as kpcl,KPTCL_Per as kptcl,IPPS_Per as  ipps FROM dcu_monthly_data_avail_info WHERE DATE(block_date) = DATE(DATE_SUB(CURDATE(), INTERVAL 1 DAY))"

        result = get_key_value_pairs(query)
        today = datetime.now()
        yesterday = today - timedelta(days=1)
        formatted_yesterday = yesterday.strftime('%d-%m-%Y')

        if result == () or result == None or result == []:
            loc_data = {
                "status": "found",
                "details": {
                    "date": formatted_yesterday,
                    "kpcl": 0,
                    "kptcl": 0,
                    "ipps": 0
                }

            }
        else:
            row_dict={}
            for key, value in result[0].items():
                
                if value==None or value=="0.0" or value=="0":
                   value=0
                   row_dict[key]=value
                   
                else :
                    row_dict[key]=value
              

            loc_data = {
                    "status": "found",
                    "details":row_dict

                }
   
        return Response(json.dumps(loc_data), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def get_data_avail_monthly_graph():
    fn = 'get_data_avail_monthly_graph'
    try:
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")

        request_data = json.loads(request.data)

        schema_result=check_schema(request_data,connectivity_monthly_graph)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")

        if request_data['site_type'] == 'IPPS' and request_data["gen_type"] == "":
            return Response(json.dumps({"status": "Empty value found for key gen_type"}), mimetype="application/json")

        keys_to_check = ["start_month", "site_type"]
        for key in keys_to_check:
            if request_data[key] == "":
                return Response(json.dumps({"status": "Empty value found for key '{}'".format(key)}), mimetype="application/json")
        start_month = request_data["start_month"]
        month_name, year = start_month.split()

        month_numeric_value = Month[month_name]
        num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]


        if request_data["gen_type"] == "All" or request_data["gen_type"] == "":
            site_type_req = request_data["site_type"].upper()
            site_type = site_type_req+"_Per"
           
        else:
            site_type_req = request_data["site_type"].upper()
         
          
            gen_type = request_data["gen_type"]

            site_type = site_type_req+"_"+gen_type+"_"+"Per"

        query = "SELECT {}, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM dcu_monthly_data_avail_info WHERE YEAR(block_date) = '{}' AND MONTH(block_date) = '{}' ORDER BY block_date".format(
            site_type, year, month_numeric_value)
        result = execute(2, query)
        if result == () or result == None:
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")

        final_list = []
        data_list = []

        tot_vals = len(result)
        tuple_idx = 0
        for idx in range(0, num_days):

            day_value = str(idx+1).zfill(2)
            day = f"{year}-{month_numeric_value}-{day_value}"

            if tot_vals > tuple_idx:
                data = result[tuple_idx]
            else:
                local_dict = {
                    "x": day,
                    "y": 0
                }

                data_list.append(local_dict)

                continue

            if day == str(data[1]):
                tuple_idx += 1

                if data[0] == None:
                    data = list(data)
                    data[0] = 0

                local_dict = {
                    "x": data[1],
                    "y": float(data[0])
                }

                data_list.append(local_dict)

            else:

                local_dict = {
                    "x": day,
                    "y": 0
                }

                data_list.append(local_dict)

        avail_data = {
            "name": request_data["site_type"],
            "data": data_list
        }
        final_list.append(avail_data)

        return Response(json.dumps({"status": "found", "details": final_list}), content_type="application/json")
    except jwt.ExpiredSignatureError as e:
            return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)

    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def data_avail_monthly_offline_count():
    fn = 'data_avail_monthly_offline_count'
    global sec_code, ser_err
    try:
        
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")

        request_data = json.loads(request.data)
       

        schema_result=check_schema(request_data,connectivity_monthly_graph)
        if schema_result!=True:
            return Response(json.dumps({"status": schema_result}), mimetype="application/json")
        if request_data['site_type'] == 'IPPS' and request_data["gen_type"] == "":
            return Response(json.dumps({"status": "Empty value found for key gen_type"}), mimetype="application/json")

        keys_to_check = ["start_month", "site_type","count_type"]
        for key in keys_to_check:
            if request_data[key] == "":
                return Response(json.dumps({"status": "Empty value found for key '{}'".format(key)}), mimetype="application/json")
            
        final_list = data_avail_monthly_offline_chart_details(request_data['start_month'], request_data['gen_type'], request_data['site_type'], request_data['count_type'])

        return Response(json.dumps(final_list), content_type="application/json")
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")

def dcu():
    fn='dev_connections'
    global ser_err, sec_code, db_conn, refresh_code
    try:
        request_data = json.loads(request.data) 
        
        if request_data["modem_link_sts"]=="external":
            query="insert into  modem_link_tbl (dcu_id,modem_link_sts,router_id) values{}".format((request_data["dev_id"],request_data["modem_link_sts"],request_data["router_id"]))
           
        if request_data["modem_link_sts"]=="Internal":
           # print(request_data["modem_details"]["user_name"])
            query="insert into  dcu_gprs_cfg_tbl (dev_id,user_name,pwd,phone_num,apn,ping_ip1,ping_ip2,interval) values{}".format(
                (request_data["dev_id"],request_data["modem_details"]["user_name"],request_data["modem_details"]["pwd"],request_data["modem_details"]["phone_num"],
                 request_data["modem_details"]["apn"],request_data["modem_details"]["ping_ip1"],request_data["modem_details"]["ping_ip2"],request_data["modem_details"]["interval"]))
            
            
        if request_data["modem_link_sts"]=="VSAT":    
            query="insert into  modem_link_tbl (dcu_id,modem_link_sts) values{}".format((request_data["dev_id"],request_data["modem_link_sts"]))
            
            
        update_tbl(query)
        return Response(json.dumps({"status": "success"}), content_type="application/json")  
            
        
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json') 
    

def data_avail_monthly_offline_count_details():
    fn = 'data_avail_monthly_offline_count'
    global sec_code, ser_err
    try:
        header = request.headers.get('Authorization')
        jwt.decode(header, sec_code, algorithms="HS256")

        request_data = json.loads(request.data)
     
            
        start_month = request_data["start_month"]
        month_name, year = start_month.split()
        month_numeric_value = Month[month_name]
        date=datetime.now().date()
        #date="2023-11-18"
        date_format = f"{year}-{month_numeric_value}"
    
        if request_data["gen_type"] == "All" or request_data["gen_type"] == "":
            site_name = request_data["site_type"].upper()

        else:
            site_type_req = request_data["site_type"].upper()
            gen_type = request_data["gen_type"]
            site_name = site_type_req+"_"+gen_type

        query = "select dev_id from dev_tbl where dev_common_status='Not Communicating' and  dev_reg_status='registered'"
       
        result = execute(2, query)
      
        if result == () or result == None:
            return Response(json.dumps({"status": "No Records Found", "details": []}), content_type="application/json")
        
        dev_list = []
        for iner_tuple in result:
            dev_list.append(iner_tuple[0])
       
        query_1 = "select dev_id from dcu_offline_status_tbl where site_name='{}' and DATE_FORMAT(block_date, '%Y-%m')='{}' and  dev_id in {} order by block_date".format(
            site_name, date_format,tuple(dev_list))
        result = execute(2, query_1)
     
        counts_dict = {}
        for element in result:
            element_str = element[0]
            counts_dict[element_str] = counts_dict.get(element_str, 0) + 1

      
       
        final_list=[]
    
        for element, count in counts_dict.items():
         
            if  request_data["chart_point"] ==0 and count >= 30:
             
                devinfo_query="SELECT dev_id,dev_name ,loc_name ,last_msg_recv_time as last_cmm_time,num_meters as met_count ,loc_id FROM dev_tbl where dev_id='{}'".format(
            element)
            elif  request_data["chart_point"] ==1 and( count>=10 and count<=29):
                
                devinfo_query="SELECT dev_id,dev_name ,loc_name ,last_msg_recv_time as last_cmm_time,num_meters as met_count ,loc_id FROM dev_tbl where dev_id='{}'".format(
            element)
            elif  request_data["chart_point"] ==2 and count<=9:
             
                devinfo_query="SELECT dev_id,dev_name ,loc_name ,last_msg_recv_time as last_cmm_time,num_meters as met_count ,loc_id FROM dev_tbl where dev_id='{}'".format(
            element)
            else:
             
                continue
          
            dev_info=get_key_value_pairs(devinfo_query)
        
          
            if dev_info==None:
                    return Response(json.dumps({"status": "not found"}), content_type="application/json")
                
            loc_info=execute(2,"select gen_type,site_type from loc_mgmt where id={}".format(dev_info[0]["loc_id"]))
            dev_info[0]["gen_type"]=loc_info[0][0]
            dev_info[0]["site_type"]=loc_info[0][1]
            modem_info=execute(1,"select modem_link_sts from modem_link_tbl where id={}".format(element))
        
            if modem_info==() or modem_info==None:
                  dev_info[0]["modem_type"]="-"
            else:
                dev_info[0]["modem_type"]=modem_info[0]

            met_info_query = "SELECT ser_port_id, meter_id,met_name ,met_sn ,met_manf_name FROM dcu_dlms_meter_tbl where dev_id='{}' and met_enable='Yes'".format(
                element)
        
            met_info=get_key_value_pairs(met_info_query)
            if met_info!=None:
                dev_info[0]["met_count"]=len(met_info)
                tempo_query="SELECT dev_name FROM dev_tbl where dev_id='{}'".format(
                element)
                tempo=execute(1,tempo_query)
          
            
                for i in range (len(met_info)):
                    table_name = "dcu_inst_data_{}_{}_{}".format(
                        element, met_info[i]['ser_port_id'], met_info[i]['meter_id'])
                    if check_table(table_name):
                        met_time=execute(1,"select met_time from {} order by 'met_time' DESC limit 1".format(table_name))
                    
                        met_info[i]["last_cmm_time"]=met_time[0].strftime('%Y-%m-%d %H:%M:%S')
                    else:
                        met_info[i]["last_cmm_time"]="-"
                    met_info[i]["dev_id"]=element
                    met_info[i]["dev_name"]=tempo[0]
                
                dev_info[0]["met_details"]=met_info
              
            else:
                dev_info[0]["met_details"]=[]
            final_list.append(dev_info[0])
     
            
        return Response(json.dumps({"status": "found","dev_count": len(counts_dict), "dev_details": final_list}), content_type="application/json")
            
         
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
    
def get_connectivity_device_monthly_details(gen_type, start_month):
    global month_numeric_value, year, num_days
    fn = "get_connectivity_device_monthly_graph"
    try:
        month_name, year = start_month.split()
        if gen_type == "All":
            site_type = "IPPS_Per"
            month_numeric_value = Month[month_name]
            num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]
        else:
            site_type = f'IPPS_{gen_type}_Per'
            month_numeric_value = Month[month_name]
            num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]
        query = "SELECT {}, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM dcu_monthly_connectivity_tbl WHERE YEAR(block_date) = '{}' AND MONTH(block_date) = '{}' ORDER BY block_date".format(
            site_type, year, month_numeric_value)
        result = execute(2, query)
        if result == () or result is None:
            return json.dumps({"status": "No Records Found", "details": []})
        final_dict = []
        data_list = []
        data_list1 = []
        tot_vals = len(result)
        tuple_idx = 0
        for idx in range(0, num_days):
            day_value = str(idx + 1).zfill(2)
            day = f"{year}-{month_numeric_value}-{day_value}"
            if tot_vals > tuple_idx:
                data = result[tuple_idx]
            else:
                local_dict = {
                    "x": day,
                    "y": "0"
                }
                local_dict1 = {
                    "x": day,
                    "y": "0"
                }
                data_list.append(local_dict)
                data_list1.append(local_dict1)
                continue
            if day == str(data[1]):
                tuple_idx += 1
                if data[0] is None or data[0] == 0:
                    data = list(data)
                    data[0] = "0"
                local_dict = {
                    "x": str(data[1]),
                    "y": str(data[0])
                }
                local_dict1 = {
                    "x": str(data[1]),
                    "y": "{:.2f}".format(100 - float(data[0]))
                }
                data_list.append(local_dict)
                data_list1.append(local_dict1)
            else:
                local_dict = {
                    "x": day,
                    "y": '0'
                }
                local_dict1 = {
                    "x": day,
                    "y": '0'
                }
                data_list.append(local_dict)
                data_list1.append(local_dict1)
        connected_data = {
            "name": "Connected",
            "data": data_list
        }
        final_dict.append(connected_data)
        disconnected_data = {
            "name": "DisConnected",
            "data": data_list1

        }
        final_dict.append(disconnected_data)
        return {"status": "found", "details": final_dict}
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
   
def offline_dcu_list():
    fn = 'offline_dcu_list()'
    try:
        query = "select dev_id, dev_name, loc_name, last_msg_recv_time as last_cmm_time from dev_tbl where dev_reg_status = 'registered' and dev_common_status = 'Not Communicating'"
        result = get_key_value_pairs(query)
        if result is None:
            return {"status": "not found"}
        for row in result:
            loc = "select gen_type, site_type from loc_mgmt where loc_name='{}'".format(row["loc_name"])
            loc_result = execute(2, loc)
            if loc_result is None or loc_result == ():
                gentype = "-"
                sitetype = "-"
            else:
                gentype = loc_result[0][0]
                sitetype = loc_result[0][1]

            modem = "select modem_link_sts from modem_link_tbl where dcu_id = '{}'".format(row["dev_id"])
            modem_result = execute(1, modem)
            if modem_result is None or modem_result == ():
                modemtype = "-"
            else:
                modemtype = modem_result[0]
                
            row["gen_type"] = gentype
            row["site_type"] = sitetype
            row["modem_type"] = modemtype
        order = ["KPCL", "KPTCL", "IPPS"]
        sorted_data = sorted(result, key=lambda x: order.index(x["site_type"]))
        for index, item in enumerate(sorted_data):
            item["id"] = index + 1
        return {"status": "found", "dev_details": sorted_data}
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
  
def get_data_avail_monthly_graph_details(start_month, gen_type, site_name):
    fn = 'get_data_avail_monthly_graph_details'
    try:
        month_name, year = start_month.split()
        month_numeric_value = Month[month_name]
        num_days = calendar.monthrange(int(year), int(month_numeric_value))[1]
        if gen_type == "All":
            site_type_req = site_name.upper()
            site_type = site_type_req + "_Per"
        else:
            site_type_req = site_name.upper()
            site_type = site_type_req + "_" + gen_type + "_" + "Per"
        query = "SELECT {}, DATE_FORMAT(block_date, '%Y-%m-%d') AS block_date FROM dcu_monthly_data_avail_info WHERE YEAR(block_date) = '{}' AND MONTH(block_date) = '{}' ORDER BY block_date".format(
            site_type, year, month_numeric_value)

        result = execute(2, query)
        if result == () or result is None:
            return json.dumps({"status": "No Records Found", "details": []})
        final_list = []
        data_list = []
        tot_vals = len(result)
        tuple_idx = 0
        for idx in range(0, num_days):
            day_value = str(idx + 1).zfill(2)
            day = f"{year}-{month_numeric_value}-{day_value}"
            if tot_vals > tuple_idx:
                data = result[tuple_idx]
            else:
                local_dict = {
                    "x": day,
                    "y": 0
                }
                data_list.append(local_dict)
                continue
            if day == str(data[1]):
                tuple_idx += 1
                if data[0] is None:
                    data = list(data)
                    data[0] = 0
                local_dict = {
                    "x": data[1],
                    "y": float(data[0])
                }
                data_list.append(local_dict)
            else:
                local_dict = {
                    "x": day,
                    "y": 0
                }
                data_list.append(local_dict)
        avail_data = {
            "name": site_name,
            "data": data_list
        }
        final_list.append(avail_data)
        return {"status": "found", "details": final_list}
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype='application/json')
  
def data_avail_monthly_offline_chart_details(start_month, gen_type, site_type, count_type):
    fn = 'data_avail_monthly_offline_chart_details'
    global sec_code, ser_err
    try:
        
        start_month = start_month
        month_name, year = start_month.split()
        month_numeric_value = Month[month_name]
        date= datetime.now().date()
        #date="2023-11-18"
        date_format = f"{year}-{month_numeric_value}"

        if gen_type == "All" or gen_type == "":
            site_name = site_type.upper()

        else:
            site_type_req = site_type.upper()
            site_name = site_type_req + "_" + gen_type

        query = "select dev_id from dev_tbl where dev_common_status='Not Communicating' and  dev_reg_status='registered'"
        result = execute(2, query)
        if result == () or result == None:
            return {"status": "No Records Found", "details": []}
        
        dev_list = []
        for iner_tuple in result:
            dev_list.append(iner_tuple[0])
        if len(dev_list) == 1:
         dev_list.append('no dev id')   
        query_1 = "select dev_id from dcu_offline_status_tbl where site_name='{}' and DATE_FORMAT(block_date, '%Y-%m')='{}' and  dev_id in {} order by block_date".format(
            site_name, date_format,tuple(dev_list))
        result = execute(2, query_1)
 
        if result == None or  result ==():
            return {"status": "No Records Found", "details": []}
            
        counts_dict = {}
        for element in result:
            element_str = element[0]
            counts_dict[element_str] = counts_dict.get(element_str, 0) + 1

        final_list = []
        dcu_offline_list = []
        meter_offline_list = []

        morethan_30 = 0
        less_than_30 = 0
        lessthan_10 = 0

        met_morethan_30 = 0
        met_lessthan_30 = 0
        met_lessthan_10 = 0

        for element, count in counts_dict.items():
            query = "SELECT met_name FROM dcu_dlms_meter_tbl where dev_id='{}' and met_enable='Yes'".format(
                element)
            num_meters_list = get_key_value_pairs(query)
            if num_meters_list == [] or num_meters_list == None:
                num_meters = 0
            else:
                num_meters = len(num_meters_list)
          
            if count >= 30:
                morethan_30 += 1
                met_morethan_30 += num_meters

            elif (count>10) and (count<29):
                less_than_30 += 1
                met_lessthan_30 += num_meters

            elif count < 10:
                lessthan_10 += 1
                met_lessthan_10 += num_meters
        

        dcu_offline_list.append(morethan_30)
        dcu_offline_list.append(less_than_30)
        dcu_offline_list.append(lessthan_10)

        meter_offline_list.append(met_morethan_30)
        meter_offline_list.append(met_lessthan_30)
        meter_offline_list.append(met_lessthan_10)

        final_dict = {}

        if count_type == "DCU Wise":
            final_dict = {
               "name": "No of DCU's Offline",
                "data": dcu_offline_list,
                "max_axis":max(dcu_offline_list)
            }

        if count_type == "Meter Wise":
            final_dict = {
                "name": "No of Meter's Offline",
                "data": meter_offline_list,
                 "max_axis":max(meter_offline_list)
            }
        final_list.append(final_dict)
        return {"status": "found", "details": final_list}
    
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return ser_err
  
def data_avail_monthly_offline_details(start_month, gen_type, site_type):
    fn = 'data_avail_monthly_offline_details'
    global sec_code, ser_err
    try:
        month_name, year = start_month.split()
        month_numeric_value = Month[month_name]
        date = datetime.now().date()
        date_format = f"{year}-{month_numeric_value}"
        if gen_type == "All" or gen_type == "":
            site_name = site_type.upper()
        else:
            site_type_req = site_type.upper()
            site_name = site_type_req + "_" + gen_type
        query = "select dev_id from dev_tbl where dev_common_status='Not Communicating' and  dev_reg_status='registered'"
        result = execute(2, query)
        # print("Details today: ", result)
        if result == () or result is None:
            return {"status": "No Records Found", "dev_details": []}
        dev_list = []
        for iner_tuple in result:
            dev_list.append(iner_tuple[0])
            dev_list.append(0)
        query_1 = "select dev_id from dcu_offline_status_tbl where site_name='{}' and DATE_FORMAT(block_date, '%Y-%m')='{}' and  dev_id in {} order by block_date".format(
            site_name, date_format, tuple(dev_list))
        # print("query site type: ", query_1)
        result = execute(2, query_1)
        # print("Details result: ", result)
        if result is None:
            return {"status": "No Records Found", "dev_details": []}
        counts_dict = {}
        for element in result:
            element_str = element[0]
            counts_dict[element_str] = counts_dict.get(element_str, 0) + 1
        # print("----counts_dict----",counts_dict)
        final_list = []
        less_than_9_list = []
        less_than_29_list = []
        greater_than_30_list = []
        for element, count in counts_dict.items():
            if count >= 30:
                dict_name = "greater_than_30"
                devinfo_query = "SELECT dev_id,dev_name ,loc_name ,last_msg_recv_time as last_cmm_time,num_meters as met_count ,loc_id FROM dev_tbl where dev_id='{}'".format(
                    element)
            elif 10 <= count <= 29:
                dict_name = "less_than_29"
                devinfo_query = "SELECT dev_id,dev_name ,loc_name ,last_msg_recv_time as last_cmm_time,num_meters as met_count ,loc_id FROM dev_tbl where dev_id='{}'".format(
                    element)
            elif count <= 9:
                dict_name = "less_than_9"
                devinfo_query = "SELECT dev_id,dev_name ,loc_name ,last_msg_recv_time as last_cmm_time,num_meters as met_count ,loc_id FROM dev_tbl where dev_id='{}'".format(
                    element)
            else:
                continue
            # print("dev_info_query: ", devinfo_query)
            dev_info = get_key_value_pairs(devinfo_query)
            if dev_info is None:
                return {"status": "not found", "dev_details": []}
            result = execute(2, "select gen_type,site_type from loc_mgmt where id={}".format(dev_info[0]["loc_id"]))
            dev_info[0]["gen_type"] = result[0][0]
            dev_info[0]["site_type"] = result[0][1]
            modem_info = execute(2, "select modem_link_sts from modem_link_tbl where id={}".format(element))
            # print(modem_info)
            if modem_info == () or modem_info is None:
                dev_info[0]["modem_type"] = "-"
            else:
                dev_info[0]["modem_type"] = modem_info[0]
            met_info_query = "SELECT ser_port_id, meter_id,met_name ,met_sn ,met_manf_name, block_interval, met_version as fw_ver FROM dcu_dlms_meter_tbl where dev_id='{}' and met_enable='Yes'".format(
                element)
            met_info = get_key_value_pairs(met_info_query)
            if met_info is not None:
                dev_info[0]["met_count"] = len(met_info)
                tempo_query = "SELECT dev_name FROM dev_tbl where dev_id='{}'".format(
                    element)
                tempo = execute(2, tempo_query)
                # print("tempo:", tempo)
                for i in range(len(met_info)):
                    table_name = "dcu_inst_data_{}_{}_{}".format(
                        element, met_info[i]['ser_port_id'], met_info[i]['meter_id'])
                    if check_table(table_name):
                        met_time = execute(2, "select met_time from {} order by 'met_time' DESC limit 1".format(
                            table_name))
                        # print("met_time: : : - : ", met_time[0])
                        met_info[i]["last_cmm_time"] = met_time[0][0].strftime('%Y-%m-%d %H:%M:%S')
                    else:
                        met_info[i]["last_cmm_time"] = "-"
                    met_info[i]["dev_id"] = element
                    met_info[i]["dev_name"] = tempo[0][0]
                dev_info[0]["met_details"] = met_info
                # print("dev and met---------------->", dev_info[0])
            else:
                dev_info[0]["met_details"] = []
            if dict_name == "less_than_9":
                less_than_9_list.append(dev_info[0])
                # print("less_than_9_list: ", less_than_9_list)
                # print("final_list: ", final_list)
            elif dict_name == "less_than_29":
                # print("DICT NAME: ", dict_name)
                less_than_29_list.append(dev_info[0])
            elif dict_name == "greater_than_30":
                greater_than_30_list.append(dev_info[0])
        dict_name_list = ['less_than_9', 'less_than_29', 'greater_than_30']
        for dict_name in dict_name_list:
            if dict_name == "less_than_9":
                if less_than_9_list:
                    final_list.append({dict_name: less_than_9_list})
            elif dict_name == "less_than_29":
                if less_than_29_list:
                    final_list.append({dict_name: less_than_29_list})
            else:
                if greater_than_30_list:
                    final_list.append({dict_name: greater_than_30_list})
        return {"status": "found", "dev_count": len(counts_dict), "dev_details": final_list}
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(' %s Error on line number : %s : %s' %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return ser_err


def offline_list():
    fn="offline_list"
    try: 
        
        query="select dev_id,dev_name,loc_name,last_msg_recv_time as last_cmm_time from dev_tbl where dev_reg_status= 'registered' and dev_common_status='Not Communicating'"
        result=get_key_value_pairs(query)
        if result==None:
              return Response(json.dumps({"status": "not found"}), content_type="application/json")
     
        for row in result:
            loc="select gen_type,site_type from loc_mgmt where loc_name='{}'".format(row["loc_name"])
            loc_result=execute (2,loc)
            
            if loc_result==None or loc_result==():
                gentype="-"
                sitetype="-"
            else:
                
                gentype=loc_result[0][0]
                sitetype=loc_result[0][1]
            
            modem="select modem_link_sts from  modem_link_tbl where dcu_id='{}'".format(row["dev_id"])
            modem_result=execute (1,modem)
            
            if modem_result==None or modem_result==():
                modemtype="-"
            else:
                modemtype=modem_result[0]
          
            row["gen_type"]=gentype
            row["site_type"]=sitetype
            row["modem_type"]=modemtype
            
            
        order = ["KPCL", "KPTCL", "IPPS"]

        sorted_data = sorted(result, key=lambda x: order.index(x["site_type"]))

        for index, item in enumerate(sorted_data):
            item["id"] = index + 1
        
        return Response(json.dumps({"status": "found", "dev_details":sorted_data}), content_type="application/json")
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
               (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")





def get_data_poll():
    fn="get_data_poll"
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
       
        dev_id = request.args.get('dev_id')
        loc_id = request.args.get('loc_id')
        if not dev_id:
            return Response(json.dumps({"status": "error", "message": "dev_id is required"}), content_type="application/json", status=400)
 
        # Query the database
        query = "SELECT  dev_id,loc_id, data_poll , debug_poll as 'debug_log' FROM dcu_poll_config_data WHERE dev_id = {}".format(dev_id)
 
        result = get_key_value_pairs(query)  
       
 
        final_json = {
                "status": "found"
            }
       
       
        if result and len(result) > 0:
            result[0]["data_poll"] = json.loads(result[0]["data_poll"])
            result[0]["debug_log"] = json.loads(result[0]["debug_log"])
            final_json.update(result[0])
            print(final_json)
            return Response(json.dumps(final_json), content_type="application/json")
        else:
            return Response(json.dumps({
                "status": "not found"
            }), content_type="application/json")
 
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
        logerr(" %s Error on line number : %s : %s" %
                (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype="application/json")
 
   
def save_data_poll():
    fn="save_data_poll"
    try:
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
        # Parse input JSON
        input_data = request.get_json()
 
        dev_id = input_data.get("dev_id")
        loc_id = input_data.get("loc_id")
        data_poll = json.dumps(input_data.get("data_poll"))  
        debug_poll = json.dumps(input_data.get("debug_log"))
        print("data_poll---------------------------------------->",data_poll)
        print("debug_poll---------------------------------------->",debug_poll)
 
        check_query = "SELECT * FROM dcu_poll_config_data WHERE dev_id = {}".format(dev_id)
        result = execute(1,check_query)
 
        if result:
            # Update record
            update_query = "UPDATE dcu_poll_config_data SET loc_id = '{}', data_poll = '{}', debug_poll = '{}' WHERE dev_id = {}".format(loc_id,data_poll,debug_poll,dev_id)
            values = update_tbl(update_query)
           
        else:
            # Insert new record
            insert_query = "INSERT INTO dcu_poll_config_data (dev_id, loc_id, data_poll, debug_poll) VALUES ('{}', '{}', '{}', '{}')".format(dev_id, loc_id, data_poll, debug_poll)
            values = update_tbl(insert_query)
 
        if values is True:
            return Response(json.dumps({"status": "success"}), mimetype="application/json")
        else:
            return Response(json.dumps({"status": "error"}), mimetype="application/json")
 
    except Exception as e:
        print("---------------------------------")
        print(f"{fn}: Error occurred - {e}")
        print("---------------------------------")
        return Response(json.dumps({"status": "error", "message": str(e)}), mimetype="application/json")
 
 

# def alert():
#     print("----------------------------------------")
#     # dev=get_key_value_pairs("select dev_name from dev_tbl")
#     # for dev_id in dev:
#     #     print(dev_id["dev_name"])
#     #     meter=get_key_value_pairs("select met_name from dcu_dlms_mater_tbl whwre ")
    
#     for i in range(0,2):
#         current_time=datetime.now().strftime('%Y-%m-%d %H:%M:%S')
#         random_number = random.randint(1000, 9999)
      
#         query="insert into alerts  (date_time,msg_type,msg,dcu_name,flag,unique_id) values ('{}','Communication Events','Device Communicating','DCU_133','true','1607009635_2_5')".format(current_time)
#         # query="insert into plcc_tbl  (rs_status,meter_ser_num,node_id,parent_id,nc_id,nc_sn,dev_id,meter_id) values ('communicating','{}','23','14','2','85647','64304','{}')".format(random_number,str(i))
#         print(query)
#         update_tbl(query)
#         print(i)
#         time.sleep(1)
        
#     return "ok"

# def bool():
#     flag="update alerts set flag ='true' where flag ='false'"
#     update_tbl(flag)
#     return "ok"
def to_mm_ss(seconds):
    if seconds is None:
        return None
    minutes = int(seconds) // 60
    sec = int(seconds) % 60
    return f"{minutes:02}:{sec:02}"

# def dcu_event_getlist_report_backend():
#     fn="dcu_event_getlist_report_backend"
#     try:
#         header = request.headers.get('Authorization')
#         jwt.decode(header, sec_code, algorithms="HS256")
#         input=json.loads(request.data)
   
   
#         s_day,s_month_name, s_year = input["start_date"].split()
#         s_month = Month[s_month_name]
#         start_date = f"{s_year}-{s_month}-{s_day}"
       
#         e_day,e_month_name, e_year = input["end_date"].split()
#         e_month = Month[e_month_name]
#         end_date = f"{e_year}-{e_month}-{e_day}"
#         if input["meter_name"] !="All":
#             port,meter,meter_name=input["meter_name"].split("$$")
#             input["meter_name"]=meter_name
#             input["unique_id"]="{}_{}_{}".format(input["dev_id"],port,meter)
        
     
#         for key, value in input.items():
#             if value is None:
#                 return Response(json.dumps({"status": "Empty value found for key {}".format(key)}), mimetype="application/json")
#         table_name="dcu_event_{}_tbl".format(input["dev_id"])
#         if check_table(table_name)==True:
        
#             if input["meter_name"] =="All":
#                 if  input["type"] =="All":
#                     query="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}'".format(input["dcu_name"],table_name,start_date,end_date)
#                     final_result=get_key_value_pairs(query)
#                     if final_result!=None:
#                         final_result=get_meter_name(final_result)
#                     result=dcu_alert_all(start_date,end_date,input,final_result)
                    
#                 else:
#                     if input["type"] !="Meter Events":
#                         query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' ".format(input["dcu_name"],table_name,start_date,end_date,input["type"])
#                         final_result=get_key_value_pairs(query1)
#                         if final_result!=None:
#                             result=get_meter_name(final_result)
#                         result=final_result
                    
#                     else:
#                         if input["event_code"]=="All":
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' ".format(input["dcu_name"],table_name,start_date,end_date,input["type"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                        
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
#                         else:
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' and event_desc='{}'".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["event_code"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                        
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
                        
            
#             if input["meter_name"] !="All":
#                 if  input["type"] =="All":
#                     query1="select event_time, event_type as type,event_desc as events ,'{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and unique_id='{}'".format(input["dcu_name"],table_name,start_date,end_date,input["unique_id"])
#                     final_result=get_key_value_pairs(query1)
#                     if final_result!=None:
#                             final_result=get_meter_name(final_result)
#                     result=dcu_alert_all(start_date,end_date,input,final_result)
                    
#                 else:
#                     if input["type"] !="Meter Events":
#                         query1="select event_time, event_type as type,event_desc as events ,'{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}' and unique_id='{}' ".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["unique_id"])
#                         final_result=get_key_value_pairs(query1)
#                         if final_result!=None:
#                             result=get_meter_name(final_result)
#                         result=final_result
                    
#                     else:
#                         if input["event_code"]=="All":
                    
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}'  and unique_id='{}' ".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["unique_id"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                            
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
#                         else:
#                             query1="select event_time , event_type as type,event_desc as events , '{}' as dcu_name , unique_id from {} where DATE(event_time) BETWEEN '{}' and '{}' and event_type='{}'  and unique_id='{}' and event_desc='{}'".format(input["dcu_name"],table_name,start_date,end_date,input["type"],input["unique_id"],input["event_code"])
#                             final_result=get_key_value_pairs(query1)
#                             if final_result!=None:
#                                 final_result=get_meter_name(final_result)
                            
#                             result=dcu_alert_all(start_date,end_date,input,final_result)
#             print(result)
#             result = sort_by_meter_name(result)
#             if result!=None and result!=[]:
#                 return Response(json.dumps(  {"status": "found", "alerts_count": len(result), "details": result}),content_type="application/json")
#             else:
#                 return Response(json.dumps(  {"status": "not found", "alerts_count": 0, "details": result}),content_type="application/json")
#         else:
#             return Response(json.dumps(  {"status": "not found", "alerts_count": 0, "details": []}),content_type="application/json")    
#     except Exception as e:
#         print("---------------------------------")
#         exc_type, exc_obj, exc_tb = sys.exc_info()
#         logerr(" %s Error on line number : %s : %s" %
#                 (fn, exc_tb.tb_lineno, e))
#         print("---------------------------------")
#         return Response(ser_err, mimetype="application/json")
    
def sort_by_meter_name(data: List[Dict]) -> List[Dict]:
    return sorted(data, key=lambda x: x.get('meter_name', ''))

   
# def select_execute(query, data_type=None):
#     fn= "select_execute()"
#     try:
    
#         connection = mysql.connector.connect(
#             host=127.0.0.1
#             database=mdas,
#             user= root,
#             password="Softel@123",
#             connection_timeout=10
#         )
#         cursor = connection.cursor()
#         attempts = 0

#         while attempts < 3:
#             try:
#                 # print(query)
#                 cursor.execute(query)
#                 # if query.strip().upper().startswith("SELECT"):
#                 if data_type == "all":
#                     result = cursor.fetchall()
#                     cursor.close()  # Close cursor after fetching results
#                     return result

#                 elif data_type == "one":
#                     result = cursor.fetchone()
#                     cursor.close()  # Close cursor after fetching results
#                     return result
                
#                 else:
#                     connection.commit()
#                     cursor.close()  # Close cursor after executing non-select query
#                     return "Query executed successfully."

#             except Error as e:
#                 attempts += 1
#                 exc_type, exc_obj, exc_tb = sys.exc_info()
#                 logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
#                 # print(f"Error: {e}. Retrying...")
#                 time.sleep(5)
#         else:
#             print("Failed after 3 attempts.")
#             return None

#     except Error as e:
#         exc_type, exc_obj, exc_tb = sys.exc_info()
#         logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
#         # print(f"Error connecting to MySQL: {e}")
#         return None

#     finally:
#         if 'connection' in locals() and connection.is_connected():
#             connection.close()  # Close connection in the finally block
#             # print("connection got closed!")


    
# def da_module():
#     fn="dcu_event_getlist_report_backend"
#     try:
#         # qry = "SELECT dev_common_status FROM `dev_tbl` WHERE dev_id= '{}' and dev_type = '{}' and dev_reg_status = 'registered'".format(DEV_ID, DEV_TYPE)
#         # res = execute(qry, "one")
#         res="ueihudf"
        
#         now = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
#         recv_json=json.loads(request.data)
        
#         for idx in recv_json["DATA"]["INST_VAL"]:
            
#             if idx[12]!="Not Configured":
#                 unique_id="{}_{}_{}".format(1607001111,idx[5],idx[6])
#                 if res != None:
#                     tbl1 = "met_connectivity_{}".format( unique_id)
#                     # check_tbls(tbl1, 'met_connectivity')
                    
                    
#                     query="select comn_sts, start_time,end_time, id  from {} order by id desc limit 1".format(tbl1)
#                     new_entry = execute(1,query)
                    
#                     if new_entry==None:
#                         if idx[12]=="Communicating":
#                             update_new = 'INSERT INTO `{}` (start_time, comn_sts) VALUES{}'.format(tbl1, (now, idx[12]))
#                             print(update_new)
#                             update_tbl(update_new)
#                     else:
#                         timestamp1 = datetime.strptime(now, "%Y-%m-%d %H:%M:%S")
#                         timestamp2 = new_entry[1]
#                         time1 =timestamp1 
#                         time2 = timestamp2
#                         duration = time1 - time2
#                         duration = str(timedelta(seconds=duration.total_seconds()))
                        
#                         if  idx[12]=="Not Communicating":
#                             if new_entry[0]=="Communicating":
#                                 qry = "UPDATE `{}` SET   end_time ='{}', duration = '{}' WHERE id= '{}'".format(tbl1,now, duration, new_entry[3])
#                                 update_tbl(qry)
                                
#                                 update_new = 'INSERT INTO `{}` (start_time,comn_sts) VALUES{}'.format(tbl1, (now, "Not Communicating"))
#                                 update_tbl(update_new)
                               
                                
#                         if  idx[12]=="Communicating":
#                                 if new_entry[0]=="Not Communicating":
#                                     qry = "UPDATE `{}` SET   end_time ='{}', duration = '{}' WHERE id= '{}'".format(tbl1,now, duration, new_entry[3])
#                                     update_tbl(qry)
#                                     update_new = 'INSERT INTO `{}` (start_time,comn_sts) VALUES{}'.format(tbl1, (now, "Communicating"))
#                                     update_tbl(update_new)
                                
#         return "ok"
                                            
#     except Exception as e:
#         print("---------------------------------")
#         exc_type, exc_obj, exc_tb = sys.exc_info()
#         logerr(" %s Error on line number : %s : %s" %
#                 (fn, exc_tb.tb_lineno, e))
#         print("---------------------------------")
#         return Response(ser_err, mimetype="application/json")                            
                            
                            
                            

def test():
# Step 1: Retrieve table names
    qry1=("""
    SELECT table_name
    FROM information_schema.tables
    WHERE table_schema = 'rms_kptcl'
      AND (table_name LIKE 'meter_data_avail_info_%' OR table_name LIKE '%meter_data_avail_info_
      
      ');
""")
    tables = execute(2,qry1)
    
    print(tables)
    # return "ok"

# Step 2: Copy data from each table
    for table in tables:
        table_name = table[0] 
        print(table_name+"_test")
        test=table_name+"_test"
        qry= """CREATE TABLE `{}` (
            `id` int NOT NULL AUTO_INCREMENT,
            `recv_count` varchar(45) DEFAULT NULL,
            `day_per` varchar(45) DEFAULT NULL,
            `ls_date` varchar(45) DEFAULT NULL,
            `expect_count` varchar(45) DEFAULT NULL,
            PRIMARY KEY (`id`),
            UNIQUE KEY `ls_date_UNIQUE` (`ls_date`)
            ) ENGINE=MyISAM  DEFAULT CHARSET=latin1""".format(test)
        execute(8,qry)

     
        copy_query = f"""
        INSERT INTO {test} (recv_count, day_per, ls_date, expect_count)
        SELECT recv_count, day_per, ls_date, expect_count
        FROM rms_kptcl.{table_name};
        """
        
        
        print(copy_query)
        execute(8,copy_query)


    return "ok"

def add_dev():
    fn = "add_device()"
    global ser_err1, sec_code, db_conn
    try:
       
        header = request.headers.get('Authorization')
        header = header.encode()
        jwt.decode(header, sec_code, algorithms="HS256")
       
        login_dict = json.loads(request.data)
        results= execute(2,"SELECT dev_id, dev_name,dev_ip_addr FROM dev_tbl")
        id = len(results)
       
        print("==========input=================",login_dict)
     
        result= execute(1,"SELECT dev_model,dev_type FROM dcu_serialnum_details WHERE serial_num ='{}'".format(login_dict["device_id"]))
        device_type=result[1]
        device_model=result[0]
     
 
        if "single" in device_model or "MIM_1PORT" in device_model or "DCU_1PORT" in device_model:
            login_dict["port_no"] = "1"
       
 
        elif "2" in device_model :
            login_dict["port_no"] = "2"
           
        elif "4" in device_model :
            login_dict["port_no"] = "4"
        elif "DCU_PLCC" in device_model:
            login_dict["port_no"] = "1"
       
        # print(login_dict["port_no"])
        # dev_type_re= execute(2,"SELECT  FROM dcu_serialnum_details WHERE serial_num = '{}'".format(login_dict["device_id"]))
        # dev_type_result = dev_type_re[0][0]
       
 
        for indx in range(0, id):
           
            if results[indx][1] == login_dict["device_name"]:
                return Response(json.dumps({"status": "Device name already exists!"}), mimetype='application/json')
            if results[indx][2] == login_dict["ip_address"]:
                return Response(json.dumps({"status": "Device IP already exists!"}), mimetype='application/json')
           
     
        sql= "UPDATE dcu_serialnum_details SET flag='true' WHERE serial_num = '{}'".format(login_dict["device_id"])
        update_tbl(sql)
       
        sql = "INSERT IGNORE INTO meter_type_password_tbl (`meter_type_name`, `password`) VALUES ('{}','{}')".format("L&T", "Admin@123")
        update_tbl(sql)
        print(login_dict["port_no"])
 
        if login_dict["port_no"] == "1":
            
            
       
 

 
            if device_type =="DCU" and  device_model != "DCU_PLCC":
                print("==========dev_type_result==1==================",device_type)
                
                if device_model=='DCU-Single Port General' or 'DCU_1PORT':
                    device_model='DCU_1P'

                sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1) VALUES ('{}','{}', '{}','{}', '{}','{}','{}')".format(
                login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8")
                update_tbl(sql)
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"DCU",device_model,login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","5", datetime.now(), "FW_1234")
                update_tbl(sql)
                print("==========sql==========1==========",sql)
 
                for meter in range(0, 5):
                    if meter <2:
                        sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1", login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql)
                   
                    if meter >=2 and meter<5:
                        sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1", login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql)
           
            elif device_type =="Modem" and device_model== "MIM_1PORT":
                if device_model=='MIM_1PORT':
                    device_model='MIM_1P'
                sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1) VALUES ('{}','{}', '{}','{}', '{}','{}','{}')".format(
                login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8")
                update_tbl(sql)
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"MIM",device_model,login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","1", datetime.now(), "FW_1234")
                update_tbl(sql)
 
                sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], 1, "No", "Incomer", 1, "123", "123","meter_0_{}".format(1), "{}_1_{}".format(login_dict["device_id"],1),"1", login_dict["device_name"],"{}_{}".format( login_dict["device_id"],1), "met_1_{}".format(1), "Not Configured", "L&T")
                update_tbl(sql)
            
            elif device_type =="DCU" and device_model == "DCU_PLCC":
                if device_model=='DCU_PLCC':
                    device_model=='DCU_PLCC'
                sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1) VALUES ('{}','{}', '{}','{}', '{}','{}','{}')".format(
                login_dict["device_id"], "RS485_MODE", "2", "9600","1", "Even","8")
                update_tbl(sql)

                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"DCU",device_model,login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","1", datetime.now(), "FW_1234")
                update_tbl(sql)
 
                # for meter in range(0, 20):
                #         sql_111 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name,comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                #         login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"123_{}".format(meter),"met_1_{}".format(meter+1), "Not Configured", "L&T")
                #         update_tbl(sql_111)
 
            else:
                
                sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1) VALUES ('{}','{}', '{}','{}', '{}','{}','{}')".format(
                login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8")
                update_tbl(sql) 
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"-","-",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","5", datetime.now(), "FW_1234")
                update_tbl(sql)
 
                for meter in range(0, 5):
                    if meter <2:
                        sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1", login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql)
                   
                    if meter >=2 and meter<5:
                        sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1", login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql)
                   
        if login_dict["port_no"] == "2":
 
            #dcu_ser_port_cfg_tbl
            sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1, port_type2,  ser_met_add_size2, ser_baud2, ser_stop_bit2, ser_parity2, ser_data_bits2) VALUES ('{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
            login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8", "RS485_MODE", "2", "9600","1", "Even","8")
            update_tbl(sql)
            
            print("====device model=======",device_model,type(device_model))
 
            if device_type =="DCU":
                if device_model=='DCU_2PORT':
                    print("-------------1----------")
                    device_model='DCU_2P'
                elif device_model=='DCU_MDM_2PORT':
                    print("-------------2----------")
                    device_model='DCU_MDM_2P'
                
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"DCU",device_model,login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","10", datetime.now(), "FW_1234")
                update_tbl(sql)
           
            else:
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"-","-",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","10", datetime.now(), "FW_1234")
                update_tbl(sql)
 
            for meter in range(0, 10):
                if meter <2:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql)
               
                if meter >=2 and meter<5:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter), "met_1_{}".format(meter+1), "Not Configured", "L&T")
                    update_tbl(sql)
 
                if meter >=5 and meter<7:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter-4, "No", "Incomer", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4),"2",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_2_{}".format(meter-4) , "Not Configured", "L&T")
                    update_tbl(sql)
                if meter >=7 and meter<10:
                    sql = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                    login_dict["device_id"], meter-4, "No", "Live Outgoing", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4),"2",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_2_{}".format(meter-4), "Not Configured", "L&T" )
                    update_tbl(sql)
 
        if login_dict["port_no"] == "4":
            #dcu_ser_port_cfg_tbl
            sql = "INSERT INTO dcu_ser_port_cfg_tbl (`dev_id`, port_type1,  ser_met_add_size1, ser_baud1, ser_stop_bit1, ser_parity1, ser_data_bits1, port_type2,  ser_met_add_size2, ser_baud2, ser_stop_bit2, ser_parity2, ser_data_bits2, port_type3,  ser_met_add_size3, ser_baud3, ser_stop_bit3, ser_parity3, ser_data_bits3, port_type4,  ser_met_add_size4, ser_baud4, ser_stop_bit4, ser_parity4, ser_data_bits4) VALUES ('{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
            login_dict["device_id"], "RS232_MODE", "2", "9600","1", "Even","8", "RS232_MODE", "2", "9600","1", "Even","8", "RS485_MODE", "2", "9600","1", "Even","8", "RS485_MODE", "2", "9600","1", "Even","8")
            update_tbl(sql)
 
            if device_type =="DCU" and device_model != "DCU_PLCC":
              
                if device_model=='DCU_4PORT':
                    device_model='DCU_4P'
                elif device_model=='DCU_MDM_4PORT':
                    device_model='DCU_MDM_4P'
                
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"DCU",device_model,login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","20", datetime.now(), "FW_1234")
                update_tbl(sql)
                print("=========sql===============",sql)
 
                for meter in range(0, 20):
                    if meter <2:
                        sql_111 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name,comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql_111)
 
                    if meter >=2 and meter<5:
                        sql_111 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql_111)
 
                    if meter >=5 and meter<7:
                        sql_112 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-4, "No", "Incomer", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4), "2",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_2_{}".format(meter-4), "Not Configured", "L&T")
                        update_tbl(sql_112)
                   
                    if meter >=7 and meter<10:
                        sql_112 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-4, "No", "Live Outgoing", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4), "2",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_2_{}".format(meter-4), "Not Configured", "L&T")
                        update_tbl(sql_112)
 
                    if meter >=10 and meter<12:
                        sql_113 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-9, "No", "Incomer", meter-9, "123", "123","meter_2_{}".format(meter), "{}_3_{}".format(login_dict["device_id"],meter-9),"3",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_3_{}".format(meter-9), "Not Configured", "L&T" )
                        update_tbl(sql_113)
                    if meter >=12 and meter<15:
                        sql_113 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-9, "No", "Live Outgoing", meter-9, "123", "123","meter_2_{}".format(meter), "{}_3_{}".format(login_dict["device_id"],meter-9),"3",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_3_{}".format(meter-9), "Not Configured", "L&T" )
                        update_tbl(sql_113)
 
                    if meter >=15 and meter<17:
                        sql_114 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-14, "No", "Incomer", meter-14, "123", "123","meter_3_{}".format(meter), "{}_4_{}".format(login_dict["device_id"],meter-14),"4",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_4_{}".format(meter-14), "Not Configured" , "L&T")
                        update_tbl(sql_114)
                   
                    if meter >=17 and meter<20:
                        sql_114 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-14, "No", "Live Outgoing", meter-14, "123", "123","meter_3_{}".format(meter), "{}_4_{}".format(login_dict["device_id"],meter-14),"4",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_4_{}".format(meter-14), "Not Configured" , "L&T")
                        update_tbl(sql_114)            
           
            else:
                #dev_tbl
                sql = "INSERT INTO dev_tbl (`loc_id`, loc_name, dev_id, dev_name, dev_type, dev_model, dev_sn, `dev_ip_addr`, num_ser_port, dev_common_status,num_meters, last_msg_recv_time, fw_ver) VALUES ('{}','{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
                login_dict["location_id"], login_dict["location_name"],login_dict["device_id"],login_dict["device_name"],"-","-",login_dict["device_id"],  login_dict["ip_address"], login_dict["port_no"], "Not Communicating","20", datetime.now(), "FW_1234")
                update_tbl(sql)
 
                for meter in range(0, 20):
                    if meter <2:
                        sql_111 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name,comn_status, meter_type_name) VALUES ('{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Incomer", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql_111)
 
                    if meter >=2 and meter<5:
                        sql_111 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter+1, "No", "Live Outgoing", meter+1, "123", "123","meter_0_{}".format(meter), "{}_1_{}".format(login_dict["device_id"],meter+1),"1",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_1_{}".format(meter+1), "Not Configured", "L&T")
                        update_tbl(sql_111)
 
                    if meter >=5 and meter<7:
                        sql_112 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-4, "No", "Incomer", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4), "2",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_2_{}".format(meter-4), "Not Configured", "L&T")
                        update_tbl(sql_112)
                   
                    if meter >=7 and meter<10:
                        sql_112 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-4, "No", "Live Outgoing", meter-4, "123", "123","meter_1_{}".format(meter), "{}_2_{}".format(login_dict["device_id"],meter-4), "2",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_2_{}".format(meter-4), "Not Configured", "L&T")
                        update_tbl(sql_112)
 
                    if meter >=10 and meter<12:
                        sql_113 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-9, "No", "Incomer", meter-9, "123", "123","meter_2_{}".format(meter), "{}_3_{}".format(login_dict["device_id"],meter-9),"3",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_3_{}".format(meter-9), "Not Configured", "L&T" )
                        update_tbl(sql_113)
                    if meter >=12 and meter<15:
                        sql_113 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn,met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-9, "No", "Live Outgoing", meter-9, "123", "123","meter_2_{}".format(meter), "{}_3_{}".format(login_dict["device_id"],meter-9),"3",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_3_{}".format(meter-9), "Not Configured", "L&T" )
                        update_tbl(sql_113)
 
                    if meter >=15 and meter<17:
                        sql_114 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-14, "No", "Incomer", meter-14, "123", "123","meter_3_{}".format(meter), "{}_4_{}".format(login_dict["device_id"],meter-14),"4",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_4_{}".format(meter-14), "Not Configured" , "L&T")
                        update_tbl(sql_114)
                   
                    if meter >=17 and meter<20:
                        sql_114 = "INSERT INTO dcu_dlms_meter_tbl (`dev_id`, meter_id, met_enable, met_type, `met_addr`, comm_port, met_pass, met_loc, unique_id, ser_port_id, dev_name, met_sn, met_name, comn_status, meter_type_name) VALUES ('{}','{}','{}', '{}','{}', '{}','{}','{}','{}', '{}','{}', '{}','{}','{}','{}')".format(
                        login_dict["device_id"], meter-14, "No", "Live Outgoing", meter-14, "123", "123","meter_3_{}".format(meter), "{}_4_{}".format(login_dict["device_id"],meter-14),"4",login_dict["device_name"],"{}_{}".format( login_dict["device_id"],meter),"met_4_{}".format(meter-14), "Not Configured" , "L&T")
                        update_tbl(sql_114)
 
 
 
            #dcu_dlms_meter_tbl
                   #dcu_gen_config_tbl
        
        sql_1 = "INSERT INTO dcu_gen_config_tbl(dev_id, dbglog_enable,dbglog_ip, net_type, eth_ip1,eth_subnet1,  eth_gateway1,  eth_ip2,eth_subnet2,  eth_gateway2, eth_ip3,eth_subnet3,  eth_gateway3, eth_ip4,eth_subnet4, eth_gateway4, ntp_enable1, ntp_enable2, ntp_ipadd1, ntp_port1,ntp_ipadd2, ntp_port2, ntp_interval, iec_enable,ftp_enable, modtcp_enable, vpn_enable,restapi_enable, gprs_enable, mqtt_enable) VALUES ('{}', '{}','{}', '{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}','{}', '{}')".format(
        login_dict["device_id"] ,"No","192.168.10.105","na","192.168.10.58","255.255.255.0","192.168.10.1","192.168.11.58","255.255.255.0","192.168.11.1","192.168.10.107","255.255.255.0","192.168.10.118",'192.168.10.109',"255.255.255.0","192.168.10.128","No","No","192.168.10.141","123","192.168.10.142","1234","10","No","No","No","No","No","No", "Yes")       #, eth_new_ip1, eth_new_ip2, , login_dict["ip_address"], login_dict["ip_address"]     , '{}'
        update_tbl(sql_1)
       
        #dcu_iec104_cfg_tbl
        sql_12 = "INSERT INTO dcu_iec104_cfg_tbl(dev_id, asdu, port, t1, t2, t3,iot_size, cot_size, cyc_int,enable_allow_master,enable_ip1,master1_ip,enable_ip2,master2_ip,enable_ip3,master3_ip,enable_ip4,master4_ip, start_ioa) VALUES ('{}', '{}', '{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}','{}', '{}', '{}')".format(
        login_dict["device_id"] ,"192.168.10.105","1234","12","13","10","3","2","30","No","No","192.168.10.160",'No',"192.168.10.81","No","192.168.10.82","No","192.168.10.83", "2000")
        update_tbl(sql_12)
 
        #dcu_ftp_cfg_tbl
        sql_13 = "INSERT INTO dcu_ftp_cfg_tbl(dev_id, ip, port, user_name, pwd, directory, periodicity) VALUES ('{}', '{}', '{}','{}', '{}', '{}', '{}')".format(
        login_dict["device_id"] ,"192.168.10.1","21","internet","internet","local/files/", "60")
        update_tbl(sql_13)
 
        #dcu_modtcp_cfg_tbl
        sql_14 = "INSERT INTO dcu_modtcp_cfg_tbl(dev_id, ip, port, slave_id, resp_to_all, hold_reg_st_addr, input_reg_st_addr, read_coil_st_addr, read_discrete_st_addr) VALUES ('{}', '{}', '{}','{}', '{}', '{}', '{}', '{}', '{}')".format(
        login_dict["device_id"] ,"192.168.10.1","21","1","1","1000", "1000", "3000", "5000")
        update_tbl(sql_14)
 
        #dcu_vpn_cfg_tbl
        # sql_15 = "INSERT INTO dcu_vpn_cfg_tbl(dev_id, instance, gen_details, phase1_details, phase2_details, vpmppt_details) VALUES ('{}', '{}', '{}','{}', '{}', '{}')".format(
        # login_dict["device_id"] ,"1","NA","NA","NA","NA")
        # update_tbl(sql_15)
 
        #dcu_gprs_cfg_tbl
        sql_16 = "INSERT INTO dcu_gprs_cfg_tbl(dev_id, user_name, pwd, phone_num, apn, ping_ip1, ping_ip2, `interval`, num_attempts, ping_enable, connection_type) VALUES ('{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}', '{}')".format(
        login_dict["device_id"], "internet", "internet", "*99#", "uninet", "0.0.0.0", "0.0.0.0", "0", "2", "No", "NA")
        update_tbl(sql_16)
       
        query="select dev_type, dev_model from dcu_serialnum_details where serial_num= '{}'".format(  login_dict["device_id"])
        ser=execute(2,query)
   
        if ser[0][0]=="DCU" or ser[0][1] in ["MIM_1PORT"]:
           
            query_modem="insert into  modem_link_tbl (dcu_id,modem_link_sts,router_id,modem_enable) values ('{}','{}','{}','{}')".format( login_dict["device_id"],"Internal","NA","NA")
            update_tbl(query_modem)
            query_vpn= "INSERT INTO dcu_vpn_cfg_tbl(dev_id,instance,gen_details,phase1_details,phase2_details,vpmppt_details) values ('{}','{}','{}','{}','{}','{}')".format( login_dict["device_id"],"NA","{}","{}","{}","{}")
            update_tbl(query_vpn)
       
            # query_router_vpn= "INSERT INTO router_vpn_cfg_tbl(dev_id,instance,gen_details,phase1_details,phase2_details,vpmppt_details) values ('{}','{}','{}','{}','{}','{}')".format( login_dict["device_id"],"NA","NA","NA","NA","NA")
            # update_tbl(query_router_vpn)
 
        else:
            query_modem="insert into  modem_link_tbl (dcu_id,modem_link_sts,router_id,modem_enable) values ('{}','{}','{}','{}')".format( login_dict["device_id"],"VSAT","NA","NA")
            update_tbl(query_modem)
     
        print("1",sql_1)
        print("2",sql_12)
        print("3",sql_13)
        print("4",sql_14)
        print("6",sql_16)
        #dcu_mqtt_cfg_tbl
        query_DCU_MQTT="INSERT INTO dcu_mqtt_cfg_tbl(dev_id, mqtt_broker, mqtt_broker_port, user_name, pwd, client_id, hc_int, publish_topic, subscribe_topic, dcu_da_mod_topic, dcu_eve_mod_topic, dcu_diag_mod_topic) values ('{}','{}','{}','{}','{}','{}', '{}','{}','{}','{}','{}','{}')".format(login_dict["device_id"],login_dict["ip_address"],login_dict["port_no"], login_dict["active_user"], "softel", "NA",60,"NA","NA","NA","NA","NA")
        update_tbl(query_DCU_MQTT)
 
        #dcu_restapi_cfg_tbl
        query_restapi="INSERT INTO dcu_restapi_cfg_tbl(dev_id, user_name, pwd, url, periodicity) values ('{}','{}','{}','{}','{}')".format( login_dict["device_id"],login_dict["active_user"],"NA","NA","NA")
        update_tbl(query_restapi)
       
        query=update_tbl("UPDATE dev_tbl SET `lat`='{}', lng='{}'  where dev_id='{}'".format(login_dict["lat"],login_dict["lng"],login_dict["device_id"]))
        #audit_table
        sql = "INSERT INTO audit_table (date_time, active_role,  active_user, `audit_type`, `audit_text`) VALUES ('{}', '{}', '{}', '{}', '{}')".format(datetime.now(),
        login_dict["active_role"],  login_dict["active_user"], "Device Management", "device ({}) added successfully".format(login_dict["device_name"]))
        print("7",query_DCU_MQTT)
        print("8",query_restapi)
        print("9",query)
        update_tbl(sql)
        return Response(json.dumps({"status": "success"}), mimetype='application/json')
   
    except jwt.ExpiredSignatureError as e:
        return Response(json.dumps({"status": "Signature expired"}), mimetype='application/json', status=200)
    except Exception as e:
        print("---------------------------------")
        exc_type, exc_obj, exc_tb = sys.exc_info()
 
        logerr(' %s Error on line number : %s : %s' % (fn, exc_tb.tb_lineno, e))
        print("---------------------------------")
        return Response(ser_err, mimetype ='application/json')
 
 
def convert_to_days_hms(time_str):

    time_str = time_str.lstrip(' ')
    if ',' in time_str:
        time_list = [t.strip() for t in time_str.split(',')]
        time_list = sorted(time_list, key=lambda x: datetime.strptime(x, "%H:%M"))
        max_time = time_list[-1]
        return f"0 days {max_time}:00"

    # If format is hh:mm, treat as 0 days hh:mm:ss
    if re.match(r"^\d{1,2}:\d{2}$", time_str):
        return f"0 days {time_str}:00"
    
    # If format is hh:mm:ss, treat as 0 days hh:mm:ss
    elif re.match(r"^\d{1,2}:\d{2}:\d{2}$", time_str):
        return f"0 days {time_str}"

    # If format is '15 min' or '28 min'
    elif re.match(r"^\d+\s*min$", time_str):
        minutes = int(re.findall(r"\d+", time_str)[0])
        return f"0 days 00:{minutes:02d}:00"
    
    # If format is '5 days'
    elif re.match(r"^\d+\s*days?$", time_str):
        days = int(re.findall(r"\d+", time_str)[0])
        return f"{days} days 00:00:00"

    # If format is '1 day 2:42' or '1 days 19:17'
    elif re.match(r"^\d+\s*day[s]?\s+\d{1,2}:\d{2}$", time_str):
        parts = re.findall(r"\d+", time_str)
        days, hours, minutes = int(parts[0]), int(parts[1]), int(parts[2])
        return f"{days} days {hours:02d}:{minutes:02d}:00"

    # If format is '0 min'
    elif time_str.strip() == "0 min":
        return "0 days 00:00:00"
    
    else:
        return "Invalid Format"
 
 
 
 
