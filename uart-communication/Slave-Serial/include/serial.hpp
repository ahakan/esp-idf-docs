#pragma once

#ifndef Serial_H
#define Serial_H

#include <iostream>
#include <string>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"

static const int RX_BUF_SIZE = 1024;

#define TXD_PIN (GPIO_NUM_17)
#define RXD_PIN (GPIO_NUM_16)

class Serial
{
    private:
        void SerialTask();
        static void SerialTaskImpl(void *);

        void tx_task();
        void rx_task();

        uint8_t sendData(std::string data);

    public:
        Serial();
        void StartTask();
};

#endif