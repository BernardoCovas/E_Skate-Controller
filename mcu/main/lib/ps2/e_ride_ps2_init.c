#include <e_ride_err.h>
#include <e_ride_config.h>
#include <e_ride_ps2.h>
#include <e_ride_ps2_utils.h>

#include <esp_intr_alloc.h>
#include <driver/gpio.h>
#include <driver/timer.h>

#include <math.h>


void e_ride_ps2_isr(

    void* param

)
{
    e_ride_ps2_handle_t*   ps2Handle = (e_ride_ps2_handle_t*) param;
    e_ride_ps2_config_t*   ps2Config = &( ps2Handle->ps2Config );

    timer_group_t tg = ps2Config->timerConfig.timerGroup;
    timer_idx_t   ti = ps2Config->timerConfig.timerIdx;
    gpio_num_t d_pin = ps2Config->gpioConfig.dataPin;
    gpio_num_t c_pin = ps2Config->gpioConfig.clockPin;

    if (ps2Config->dataDrctn == PS2_DIRCN_RECV)
    {
        e_ride_ps2_bit_t newBit;
        newBit.bit = gpio_get_level(d_pin);                             /* NOTE (b.covas): Measure the data pin right away  */
        timer_get_counter_time_sec(tg, ti, &newBit.bitInterval_s);      /* NOTE (b.covas): Get the time since the last bit  */
        timer_set_counter_value(tg, ti, (uint64_t) 0);                  /* NOTE (b.covas): Reset Timer                      */
        xQueueSendFromISR(ps2Handle->rxBitQueueHandle, &newBit, NULL);  /* NOTE (b.covas): Send to queue                    */
        return;
    }

    if (ps2Handle->ps2Config.dataDrctn == PS2_DIRCN_SEND)
    {
        if (ps2Handle->txTaskToNotift == NULL)
            return;

        bool bitVal;
        e_ride_err_t errCode = e_ride_ps2_take_bit(&ps2Handle->txPkt, &bitVal);
        gpio_set_level(d_pin, (uint32_t) bitVal);

        if (errCode == E_RIDE_PS2_ERR_VALUE_READY && ps2Handle->txPkt.frameIndex++ >= 12)
        {
            ps2Config->dataDrctn = PS2_DIRCN_RECV;

            gpio_set_direction(d_pin, GPIO_MODE_INPUT);
            gpio_set_intr_type(c_pin, GPIO_INTR_NEGEDGE);

            vTaskNotifyGiveFromISR(ps2Handle->txTaskToNotift, NULL);
        }
    }
}


/* NOTE (b.covas): Infinite Task */
void ps2_rx_consumer_task(

    void* param

) 
{
    e_ride_ps2_handle_t*   ps2Handle = (e_ride_ps2_handle_t*) param;
    e_ride_ps2_pkt_t*      ps2Pkt    = &ps2Handle->rxPkt;
    e_ride_ps2_bit_t       newData;

    while(true)
    {
        if(!xQueueReceive(ps2Handle->rxBitQueueHandle, &newData, portMAX_DELAY)) {
            e_ride_ps2_reset_pkt(ps2Pkt);
            continue; }

        if (newData.bitInterval_s > (double) E_RIDE_PS2_PACKET_TIMEOUT_MS / 1000)
            e_ride_ps2_reset_pkt(ps2Pkt);

        if (e_ride_ps2_add_bit(ps2Pkt, newData.bit) == E_RIDE_PS2_ERR_VALUE_READY) {
            if (e_ride_ps2_check_pkt(ps2Pkt) == E_RIDE_SUCCESS) // NOTE (b.covas): If this is not true, we lost a packet.
                xQueueSend(ps2Handle->rxByteQueueHandle, &ps2Pkt->newByte, 0);
            else
                printf("[Lost packet: %d %d %d %d]\n", ps2Pkt->newStart, ps2Pkt->newByte, ps2Pkt->newParity, ps2Pkt->newStop);

            e_ride_ps2_reset_pkt(ps2Pkt); }
    }
}


e_ride_err_t e_ride_ps2_init(

    e_ride_ps2_handle_t* ps2Handle,
    e_ride_ps2_config_t* ps2Config

)
{
    ps2Handle->ps2Config = *ps2Config;

    timer_config_t espTimerConf = {
        .divider        = 16,
        .counter_dir    = TIMER_COUNT_UP    ,
        .counter_en     = TIMER_START       ,
        .alarm_en       = TIMER_ALARM_DIS   ,
        .intr_type      = TIMER_INTR_LEVEL  ,
        .auto_reload    = false
    };

    ps2Handle->rxBitQueueHandle  = xQueueCreate(E_RIDE_PS2_BIT_QUEUE_LENGTH , sizeof(e_ride_ps2_bit_t));
    ps2Handle->rxByteQueueHandle = xQueueCreate(E_RIDE_PS2_BYTE_QUEUE_LENGTH, 1);
    ps2Handle->rxTaskHandle = NULL;

    xTaskCreate(
        ps2_rx_consumer_task,
        "ps2_rx_consumer_task",
        2048, ps2Handle,
        E_RIDE_PS2_RX_TASK_PRIORITY,
        &ps2Handle->rxTaskHandle);

    if  (
            ps2Handle->rxBitQueueHandle  == NULL ||
            ps2Handle->rxByteQueueHandle == NULL ||
            ps2Handle->rxTaskHandle      == NULL
        )
    {
        e_ride_ps2_deinit(ps2Handle, false);
        return E_RIDE_ERR_OOM;
    }


    /* NOTE (b.covas): GPIO Interrupt Init */
    gpio_num_t d_pin, c_pin;
    d_pin = ps2Config->gpioConfig.dataPin;
    c_pin = ps2Config->gpioConfig.clockPin;
    
    ESP_ERROR_CHECK(gpio_set_pull_mode(c_pin, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_pull_mode(d_pin, GPIO_PULLUP_ONLY));
    ESP_ERROR_CHECK(gpio_set_drive_capability(c_pin, GPIO_DRIVE_CAP_0));
    ESP_ERROR_CHECK(gpio_set_drive_capability(d_pin, GPIO_DRIVE_CAP_0));
    ESP_ERROR_CHECK(gpio_set_direction(c_pin, GPIO_MODE_INPUT));
    ESP_ERROR_CHECK(gpio_set_direction(d_pin, GPIO_MODE_INPUT));

    /* NOTE (b.covas): Falling edge intr, as per Ps2. */
    ESP_ERROR_CHECK(gpio_set_intr_type(c_pin, GPIO_INTR_NEGEDGE));

    esp_err_t isrErrCode = gpio_install_isr_service(0);

    /* NOTE (b.covas): Service might already be installed. */
    if  (isrErrCode != ESP_OK && isrErrCode != ESP_ERR_INVALID_STATE)
        ESP_ERROR_CHECK(isrErrCode);

    ESP_ERROR_CHECK(gpio_isr_handler_add(c_pin, e_ride_ps2_isr, ps2Handle));

    /* NOTE (b.covas): Timer Init */
    ESP_ERROR_CHECK(timer_init(
        ps2Config->timerConfig.timerGroup,
        ps2Config->timerConfig.timerIdx,
        &espTimerConf));

    e_ride_ps2_reset_pkt(&ps2Handle->rxPkt);
    e_ride_ps2_reset_pkt(&ps2Handle->txPkt);

    return E_RIDE_SUCCESS;
}