################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Middlewares/Third_Party/FreeRTOS/Source/croutine.c \
../Middlewares/Third_Party/FreeRTOS/Source/event_groups.c \
../Middlewares/Third_Party/FreeRTOS/Source/list.c \
../Middlewares/Third_Party/FreeRTOS/Source/queue.c \
../Middlewares/Third_Party/FreeRTOS/Source/tasks.c \
../Middlewares/Third_Party/FreeRTOS/Source/timers.c 

OBJS += \
./Middlewares/Third_Party/FreeRTOS/Source/croutine.o \
./Middlewares/Third_Party/FreeRTOS/Source/event_groups.o \
./Middlewares/Third_Party/FreeRTOS/Source/list.o \
./Middlewares/Third_Party/FreeRTOS/Source/queue.o \
./Middlewares/Third_Party/FreeRTOS/Source/tasks.o \
./Middlewares/Third_Party/FreeRTOS/Source/timers.o 

C_DEPS += \
./Middlewares/Third_Party/FreeRTOS/Source/croutine.d \
./Middlewares/Third_Party/FreeRTOS/Source/event_groups.d \
./Middlewares/Third_Party/FreeRTOS/Source/list.d \
./Middlewares/Third_Party/FreeRTOS/Source/queue.d \
./Middlewares/Third_Party/FreeRTOS/Source/tasks.d \
./Middlewares/Third_Party/FreeRTOS/Source/timers.d 


# Each subdirectory must supply rules for building sources it contributes
Middlewares/Third_Party/FreeRTOS/Source/%.o: ../Middlewares/Third_Party/FreeRTOS/Source/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DUSE_HAL_DRIVER -DSTM32L151xC -I"D:/stm32 Workspace/laske/Application/Base/Inc" -I"D:/stm32 Workspace/laske/Application/Drivers/Inc" -I"D:/stm32 Workspace/laske/Application/Functionality/Inc" -I"D:/stm32 Workspace/laske/Application/USB/Inc" -I"D:/stm32 Workspace/laske/Drivers/STM32L1xx_HAL_Driver/Inc" -I"D:/stm32 Workspace/laske/Drivers/STM32L1xx_HAL_Driver/Inc/Legacy" -I"D:/stm32 Workspace/laske/Middlewares/ST/STM32_USB_Device_Library/Core/Inc" -I"D:/stm32 Workspace/laske/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc" -I"D:/stm32 Workspace/laske/Drivers/CMSIS/Device/ST/STM32L1xx/Include" -I"D:/stm32 Workspace/laske/Drivers/CMSIS/Include" -I"D:/stm32 Workspace/laske/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3" -I"D:/stm32 Workspace/laske/Middlewares/Third_Party/FreeRTOS/Source/include" -I"D:/stm32 Workspace/laske/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS"  -Og -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


