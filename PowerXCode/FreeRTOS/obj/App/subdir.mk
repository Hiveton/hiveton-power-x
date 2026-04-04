################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../App/app_controller.c \
../App/app_tasks.c 

C_DEPS += \
./App/app_controller.d \
./App/app_tasks.d 

OBJS += \
./App/app_controller.o \
./App/app_tasks.o 

DIR_OBJS += \
./App/*.o \

DIR_DEPS += \
./App/*.d \

DIR_EXPANDS += \
./App/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
App/%.o: ../App/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Debug" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Core" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/User" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Peripheral/inc" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Assets" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

