#include "example_queue.h"

QueueHandle_t xExampleQueue;
SemaphoreHandle_t xExampleSemaphore;

esp_err_t example_queue_initialize()
{
    xExampleSemaphore = xSemaphoreCreateMutex();

    if (xExampleSemaphore == NULL)
    {
        return ESP_FAIL;
    }

    xExampleQueue = xQueueCreate(QUEUE_SIZE, sizeof(xExampleData));

    if (xExampleQueue == NULL)
    {
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t example_queue_deinitialize()
{
    esp_err_t xErr = ESP_OK;

    if (xExampleQueue != NULL)
    {
        if (xSemaphoreTake(xExampleSemaphore, (TickType_t) 10 ) == pdTRUE)
        {
            vQueueDelete(xExampleQueue);
        }
    }

    if (xExampleSemaphore != NULL)
    {
        vSemaphoreDelete(xExampleQueue);
    }

    return xErr;
}

esp_err_t example_queue_write_data(xExampleData *xData)
{
    esp_err_t xErr = ESP_FAIL;

    if (xExampleQueue != NULL && xExampleSemaphore != NULL && xData != NULL)
    {
        if (example_queue_available_size() > 0)
        {
            if (xSemaphoreTake(xExampleSemaphore, (TickType_t) 10 ) == pdTRUE)
            {
                xExampleData *xDataPtr = (xExampleData*)pvPortMalloc(sizeof(xExampleData));

                xDataPtr->size = xData->size;
                xDataPtr->message = pvPortMalloc(xData->size);

                memcpy(xDataPtr->message, xData->message, xData->size);

                xQueueSend(xExampleQueue, &xDataPtr, (TickType_t) 0);

                xSemaphoreGive(xExampleSemaphore);

                xErr = ESP_OK;
            }
        }
    }

    return xErr;
}

esp_err_t example_queue_write_data_front(xExampleData *xData)
{
    esp_err_t xErr = ESP_FAIL;

    if (xExampleQueue != NULL && xExampleSemaphore != NULL && xData != NULL)
    {
        if (example_queue_available_size() > 0)
        {
            if (xSemaphoreTake(xExampleSemaphore, (TickType_t) 10 ) == pdTRUE)
            {
                xExampleData *xDataPtr = (xExampleData*)pvPortMalloc(sizeof(xExampleData));

                xDataPtr->size = xData->size;
                xDataPtr->message = pvPortMalloc(xData->size);

                memcpy(xDataPtr->message, xData->message, xData->size);

                xQueueSendToFront(xExampleQueue, &xDataPtr, (TickType_t) 0);

                xSemaphoreGive(xExampleSemaphore);

                xErr = ESP_OK;
            }
        }
    }

    return xErr;
}

xExampleData* example_queue_read_data()
{
    xExampleData *xDataPtr = NULL;

    if (xExampleQueue != NULL && xExampleSemaphore != NULL)
    {
        if (example_queue_available_size() != QUEUE_SIZE)
        {
            if (xSemaphoreTake(xExampleSemaphore, (TickType_t) 10 ) == pdTRUE)
            {
                xQueueReceive(xExampleQueue, &xDataPtr, (TickType_t) 0);

                xSemaphoreGive(xExampleSemaphore);
            }
        }
    }

    return xDataPtr;
}

size_t example_queue_available_size()
{
    size_t xQueueSize = 0;

    if (xExampleQueue != NULL && xExampleSemaphore != NULL)
    {
        if (xSemaphoreTake(xExampleSemaphore, (TickType_t) 10 ) == pdTRUE)
        {
            xQueueSize = uxQueueSpacesAvailable(xExampleQueue);

            xSemaphoreGive(xExampleSemaphore);
        }
    }

    return xQueueSize;
}

void example_queue_receive_data_task(xDataReceivedCallback callback)
{
    while (1) 
    {
        if (example_queue_available_size() != QUEUE_SIZE)
        {
            xExampleData *receivedData = example_queue_read_data();

            if (receivedData != NULL)
            {
                callback(receivedData);
            }
        }
    
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}