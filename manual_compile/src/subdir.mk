################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/main.c \
../src/SID_def.c \
../src/stm32f4xx_hal_msp.c \
../src/stm32f429i_discovery.c \
../src/stm32f429i_discovery_eeprom.c \
../src/stm32f429i_discovery_io.c \
../src/stm32f429i_discovery_lcd.c \
../src/stm32f429i_discovery_sdram.c \
../src/stm32f429i_discovery_ts.c \
../src/stm32f4xx_it.c 

OBJS += \
./src/main.o \
./src/SID_def.o \
./src/stm32f4xx_hal_msp.o \
./src/stm32f429i_discovery.o \
./src/stm32f429i_discovery_eeprom.o \
./src/stm32f429i_discovery_io.o \
./src/stm32f429i_discovery_lcd.o \
./src/stm32f429i_discovery_sdram.o \
./src/stm32f429i_discovery_ts.o \
./src/stm32f4xx_it.o 

C_DEPS += \
./src/main.d \
./src/SID_def.d \
./src/stm32f4xx_hal_msp.d \
./src/stm32f429i_discovery.d \
./src/stm32f429i_discovery_eeprom.d \
./src/stm32f429i_discovery_io.d \
./src/stm32f429i_discovery_lcd.d \
./src/stm32f429i_discovery_sdram.d \
./src/stm32f429i_discovery_ts.d \
./src/stm32f4xx_it.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g3 -DDEBUG -DUSE_FULL_ASSERT -DTRACE -DOS_USE_TRACE_ITM -DSTM32F429xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


