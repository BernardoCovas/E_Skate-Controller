#include <esk8_err.h>
#include <esk8_pwm.h>
#include <esk8_remote.h>

#include <esp_log.h>
#include <nvs_flash.h>

#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_gattc_api.h>
#include <esp_gap_ble_api.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>


esk8_remote_t esk8_remote = { 0 };

void
esk8_remote_gattc_cb(
    esp_gattc_cb_event_t        event,
    esp_gatt_if_t               gattc_if,
    esp_ble_gattc_cb_param_t*   param
);

void
esk8_remote_gap_cb(
    esp_gap_ble_cb_event_t event,
    esp_ble_gap_cb_param_t *param
);

esk8_err_t
esk8_remote_start()
{
    if (esk8_remote.state)
        return ESK8_ERR_REMT_REINIT;

    esk8_remote.state = ESK8_REMOTE_STATE_INIT;

    esp_err_t ret = nvs_flash_init();

    if  (
            ret == ESP_ERR_NVS_NO_FREE_PAGES ||
            ret == ESP_ERR_NVS_NEW_VERSION_FOUND
        )
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    if (ret)
        return ESK8_NVS_NOT_AVAILABLE;

    esp_bt_controller_config_t bt_cnfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    ESP_ERROR_CHECK(    ret                                                             );
    ESP_ERROR_CHECK(    esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT)           );
    ESP_ERROR_CHECK(    esp_bt_controller_init(&bt_cnfg)                                );
    ESP_ERROR_CHECK(    esp_bt_controller_enable(ESP_BT_MODE_BLE)                       );

    ESP_ERROR_CHECK(    esp_bluedroid_init()                                            );
    ESP_ERROR_CHECK(    esp_bluedroid_enable()                                          );

    ESP_ERROR_CHECK(    esp_ble_gap_register_callback(esk8_remote_gap_cb)               );
    ESP_ERROR_CHECK(    esp_ble_gattc_register_callback(esk8_remote_gattc_cb)           );

    if  (
            xTaskCreate(
                esk8_remote_task_ps2,
                "esk8_remote_task_ps2",
                2048, NULL, 2,
                esk8_remote.ps2_task
            ) != pdPASS
        )
    {
        esk8_remote.state = ESK8_REMOTE_STATE_STOPPED;
        return ESK8_ERR_OOM;
    }

    esk8_err_t err;

    err = esk8_pwm_sgnl_init(&esk8_remote.pwm_cnfg);
    if (err)
    {
        esk8_remote_stop();
        return err;
    }

    esk8_remote.state = ESK8_REMOTE_STATE_NOT_CONNECTED;

    return ESK8_SUCCESS;
}

esk8_err_t
esk8_remote_stop()
{
    return ESK8_SUCCESS;
}

esk8_err_t
esk8_remote_incr_speed(
    int incr
)
{
    esk8_remote.speed += incr;
    esk8_remote.speed = esk8_remote.speed > 255 ? 255 : esk8_remote.speed;
    esk8_remote.speed = esk8_remote.speed < 0   ? 0   : esk8_remote.speed;

    esk8_pwm_sgnl_set(&esk8_remote.pwm_cnfg, esk8_remote.speed);

    return ESK8_SUCCESS;
}

void
esk8_remote_gap_cb(
    esp_gap_ble_cb_event_t  event,
    esp_ble_gap_cb_param_t *param
)
{
    printf("[ GAP ] Got event: %d\n", event);

    switch (event)
    {
    case ESP_GAP_BLE_SCAN_RESULT_EVT:
    {
        if (esk8_remote.state != ESK8_REMOTE_STATE_SEARCHING)
        {
            esp_ble_gap_stop_scanning();
            break;
        }

        if (param->scan_rst.search_evt != ESP_GAP_SEARCH_INQ_RES_EVT)
            break;

        printf("[ GAP ] Dev: ");
        for (int i = 0; i < 6; i++)
            printf("%s%02x", i==0 ? "":":", param->scan_rst.bda[i]);

        printf(", RSSI: %d, evt: %d\n",
            param->scan_rst.rssi,
            param->scan_rst.search_evt
        );

        break;
    }

    default:
        break;
    }
}

void
esk8_remote_gattc_cb(
    esp_gattc_cb_event_t        event,
    esp_gatt_if_t               gattc_if,
    esp_ble_gattc_cb_param_t*   param
)
{
    printf("[ GATTC ] Got event: %d\n", event);

    switch (event)
    {
    case ESP_GATTC_DISCONNECT_EVT:
        break;

    case ESP_GATTC_CONNECT_EVT:
        break;

    default:
        break;
    }
}

esk8_err_t
esk8_remote_connect(
    uint32_t sec
)
{
    if (esk8_remote.state != ESK8_REMOTE_STATE_NOT_CONNECTED)
        return ESK8_ERR_REMT_BAD_STATE;

    esp_err_t err = esp_ble_gap_start_scanning(sec);
    if (err)
        return ESK8_ERR_REMT_BAD_STATE;

    return ESK8_SUCCESS;
}