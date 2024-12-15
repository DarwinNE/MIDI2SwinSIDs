RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include system/src/stm32f4-hal/subdir.mk
-include system/src/newlib/subdir.mk
-include system/src/diag/subdir.mk
-include system/src/cortexm/subdir.mk
-include system/src/cmsis/subdir.mk
-include system/include/BSP/Components/wm8994/subdir.mk
-include system/include/BSP/Components/ts3510/subdir.mk
-include system/include/BSP/Components/stmpe811/subdir.mk
-include system/include/BSP/Components/stmpe1600/subdir.mk
-include system/include/BSP/Components/ov2640/subdir.mk
-include system/include/BSP/Components/lsm303dlhc/subdir.mk
-include system/include/BSP/Components/lis3dsh/subdir.mk
-include system/include/BSP/Components/lis302dl/subdir.mk
-include system/include/BSP/Components/l3gd20/subdir.mk
-include system/include/BSP/Components/ili9341/subdir.mk
-include system/include/BSP/Components/ili9325/subdir.mk
-include system/include/BSP/Components/cs43l22/subdir.mk
-include src/Utilities/Log/subdir.mk
-include src/Utilities/Fonts/subdir.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
SECONDARY_FLASH += \
Display.hex Display.bin \

SECONDARY_SIZE += \
Display.siz \


# All Target
all: Display.elf secondary-outputs

# Tool invocations
Display.elf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Compiling startup code'
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g3 -DDEBUG -DUSE_FULL_ASSERT -DTRACE -DOS_USE_TRACE_ITM -DSTM32F429xx -DUSE_HAL_DRIVER -DHSE_VALUE=8000000 -c -o startup_stm32f4xx.o startup_stm32f4xx.s
	@echo ' '
	@echo 'Invoking: Cross ARM C++ Linker'
	#arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g3 -T mem.ld -T libs.ld -T sections.ld -nostartfiles -Xlinker --gc-sections -L"../ldscripts" -Wl,-Map,"Display.map" --specs=nano.specs -o "Display.elf" $(OBJS) $(USER_OBJS) $(LIBS) startup_stm32f4xx.o
	# 2024_12_15 I get an error with --specs=nano.specs (stdc++ lib absent)
	arm-none-eabi-g++ -mcpu=cortex-m4 -mthumb -mfloat-abi=soft -Og -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra  -g3 -T mem.ld -T libs.ld -T sections.ld -nostartfiles -Xlinker --gc-sections -L"../ldscripts" -Wl,-Map,"Display.map" -o "Display.elf" $(OBJS) $(USER_OBJS) $(LIBS) startup_stm32f4xx.o
	@echo 'Finished building target: $@'
	@echo ' '

Display.hex: Display.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O ihex "Display.elf"  "Display.hex"
	@echo 'Finished building: $@'
	@echo ' '

Display.bin: Display.elf
	@echo 'Invoking: Cross ARM GNU Create Flash Image'
	arm-none-eabi-objcopy -O binary "Display.elf"  "Display.bin"
	@echo 'Finished building: $@'
	@echo ' '

Display.siz: Display.elf
	@echo 'Invoking: Cross ARM GNU Print Size'
	arm-none-eabi-size --format=berkeley "Display.elf"
	@echo 'Finished building: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(SECONDARY_SIZE)$(C++_DEPS)$(OBJS)$(C_DEPS)$(ASM_DEPS)$(CC_DEPS)$(SECONDARY_FLASH)$(CPP_DEPS)$(CXX_DEPS)$(C_UPPER_DEPS)$(S_UPPER_DEPS) Display.elf
	-@echo ' '

secondary-outputs: $(SECONDARY_FLASH) $(SECONDARY_SIZE)

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
