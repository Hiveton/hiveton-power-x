################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/portable/MemMang/heap_4.c 

C_DEPS += \
./FreeRTOS/portable/MemMang/heap_4.d 

OBJS += \
./FreeRTOS/portable/MemMang/heap_4.o 

DIR_OBJS += \
./FreeRTOS/portable/MemMang/*.o \

DIR_DEPS += \
./FreeRTOS/portable/MemMang/*.d \

DIR_EXPANDS += \
./FreeRTOS/portable/MemMang/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/portable/MemMang/%.o: ../FreeRTOS/portable/MemMang/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Debug" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Core" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/User" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/SRC/Peripheral/inc" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/App/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Bsp/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Service/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/UI/include" -I"/Users/xuehui/CodeingProjects/hiveton-power-x/PowerXCode/FreeRTOS/Assets" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

