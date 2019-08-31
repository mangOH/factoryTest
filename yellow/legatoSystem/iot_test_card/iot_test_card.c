#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/gpio/driver.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>

#include <linux/i2c/ads1015.h>
#include <linux/eeprom_93cx6.h>
#include <linux/platform_data/at24.h>

#include "../mangoh/mangoh_common.h"

/*
 *-----------------------------------------------------------------------------
 * Constants
 *-----------------------------------------------------------------------------
 */
#define IOT_TEST_CARD_I2C_SW_BUS_BASE	(PRIMARY_I2C_BUS + 1)
#define IOT_TEST_CARD_I2C_BUS_PORT0	(IOT_TEST_CARD_I2C_SW_BUS_BASE + 0)
#define IOT_TEST_CARD_I2C_BUS_PORT1 	(IOT_TEST_CARD_I2C_SW_BUS_BASE + 1)
#define IOT_TEST_CARD_I2C_BUS_PORT2	(IOT_TEST_CARD_I2C_SW_BUS_BASE + 2)
#define IOT_TEST_CARD_I2C_BUS_PORT3	(IOT_TEST_CARD_I2C_SW_BUS_BASE + 3)

/*
 *-----------------------------------------------------------------------------
 * Static Function Declarations
 *-----------------------------------------------------------------------------
 */
static void iot_test_card_release(struct device* dev);
static int iot_test_card_probe(struct platform_device* pdev);
static int iot_test_card_remove(struct platform_device* pdev);
static void eeprom_setup(struct memory_accessor *mem_acc, void *context);

static struct platform_driver iot_test_card_driver = {
	.probe = iot_test_card_probe,
	.remove = iot_test_card_remove,
	.driver  = {
		.name = "iot test card",
		.owner = THIS_MODULE,
		.bus = &platform_bus_type,
	},
};

static struct ads1015_platform_data iot_test_card_adc_data = {
	.channel_data = {
		[0] = { .enabled = true, .pga = 1, .data_rate = 0 },
		[1] = { .enabled = true, .pga = 1, .data_rate = 0 },
		[2] = { .enabled = true, .pga = 1, .data_rate = 0 },
		[3] = { .enabled = true, .pga = 1, .data_rate = 0 },
		[4] = { .enabled = true, .pga = 1, .data_rate = 0 },
		[5] = { .enabled = true, .pga = 1, .data_rate = 0 },
		[6] = { .enabled = true, .pga = 1, .data_rate = 0 },
		[7] = { .enabled = true, .pga = 1, .data_rate = 0 },
	}
};
static struct i2c_board_info iot_test_card_adc_info = {
	I2C_BOARD_INFO("ads1115", 0x48),
	.platform_data = &iot_test_card_adc_data,
};

static struct iot_test_card_driver_data {
	struct i2c_client* adc;
	struct i2c_client* eeprom;
	struct spi_device* spi_eeprom;

} iot_test_card_driver_data = {
};

static struct platform_device iot_test_card_device = {
	.name = "iot test card",
	.id = -1,
	.dev = {
		.release = iot_test_card_release,
	},
};

static void iot_test_card_release(struct device* dev)
{
	/* Nothing alloc'd, so nothign to free */
}

/*
 * The EEPROM is marked as read-only to prevent accidental writes. The mangOH
 * Yellow has the write protect (WP) pin pulled high which has the effect of
 * making the upper 1/4 of the address space of the EEPROM write protected by
 * hardware.
 */
static struct at24_platform_data iot_test_card_eeprom_data = {
	.byte_len = 4096,
	.page_size = 32,
	.flags = (AT24_FLAG_ADDR16),
	.setup = eeprom_setup,
};
static struct i2c_board_info iot_test_card_eeprom_info = {
	I2C_BOARD_INFO("at24", 0x53),
	.platform_data = &iot_test_card_eeprom_data,
};

