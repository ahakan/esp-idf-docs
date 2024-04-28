#ifndef EXAMPLE_QUEUE_H
#define EXAMPLE_QUEUE_H

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"

#define TAG "EXAMPLE_QUEUEU"

#define QUEUE_SIZE 10

typedef struct {
    size_t size;
    unsigned char* message;
} xExampleData;

typedef void (*xDataReceivedCallback)(xExampleData *);

esp_err_t example_queue_initialize();
esp_err_t example_queue_deinitialize();

esp_err_t example_queue_write_data(xExampleData *);
esp_err_t example_queue_write_data_front(xExampleData *);

xExampleData* example_queue_read_data();

size_t example_queue_available_size();

void example_queue_receive_data_task(xDataReceivedCallback);

#endif