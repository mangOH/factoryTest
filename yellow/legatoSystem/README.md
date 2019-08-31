1. Check out mangOH repository and YellowTest next to each other.
2. In ```linux_kernel_modules\mangoh\mangoh_yellow_dev.mdef``` update ```allow_eeprom_write = "1"``` to make eeprom writable
3. In ```yellow.sdef``` add to ```apps```:
    $CURDIR/../YellowTest/YellowTestService/YellowTestService
    $CURDIR/../YellowTest/YellowTest/YellowTest

   In ```yellow.sdef``` remove from ```apps```:
    $CURDIR/apps/YellowSensorToCloud/light
    $CURDIR/apps/YellowSensorToCloud/button
    $CURDIR/apps/DataHub-Buzzer/buzzer
    $CURDIR/apps/YellowOnBoardActuators/leds
    $CURDIR/apps/Welcome/helloYellow
    $CURDIR/apps/VegasMode/vegasMode

   In ```yellow.sdef``` remove from ```bindings```:
    light.dhubIO -> dataHub.io
    button.dhubIO -> dataHub.io
    button.gpio -> gpioService.le_gpioPin25
    vegasMode.dhubIO -> dataHub.io

   In ```yellow.sdef``` remove from ```commands```:
    hello = helloYellow:/bin/hello

   In ```yellow.sdef``` add to ```commands```:
    yellowtest = YellowTest:/bin/yellowtest

   In ```yellow.sdef``` add to ```kernelModules```:
    $CURDIR/../YellowTest/iot_test_card/ads1015
    $CURDIR/../YellowTest/iot_test_card/iot_test_card

4. Build system .update file: ```make yellow_wp76xx```
5. Copy .update file to the YellowTest_Shell/system directory.
