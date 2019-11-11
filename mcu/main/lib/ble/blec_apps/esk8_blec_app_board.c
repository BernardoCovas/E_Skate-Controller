#include <esk8_blec_apps.h>

static esk8_err_t
app_init(
);

static esk8_err_t
app_deinit(
);

static esk8_err_t
app_conn_add(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p
);

static esk8_err_t
app_conn_del(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p
);

static esk8_err_t
app_conn_notif(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p,
    int attr_idx,
    size_t len,
    uint8_t* val
);

static esk8_err_t
app_conn_read(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p,
    int attr_idx,
    size_t len,
    uint8_t* val
);

static esk8_err_t
app_evt_cb(
    esp_gattc_cb_event_t event,
    esp_ble_gattc_cb_param_t *param
);


esk8_blec_app_t
esk8_blec_app_board =
{
    .app_name = "blec_app_board",

    .app_init = app_init,
    .app_deinit = app_deinit,
    .app_conn_add = app_conn_add,
    .app_conn_del = app_conn_del,
    .app_conn_notif = app_conn_notif,
    .app_conn_read = app_conn_read,
    .app_evt_cb = app_evt_cb
};


static esk8_err_t
app_init(
)
{
    return ESK8_OK;
}


static esk8_err_t
app_deinit(
)
{
    return ESK8_OK;
}


static esk8_err_t
app_conn_add(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p
)
{
    return ESK8_OK;
}


static esk8_err_t
app_conn_del(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p
)
{
    return ESK8_OK;
}


static esk8_err_t
app_conn_notif(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p,
    int attr_idx,
    size_t len,
    uint8_t* val
)
{
    return ESK8_OK;
}


static esk8_err_t
app_conn_read(
    esk8_blec_dev_t* dev,
    void** conn_ctx_p,
    int attr_idx,
    size_t len,
    uint8_t* val
)
{
    return ESK8_OK;
}


static esk8_err_t
app_evt_cb(
    esp_gattc_cb_event_t event,
    esp_ble_gattc_cb_param_t *param
)
{
    return ESK8_OK;
}