static void eeprom_setup(struct memory_accessor *mem_acc, void *context)
{
	const size_t eeprom_size = iot_test_card_eeprom_data.byte_len;
	u8 *data;

	data = kmalloc(eeprom_size, GFP_KERNEL);
	if (data == NULL) {
		pr_err("Couldn't allocate memory for EEPROM data");
		return;
	}

	if (mem_acc->read(mem_acc, data, 0, eeprom_size) != eeprom_size) {
		pr_err("Error reading from board identification EEPROM.\n");
		goto done;
	}

	printk("EEPROM content: %s\n", data);

done:
	kfree(data);
}

static int iot_test_card_probe(struct platform_device* pdev)
{
	int ret = 0;
	struct i2c_adapter *i2c_adapter_port0 = NULL;

	dev_info(&pdev->dev, "%s(): probe\n", __func__);

	i2c_adapter_port0 = i2c_get_adapter(IOT_TEST_CARD_I2C_BUS_PORT0);
	if (!i2c_adapter_port0) {
		dev_err(&pdev->dev,
			"Couldn't get IoT card I2C buses\n");
		ret = -ENODEV;
		goto done;
	}

	/*
	 * This is a workaround that needs to be tested more for issue first
	 * seen on mangOH Green
	 */
	msleep(5000);

	platform_set_drvdata(pdev, &iot_test_card_driver_data);

	/* map the EEPROM */
	dev_dbg(&pdev->dev, "mapping eeprom\n");
	// if (!allow_eeprom_write)
	// 	iot_test_card_eeprom_data.flags |= AT24_FLAG_READONLY;
	iot_test_card_driver_data.eeprom =
		i2c_new_device(i2c_adapter_port0, &iot_test_card_eeprom_info);
	// if (!iot_test_card_driver_data.eeprom) {
	// 	if (iot_test_card_driver_data.board_rev == BOARD_REV_UNKNOWN) {
	// 		/*
	// 		 * Assume board is a DV2 since the EEPROM was missing on
	// 		 * that revision
	// 		 */
	// 		iot_test_card_driver_data.board_rev = BOARD_REV_DV2;
	// 	} else if (iot_test_card_driver_data.board_rev !=
	// 		   BOARD_REV_DV2) {
	// 		/*
	// 		 * If the revision was forced to something other than
	// 		 * DV2, the EEPROM is expected to be present.
	// 		 */
	// 		dev_err(&pdev->dev, "Failed to register %s\n",
	// 			iot_test_card_eeprom_info.type);
	// 		ret = -ENODEV;
	// 		goto cleanup;
	// 	}
	// }

	/* map the adc */
	dev_dbg(&pdev->dev, "mapping i2c adc\n");
	iot_test_card_driver_data.adc =
		i2c_new_device(i2c_adapter_port0, &iot_test_card_adc_info);
	if (!iot_test_card_driver_data.adc) {
		dev_err(&pdev->dev, "Failed to register i2c adc\n");
		ret = -ENODEV;
		goto cleanup;
	}

cleanup:
	i2c_put_adapter(i2c_adapter_port0);
	if (ret != 0)
		iot_test_card_remove(pdev);
done:
	return ret;
}

static void safe_i2c_unregister_device(struct i2c_client *client)
{
	if (client)
		i2c_unregister_device(client);
}

static int iot_test_card_remove(struct platform_device* pdev)
{
	struct iot_test_card_driver_data *dd = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "Removing iot test card devices\n");

	safe_i2c_unregister_device(dd->adc);
	safe_i2c_unregister_device(dd->eeprom);
	return 0;
}

static int __init iot_test_card_init(void)
{
	platform_driver_register(&iot_test_card_driver);
	printk(KERN_DEBUG "mangoh: registered platform driver\n");

	if (platform_device_register(&iot_test_card_device)) {
		platform_driver_unregister(&iot_test_card_driver);
		return -ENODEV;
	}

	return 0;
}

static void __exit iot_test_card_exit(void)
{
	platform_device_unregister(&iot_test_card_device);
	platform_driver_unregister(&iot_test_card_driver);
}

module_init(iot_test_card_init);
module_exit(iot_test_card_exit);

MODULE_ALIAS(PLATFORM_MODULE_PREFIX "iot test card");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Sierra Wireless");
MODULE_DESCRIPTION("For factory hardware testing purposes");
MODULE_VERSION("1.0");
