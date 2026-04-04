################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../FreeRTOS/croutine.c \
../FreeRTOS/event_groups.c \
../FreeRTOS/list.c \
../FreeRTOS/queue.c \
../FreeRTOS/stream_buffer.c \
../FreeRTOS/tasks.c \
../FreeRTOS/timers.c 

C_DEPS += \
./FreeRTOS/croutine.d \
./FreeRTOS/event_groups.d \
./FreeRTOS/list.d \
./FreeRTOS/queue.d \
./FreeRTOS/stream_buffer.d \
./FreeRTOS/tasks.d \
./FreeRTOS/timers.d 

OBJS += \
./FreeRTOS/croutine.o \
./FreeRTOS/event_groups.o \
./FreeRTOS/list.o \
./FreeRTOS/queue.o \
./FreeRTOS/stream_buffer.o \
./FreeRTOS/tasks.o \
./FreeRTOS/timers.o 

DIR_OBJS += \
./FreeRTOS/*.o \

DIR_DEPS += \
./FreeRTOS/*.d \

DIR_EXPANDS += \
./FreeRTOS/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/%.o: ../FreeRTOS/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -g -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Debug" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Core" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/User" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/SRC/Peripheral/inc" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/Common" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/GCC/RISC-V/chip_specific_extensions/RV32I_PFIC_no_extensions" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/FreeRTOS/portable/MemMang" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/App/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Bsp/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Service/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/UI/include" -I"/Users/hiveton/HivetonCode/HivetonPowerX/PowerXCode/FreeRTOS/Assets" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

