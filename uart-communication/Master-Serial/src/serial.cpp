#include "serial.hpp"

/**
 * @brief 
 * 
 */
void Serial::SerialTask()
{
    while (1)
    {
        std::cout << "Serial Master Task." << std::endl;

        rx_task();
        tx_task();
        
        // vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

/**
 * @brief 
 * 
 * @param _this 
 */
void Serial::SerialTaskImpl(void* _this)
{
    Serial* pSerial = (Serial*) _this;

    pSerial->SerialTask();
}

/**
 * @brief 
 * 
 * @param data 
 * @return uint8_t 
 */
uint8_t Serial::sendData(std::string data)
{
    const int len = data.length();

    const int txBytes = uart_write_bytes(UART_NUM_1, data.c_str(), len);

    std::cout << "Wrote " << txBytes << " bytes" << std::endl;

    return txBytes;
}

/**
 * @brief 
 * 
 * @param arg 
 */
void Serial::tx_task()
{
    sendData("Hello Slave!");

    vTaskDelay(100 / portTICK_PERIOD_MS);
}

/**
 * @brief 
 * 
 * @param arg 
 */
void Serial::rx_task()
{
    uint8_t* data = (uint8_t*) malloc(RX_BUF_SIZE+1);
   
    const int rxBytes = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 1000 / portTICK_PERIOD_MS);
    if (rxBytes > 0) 
    {
        data[rxBytes] = 0;
        std::cout << "Read " << rxBytes << " bytes: " << data << std::endl;
    }

    free(data);
}

/**
 * @brief Construct a new Serial:: Serial object
 * 
 */
Serial::Serial()
{
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };

    // We won't use a buffer for sending data.
    uart_driver_install(UART_NUM_1, RX_BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

    std::cout << "Master configured." << std::endl;
}

/**
 * @brief 
 * 
 */
void Serial::StartTask()
{
    xTaskCreate(this->SerialTaskImpl, 
                "Serial",
                2048,
                this,
                12,
                NULL);
}