################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Bsp/bsp_adc_dma.c \
../Bsp/bsp_backlight.c \
../Bsp/bsp_board.c \
../Bsp/bsp_cc_ext_rd.c \
../Bsp/bsp_dpdm.c \
../Bsp/bsp_keys.c \
../Bsp/bsp_lcd_st7735.c \
../Bsp/bsp_usbpd_port.c 

C_DEPS += \
./Bsp/bsp_adc_dma.d \
./Bsp/bsp_backlight.d \
./Bsp/bsp_board.d \
./Bsp/bsp_cc_ext_rd.d \
./Bsp/bsp_dpdm.d \
./Bsp/bsp_keys.d \
./Bsp/bsp_lcd_st7735.d \
./Bsp/bsp_usbpd_port.d 

OBJS += \
./Bsp/bsp_adc_dma.o \
./Bsp/bsp_backlight.o \
./Bsp/bsp_board.o \
./Bsp/bsp_cc_ext_rd.o \
./Bsp/bsp_dpdm.o \
./Bsp/bsp_keys.o \
./Bsp/bsp_lcd_st7735.o \
./Bsp/bsp_usbpd_port.o 

DIR_OBJS += \
./Bsp/*.o \

DIR_DEPS += \
./Bsp/*.d \

DIR_EXPANDS += \
./Bsp/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Bsp/%.o: ../Bsp/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Debug" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Core" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/User" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Peripheral/inc" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/App/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Bsp/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Service/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/UI/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Assets" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
