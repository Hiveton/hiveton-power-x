/********************************** (C) COPYRIGHT *******************************
 * File Name          : ch32l103_it.c
 * Author             : WCH
 * Version            : V1.0.0
 * Date               : 2024/10/30
 * Description        : Main Interrupt Service Routines.
 *********************************************************************************
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * Attention: This software (modified or not) and binary are used for
 * microcontroller manufactured by Nanjing Qinheng Microelectronics.
 *******************************************************************************/
#include "ch32l103_it.h"
#include "bsp_adc_dma.h"
#include "bsp_usbpd_port.h"

void NMI_Handler(void) __attribute__((interrupt()));
void HardFault_Handler(void) __attribute__((interrupt()));
void DMA1_Channel1_IRQHandler(void) __attribute__((interrupt()));
void USBPD_IRQHandler(void) __attribute__((interrupt()));

/*********************************************************************
 * @fn      NMI_Handler
 *
 * @brief   This function handles NMI exception.
 *
 * @return  none
 */
void NMI_Handler(void)
{
    while (1)
    {
    }
}


/*********************************************************************
 * @fn      HardFault_Handler
 *
 * @brief   This function handles Hard Fault exception.
 *
 * @return  none
 */
void HardFault_Handler(void)
{
    NVIC_SystemReset();
    while (1)
    {
    }
}

void DMA1_Channel1_IRQHandler(void)
{
    bsp_adc_dma_irq_handler();
}

void USBPD_IRQHandler(void)
{
    bsp_usbpd_irq_handler();
}

