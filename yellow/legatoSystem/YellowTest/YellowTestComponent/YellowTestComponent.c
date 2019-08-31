#include "legato.h"
#include "interfaces.h"

#define BMI160_REG_CHIP_ID 0x00

static int32_t adc3_value = 0;
static int32_t battery_value = 0;

static int32_t adc1_i2c_value = 0;
static int32_t adc2_i2c_value = 0;
static int32_t adc3_i2c_value = 0;
static int32_t adc4_i2c_value = 0;

static bool ExitStatus = EXIT_SUCCESS;


static void Pass(const char* testDescription)
{
    LE_INFO("%s: PASSED", testDescription);
    printf("%s: PASSED\n", testDescription);
}


static void Fail(const char* testDescription, le_result_t resultCode)
{
    const char* resultCodeStr = LE_RESULT_TXT(resultCode);

    LE_CRIT("%s: FAILED (%s)", testDescription, resultCodeStr);
    printf("%s: FAILED (%s)\n", testDescription, resultCodeStr);

    ExitStatus = EXIT_FAILURE;
}


static void ReportResult(const char* testDescription, le_result_t resultCode)
{
    if (resultCode == LE_OK)
    {
        Pass(testDescription);
    }
    else
    {
        Fail(testDescription, resultCode);
    }
}

COMPONENT_INIT
{
    putenv("PATH=/legato/systems/current/bin:/usr/local/bin:"
             "/usr/bin:/bin:/usr/local/sbin:/usr/sbin:/sbin");

    le_result_t res;
    uint8_t data = 10;
    const char* testDesc;

    testDesc = "Check SIM card state";
    res = yellow_test_CheckSimState();
    ReportResult(testDesc, res);

    testDesc = "Check cellular signal strength";
    uint32_t signal_qual = 0;
    res = yellow_test_MeasureSignalStrength(&signal_qual);
    if (res == LE_OK)
    {
        LE_INFO("Signal strength = %d\n", signal_qual);

        Pass(testDesc);
    }
    else
    {
        Fail(testDesc, res);
    }

    // testDesc = "SD card read/write test";
    // res = yellow_test_SDCard();
    // ReportResult(testDesc, res);

    testDesc = "SD card read/write test";
    res = yellow_test_BatteryVoltage(&battery_value);
    if (res == LE_OK)
    {
        LE_INFO("Battery value is: %d", battery_value);
        Pass(testDesc);
    }
    else
    {
        Fail(testDesc, res);
    }

    testDesc = "IoT card power and I2C";
    res = yellow_test_IoTCardReadADCs(&adc1_i2c_value, &adc2_i2c_value, &adc3_i2c_value, &adc4_i2c_value);
    if(res == LE_OK)
    {
        LE_INFO("ADC1, ADC2, ADC3, ADC4 is: %d, %d, %d, %d",adc1_i2c_value, adc2_i2c_value,
                                                            adc3_i2c_value, adc4_i2c_value);

        if (adc1_i2c_value > 900 && adc2_i2c_value > 900 && adc3_i2c_value > 900 && adc4_i2c_value > 900)
        {
            Pass(testDesc);
        }
        else
        {
            LE_ERROR("ADC readings are out of range");
            Fail(testDesc, LE_OUT_OF_RANGE);
        }
    }
    else
    {
        Fail(testDesc, res);
    }

    testDesc = "IoT card USB";
    res = yellow_test_UARTLoopBack();
    ReportResult(testDesc, res);

    testDesc = "IoT card reset";
    res = yellow_test_IoTCardReset();
    ReportResult(testDesc, res);

    // testDesc = "IoT card SPI test";
    // res = yellow_test_SPIEeprom();
    // ReportResult(testDesc, res);

    testDesc = "Accelerometer and gyro";
    res = yellow_test_AcceGyroRead(BMI160_REG_CHIP_ID, &data);
    if (res == LE_OK)
    {
        LE_INFO("chip id = %x", data);
        Pass(testDesc);
    }
    else
    {
        Fail(testDesc, res);
    }

    testDesc = "IoT slot ADC";
    res = yellow_test_Adc3Read(&adc3_value);
    if (res == LE_OK)
    {
        if (adc3_value < 900)
        {
            LE_CRIT("ADC3 is %d",adc3_value);
            Fail(testDesc, LE_OUT_OF_RANGE);
        }
        else
        {
            Pass(testDesc);
        }
    }
    else
    {
        Fail(testDesc, res);
    }

    exit(ExitStatus);
}
