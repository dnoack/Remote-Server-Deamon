################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/ConnectionContext.cpp \
../src/RSD.cpp \
../src/RegServer.cpp \
../src/Registration.cpp 

OBJS += \
./src/ConnectionContext.o \
./src/RSD.o \
./src/RegServer.o \
./src/Registration.o 

CPP_DEPS += \
./src/ConnectionContext.d \
./src/RSD.d \
./src/RegServer.d \
./src/Registration.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -g -DTESTMODE -I/home/dave2/git/rapidjson/include/rapidjson -I"/home/dave2/git/Remote-Server-Deamon/include/mocks" -I/home/dave2/git/cpputest/include -I"/home/dave2/git/rpcUtils/include" -I/home/dave2/git/cpputest/include/CppUTest -I/home/dave2/git/cpputest/include/CppUTestExt -O0 -Wall -c -fmessage-length=0 ${CXXFLAGS} -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


