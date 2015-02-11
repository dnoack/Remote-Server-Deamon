################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/JsonRPC.cpp \
../src/RSD.cpp \
../src/TcpWorker.cpp \
../src/UdsClient.cpp \
../src/UdsWorker.cpp 

OBJS += \
./src/JsonRPC.o \
./src/RSD.o \
./src/TcpWorker.o \
./src/UdsClient.o \
./src/UdsWorker.o 

CPP_DEPS += \
./src/JsonRPC.d \
./src/RSD.d \
./src/TcpWorker.d \
./src/UdsClient.d \
./src/UdsWorker.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/home/dnoack/libs/zeromq-4.0.5/include -I/home/dnoack/cpputest/include/CppUTest -I/home/dnoack/cpputest/include/CppUTestExt -I"/home/dnoack/workspace/Remote-Server-Daemon/include" -I/home/dnoack/libs/rapidjson/include/rapidjson -I/home/dnoack/libs/aardvark-api-linux-i686-v5.15/c -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


