
#include <stdio.h>
#include "example_encryption.h"

void app_main() 
{
  char *enc_input = "EncryptionString";

  example_cryption_initialize();
  unsigned char *encrypted_data = example_crpytion_encrypt(enc_input);

  example_cryption_decrypt(encrypted_data, 16);
}

