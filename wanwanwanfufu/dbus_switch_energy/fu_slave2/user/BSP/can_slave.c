#include "can_slave.h"
#include "energy_slave_config.h"

volatile CanSlaveDebug_t g_can_slave_debug = {
    .mode = ENERGY_MODE_SMALL
};

static CAN_HandleTypeDef *s_can = NULL;

HAL_StatusTypeDef CanSlave_Init(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef filter = {0};
    HAL_StatusTypeDef result;

    if (hcan == NULL)
    {
        return HAL_ERROR;
    }

    s_can = hcan;
    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;

    result = HAL_CAN_ConfigFilter(s_can, &filter);
    if (result != HAL_OK)
    {
        return result;
    }

    result = HAL_CAN_Start(s_can);
    if (result != HAL_OK)
    {
        return result;
    }

    result = HAL_CAN_ActivateNotification(s_can, CAN_IT_RX_FIFO0_MSG_PENDING);
    if (result == HAL_OK)
    {
        g_can_slave_debug.active = 0U;
    }
    return result;
}

uint8_t CanSlave_IsActive(void)
{
    return g_can_slave_debug.active;
}

EnergyMode_e CanSlave_GetMode(void)
{
    return g_can_slave_debug.mode;
}

CanSlaveCommand_e CanSlave_GetLastControlCommand(void)
{
    return g_can_slave_debug.last_control_command;
}

uint32_t CanSlave_GetControlCommandCount(void)
{
    return g_can_slave_debug.control_command_count;
}

HAL_StatusTypeDef CanSlave_SendHitFeedback(void)
{
    CAN_TxHeaderTypeDef header = {0};
    uint8_t data[8] = {0};
    uint32_t mailbox = 0U;
    HAL_StatusTypeDef result;

    if (s_can == NULL)
    {
        return HAL_ERROR;
    }

    header.StdId = ENERGY_CAN_HIT_ID(ENERGY_SLAVE_INDEX);
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = 8U;
    header.TransmitGlobalTime = DISABLE;

    result = HAL_CAN_AddTxMessage(s_can, &header, data, &mailbox);
    g_can_slave_debug.last_hit_tx_result = result;
    if (result == HAL_OK)
    {
        g_can_slave_debug.hit_tx_count++;
    }
    else
    {
        g_can_slave_debug.hit_tx_error_count++;
    }
    return result;
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    CAN_RxHeaderTypeDef header;
    uint8_t data[8];

    if ((s_can == NULL) || (hcan->Instance != s_can->Instance))
    {
        return;
    }

    while (HAL_CAN_GetRxFifoFillLevel(hcan, CAN_RX_FIFO0) > 0U)
    {
        if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, data) != HAL_OK)
        {
            g_can_slave_debug.rx_error_count++;
            break;
        }

        g_can_slave_debug.last_rx_id = header.StdId;
        g_can_slave_debug.rx_count++;

        if ((header.IDE != CAN_ID_STD) || (header.RTR != CAN_RTR_DATA))
        {
            continue;
        }

        if (header.StdId == ENERGY_CAN_ON_ID(ENERGY_SLAVE_INDEX))
        {
            if ((header.DLC >= 1U) && ENERGY_MODE_IS_VALID((EnergyMode_e)data[0]))
            {
                g_can_slave_debug.mode = (EnergyMode_e)data[0];
            }
            g_can_slave_debug.active = 1U;
            g_can_slave_debug.last_control_command = CAN_SLAVE_COMMAND_ON;
            g_can_slave_debug.control_command_count++;
        }
        else if (header.StdId == ENERGY_CAN_OFF_ID(ENERGY_SLAVE_INDEX))
        {
            g_can_slave_debug.active = 0U;
            g_can_slave_debug.last_control_command = CAN_SLAVE_COMMAND_LOCAL_OFF;
            g_can_slave_debug.control_command_count++;
        }
        else if (header.StdId == ENERGY_CAN_ALL_OFF_ID)
        {
            g_can_slave_debug.active = 0U;
            g_can_slave_debug.last_control_command = CAN_SLAVE_COMMAND_ALL_OFF;
            g_can_slave_debug.control_command_count++;
        }
    }
}
