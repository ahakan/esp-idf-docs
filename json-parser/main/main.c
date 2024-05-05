#include "esp_log.h"
#include "cJSON.h"
#include <stdbool.h>

#define TAG "EXAMPLE_JSON"

void app_main(void)
{
    ESP_LOGI(TAG, "JSON example.");

	cJSON *root;
	root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "device", "ESP32C3");
	cJSON_AddNumberToObject(root, "version", 1.10);
	cJSON_AddBoolToObject(root, "enable", true);

	char *example_json_string = cJSON_Print(root);
    cJSON_Delete(root);

    cJSON *root2;
    root2 = cJSON_Parse(example_json_string);

    if (cJSON_GetObjectItem(root2, "device")) {
		char *device = cJSON_GetObjectItem(root2, "device")->valuestring;
		ESP_LOGI(TAG, "device=%s", device);
	}
	if (cJSON_GetObjectItem(root2, "version")) {
		double version = cJSON_GetObjectItem(root2, "version")->valuedouble;
		ESP_LOGI(TAG, "version=%f", version);
	}
	if (cJSON_GetObjectItem(root2, "enable")) {
		bool enable = cJSON_GetObjectItem(root2, "enable")->valueint;
		ESP_LOGI(TAG, "enable=%d", enable);
	}

    cJSON_Delete(root2);

    cJSON_free(example_json_string);
}
