################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
hal.obj: ../hal.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_16.6.0.STS/bin/cl430" -vmspx --data_model=restricted -O3 --use_hw_mpy=F5 --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01/driverlib/MSP430F5xx_6xx" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01/USB_config" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_16.6.0.STS/include" --advice:power="none" --define=__MSP430F5529__ --define=DEPRECATED -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="hal.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

main_Rev01.obj: ../main_Rev01.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_16.6.0.STS/bin/cl430" -vmspx --data_model=restricted -O3 --use_hw_mpy=F5 --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01/driverlib/MSP430F5xx_6xx" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01/USB_config" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_16.6.0.STS/include" --advice:power="none" --define=__MSP430F5529__ --define=DEPRECATED -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="main_Rev01.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

system_pre_init.obj: ../system_pre_init.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_16.6.0.STS/bin/cl430" -vmspx --data_model=restricted -O3 --use_hw_mpy=F5 --include_path="C:/ti/ccsv6/ccs_base/msp430/include" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01/driverlib/MSP430F5xx_6xx" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01" --include_path="E:/Projects/CC3200 Development/CCSworkspace_1120/C0_SimpleSend_with_CompB_Rev01/USB_config" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-msp430_16.6.0.STS/include" --advice:power="none" --define=__MSP430F5529__ --define=DEPRECATED -g --printf_support=minimal --diag_warning=225 --diag_wrap=off --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU23 --silicon_errata=CPU40 --preproc_with_compile --preproc_dependency="system_pre_init.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


