################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Utilities/Fonts/font12.c \
../src/Utilities/Fonts/font16.c \
../src/Utilities/Fonts/font20.c \
../src/Utilities/Fonts/font24.c \
../src/Utilities/Fonts/font8.c 

OBJS += \
./src/Utilities/Fonts/font12.o \
./src/Utilities/Fonts/font16.o \
./src/Utilities/Fonts/font20.o \
./src/Utilities/Fonts/font24.o \
./src/Utilities/Fonts/font8.o 

C_DEPS += \
./src/Utilities/Fonts/font12.d \
./src/Utilities/Fonts/font16.d \
./src/Utilities/Fonts/font20.d \
./src/Utilities/Fonts/font24.d \
./src/Utilities/Fonts/font8.d 


# Each subdirectory must supply rules for building sources it contributes
src/Utilities/Fonts/%.o: ../src/Utilities/Fonts/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross ARM C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g3 -DDEBUG -DUSE_FULL_ASSERT -DTRACE -DOS_USE_TRACE_ITM -DSTM32F429xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -I"../include" -I"../system/include" -I"../system/include/cmsis" -I"../system/include/stm32f4-hal" -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


