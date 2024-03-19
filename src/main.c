

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>


#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>


#define SLEEP_TIME_MS   1
#define TABLE_SIZE 100

#define LED1_NODE DT_ALIAS(led1) //green
#define LED2_NODE DT_ALIAS(led2) //red
#define LED3_NODE DT_ALIAS(led3) //blue

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)
#define RVVAL -60

static const struct gpio_dt_spec green = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static const struct gpio_dt_spec red = GPIO_DT_SPEC_GET(LED2_NODE, gpios);
static const struct gpio_dt_spec blue = GPIO_DT_SPEC_GET(LED3_NODE, gpios);

bool deviceFound = false;
int deviceCounter = 0;
int maxRSSi=-100;
typedef struct node {
    char mac[18]; // MAC-адрес в формате "XX:XX:XX:XX:XX:XX"
    int rssi;
    struct node* next;
} node_t;

node_t* hash_table[TABLE_SIZE];

unsigned int hash(char* mac) {
    unsigned int hash_value = 0;
	int mac_len =  strlen(mac);
    for (int i = 0; i < mac_len; i++) {
        hash_value = hash_value * 37 + mac[i];
    }
    return hash_value % TABLE_SIZE;
}

void insert(char* mac, int rssi) {
    unsigned int index = hash(mac);
    node_t* new_node = (node_t*)malloc(sizeof(node_t));
    strcpy(new_node->mac, mac);
    new_node->rssi = rssi;
    new_node->next = hash_table[index];
    hash_table[index] = new_node;
}

node_t* search(char* mac) {
    unsigned int index = hash(mac);
    node_t* current = hash_table[index];
    while (current != NULL) {
        if (strcmp(current->mac, mac) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void delete_node(char* mac) {
    unsigned int index = hash(mac);
    node_t* current = hash_table[index], * prev = NULL;
    while (current != NULL) {
        if (strcmp(current->mac, mac) == 0) {
            if (prev == NULL) {
                hash_table[index] = current->next;
            }
            else {
                prev->next = current->next;
            }
            free(current);
            return;
        }
        prev = current;
        current = current->next;
    }
}

static uint8_t mfg_data[] = { 0xff, 0xff, 0x00 };

static const struct bt_data ad[] = {	
	 BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR), //закомментить GENERAL, чтобы не было видно в списке устройств
	
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
	
    if((tempadr.val[5]==16)&&(tempadr.val[4]==0)&&(tempadr.val[3]==0)){
		char mac[1];
		//bt_addr_le_to_str(tempadr.val, mac, 6);
		mac[0]=16;
		node_t* result = search(tempadr.val[2]);
    	if (result != NULL) {
       		 printf("Найдено: %s -> RSSI: %d\n", result->mac, result->rssi);
			 result->rssi=rssi;
    	}
    	else {
        	printf("MAC-адрес не найден\n");
			insert(tempadr.val[2], rssi);
   		}
		deviceFound = true;

	}
	mfg_data[2]++;
	
}
int count_records() {
    int count = 0;
    for (int i = 0; i < TABLE_SIZE; ++i) {
        node_t* current = hash_table[i];
        while (current != NULL) {
			if(maxRSSi<current->rssi)maxRSSi=current->rssi;
			if(current->rssi>=RVVAL)
            	count++;
            current = current->next;
        }
    }
    return count;
}

void free_table() {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        node_t* current = hash_table[i];
        while (current != NULL) {
            node_t* temp = current;
            current = current->next;
            free(temp);
        }
        hash_table[i] = NULL; // Установить указатель ячейки в NULL после освобождения всех узлов
    }
}

int main(void)
{
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

	set_public_addr();
	uint32_t max_period;
	uint32_t period;
	uint8_t dir = 0U;

	int err;	

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
	int reset_counter = 0;
	do {
		reset_counter++;
		if(reset_counter>1){
			reset_counter=0;
			free_table();
			maxRSSi=-100;
		}
		k_sleep(K_MSEC(400));
		int device_count = count_records();
		if((device_count>=2)&&(maxRSSi>=RVVAL)){ //deviceFound&&
		
		gpio_pin_set(green.port,green.pin,0);
		
		if (ret < 0) {
			return 0;
		}
		/* Start advertising */
		struct bt_le_adv_param adv_param = {
    		.id = BT_ID_DEFAULT,
    		.sid = 0,
    		.secondary_max_skip = 0,
    		.options = BT_LE_ADV_OPT_USE_IDENTITY,
    		.interval_min = BT_GAP_ADV_FAST_INT_MIN_1,
    		.interval_max = BT_GAP_ADV_FAST_INT_MIN_1,
    		.peer = NULL,
		}; //BT_LE_ADV_OPT_USE_IDENTITY, BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL
		err = bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad),
			      sd, ARRAY_SIZE(sd));
			if (err) {
				printk("Advertising failed to start (err %d)\n", err);
				return 0;
			}
		k_sleep(K_MSEC(400));
		}else {
			
			gpio_pin_set(green.port,green.pin,1);
			free_table();
		}
		err = bt_le_adv_stop();
			if (err) {
			printk("Advertising failed to stop (err %d)\n", err);
			return 0;
			}
		deviceFound = false;
	} while (1);
	return 0;
}
