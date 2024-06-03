#include "example_encryption.h"

mbedtls_aes_context aes;

char enc_key[32] = {0x84, 0x53, 0x33, 0x1d, 0x75, 0xd3, 0x85, 0x09, 0xd6, 0xd1, 0xbb, 0x79, 0x1d, 0xff, 0x9c, 0xbc,
                    0xa2, 0xba, 0x7b, 0xda, 0x3f, 0xc3, 0x0a, 0x72, 0x81, 0xb7, 0x30, 0x0f, 0x44, 0x7b, 0x21, 0xb0};
char enc_iv[16] = {0x4a, 0xcf, 0x78, 0xd3, 0xf3, 0x05, 0x91, 0x39, 0x91, 0x0d, 0x4e, 0x5a, 0x2d, 0x43, 0x33, 0x26};

esp_err_t example_cryption_initialize()
{
    mbedtls_aes_init(&aes);
    mbedtls_aes_setkey_enc(&aes, (unsigned char *)enc_key, 256);

    return ESP_OK;
}

esp_err_t example_cryption_deinitialize()
{
    return ESP_OK;
}

void example_cryption_decrypt(unsigned char *input, size_t size)
{
    unsigned char *encrypt_output = (unsigned char *)malloc(size);

    char *iv_copy = (char *)malloc(16);

    for (int i = 0; i < 16; i++)
    {
        iv_copy[i] = enc_iv[i];
    }

    mbedtls_aes_crypt_cbc(&aes,
                          MBEDTLS_AES_DECRYPT,
                          size,
                          (unsigned char *)iv_copy,
                          (unsigned char *)input,
                          encrypt_output);

    free(iv_copy);

    for (int i = 0; i < size; i++)
    {
        printf("%c", (char)encrypt_output[i]);
    }
    printf("\n");
}

unsigned char *example_crpytion_encrypt(const char *input)
{
    int padded_input_len = 0;
    int input_len = strlen(input) + 1;
    int modulo16 = input_len % 16;

    if (input_len < 16)
    {
        padded_input_len = 16;
    }
    else
    {
        padded_input_len = (strlen(input) / 16 + 1) * 16;
    }

    char *padded_input = (char *)malloc(padded_input_len);
    if (!padded_input)
    {
        printf("Failed to allocate memory\n");
    }

    memcpy(padded_input, input, strlen(input));
    uint8_t pkc5_value = (17 - modulo16);
    for (int i = strlen(input); i < padded_input_len; i++)
    {
        padded_input[i] = pkc5_value;
    }

    unsigned char *encrypt_output = (unsigned char *)malloc(padded_input_len);

    char *iv_copy = (char *)malloc(16);

    for (int i = 0; i < 16; i++)
    {
        iv_copy[i] = enc_iv[i];
    }

    mbedtls_aes_crypt_cbc(&aes,
                          MBEDTLS_AES_ENCRYPT,
                          padded_input_len,
                          (unsigned char *)iv_copy,
                          (unsigned char *)padded_input,
                          encrypt_output);

    free(iv_copy);

    for (int i = 0; i < padded_input_len; i++)
    {
        printf("%02x", encrypt_output[i]);
    }
    printf("\n");

    return encrypt_output;
}