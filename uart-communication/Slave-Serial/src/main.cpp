#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "serial.hpp"

extern "C"
void app_main() 
{
    Serial mSerial = Serial();

    mSerial.StartTask();
}