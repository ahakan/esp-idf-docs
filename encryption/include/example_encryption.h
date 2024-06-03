#ifndef H_EXAMPLE_ENCRYPTION
#define H_EXAMPLE_ENCRYPTION

#include <stdio.h>
#include <string.h>
#include "mbedtls/aes.h"
#include "esp_log.h"
#include "esp_err.h"

esp_err_t example_cryption_initialize();
esp_err_t example_cryption_deinitialize();

void example_cryption_decrypt(unsigned char* input, size_t size);
unsigned char* example_crpytion_encrypt(const char* input);

#endif