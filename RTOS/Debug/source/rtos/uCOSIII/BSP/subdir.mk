################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../source/rtos/uCOSIII/BSP/cpu_bsp.c 

OBJS += \
./source/rtos/uCOSIII/BSP/cpu_bsp.o 

C_DEPS += \
./source/rtos/uCOSIII/BSP/cpu_bsp.d 


# Each subdirectory must supply rules for building sources it contributes
source/rtos/uCOSIII/BSP/%.o: ../source/rtos/uCOSIII/BSP/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DCPU_MK64FN1M0VLL12 -D__USE_CMSIS -DDEBUG -I"C:\Users\Usuario\Documents\MCUXpressoIDE_10.2.0_759\projects\ucosiii_labo_de_micros_project\source\ucosiii_config" -I"C:\Users\Usuario\Documents\MCUXpressoIDE_10.2.0_759\projects\ucosiii_labo_de_micros_project\source\rtos\uCOSIII\src\uC-CPU\ARM-Cortex-M4\GNU" -I"C:\Users\Usuario\Documents\MCUXpressoIDE_10.2.0_759\projects\ucosiii_labo_de_micros_project\source\rtos\uCOSIII\src\uC-CPU" -I"C:\Users\Usuario\Documents\MCUXpressoIDE_10.2.0_759\projects\ucosiii_labo_de_micros_project\source\rtos\uCOSIII\src\uC-LIB" -I"C:\Users\Usuario\Documents\MCUXpressoIDE_10.2.0_759\projects\ucosiii_labo_de_micros_project\source\rtos\uCOSIII\src\uCOS-III\Ports\ARM-Cortex-M4\Generic\GNU" -I"C:\Users\Usuario\Documents\MCUXpressoIDE_10.2.0_759\projects\ucosiii_labo_de_micros_project\source\rtos\uCOSIII\src\uCOS-III\Source" -I../source -I../ -I../SDK/CMSIS -I../SDK/startup -O0 -fno-common -g3 -Wall -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


