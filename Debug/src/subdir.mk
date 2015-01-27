################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/System.cpp 

OBJS += \
./src/System.o 

CPP_DEPS += \
./src/System.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/home/dnoack/libs/zeromq-4.0.5/include -I/home/dnoack/cpputest/include/CppUTest -I/home/dnoack/cpputest/include/CppUTestExt -I"/home/dnoack/workspace/Remote-Server-Daemon/include" -I/home/dnoack/libs/rapidjson/include/rapidjson -I/home/dnoack/libs/aardvark-api-linux-i686-v5.15/c -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


