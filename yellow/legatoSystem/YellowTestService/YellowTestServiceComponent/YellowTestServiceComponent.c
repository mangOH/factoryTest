#include "legato.h"
#include "interfaces.h"
#include "i2c-utils.h"
#include "fileUtils.h"

#define I2C_HUB_MAIN_BUS    0x00
#define I2C_HUB_PORT_3      0x08
#define I2C_HUB_PORT_IOT    0x01
#define I2C_HUB_PORT_2      0x04
#define I2C_HUB_PORT_1      0x02
#define I2C_HUB_PORT_ALL    0x0F
#define BMI160_I2C_ADDR     0x68

#define LEN(x)  (sizeof(x) / sizeof(x[0]))

char i2c_bus[256] = "/dev/i2c-4";

//--------------------------------------------------------------------------------------------------
/**
 * Configure I2C hub to enable port I2C .
 *
 */
//--------------------------------------------------------------------------------------------------
int i2c_hub_select_port(uint8_t hub_address, uint8_t port)
{
    int result = 0;
    int i2c_fd = open(i2c_bus, O_RDWR);
    if (i2c_fd < 0) {
        LE_ERROR("i2cSendByte: failed to open %s", i2c_bus);
    }
    if (ioctl(i2c_fd, I2C_SLAVE_FORCE, hub_address) < 0) {
        LE_ERROR("Could not set address to 0x%02x: %s\n",
             hub_address,
             strerror(errno));
        close(i2c_fd);
        return -1;
    }
    const int writeResult = i2c_smbus_write_byte(i2c_fd, port);
    if (writeResult < 0) {
        LE_ERROR("smbus write failed with error %d\n", writeResult);
        result = -1;
    } else {
        result = 0;
    }
    close(i2c_fd);
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check: SIM State.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_CheckSimState(void)
{
    le_sim_States_t state;
    le_result_t res = le_sim_SelectCard(LE_SIM_EXTERNAL_SLOT_1);
    LE_ASSERT(res == LE_OK);

    state = le_sim_GetState(LE_SIM_EXTERNAL_SLOT_1);
    if (LE_SIM_READY == state)
    {
        return LE_OK;
    }
    else
    {
        return LE_FAULT;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Check: signal quality.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_MeasureSignalStrength
(
    uint32_t* qual
)
{
    le_result_t res;
    res = le_mrc_GetSignalQual(qual);
    return res;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check: Check Yellow UART LoopBack.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_UARTLoopBack(void)
{
    le_result_t result = LE_OK;
    const char* portFile = "/dev/ttyHS0";
    const char* msg = "The quick brown Fox jumps over the lazy Dog 0123456789\n";
    const size_t msgLen = strlen(msg);
    char buffer[64];
    memset(buffer, 0, 64);
    int fd = 0;

    // Open the UART port device file.
    fd = open("/dev/ttyHS0", O_RDWR);
    if (fd < 0) {
        LE_CRIT("Failed to open serial port '%s': %m", portFile);
        return LE_NOT_FOUND;
    }

    // Write the message to it.
    int res = write(fd, msg, msgLen);
    if (res != msgLen)
    {
        if (res == -1)
        {
            LE_CRIT("Error writing to '%s': %m", portFile);
            result = LE_IO_ERROR;
            goto done;
        }
    }

    // Wait for bytes to arrive on the port.
    struct pollfd pollStruct = { .fd = fd, .events = POLLIN, .revents = 0 };
    res = poll(&pollStruct, 1, 2000 /* ms */);
    if (res == -1)
    {
        LE_CRIT("Error returned by poll(): %m");
        result = LE_IO_ERROR;
        goto done;
    }
    if (res == 0)
    {
        LE_CRIT("Timeout waiting to receive.");
        result = LE_TIMEOUT;
        goto done;
    }
    if (res != 1)
    {
        LE_CRIT("Unexpected return value (%d) from poll().", res);
        result = LE_OUT_OF_RANGE;
        goto done;
    }
    if (pollStruct.revents != POLLIN)
    {
        LE_CRIT("Unexpected event code (%d) returned by poll().", pollStruct.revents);
        result = LE_OUT_OF_RANGE;
        goto done;
    }

    // Sleep to ensure that there has been time to receive all the bytes.
    sleep(1);

    // Read the bytes received.
    res = read(fd, buffer, sizeof(buffer));
    if (res == -1)
    {
        LE_CRIT("Error returned by read(): %m");
        result = LE_IO_ERROR;
        goto done;
    }

    // Make sure no bytes were lost.
    if (res != msgLen)
    {
        LE_CRIT("Unexpected number of bytes read: %d (expected %zu)", res, msgLen);

        // Null-terminate the received string.
        if (res == sizeof(buffer))
        {
            res -= 1;
        }
        buffer[res] = '\0';

        LE_CRIT("%s", buffer);

        result = LE_IO_ERROR;
        goto done;
    }

    // Make sure not bytes were corrupted.
    if (strncmp(buffer, msg, msgLen) != 0)
    {
        LE_CRIT("Loop-back message received does not match message sent.");
        LE_CRIT("Sent:  '%s'", msg);

        // Null-terminate the received string before logging it.
        buffer[res] = '\0';

        LE_CRIT("Recvd: '%s'", buffer);

        result = LE_IO_ERROR;
        goto done;
    }

done:
    close(fd);
    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check: Read compass heading.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_AcceGyroRead
(
    uint8_t reg,
    uint8_t* data
)
{
    int i2c_fd = open(i2c_bus, O_RDWR);
    if (i2c_fd < 0) {
        LE_ERROR("i2cSendByte: failed to open %s", i2c_bus);
    }
    if (ioctl(i2c_fd, I2C_SLAVE_FORCE, BMI160_I2C_ADDR) < 0) {
        LE_ERROR("Could not set address to 0x%02x: %s\n",
               BMI160_I2C_ADDR,
               strerror(errno));
        return LE_FAULT;
    }
    *data = i2c_smbus_read_byte_data(i2c_fd, reg);
    close(i2c_fd);
    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check: Read ADC connected to battery power.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_BatteryVoltage
(
    int32_t* value
)
{
    le_result_t result;

    result = le_adc_ReadValue("EXT_ADC1", value);

    if (result == LE_FAULT)
    {
        LE_INFO("Couldn't get ADC value");
    }
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read IoT test card power supply ADCs via I2C
 *
 * @return LE_OK if success, otherwise LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_ReadIoTCardPowerViaI2C
(
    int32_t* adc1,
    int32_t* adc2,
    int32_t* adc3,
    int32_t* adc4
)
{
    le_result_t result;

    result = file_ReadInt("/sys/devices/78b8000.i2c/i2c-4/i2c-5/5-0048/in4_input", adc1);

    if (result == LE_FAULT)
    {
        LE_INFO("Couldn't get ADC1 value by I2C");
        goto done;
    }

    result = file_ReadInt("/sys/devices/78b8000.i2c/i2c-4/i2c-5/5-0048/in5_input", adc2);

    if (result == LE_FAULT)
    {
        LE_INFO("Couldn't get ADC1 value by I2C");
        goto done;
    }

    result = file_ReadInt("/sys/devices/78b8000.i2c/i2c-4/i2c-5/5-0048/in6_input", adc3);

    if (result == LE_FAULT)
    {
        LE_INFO("Couldn't get ADC1 value by I2C");
        goto done;
    }

    result = file_ReadInt("/sys/devices/78b8000.i2c/i2c-4/i2c-5/5-0048/in7_input", adc4);

    if (result == LE_FAULT)
    {
        LE_INFO("Couldn't get ADC1 value by I2C");
        goto done;
    }

done:
    return result;
}

//--------------------------------------------------------------------------------------------------
/**
 * Read the IoT card slot ADC.
 *
 * @return LE_OK if success, otherwise LE_FAULT
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_ReadIoTCardSlotADC
(
    const char* boardRev, ///< String containing the board revision.
    int32_t* value
)
{
    const char* adcId = "EXT_ADC0";

    // DV3 boards use ADC3 for this.
    if (strcmp(boardRev, "DV3") == 0)
    {
        adcId = "EXT_ADC3";
    }

    le_result_t result = le_adc_ReadValue(adcId, value);

    if (result != LE_OK)
    {
        LE_CRIT("Couldn't get ADC value (%s)", LE_RESULT_TXT(result));
        return LE_FAULT;
    }

    return LE_OK;
}

//--------------------------------------------------------------------------------------------------
/**
 * Check:assert IoT card reset and read it back via GPIO3
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_IoTCardReset
(
    void
)
{
    le_result_t result;

    //assert IoT card reset
    result = le_gpioPin2_SetPushPullOutput(LE_GPIOPIN2_ACTIVE_LOW, true);
    if(result != LE_OK)
    {
        LE_INFO("Couldn't SetPushPullOutput for IoT Card Reset");
    }

    //Read back IOT Card Status
    int state = le_gpioPin7_Read();

    if(state == 0)
    {
        LE_INFO("Reset is detected");
    }

    result = le_gpioPin2_SetPushPullOutput(LE_GPIOPIN2_ACTIVE_HIGH, true);
    if(result != LE_OK)
    {
        LE_INFO("Couldn't SetPushPullOutput for IoT Card Reset");
    }

    state = le_gpioPin7_Read();

    if(state == 0)
    {
        LE_INFO("Reset is detected");
        return LE_FAULT;
    }

    return result;
}




//--------------------------------------------------------------------------------------------------
/**
 * Check: Read SPI EEPROM on IOT Test Card.
 *
 */
//--------------------------------------------------------------------------------------------------
// le_result_t yellow_test_SPIEeprom
// (
//     void
// )
// {
//     le_result_t result;
//     uint8_t value = 0;


//     result = eeprom_init("spidev1.0", 1000000);
//     if (result == LE_FAULT)
//     {
//         LE_ERROR("Couldn't Init eeprom");
//         return LE_FAULT;
//     }


//     uint8_t writeData[] = "123456789";
//     result = eeprom_write(0x0000, writeData, 1);
//     if (result == LE_FAULT)
//     {
//         LE_ERROR("Couldn't write eeprom");
//         return LE_FAULT;
//     }


//     result = eeprom_read(0x0000, &value, 1);
//     if (result == LE_FAULT)
//     {
//         LE_ERROR("Couldn't read eeprom");
//         return LE_FAULT;
//     }
//     else
//     {
//         LE_INFO("EEPROM read: %c", value);
//     }

//     eeprom_deinit();

//     return result;
// }



COMPONENT_INIT
{
    LE_INFO("YellowTest Service Start.");
}
