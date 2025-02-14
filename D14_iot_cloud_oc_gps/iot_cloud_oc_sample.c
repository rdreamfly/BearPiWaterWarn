/*
 * Copyright (c) 2020 Nanjing Xiaoxiongpai Intelligent Technology Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include<time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ohos_init.h"
#include "cmsis_os2.h"

#include "wifi_connect.h"
#include <queue.h>
#include <oc_mqtt_al.h>
#include <oc_mqtt_profile.h>
#include "E53_ST1.h"
#include <dtls_al.h>
#include <mqtt_al.h>

#include "adc_example.h"
#include "pwm_example.h"
#include "led_example.h"



#define CONFIG_WIFI_SSID          "Redmi K30I"                            //修改为自己的WiFi 热点账号

#define CONFIG_WIFI_PWD           "14531hqx"                        //修改为自己的WiFi 热点密码

// #define CONFIG_APP_SERVERIP       "121.36.42.100"                       //基础版平台对接地址

#define CONFIG_APP_SERVERIP       "117.78.5.125"                       //标准版平台对接地址

#define CONFIG_APP_SERVERPORT     "1883"

#define CONFIG_APP_DEVICEID       "668bd0975830dc113ecb804a_20240708"       //替换为注册设备后生成的deviceid
// #define CONFIG_APP_DEVICEID         "668bd0975830dc113ecb804a_1720501629973"

#define CONFIG_APP_DEVICEPWD      "123456789"                                   //替换为注册设备后生成的密钥

#define CONFIG_APP_LIFETIME       60     ///< seconds

#define CONFIG_QUEUE_TIMEOUT      (5*1000)

#define MSGQUEUE_OBJECTS 16 // number of Message Queue Objects

#define TEMPERATURE_THRESHOLD 70.0 // 设定的温度阈值

typedef enum
{
    en_msg_cmd = 0,
    en_msg_report,
}en_msg_type_t;

typedef struct
{
    char *request_id;
    char *payload;
} cmd_t;

typedef struct
{
    char Longitude [10];
    char Latitude  [9];
    char Temperature[10]; // 修改类型为char数组以兼容字符串
    char LightLevel[10];  // 修改类型为char数组以兼容字符串
    char WaterLevel[10];
} report_t;

typedef struct
{
    en_msg_type_t msg_type;
    union
    {
        cmd_t cmd;
        report_t report;
    } msg;
} app_msg_t;

typedef struct
{
    queue_t         *app_msg;
    int             connected;
    int             beep;
} app_cb_t;
static app_cb_t g_app_cb;

static void deal_report_msg(report_t *report)
{
    oc_mqtt_profile_service_t service;
    oc_mqtt_profile_kv_t Longitude_value;
    oc_mqtt_profile_kv_t Latitude_value;
    oc_mqtt_profile_kv_t beep;
    oc_mqtt_profile_kv_t Temperature_value;
    oc_mqtt_profile_kv_t LightLevel_value;
    oc_mqtt_profile_kv_t WaterLevel_value;

    if(g_app_cb.connected != 1){
        return;
    }

    service.event_time = NULL;
    service.service_id = "Track";
    service.service_property = &Longitude_value;
    service.nxt = NULL;

    Longitude_value.key = "Longitude";
    Longitude_value.value = report->Longitude;
    Longitude_value.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    Longitude_value.nxt = &Latitude_value;

    Latitude_value.key = "Latitude";
    Latitude_value.value = report->Latitude;
    Latitude_value.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    Latitude_value.nxt = &Temperature_value;

    Temperature_value.key = "Temperature";
    Temperature_value.value = report->Temperature;
    Temperature_value.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    Temperature_value.nxt = &LightLevel_value;

    LightLevel_value.key = "LightLevel";
    LightLevel_value.value = report->LightLevel;
    LightLevel_value.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    LightLevel_value.nxt = &WaterLevel_value;

    WaterLevel_value.key = "WaterLevel";
    WaterLevel_value.value = report->WaterLevel;
    WaterLevel_value.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    WaterLevel_value.nxt = &beep;

    beep.key = "BeepStatus";
    beep.value = g_app_cb.beep ? "ON" : "OFF";
    beep.type = EN_OC_MQTT_PROFILE_VALUE_STRING;
    beep.nxt = NULL;

    oc_mqtt_profile_propertyreport(NULL,&service);
    return;
}

//use this function to push all the message to the buffer
static int msg_rcv_callback(oc_mqtt_profile_msgrcv_t *msg)
{
    int    ret = 0;
    char  *buf;
    int    buf_len;
    app_msg_t *app_msg;

    if((NULL == msg)|| (msg->request_id == NULL) || (msg->type != EN_OC_MQTT_PROFILE_MSG_TYPE_DOWN_COMMANDS)){
        return ret;
    }

    buf_len = sizeof(app_msg_t) + strlen(msg->request_id) + 1 + msg->msg_len + 1;
    buf = malloc(buf_len);
    if(NULL == buf){
        return ret;
    }
    app_msg = (app_msg_t *)buf;
    buf += sizeof(app_msg_t);

    app_msg->msg_type = en_msg_cmd;
    app_msg->msg.cmd.request_id = buf;
    buf_len = strlen(msg->request_id);
    buf += buf_len + 1;
    memcpy(app_msg->msg.cmd.request_id, msg->request_id, buf_len);
    app_msg->msg.cmd.request_id[buf_len] = '\0';

    buf_len = msg->msg_len;
    app_msg->msg.cmd.payload = buf;
    memcpy(app_msg->msg.cmd.payload, msg->msg, buf_len);
    app_msg->msg.cmd.payload[buf_len] = '\0';

    ret = queue_push(g_app_cb.app_msg,app_msg,10);
    if(ret != 0){
        free(app_msg);
    }

    return ret;
}

///< COMMAND DEAL
#include <cJSON.h>
static void deal_cmd_msg(cmd_t *cmd)
{
    cJSON *obj_root;
    cJSON *obj_cmdname;
    cJSON *obj_paras;
    cJSON *obj_para;

    int cmdret = 1;
    oc_mqtt_profile_cmdresp_t cmdresp;
    obj_root = cJSON_Parse(cmd->payload);
    if (NULL == obj_root)
    {
        goto EXIT_JSONPARSE;
    }

    obj_cmdname = cJSON_GetObjectItem(obj_root, "command_name");
    if (NULL == obj_cmdname)
    {
        goto EXIT_CMDOBJ;
    }
    if (0 == strcmp(cJSON_GetStringValue(obj_cmdname), "Track_Control_Beep"))
    {
        obj_paras = cJSON_GetObjectItem(obj_root, "paras");
        if (NULL == obj_paras)
        {
            goto EXIT_OBJPARAS;
        }
        obj_para = cJSON_GetObjectItem(obj_paras, "Beep");
        if (NULL == obj_para)
        {
            goto EXIT_OBJPARA;
        }
        ///< operate the Beep here
        if (0 == strcmp(cJSON_GetStringValue(obj_para), "ON"))
        {
            g_app_cb.beep = 1;
            Beep_StatusSet(ON);
            printf("Beep On!\r\n");
        }
        else
        {
            g_app_cb.beep = 0;
            Beep_StatusSet(OFF);
            printf("Beep Off!\r\n");
        }
        cmdret = 0;
    }
    

EXIT_OBJPARA:
EXIT_OBJPARAS:
EXIT_CMDOBJ:
    cJSON_Delete(obj_root);
EXIT_JSONPARSE:
    ///< do the response
    cmdresp.paras = NULL;
    cmdresp.request_id = cmd->request_id;
    cmdresp.ret_code = cmdret;
    cmdresp.ret_name = NULL;
    (void)oc_mqtt_profile_cmdresp(NULL, &cmdresp);
    return;
}


static int task_main_entry(void)
{
    app_msg_t *app_msg;
    uint32_t ret ;

    WifiConnect(CONFIG_WIFI_SSID, CONFIG_WIFI_PWD);
    dtls_al_init();
    mqtt_al_init();
    oc_mqtt_init();
    
    g_app_cb.app_msg = queue_create("queue_rcvmsg",10,1);
    if(NULL ==  g_app_cb.app_msg){
        printf("Create receive msg queue failed");
        
    }
    oc_mqtt_profile_connect_t  connect_para;
    (void) memset( &connect_para, 0, sizeof(connect_para));

    connect_para.boostrap =      0;
    connect_para.device_id =     CONFIG_APP_DEVICEID;
    connect_para.device_passwd = CONFIG_APP_DEVICEPWD;
    connect_para.server_addr =   CONFIG_APP_SERVERIP;
    connect_para.server_port =   CONFIG_APP_SERVERPORT;
    connect_para.life_time =     CONFIG_APP_LIFETIME;
    connect_para.rcvfunc =       msg_rcv_callback;
    connect_para.security.type = EN_DTLS_AL_SECURITY_TYPE_NONE;
    ret = oc_mqtt_profile_connect(&connect_para);
    if((ret == (int)en_oc_mqtt_err_ok)){
        g_app_cb.connected = 1;
        printf("oc_mqtt_profile_connect succed!\r\n");
    }
    else
    {
        printf("oc_mqtt_profile_connect faild!\r\n");
    }
    while (1)
    {
        app_msg = NULL;
        (void)queue_pop(g_app_cb.app_msg,(void **)&app_msg,0xFFFFFFFF);
        if (NULL != app_msg)
        {
            switch (app_msg->msg_type)
            {
            case en_msg_cmd:
                deal_cmd_msg(&app_msg->msg.cmd);
                break;
            case en_msg_report:
                deal_report_msg(&app_msg->msg.report);
                break;
            default:
                break;
            }
            free(app_msg);
        }
    }
    return 0;
}

static int task_sensor_entry(void)
{
    E53_ST1_Data_TypeDef data;
    app_msg_t *app_msg;

    Init_E53_ST1();
    TEMPSensorInit();  // 初始化温度传感器
    LvSensorInit(); // 初始化光度传感器
    CHExampleEntry();

    while (1)
    {
        E53_ST1_Read_Data(&data);
        float Temperature = ReadTEMPSensor(); // 假设这是读取温度的函数
        float LightLevel = ReadLvSensor();    // 假设这是读取光度的函数

        srand(time(0)); // 使用当前时间作为随机数生成器的种子

    // 生成12.230000到12.247450之间的随机浮点数
        double range = 12.247450 - 12.230000;
        double scale = (double)RAND_MAX / range;
        double WaterLevel = 12.230000 + (double)rand() / scale;

        printf("\r\n******************************经度： 114.42\r\n");
        printf("\r\n******************************纬度：  23.04\r\n");
        printf("\r\n******************************水位：  %.6f\r\n", WaterLevel);
        data.Longitude=114.42;
        data.Latitude=23.04;

        // 判断温度是否超过阈值，如果超过则启动蜂鸣器
        if (Temperature > TEMPERATURE_THRESHOLD) {
            g_app_cb.beep = 1;
            Beep_StatusSet(ON);
            printf("Temperature exceeds threshold, Beep On!\r\n");
        } else {
            g_app_cb.beep = 0;
            Beep_StatusSet(OFF);
            printf("Temperature is normal, Beep Off!\r\n");
        }


        app_msg = malloc(sizeof(app_msg_t));
        if ((NULL != app_msg)  &&(Temperature != 0)&& (LightLevel != 0))
        {
            app_msg->msg_type = en_msg_report;
            sprintf(app_msg->msg.report.Longitude, "%.5f", data.Longitude);
            sprintf(app_msg->msg.report.Latitude, "%.5f", data.Latitude);
            sprintf(app_msg->msg.report.Temperature, "%.2f", Temperature); // 将浮点数转换为字符串
            sprintf(app_msg->msg.report.LightLevel, "%.2f", LightLevel);   // 将浮点数转换为字符串
            sprintf(app_msg->msg.report.WaterLevel, "%.6f", WaterLevel);   // 将浮点数转换为字符串
            if (0 != queue_push(g_app_cb.app_msg, app_msg, CONFIG_QUEUE_TIMEOUT)) {
                free(app_msg);
            }
        }
        sleep(3);
    }
    return 0;
}


static void OC_Demo(void)
{
    osThreadAttr_t attr;

    attr.name = "task_main_entry";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 10240;
    attr.priority = 24;

    if (osThreadNew((osThreadFunc_t)task_main_entry, NULL, &attr) == NULL)
    {
        printf("Falied to create task_main_entry!\n");
    }
    attr.stack_size = 4096;
    attr.priority = 25;
    attr.name = "task_sensor_entry";
    if (osThreadNew((osThreadFunc_t)task_sensor_entry, NULL, &attr) == NULL)
    {
        printf("Falied to create task_sensor_entry!\n");
    }
}

APP_FEATURE_INIT(OC_Demo);