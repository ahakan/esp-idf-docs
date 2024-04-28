#include "example_queue.h"

void sendData()
{
    while (1)
    {
        unsigned char abc[] = { 0x61, 0x62, 0x63};

        xExampleData *xSendData = (xExampleData*)pvPortMalloc(sizeof(xExampleData));
        xSendData->size = 3;
        xSendData->message = pvPortMalloc(xSendData->size);

        for (size_t i = 0; i < xSendData->size; i++)
        {
            xSendData->message[i] = abc[i];
        }

        example_queue_write_data(xSendData);

        vPortFree(xSendData->message);
        vPortFree(xSendData);

        vTaskDelay(pdMS_TO_TICKS(1000));

        ESP_LOGI(TAG, "Queue Available Size: %d", example_queue_available_size());
    }
}

void receiveData(xExampleData *data)
{
    ESP_LOGI(TAG, "Size: %d.", data->size);

    ESP_LOGI(TAG, "Message:");
    for (size_t i = 0; i < data->size; i++)
    {
        printf("%c", (char)data->message[i]);
    }
    printf("\n");

    vPortFree(data->message);
    vPortFree(data);

    ESP_LOGI(TAG, "Heap size: %d", (int)xPortGetFreeHeapSize());
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting...");

    example_queue_initialize();

    xTaskCreate(sendData, "SenderTask", 4096, NULL, 1, NULL);
    xTaskCreate(example_queue_receive_data_task, "ReceiveTask", 4096, receiveData, 1, NULL);
}
