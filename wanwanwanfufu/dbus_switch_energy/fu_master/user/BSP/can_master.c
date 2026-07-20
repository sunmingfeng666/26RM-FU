#include "can_master.h"

volatile CanMasterDebug_t g_can_master_debug = {0};

static CAN_HandleTypeDef *s_can = NULL;
static volatile uint8_t s_hit_mask = 0U;

static HAL_StatusTypeDef CanMaster_Send(uint32_t std_id, uint8_t data0)
{
    CAN_TxHeaderTypeDef header = {0};
    uint8_t data[8] = {0};
    uint32_t mailbox = 0U;
    HAL_StatusTypeDef result;

    if (s_can == NULL)
    {
        return HAL_ERROR;
    }

    header.StdId = std_id;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = 8U;
    header.TransmitGlobalTime = DISABLE;
    data[0] = data0;

    result = HAL_CAN_AddTxMessage(s_can, &header, data, &mailbox);
    g_can_master_debug.last_tx_result = result;
    if (result == HAL_OK)
    {
        g_can_master_debug.tx_count++;
    }
    else
    {
        g_can_master_debug.tx_error_count++;
    }
    return result;
}

HAL_StatusTypeDef CanMaster_Init(CAN_HandleTypeDef *hcan)
{
    CAN_FilterTypeDef filter = {0};
    HAL_StatusTypeDef result;

    if (hcan == NULL)
    {
        return HAL_ERROR;
    }

    s_can = hcan;
    filter.FilterBank = 14U;
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

    return HAL_CAN_ActivateNotification(s_can, CAN_IT_RX_FIFO0_MSG_PENDING);
}

HAL_StatusTypeDef CanMaster_SendTargetOn(uint8_t target, EnergyMode_e mode)
{
    if ((target < 1U) || (target > ENERGY_SLAVE_COUNT) || !ENERGY_MODE_IS_VALID(mode))
    {
        return HAL_ERROR;
    }
    return CanMaster_Send(ENERGY_CAN_ON_ID(target), (uint8_t)mode);
}

HAL_StatusTypeDef CanMaster_SendTargetOff(uint8_t target)
{
    if ((target < 1U) || (target > ENERGY_SLAVE_COUNT))
    {
        return HAL_ERROR;
    }
    return CanMaster_Send(ENERGY_CAN_OFF_ID(target), 0U);
}

HAL_StatusTypeDef CanMaster_SendAllOff(void)
{
    return CanMaster_Send(ENERGY_CAN_ALL_OFF_ID, 0U);
}

uint8_t CanMaster_TakeHitMask(void)
{
    uint8_t mask;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    mask = s_hit_mask;
    s_hit_mask = 0U;
    if (primask == 0U)
    {
        __enable_irq();
    }
    return mask;
}

void CanMaster_ClearHitMask(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    s_hit_mask = 0U;
    if (primask == 0U)
    {
        __enable_irq();
    }
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
            g_can_master_debug.rx_error_count++;
            break;
        }

        g_can_master_debug.last_rx_id = header.StdId;
        g_can_master_debug.rx_count++;

        if ((header.IDE != CAN_ID_STD) || (header.RTR != CAN_RTR_DATA))
        {
            continue;
        }

        for (uint8_t target = 1U; target <= ENERGY_SLAVE_COUNT; ++target)
        {
            if (header.StdId == ENERGY_CAN_HIT_ID(target))
            {
                s_hit_mask |= (uint8_t)(1U << (target - 1U));
                break;
            }
        }
    }
}
