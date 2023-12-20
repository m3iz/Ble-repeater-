/*
 * Copyright (c) 2016 Intel Corporation
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Sample app to demonstrate PWM.
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/device.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/clock_control/nrf_clock_control.h>

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

static const struct pwm_dt_spec pwm_led0 = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

#define MIN_PERIOD PWM_SEC(1U) / 128U
#define MAX_PERIOD PWM_SEC(1U)

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
	set_public_addr();
	uint32_t max_period;
	uint32_t period;
	uint8_t dir = 0U;
	int ret;

	printk("PWM-based blinky\n");

	if (!pwm_is_ready_dt(&pwm_led0)) {
		printk("Error: PWM device %s is not ready\n",
		       pwm_led0.dev->name);
		return 0;
	}

	/*
	 * In case the default MAX_PERIOD value cannot be set for
	 * some PWM hardware, decrease its value until it can.
	 *
	 * Keep its value at least MIN_PERIOD * 4 to make sure
	 * the sample changes frequency at least once.
	 */
	printk("Calibrating for channel %d...\n", PWM0_CH_NUM);
	max_period = MAX_PERIOD;
	while (pwm_set_dt(&pwm_led0, max_period, max_period / 2U)) {
		max_period /= 2U;
		if (max_period < (4U * MIN_PERIOD)) {
			printk("Error: PWM device "
			       "does not support a period at least %lu\n",
			       4U * MIN_PERIOD);
			return 0;
		}
	}

	printk("Done calibrating; maximum/minimum periods %u/%lu nsec\n",
	       max_period, MIN_PERIOD);

	period = max_period;
	
		ret = pwm_set_dt(&pwm_led0, period, period / 2U);
		if (ret) {
			printk("Error %d: failed to set pulse width\n", ret);
			return 0;
		}

		period = dir ? (period * 2U) : (period / 2U);
		if (period > max_period) {
			period = max_period / 2U;
			dir = 0U;
		} else if (period < MIN_PERIOD) {
			period = MIN_PERIOD * 2U;
			dir = 1U;
		}

		k_sleep(K_SECONDS(4U));

		int err;	
		printk("Starting Beacon Demo\n");

	/* Initialize the Bluetooth Subsystem */
	
	//bt_id_create(&custom_addr, NULL); 
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
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
		/* Start advertising */
		err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
		if (err) {
			printk("Advertising failed to start (err %d)\n", err);
			return 0;
		}

		k_sleep(K_MSEC(400));
		}
		err = bt_le_adv_stop();
		if (err) {
			printk("Advertising failed to stop (err %d)\n", err);
			return 0;
		}
	} while (1);
	return 0;
	//return 0;
}
