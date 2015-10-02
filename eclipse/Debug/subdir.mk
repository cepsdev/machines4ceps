################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
/home/tprerovs/projects/statemachines/src/main.cpp 

OBJS += \
./main.o 

CPP_DEPS += \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
main.o: /home/tprerovs/projects/statemachines/src/main.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -I/opt/ros/groovy/include -I/home/tprerovs/projects/statemachines -I/home/tprerovs/projects/ceps/core/include -I/home/tprerovs/projects/ceps/core/include/include_gen -I/home/tprerovs/projects/statemachines/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


