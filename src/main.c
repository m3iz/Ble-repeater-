

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#define SLEEP_TIME_MS   1

#define LED1_NODE DT_ALIAS(led1) //green
#define LED2_NODE DT_ALIAS(led2) //red
#define LED3_NODE DT_ALIAS(led3) //blue

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(LED3_NODE, gpios);



bool deviceFound = false;

static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, 0xaa, 0xfe),
	BT_DATA_BYTES(BT_DATA_SVC_DATA16,
		      0xaa, 0xfe, /* Eddystone UUID */
		      0x10, /* Eddystone-URL frame type */
		      0x00, /* Calibrated Tx power at 0m */
		      0x00, /* URL Scheme Prefix http://www. */
		      'z', 'e', 'p', 'h', 'y', 'r',
		      'p', 'r', 'o', 'j', 'e', 'c', 't',
		      0x08) /* .org */
};

/* Set Scan Response data */
static const struct bt_data sd[] = {
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN), //fixed mac-address
};

static void bt_ready(int err)
{
	size_t count = 1;

	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");
	
	/* Start advertising */
	//bt_id_create(&custom_addr, NULL); 
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	/* For connectable advertising you would use
	 * bt_le_oob_get_local().  For non-connectable non-identity
	 * advertising an non-resolvable private address is used;
	 * there is no API to retrieve that.
	 */

}

static void set_public_addr(void)
{
	uint8_t pub_addr[BT_ADDR_SIZE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x10};
	bt_ctlr_set_public_addr(pub_addr);
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
	bt_addr_le_t temp = *addr;
	bt_addr_t tempadr=temp.a;
    if(tempadr.val[5]==16)
		deviceFound = true;
	mfg_data[2]++;
	//deviceFound = true;
}


int main(void)
{


//
	int ret;

	if (!gpio_is_ready_dt(&red)) {
		return 0;
	}

	if (!gpio_is_ready_dt(&green)) {
		return 0;
	}

	if (!gpio_is_ready_dt(&blue)) {
		return 0;
	}
	ret = gpio_pin_configure_dt(&red, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&green, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&blue, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}


		
//

	set_public_addr();
	uint32_t max_period;
	uint32_t period;
	uint8_t dir = 0U;




		int err;	
		printk("Starting Beacon Demo\n");

	/* Initialize the Bluetooth Subsystem */
	
	//bt_id_create(&custom_addr, NULL); 
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0010,
		.window     = 0x0010,
	};
	err = bt_enable(NULL); //bt_ready
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		printk("Starting scanning failed (err %d)\n", err);
		return 0;
	}
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
	}
	printk("Starting Scanner/Advertiser Demo\n");
	do {
		
		k_sleep(K_MSEC(400));
		if(deviceFound){
				//ret = pwm_set_dt(&pwm_led0, period, period/2);
		//ret = gpio_pin_toggle_dt(&red);
		gpio_pin_set(red.port,15,0);
		
		if (ret < 0) {
			return 0;
		}
		/* Start advertising */
		err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return 0;
		}

		k_sleep(K_MSEC(400));
		
		}else {
			gpio_pin_set(red.port,15,1);
		}
		err = bt_le_adv_stop();
		deviceFound = false;
		
		if (err) {
			printk("Advertising failed to stop (err %d)\n", err);
			return 0;
		}
	} while (1);
	return 0;
	//return 0;
}
