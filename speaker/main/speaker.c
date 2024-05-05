#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "string.h"
#include "nvs_flash.h"
#include "driver/i2s.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_gatt_common_api.h"

#define TAG "BT_SPEAKER"

uint8_t my_bt_speaker_state = ESP_AVRC_PLAYBACK_STOPPED;
const char bt_device_name[] = "BT_SPEAKER";
const char ble_device_name[] = "BT_SPEAKER";
bool avrc_conn = false;

#define BLE_ADV_NAME "BT_SPEAKER"

/* BLE defines */
#define GATTS_SERVICE_UUID_A 0x00FF
#define GATTS_CHAR_UUID_A 0xFF01
#define GATTS_DESCR_UUID_A 0x3333
#define GATTS_NUM_HANDLE_A 4

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
#define PREPARE_BUF_MAX_SIZE 1024
#define PROFILE_NUM 1
#define PROFILE_A_APP_ID 0

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static uint8_t ble_char_value_str[] = {0x11, 0x22, 0x33};

esp_gatt_char_prop_t a_property = 0;

esp_attr_value_t gatts_initial_char_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(ble_char_value_str),
    .attr_value = ble_char_value_str,
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x060,
    .adv_int_max = 0x060,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RPA_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }};

static void ble_init_adv_data(const char *name)
{
    int len = strlen(name);
    uint8_t raw_adv_data[len + 5];
    // flag
    raw_adv_data[0] = 2;
    raw_adv_data[1] = ESP_BT_EIR_TYPE_FLAGS;
    raw_adv_data[2] = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
    // adv name
    raw_adv_data[3] = len + 1;
    raw_adv_data[4] = ESP_BLE_AD_TYPE_NAME_CMPL;
    for (int i = 0; i < len; i++)
    {
        raw_adv_data[i + 5] = *(name++);
    }
    // The length of adv data must be less than 31 bytes
    esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
    if (raw_adv_ret)
    {
        ESP_LOGE(TAG, "config raw adv data failed, error code = 0x%x ", raw_adv_ret);
    }
    esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_adv_data, sizeof(raw_adv_data));
    if (raw_scan_ret)
    {
        ESP_LOGE(TAG, "config raw scan rsp data failed, error code = 0x%x", raw_scan_ret);
    }
}

void my_bt_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_BT_GAP_DISC_RES_EVT:
        break;
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
        break;
    case ESP_BT_GAP_RMT_SRVCS_EVT:
        break;
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
        break;
    case ESP_BT_GAP_AUTH_CMPL_EVT:
        ESP_LOGI(TAG, "auth: state:%d, remote_name:%s", param->auth_cmpl.stat, param->auth_cmpl.device_name);
        break;
    case ESP_BT_GAP_PIN_REQ_EVT:
        break;
    case ESP_BT_GAP_CFM_REQ_EVT:
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        break;
    case ESP_BT_GAP_MODE_CHG_EVT:
        break;
    default:
        ESP_LOGI(TAG, "GAP EVENT ID:%d", event);
        break;
    }
}

void my_bt_a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param)
{
    switch (event)
    {
    case ESP_A2D_CONNECTION_STATE_EVT:
        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED)
        {
            esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
        }
        if (param->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED)
        {
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        }
        break;
    case ESP_A2D_AUDIO_STATE_EVT:
        break;
    case ESP_A2D_AUDIO_CFG_EVT:
        if (param->audio_cfg.mcc.type == ESP_A2D_MCT_SBC)
        {
            int sample_rate = 16000;
            char oct0 = param->audio_cfg.mcc.cie.sbc[0];
            if (oct0 & (0x01 << 6))
            {
                sample_rate = 32000;
            }
            else if (oct0 & (0x01 << 5))
            {
                sample_rate = 44100;
            }
            else if (oct0 & (0x01 << 4))
            {
                sample_rate = 48000;
            }
            i2s_set_clk(0, sample_rate, 16, 2);
        }

        break;
    case ESP_A2D_MEDIA_CTRL_ACK_EVT:
        break;
    case ESP_A2D_PROF_STATE_EVT:
        break;
    default:
        break;
    }
}

void my_bt_a2d_sink_data_cb(const uint8_t *buf, uint32_t len)
{
    size_t wb;
    i2s_write(0, buf, len, &wb, portMAX_DELAY);
}

