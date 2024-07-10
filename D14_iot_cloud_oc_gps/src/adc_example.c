// #include "wifiiot_adc.h"
// #include "wifiiot_uart.h"
// #include "wifiiot_gpio_ex.h"

// #include <stdio.h>
// #include <string.h>
// #include <unistd.h>
// #include "ohos_init.h"
// #include "cmsis_os2.h"

// #include "adc_example.h"

// #define TEMP_SENSOR_ADC_CHANNEL WIFI_IOT_ADC_CHANNEL_2 // 根据BearPi的ADC引脚配置

// // 函数：读取温度传感器数据并转换为温度值
// float ReadTEMPSensor(void)
// {
//     unsigned short data = 0;
//     AdcRead(TEMP_SENSOR_ADC_CHANNEL, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0);

//     // 将ADC值转换为电压（假设参考电压为3.3V）
//     float voltage = (float)data * 3.3 / 4096.0;

//     // 适用于温度传感器的电压转换公式（假设简化线性关系）
//     float tempValue = voltage * 100.0; // LM35: 10mV/°C 

//     return tempValue;
// }

// // 主任务函数
// static void TEMPSensorTask(void)
// {
//     while (1)
//     {
//         float temp = ReadTEMPSensor();
//         printf("\r\n******************************温度值： %.2f\r\n", temp);
//         usleep(3000000); // 每秒读取一次
//     }
// }

// // 初始化函数
// void TEMPSensorInit(void)
// {
//     osThreadAttr_t attr;

//     // 创建任务
//     attr.name = "TEMPSensorTask";
//     attr.attr_bits = 0U;
//     attr.cb_mem = NULL;
//     attr.cb_size = 0U;
//     attr.stack_mem = NULL;
//     attr.stack_size = 1024;
//     attr.priority = osPriorityNormal;

//     if (osThreadNew((osThreadFunc_t)TEMPSensorTask, NULL, &attr) == NULL) {
//         printf("[TEMPSensorInit] Failed to create TEMPSensorTask!\n");
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

#include "adc_example.h"

#define TEMP_SENSOR_ADC_CHANNEL WIFI_IOT_ADC_CHANNEL_2 // 根据BearPi的ADC引脚配置
#define TEMP_VALID_MIN 15.0  // 温度有效范围最小值
#define TEMP_VALID_MAX 35.0  // 温度有效范围最大值

// 函数：读取温度传感器数据并转换为温度值
float ReadTEMPSensor(void)
{
    unsigned short data = 0;
    int read_count = 5;
    float voltage_sum = 0;
    
    for (int i = 0; i < read_count; i++) {
        AdcRead(TEMP_SENSOR_ADC_CHANNEL, &data, WIFI_IOT_ADC_EQU_MODEL_8, WIFI_IOT_ADC_CUR_BAIS_DEFAULT, 0);
        float voltage = (float)data * 3.3 / 4096.0;
        voltage_sum += voltage;
        usleep(50000); // 50ms
    }
    
    float voltage_avg = voltage_sum / read_count;

    // 适用于温度传感器的电压转换公式（假设简化线性关系）
    float tempValue = voltage_avg * 25.0; // 根据具体传感器的校准曲线进行调整

    // 检查温度是否在有效范围内
    if (tempValue < TEMP_VALID_MIN || tempValue > TEMP_VALID_MAX) {
        printf("无效温度值： %.2f\n", tempValue);
        return -1; // 返回无效值
    }

    return tempValue;
}

// 主任务函数
static void TEMPSensorTask(void)
{
    while (1)
    {
        float temp = ReadTEMPSensor();
        if (temp != -1) {
            printf("******************************温度值： %.2f\n", temp);
        }
        usleep(3000000); // 每3秒读取一次
    }
}

// 初始化函数
void TEMPSensorInit(void)
{
    osThreadAttr_t attr;

    // 创建任务
    attr.name = "TEMPSensorTask";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 1024;
    attr.priority = osPriorityNormal;

    if (osThreadNew((osThreadFunc_t)TEMPSensorTask, NULL, &attr) == NULL) {
        printf("[TEMPSensorInit] Failed to create TEMPSensorTask!\n");
    }
}
