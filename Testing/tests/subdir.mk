################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../tests/TestRunner.cpp \
../tests/test_ConnectionContext.cpp \
../tests/test_RSD.cpp \
../tests/test_RegServer.cpp \
../tests/test_Registration.cpp 

OBJS += \
./tests/TestRunner.o \
./tests/test_ConnectionContext.o \
./tests/test_RSD.o \
./tests/test_RegServer.o \
./tests/test_Registration.o 

CPP_DEPS += \
./tests/TestRunner.d \
./tests/test_ConnectionContext.d \
./tests/test_RSD.d \
./tests/test_RegServer.d \
./tests/test_Registration.d 


# Each subdirectory must supply rules for building sources it contributes
tests/%.o: ../tests/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -g -DTESTMODE -I/home/dave2/git/rapidjson/include/rapidjson -I"/home/dave2/git/Remote-Server-Deamon/include/mocks" -I/home/dave2/git/cpputest/include -I"/home/dave2/git/rpcUtils/include" -I/home/dave2/git/cpputest/include/CppUTest -I/home/dave2/git/cpputest/include/CppUTestExt -O0 -Wall -c -fmessage-length=0 ${CXXFLAGS} -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


