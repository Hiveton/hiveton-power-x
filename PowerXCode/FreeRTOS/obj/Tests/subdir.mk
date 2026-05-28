################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Tests/test_bsp_adc_config.c \
../Tests/test_legacy_charge_and_emark.c \
../Tests/test_measure_service.c \
../Tests/test_protocol_snapshot.c \
../Tests/test_ui_model.c 

C_DEPS += \
./Tests/test_bsp_adc_config.d \
./Tests/test_legacy_charge_and_emark.d \
./Tests/test_measure_service.d \
./Tests/test_protocol_snapshot.d \
./Tests/test_ui_model.d 

OBJS += \
./Tests/test_bsp_adc_config.o \
./Tests/test_legacy_charge_and_emark.o \
./Tests/test_measure_service.o \
./Tests/test_protocol_snapshot.o \
./Tests/test_ui_model.o 

DIR_OBJS += \
./Tests/*.o \

DIR_DEPS += \
./Tests/*.d \

DIR_EXPANDS += \
./Tests/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Tests/%.o: ../Tests/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Debug" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Core" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/User" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Peripheral/inc" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

