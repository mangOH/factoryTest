#include "legato.h"
#include "interfaces.h"
#include "i2c-utils.h"
#include "fileUtils.h"
#include "uart.h"

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
    const char * msg = "The quick brown Fox jumps over the lazy Dog 0123456789\n";
    char buffer[64];
    memset(buffer, 0, 64);
    int fd = 0;

    fd = serial_open("/dev/ttyHS0");
    if (fd < 0) {
        fprintf(stderr, "Failed to open serial port '/dev/ttyHS0'\n");
        exit(EXIT_FAILURE);
    }

    serial_write(fd, msg, strlen(msg));
    serial_wait_for_data(3000);
    serial_read(fd, buffer, 64);
    LE_INFO("Transfer msg: '%s'", msg);
    LE_INFO("Received msg: '%s'", buffer);
    if (strncmp(buffer, msg, strlen(msg)) == 0) {
        return LE_OK;
    } else {
        LE_INFO("Failed to check UART data loopback");
        return LE_FAULT;
    }
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
 * Check: Read ADC on IOT Test Card.
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
 * Check: read ADCs on the IoT Test Board using I2C
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_IoTCardReadADCs
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
 * Check: Read ADC on IOT Test Card using ADC.
 *
 */
//--------------------------------------------------------------------------------------------------
le_result_t yellow_test_Adc3Read
(
    int32_t* value
)
{
    le_result_t result;

    result = le_adc_ReadValue("EXT_ADC3", value);

    if (result == LE_FAULT)
    {
        LE_INFO("Couldn't get ADC value");
    }

    return result;
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