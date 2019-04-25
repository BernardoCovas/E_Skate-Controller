#include <e_skate_err.h>

#include <e_skate_bms.h>
#include <e_skate_bms_utils.h>

e_skate_err_t e_skate_bms_get_deep_status(

    e_skate_bms_config_t      *bmsConfig,
    e_skate_bms_deep_status_t *outStatus

)
{
    e_skate_uart_reg_t readRegs[] = {
        E_SKATE_REG_BMS_SERIAL_NUMBER,
        E_SKATE_REG_BMS_FW_VERSION,
        E_SKATE_REG_BMS_MANUFACTURE_DATE,
        E_SKATE_REG_BMS_FACTORY_CAPACITY,
        E_SKATE_REG_BMS_ACTUAL_CAPACITY,
        E_SKATE_REG_BMS_CAPACITY,
        E_SKATE_REG_BMS_CAPACITY_mAh,
        E_SKATE_REG_BMS_CHARGE_FULL_CYCLES,
        E_SKATE_REG_BMS_CHARGE_COUNT,
        E_SKATE_REG_BMS_HEALTH,
        E_SKATE_REG_BMS_CELL0_V,
        E_SKATE_REG_BMS_CELL1_V,
        E_SKATE_REG_BMS_CELL2_V,
        E_SKATE_REG_BMS_CELL3_V,
        E_SKATE_REG_BMS_CELL4_V,
        E_SKATE_REG_BMS_CELL5_V,
        E_SKATE_REG_BMS_CELL6_V,
        E_SKATE_REG_BMS_CELL7_V,
        E_SKATE_REG_BMS_CELL8_V,
        E_SKATE_REG_BMS_CELL9_V,
        };

    void* outRegP[] = {
        (void*) &outStatus->serialNumber,
        (void*) &outStatus->firmwareVersion,
        (void*) &outStatus->manufactureDate,
        (void*) &outStatus->factoryCapacity_mAh,
        (void*) &outStatus->actualCapacity_mAh,
        (void*) &outStatus->remainingCapacity_prc,
        (void*) &outStatus->remainingCapacity_mAh,
        (void*) &outStatus->chargeFullCycles,
        (void*) &outStatus->chargeCount,
        (void*) &outStatus->packHeath_prc,
        (void*) &outStatus->cellVoltage_mV[0],
        (void*) &outStatus->cellVoltage_mV[1],
        (void*) &outStatus->cellVoltage_mV[2],
        (void*) &outStatus->cellVoltage_mV[3],
        (void*) &outStatus->cellVoltage_mV[4],
        (void*) &outStatus->cellVoltage_mV[5],
        (void*) &outStatus->cellVoltage_mV[6],
        (void*) &outStatus->cellVoltage_mV[7],
        (void*) &outStatus->cellVoltage_mV[8],
        (void*) &outStatus->cellVoltage_mV[9],
        };

    uint16_t nReg = sizeof(readRegs) / sizeof(readRegs[0]);

    for (int i=0; i<nReg; i++)
    {
        size_t readSize = readRegs[i]==E_SKATE_REG_BMS_SERIAL_NUMBER?0x0E:0x02;

        e_skate_err_t errCode = get_data_with_response(
            bmsConfig,
            E_SKATE_ADDR_BMS,
            readRegs[i],
            readSize,
            outRegP[i]);

        if (errCode != E_SKATE_SUCCESS)
            return errCode;

    }

    return E_SKATE_SUCCESS;
}