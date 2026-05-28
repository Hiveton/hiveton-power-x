################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/ch32l103_it.c \
../User/main.c \
../User/system_ch32l103.c 

C_DEPS += \
./User/ch32l103_it.d \
./User/main.d \
./User/system_ch32l103.d 

OBJS += \
./User/ch32l103_it.o \
./User/main.o \
./User/system_ch32l103.o 

DIR_OBJS += \
./User/*.o \

DIR_DEPS += \
./User/*.d \

DIR_EXPANDS += \
./User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Debug" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Core" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/User" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Peripheral/inc" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/App/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Bsp/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Service/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/UI/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Assets" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