esp_avrc_rn_evt_cap_mask_t rn_cap_mask = {0};
void my_bt_avrc_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param)
{
    char *text;
    switch (event)
    {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT:
        if (param->conn_stat.connected)
        {
            esp_avrc_ct_send_get_rn_capabilities_cmd(1);
            avrc_conn = 1;
            esp_avrc_ct_send_register_notification_cmd(1, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
        }
        else
        {
            rn_cap_mask.bits = 0;
            avrc_conn = 0;
        }
        break;
    case ESP_AVRC_CT_PASSTHROUGH_RSP_EVT:
        break;
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT:
        switch (param->change_ntf.event_id)
        {
        case ESP_AVRC_RN_VOLUME_CHANGE:
            break;
        case ESP_AVRC_RN_PLAY_STATUS_CHANGE:
            my_bt_speaker_state = param->change_ntf.event_parameter.playback;
            break;
        default:
            break;
        }
        break;
    case ESP_AVRC_CT_GET_RN_CAPABILITIES_RSP_EVT:
        rn_cap_mask.bits = param->get_rn_caps_rsp.evt_set.bits;
        break;
    case ESP_AVRC_CT_SET_ABSOLUTE_VOLUME_RSP_EVT:
        break;
    case ESP_AVRC_CT_METADATA_RSP_EVT:
        text = (char *)param->meta_rsp.attr_text;
        text[param->meta_rsp.attr_length] = '\0';
        ESP_LOGI(TAG, "metadata_rsp:%d, %s, %d", param->meta_rsp.attr_id, text, param->meta_rsp.attr_length);
        break;
    default:
        break;
    }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    esp_gatt_status_t status = ESP_GATT_OK;

    if (param->write.need_rsp)
    {
        if (param->write.is_prep)
        {
            if (param->write.offset > PREPARE_BUF_MAX_SIZE)
            {
                status = ESP_GATT_INVALID_OFFSET;
            }
            else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
            {
                status = ESP_GATT_INVALID_ATTR_LEN;
            }

            if (status == ESP_GATT_OK && prepare_write_env->prepare_buf == NULL)
            {
                prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
                prepare_write_env->prepare_len = 0;
                if (prepare_write_env->prepare_buf == NULL)
                {
                    ESP_LOGE(TAG, "Gatt_server prep no mem");
                    status = ESP_GATT_NO_RESOURCES;
                }
            }

            esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));

            if (gatt_rsp) // -----> Buraya bak.
            {
                gatt_rsp->attr_value.len = param->write.len;
                gatt_rsp->attr_value.handle = param->write.handle;
                gatt_rsp->attr_value.offset = param->write.offset;
                gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
                memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);

                esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
                
                if (response_err != ESP_OK)
                {
                    ESP_LOGE(TAG, "Send response error\n");
                }
                free(gatt_rsp);
            }
            else
            {
                ESP_LOGE(TAG, "%s, malloc failed", __func__);
                status = ESP_GATT_NO_RESOURCES;
            }
            if (status != ESP_GATT_OK)
            {
                return;
            }
            memcpy(prepare_write_env->prepare_buf + param->write.offset,
                   param->write.value,
                   param->write.len);
            prepare_write_env->prepare_len += param->write.len;
        }
        else
        {
            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
        }
    }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
    if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
    {
        esp_log_buffer_hex(TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
    }
    else
    {
        ESP_LOGI(TAG, "ESP_GATT_PREP_WRITE_CANCEL");
    }
    if (prepare_write_env->prepare_buf)
    {
        free(prepare_write_env->prepare_buf);
        prepare_write_env->prepare_buf = NULL;
    }
    prepare_write_env->prepare_len = 0;
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        // esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        // advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Advertising start failed");
        }
        else
        {
            ESP_LOGI(TAG, "Start adv successfully");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Advertising stop failed");
        }
        else
        {
            ESP_LOGI(TAG, "Stop adv successfully");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                 param->update_conn_params.status,
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.timeout);
        break;
    default:
        break;
    }
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
        esp_ble_gap_config_local_privacy(true);
        gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_A;
        // init BLE adv data and scan response data
        ble_init_adv_data(BLE_ADV_NAME);
        esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_A);
        break;
    case ESP_GATTS_READ_EVT:
    {
        ESP_LOGI(TAG, "GATT_READ_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
        esp_gatt_rsp_t rsp;
        memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
        rsp.attr_value.handle = param->read.handle;
        rsp.attr_value.len = 4;
        rsp.attr_value.value[0] = 0xde;
        rsp.attr_value.value[1] = 0xed;
        rsp.attr_value.value[2] = 0xbe;
        rsp.attr_value.value[3] = 0xef;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                    ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT:
    {
        ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %" PRIu32 ", handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
        if (!param->write.is_prep)
        {
            ESP_LOGI(TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
            esp_log_buffer_hex(TAG, param->write.value, param->write.len);
            if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
                    {
                        ESP_LOGI(TAG, "notify enable");
                        uint8_t notify_data[15];
                        for (int i = 0; i < sizeof(notify_data); ++i)
                        {
                            notify_data[i] = i % 0xff;
                        }
                        // the size of notify_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                    sizeof(notify_data), notify_data, false);
                    }
                }
                else if (descr_value == 0x0002)
                {
                    if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
                    {
                        ESP_LOGI(TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        // the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                                    sizeof(indicate_data), indicate_data, true);
                    }
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(TAG, "notify/indicate disable ");
                }
                else
                {
                    ESP_LOGE(TAG, "unknown descr value");
                    esp_log_buffer_hex(TAG, param->write.value, param->write.len);
                }
            }
        }
        example_write_event_env(gatts_if, &a_prepare_write_env, param);
        break;
    }
    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        example_exec_write_event_env(&a_prepare_write_env, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
        break;
    case ESP_GATTS_UNREG_EVT:
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d", param->create.status, param->create.service_handle);
        
        gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_A;

        esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);

        a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

        esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, 
                                                        &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                                        ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                        a_property,
                                                        &gatts_initial_char_val, NULL);
        if (add_char_ret)
        {
            ESP_LOGE(TAG, "add char failed, error code = 0x%x", add_char_ret);
        }
        break;
    case ESP_GATTS_ADD_INCL_SRVC_EVT:
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
    {
        ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

        gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
        gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;

        esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, 
                                                               &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                                                               ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, 
                                                               NULL, NULL);
        if (add_descr_ret)
        {
            ESP_LOGE(TAG, "add char descr failed, error code = 0x%x", add_descr_ret);
        }
        break;
    }
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
        ESP_LOGI(TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        break;
    case ESP_GATTS_DELETE_EVT:
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d",
                 param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_STOP_EVT:
        break;
    case ESP_GATTS_CONNECT_EVT:
    {
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT");
        esp_ble_gap_start_advertising(&adv_params);
        break;
    case ESP_GATTS_CONF_EVT:
        ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
        if (param->conf.status != ESP_GATT_OK)
        {
            esp_log_buffer_hex(TAG, param->conf.value, param->conf.len);
        }
        break;
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(TAG, "Reg app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do
    {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++)
        {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == gl_profile_tab[idx].gatts_if)
            {
                if (gl_profile_tab[idx].gatts_cb)
                {
                    gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

static void ble_gatts_init(void)
{
    esp_err_t err;

    if ((err = esp_ble_gatts_register_callback(gatts_event_handler)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s gatts register failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_ble_gap_register_callback(gap_event_handler)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s gap register failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_ble_gatts_app_register(PROFILE_A_APP_ID)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s gatts app register failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_ble_gatt_set_local_mtu(500)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s set local MTU failed: %s", __func__, esp_err_to_name(err));
        return;
    }
}

void app_main(void)
{
    esp_err_t err;

    if ((err = nvs_flash_init()) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    i2s_config_t i2s_config =
        {
            .mode = I2S_MODE_MASTER | I2S_MODE_TX, // Only TX
            .sample_rate = 44100,
            .bits_per_sample = 16,
            .channel_format = I2S_CHANNEL_FMT_ALL_LEFT,        // 1 Channel  -> //2-channels RIGHT_LEFT
            .communication_format = I2S_COMM_FORMAT_STAND_I2S, // MAX98357A I2S Philips
            .dma_buf_count = 6,
            .dma_buf_len = 60,
            .intr_alloc_flags = 0,     // Default interrupt priority
            .tx_desc_auto_clear = true // Auto clear tx descriptor on underflow
        };
    i2s_driver_install(0, &i2s_config, 0, NULL);

    i2s_pin_config_t pin_config =
        {
            .bck_io_num = 26,
            .ws_io_num = 22,
            .data_out_num = 25,
            .data_in_num = -1 // Not used
        };

    i2s_set_pin(0, &pin_config);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    esp_bluedroid_config_t bluedroid_cfg = BT_BLUEDROID_INIT_CONFIG_DEFAULT();
    if ((err = esp_bluedroid_init_with_cfg(&bluedroid_cfg)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_bluedroid_enable()) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_avrc_ct_register_callback(my_bt_avrc_cb)) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s initialize avrc failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    if ((err = esp_avrc_ct_init()) != ESP_OK)
    {
        ESP_LOGE(TAG, "%s itialize avrc failed: %s", __func__, esp_err_to_name(err));
        return;
    }

    err = esp_a2d_sink_init();
    err = esp_a2d_register_callback(my_bt_a2d_cb);
    err = esp_a2d_sink_register_data_callback(my_bt_a2d_sink_data_cb);

    err = esp_bt_gap_register_callback(my_bt_gap_cb);
    err = esp_bt_dev_set_device_name(bt_device_name);
    err = esp_ble_gap_set_device_name(ble_device_name);

    esp_bt_io_cap_t io_cap = ESP_BT_IO_CAP_OUT;
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    err = esp_bt_gap_set_security_param(param_type, &io_cap, sizeof(uint8_t));

    err = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    ble_gatts_init();
}