== Building a Factory Test .update ==

To build a .update file for use in factory testing,

1. Create a leaf workspace with a profile containing the appropriate release toolchain.
    ```
    mkdir factoryTest-0.1.0
    cd !$
    leaf setup yellow-wp77xx -p mangOH-yellow-wp77xx_0.1.0
    git clone --recursive https://github.com/mangOH/mangOH
    ```
2. Clone the factory test repository
    ```
    git clone https://github.com/mangOH/factoryTest
    ```
2. In ```mangOH/linux_kernel_modules/mangoh/mangoh_yellow_dev.mdef```:
  - update ```allow_eeprom_write = "1"``` to make eeprom writable
  - if the EEPROM may not be empty on some units, uncomment ```force_revision = "1.0"```
    and set the revision to the appropriate number.
3. In ```mangOH/yellow.sdef``` add to ```apps```:
    ```
    $CURDIR/../factoryTest/yellow/legatoSystem/YellowTestService/YellowTestService
    $CURDIR/../factoryTest/yellow/legatoSystem/YellowTest/YellowTest
    ```
   In ```yellow.sdef``` remove from ```apps```:
    ```
    $CURDIR/apps/YellowSensorToCloud/light
    $CURDIR/apps/YellowSensorToCloud/button
    $CURDIR/apps/DataHub-Buzzer/buzzer
    $CURDIR/apps/YellowOnBoardActuators/leds
    $CURDIR/apps/VegasMode/vegasMode
    $CURDIR/apps/Welcome/helloYellow
    $CURDIR/samples/BluetoothSensorTag/bluetoothSensorTag
    ```
   In ```yellow.sdef``` remove from ```bindings```:
    ```
    light.dhubIO -> dataHub.io
    button.dhubIO -> dataHub.io
    button.gpio -> gpioService.le_gpioPin25
    vegasMode.dhubIO -> dataHub.io
    ```
   In ```yellow.sdef``` remove from ```commands```:
    ```
    hello = helloYellow:/bin/hello
    ```
   In ```yellow.sdef``` add to ```commands```:
    ```
    yellowtest = YellowTest:/bin/yellowtest
    ```
   In ```yellow.sdef``` add to ```kernelModules```:
    ```
    $CURDIR/../factoryTest/yellow/legatoSystem/iot_test_card/ads1015
    $CURDIR/../factoryTest/yellow/legatoSystem/iot_test_card/iot_test_card
    ```
4. Build the system ```.update``` file in the ```mangOH``` directory:
    ```
    cd mangOH
    make yellow BSEC_DIR=~/BSEC_1.4.7.2_GCC_CortexA7_20190225/algo/bin/Normal_version/Cortex_A7
    ```

The ```.update``` file to be used is now available at mangOH/build/update_files/yellow.wp77xx.update.

This should be copied to ```factoryTest/yellow/pc/system/yellow_factory_test.wp77xx.update```.
    ```
    cp build/update_files/yellow.wp77xx.update ../factoryTest/yellow/pc/system/yellow_factory_test.wp77xx.update
    ```
