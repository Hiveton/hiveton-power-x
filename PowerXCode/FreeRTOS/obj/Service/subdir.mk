################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Service/service_charge_protocols.c \
../Service/service_emark.c \
../Service/service_legacy_charge.c \
../Service/service_measure.c \
../Service/service_pd_objects.c \
../Service/service_pd.c 

C_DEPS += \
./Service/service_charge_protocols.d \
./Service/service_emark.d \
./Service/service_legacy_charge.d \
./Service/service_measure.d \
./Service/service_pd_objects.d \
./Service/service_pd.d 

OBJS += \
./Service/service_charge_protocols.o \
./Service/service_emark.o \
./Service/service_legacy_charge.o \
./Service/service_measure.o \
./Service/service_pd_objects.o \
./Service/service_pd.o 

DIR_OBJS += \
./Service/*.o \

DIR_DEPS += \
./Service/*.d \

DIR_EXPANDS += \
./Service/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Service/%.o: ../Service/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Debug" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Core" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/User" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Peripheral/inc" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/App/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Bsp/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Service/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/UI/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Assets" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
