// #include "wifiiot_adc.h"
// #include "wifiiot_uart.h"
// #include "wifiiot_gpio_ex.h"

// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>

// #include "ohos_init.h"
// #include "cmsis_os2.h"

// #include "pwm_example.h"

// #define Lv_SENSOR_ADC_CHANNEL WIFI_IOT_ADC_CHANNEL_4 // 根据BearPi的ADC引脚配置

// // 函数：读取光度传感器数据并转换为光度值
// float ReadLvSensor(void)
// {
//     unsigned short data = 0;
//     AdcRead(Lv_SENSOR_ADC_CHANNEL, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0);

//     // 将ADC值转换为电压（假设参考电压为3.3V）
//     float voltage = (float)data * 3.3 / 4096.0;

//     // 适用于光度传感器的电压转换公式
//     float LvValue = voltage * 100.0; // 根据具体传感器的校准曲线进行调整

//     return LvValue;
// }

// // 主任务函数
// static void LvSensorTask(void)
// {
//     while (1)
//     {
//         float lv = ReadLvSensor();
//         printf("\r\n******************************光度值： %.2f\r\n", lv);
//         usleep(3000000); // 每秒读取一次
//     }
// }

// // 初始化函数
// void LvSensorInit(void)
// {
//     osThreadAttr_t attr;

//     // 创建任务
//     attr.name = "LvSensorTask";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = 1024;
//     attr.priority = osPriorityNormal;

//     if (osThreadNew((osThreadFunc_t)LvSensorTask, NULL, &attr) == NULL) {
//         printf("[LvSensorInit] Failed to create LvSensorTask!\n");
//     }
// }


#include "wifiiot_adc.h"
#include "wifiiot_uart.h"
#include "wifiiot_gpio_ex.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"

#include "pwm_example.h"

#define Lv_SENSOR_ADC_CHANNEL WIFI_IOT_ADC_CHANNEL_4 // 根据BearPi的ADC引脚配置

// 函数：读取光度传感器数据并转换为光度值
float ReadLvSensor(void)
{
    unsigned short data = 0;
    int read_count = 5;
    float voltage_sum = 0;

    for (int i = 0; i < read_count; i++) {
        AdcRead(Lv_SENSOR_ADC_CHANNEL, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0);
        float voltage = (float)data * 3.3 / 4096.0;
        voltage_sum += voltage;
        usleep(50000); // 50ms
    }

    float voltage_avg = voltage_sum / read_count;

    // 适用于光度传感器的电压转换公式
    float LvValue = voltage_avg * 100.0; // 根据具体传感器的校准曲线进行调整

    return LvValue;
}

// 主任务函数
static void LvSensorTask(void)
{
    while (1)
    {
        float lv = ReadLvSensor();
        printf("******************************光度值： %.2f\n", lv);
        usleep(3000000); // 每3秒读取一次
    }
}

// 初始化函数
void LvSensorInit(void)
{
    osThreadAttr_t attr;

    // 创建任务
    attr.name = "LvSensorTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)LvSensorTask, NULL, &attr) == NULL) {
        printf("[LvSensorInit] Failed to create LvSensorTask!\n");
    }
}
