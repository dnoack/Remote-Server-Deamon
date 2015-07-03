################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../include/mocks/ComPointMock.cpp \
../include/mocks/SocketTestHelper.cpp 

OBJS += \
./include/mocks/ComPointMock.o \
./include/mocks/SocketTestHelper.o 

CPP_DEPS += \
./include/mocks/ComPointMock.d \
./include/mocks/SocketTestHelper.d 


# Each subdirectory must supply rules for building sources it contributes
include/mocks/%.o: ../include/mocks/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -g -DTESTMODE -I/home/dave2/git/rapidjson/include/rapidjson -I"/home/dave2/git/Remote-Server-Daemon/include/mocks" -I/home/dave2/git/cpputest/include -I"/home/dave2/git/rpcUtils/include" -I/home/dave2/git/cpputest/include/CppUTest -I/home/dave2/git/cpputest/include/CppUTestExt -O0 -Wall -c -fmessage-length=0 ${CXXFLAGS} -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


