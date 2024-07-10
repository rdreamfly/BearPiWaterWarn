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

#include <stdio.h>
#include <unistd.h>

#include "ohos_init.h"
#include "cmsis_os2.h"
#include "wifiiot_gpio.h"
#include "wifiiot_gpio_ex.h"
#include "led_example.h"
uint8_t ch9300_state = 0;

static void CH9300Task(void)
{
    //初始化GPIO
    GpioInit();

    //设置GPIO_13的复用功能为普通GPIO
    IoSetFunc(WIFI_IOT_IO_NAME_GPIO_13, WIFI_IOT_IO_FUNC_GPIO_13_GPIO);

    //设置GPIO_13为输出模式
    GpioSetDir(WIFI_IOT_GPIO_IDX_13, WIFI_IOT_GPIO_DIR_OUT);

    while (1)
    {

           if (ch9300_state == 0) {
             GpioSetOutputVal(WIFI_IOT_GPIO_IDX_13, 0);
             usleep(10000000);
             ch9300_state = 1;
      } else {
             GpioSetOutputVal(WIFI_IOT_GPIO_IDX_13, 1);
             usleep(10000000);
            ch9300_state = 0;
      }
    }
}

void CHExampleEntry(void)
{
    osThreadAttr_t attr;

    attr.name = "CH9300Task";
    attr.attr_bits = 0U;
    attr.cb_mem = NULL;
    attr.cb_size = 0U;
    attr.stack_mem = NULL;
    attr.stack_size = 512;
    attr.priority = 25;

    if (osThreadNew((osThreadFunc_t)CH9300Task, NULL, &attr) == NULL)
    {
        printf("Falied to create LedTask!\n");
    }
}
