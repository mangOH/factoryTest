#include "legato.h"
#include "interfaces.h"

#define BMI160_REG_CHIP_ID 0x00

int32_t adc3_value = 0;
int32_t battery_value = 0;

int32_t adc1_i2c_value = 0;
int32_t adc2_i2c_value = 0;
int32_t adc3_i2c_value = 0;
int32_t adc4_i2c_value = 0;


COMPONENT_INIT
{
    LE_INFO("Yellow Automation Test App start");
    putenv("PATH=/legato/systems/current/bin:/usr/local/bin:"
             "/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin");

    le_result_t res;
    uint32_t signal_qual = 0;
    uint8_t data = 10;
   
    //Check external SIM card state
    res = yellow_test_CheckSimState();

    if (res == LE_OK)
    {
        LE_INFO("Check SIM state: PASSED\n");
        puts("Check SIM state: PASSED");
    }
    else
    {
        LE_ERROR("Check external SIM card state FAILED");
        return;
    }

    //Measure signal strength
    res = yellow_test_MeasureSignalStrength(&signal_qual);
    if (res == LE_OK)
    {
        LE_INFO("Signal strength quality = %d\n", signal_qual); 
        puts("Signal strength quality = 123 ");
        LE_INFO("Check signal quality: PASSED\n");
        //puts("\nCheck signal quality: PASSED\n");
    }
    else
    {
        LE_ERROR("Check signal quality: FAILED\n");
        return;
    }

    // res = yellow_test_SDCard();
    // if(res == LE_OK)
    // {
    //     LE_INFO("SDCard Read/Wrire test PASSED");
    // }
    // else
    // {
    //     LE_ERROR("SDCard Read/Wrire test FAILED");
    //     return;
    // }

    res = yellow_test_BatteryVoltage(&battery_value);
    if(res == LE_OK)
    {
        LE_INFO("Battery value is: %d", battery_value);
        //puts("Battery value is: " + battery_value);
        LE_INFO("Read Battery Voltage: PASSED");
        //puts("\nRead Battery Voltage: PASSED\n");
    }
    else
    {
        LE_ERROR("Read Battery Voltage FAILED");
        return;
    }

    res = yellow_test_IoTCardReadADCs(&adc1_i2c_value, &adc2_i2c_value, &adc3_i2c_value, &adc4_i2c_value);
    if(res == LE_OK)
    {
        if(adc1_i2c_value > 900 && adc2_i2c_value > 900 && adc3_i2c_value > 900 && adc4_i2c_value > 900)
        {
            LE_INFO("ADC1, ADC2, ADC3, ADC4 is: %d, %d, %d, %d",adc1_i2c_value, adc2_i2c_value,
                                                                adc3_i2c_value, adc4_i2c_value);
            //puts("ADC1: " + adc1_i2c_value);

            LE_INFO("Check IoTCardReadADCs: PASSED");
            //puts("Read Battery Voltage: PASSED\n");
        }
        else
        {
            LE_ERROR("ADCs Value is incorrect");
        }
    }
    else
    {
        LE_ERROR("IoTCardReadADCs: FAILED");
        return;
    }

    res = yellow_test_UARTLoopBack();
    if(res == LE_OK)
    {
        LE_INFO("Check UART Loop back: PASSED");
        //puts("Check IOTCardReset: PASSED\n");
    }
    else
    {
        LE_ERROR("UARTLoopBack FAILED");
        return;
    }


    res = yellow_test_IoTCardReset();
    if(res == LE_OK)
    {
        LE_INFO("Check IOTCardReset: PASSED");
        //puts("Check IOTCardReset: PASSED\n");
    }
    else
    {
        LE_ERROR("IOTCardReset FAILED");
        return;
    }

    // res = yellow_test_SPIEeprom();
    // if(res == LE_OK)
    // {
    //     LE_INFO("SPIEepromRead/Wrire test PASSED");
    // }
    // else
    // {
    //     LE_ERROR("SPIEeprom Read/Wrire test FAILED");
    //     return;
    // }  

    //Check main bus i2c
    // res = yellow_test_MainBusI2C();
    // if (res == LE_OK)
    // {
    //     LE_INFO("Check main bus I2C: PASSED\n");
    // }
    // else
    // {
    //     LE_INFO("Check main bus I2C: FAILED\n");
    //     return;
    // }

    //Check port 1 I2C address
    // res = yellow_test_Port1HubI2C();
    // if (res == LE_OK)
    // {
    //     LE_INFO("Check port 1 hub I2C: PASSED\n");
    // }
    // else
    // {
    //     LE_INFO("Check port 1 hub I2C: FAILED\n");
    //     return;
    // }

    //Check port 2 I2C address
    // res = yellow_test_Port2HubI2C();
    // if (res == LE_OK)
    // {
    //     LE_INFO("Check port 2 hub I2C: PASSED\n");
    // }
    // else
    // {
    //     LE_INFO("Check port 2 hub I2C: FAILED\n");
    //     return;
    // }

    //Check port 3 I2C address
    // res = yellow_test_Port3HubI2C();
    // if (res == LE_OK)
    // {
    //     LE_INFO("Check port 3 hub I2C: PASSED\n");
    // }
    // else
    // {
    //     LE_INFO("Check port 3 hub I2C: FAILED\n");
    //     return;
    // }

    //Read Compass
    res = yellow_test_AcceGyroRead(BMI160_REG_CHIP_ID, &data);
    if (res == LE_OK)
    {
        LE_INFO("chip id = %x", data);
        //puts("chip id = " + data);
        LE_INFO("Read accelerometer and gyroscope connection: PASSED\n");
        //puts("Read accelerometer and gyroscope connection: PASSED\n");
    }
    else
    {
        LE_INFO("Read accelerometer and gyroscope: FAILED\n");
        return;
    }


    //Test ADC IOT Test Card
    res = yellow_test_Adc3Read(&adc3_value);
    if(res == LE_OK)
    {
        if(adc3_value < 900)
        {
            LE_ERROR("ADC3 is %d",adc3_value);
            return;
        }
        else
        {
            LE_INFO("Read ADC3: PASSED");
            //puts("Read ADC3: PASSED\n");
            LE_INFO("EXT_ADC3 value is: %d", adc3_value);
            //puts("EXT_ADC3 value is: " + adc3_value);
        }
    }
    else
    {
        LE_ERROR("Check ADC3 value FAILED");
        return;
    }
    
}
