################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/main.cpp \
../src/mongoose.cpp 

OBJS += \
./src/main.o \
./src/mongoose.o 

CPP_DEPS += \
./src/main.d \
./src/mongoose.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: Cross G++ Compiler'
	aarch64-fslc-linux-g++ -fpermissive -DIMX8_SOM -I/home/hk/sdkinstall/sysroots/cortexa53-crypto-fslc-linux/usr/include -I/home/hk/sdkinstall/sysroots/cortexa53-crypto-fslc-linux/usr/lib/glib-2.0/include -I/home/hk/sdkinstall/sysroots/cortexa53-crypto-fslc-linux/usr/include/libnm -I/home/hk/sdkinstall/sysroots/cortexa53-crypto-fslc-linux/usr/include/glib-2.0 -I"/home/hk/eclipse-workspace/32_IMX8_Server_arm64_test/include" -O0 -g3 -Wall -c -fmessage-length=0 --sysroot=/home/hk/sdkinstall/sysroots/cortexa53-crypto-fslc-linux -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


