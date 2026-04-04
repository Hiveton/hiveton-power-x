################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../UI/ui_model.c \
../UI/ui_pages.c \
../UI/ui_renderer.c \
../UI/ui_widgets.c 

C_DEPS += \
./UI/ui_model.d \
./UI/ui_pages.d \
./UI/ui_renderer.d \
./UI/ui_widgets.d 

OBJS += \
./UI/ui_model.o \
./UI/ui_pages.o \
./UI/ui_renderer.o \
./UI/ui_widgets.o 

DIR_OBJS += \
./UI/*.o \

DIR_DEPS += \
./UI/*.d \

DIR_EXPANDS += \
./UI/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
UI/%.o: ../UI/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Debug" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Core" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/User" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Peripheral/inc" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Assets" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

