################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../tests/TestRunner.cpp \
../tests/WorkerInterfaceMock.cpp \
../tests/test_ConnectionContext.cpp \
../tests/test_RSD.cpp \
../tests/test_Registration.cpp 

OBJS += \
./tests/TestRunner.o \
./tests/WorkerInterfaceMock.o \
./tests/test_ConnectionContext.o \
./tests/test_RSD.o \
./tests/test_Registration.o 

CPP_DEPS += \
./tests/TestRunner.d \
./tests/WorkerInterfaceMock.d \
./tests/test_ConnectionContext.d \
./tests/test_RSD.d \
./tests/test_Registration.d 


# Each subdirectory must supply rules for building sources it contributes
tests/%.o: ../tests/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -g -DTESTMODE -DDEBUG -I/home/dnoack/cpputest-3.6/include/CppUTest -I/home/dnoack/cpputest-3.6/include/CppUTestExt -I/home/dnoack/libs/rapidjson/include/rapidjson -I"/home/dnoack/workspace/RSD-and-Plugin-lib/include" -O3 -Wall -c -fmessage-length=0 ${CXXFLAGS} -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


