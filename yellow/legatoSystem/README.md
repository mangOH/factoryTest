# YellowTest
1. Copy ```YellowTest``` and ```YellowTestService``` to ```apps``` directory
2. Copy ```iot_test_card``` to ```linux_kernel_modules``` directory
3. In ```linux_kernel_modules\mangoh\mangoh_yellow_dev.mdef``` update ```allow_eeprom_write = "1"``` to make eeprom writable
4. In ```yellow.sdef``` add to ```apps```:   
    $CURDIR/apps/YellowTestService/YellowTestService   
    $CURDIR/apps/YellowTest/YellowTest   
   In ```yellow.sdef``` add to ```kernelModules```:   
    $CURDIR/linux_kernel_modules/iot_test_card/ads1015   
    $CURDIR/linux_kernel_modules/iot_test_card/iot_test_card   
    
4. Build system file by: ```make yellow_wp76xx```
5. Copy update file to your test program directory then run.
