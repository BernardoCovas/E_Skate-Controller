#include <e_skate_err.h>
#include <e_skate_uart.h>

#include <stdio.h>


e_skate_err_t e_skate_uart_msg_chk(
    e_skate_uart_msg_t msg
)
{
    uint8_t calcChkSum[2];
    e_skate_uart_msg_chk_calc(msg, calcChkSum);

    if (memcmp(calcChkSum, msg.chk_sum, 2) != 0)
        return E_SKATE_UART_MSG_ERR_INVALID_CHKSUM;        

    return E_SKATE_SUCCESS;
}