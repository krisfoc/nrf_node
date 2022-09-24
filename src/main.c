#include <zephyr.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(CONFIG_NRF_MODEM_LIB)
#include <modem/lte_lc.h>
#include <modem/nrf_modem_lib.h>
#include <modem/at_cmd.h>
#include <modem/at_notif.h>
#include <modem/modem_info.h>
#include <nrf_modem.h>
#endif
#include <net/aws_iot.h>
#include <sys/reboot.h>
#include <date_time.h>
#include <dfu/mcuboot.h>
#include <cJSON.h>
#include <cJSON_os.h>

#include <timing/timing.h>

#include <drivers/gpio.h>
#include <device.h>

/*
#define CONFIG_AWS_IOT_BROKER_HOST_NAME "a10lrrmt9cmd0k-ats.iot.eu-north-1.amazonaws.com"
#define CONFIG_AWS_IOT_SEC_TAG "" //default
#define CONFIG_AWS_IOT_CLIENT_ID_STATIC "test1"  
set in prj.conf */

#define LED 3
const struct device *dev_led;

static int json_add_obj(cJSON *parent, const char *str, cJSON *item)
{
	cJSON_AddItemToObject(parent, str, item);

	return 0;
}


static int json_add_str(cJSON *parent, const char *str, const char *item)
{
	cJSON *json_str;

	json_str = cJSON_CreateString(item);
	if (json_str == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_str);
}

static int json_add_number(cJSON *parent, const char *str, double item)
{
	cJSON *json_num;

	json_num = cJSON_CreateNumber(item);
	if (json_num == NULL) {
		return -ENOMEM;
	}

	return json_add_obj(parent, str, json_num);
}

static char topic_name[75]="general/nb/temperature";
//static size_t size_topic=strlen(topic_name);
struct aws_iot_topic_data generalNbTemperature = {
	.type = AWS_IOT_SHADOW_TOPIC_UNKNOWN,
	.str = topic_name,
	.len= 22 //number of characters in .str
};

static char topic_name_2[75]="general/nb/metering";
struct aws_iot_topic_data metering_topic = {
	.type = AWS_IOT_SHADOW_TOPIC_UNKNOWN,
	.str = topic_name_2,
	.len= 19 //number of characters in .str
};

static char topic_name_3[75]="general/nb/ping";
struct aws_iot_topic_data ping_topic = {
	.type = AWS_IOT_SHADOW_TOPIC_UNKNOWN,
	.str = topic_name_3,
	.len= 15 //number of characters in .str
};

void run_on_startup(){
	dev_led = device_get_binding("GPIO_0");
	gpio_pin_configure(dev_led, 2, GPIO_OUTPUT);
}

void led_off()
{
	printk("Initiating demand response \n");
	dev_led = device_get_binding("GPIO_0");
	gpio_pin_configure(dev_led, 2, GPIO_OUTPUT);
	gpio_pin_set(dev_led, 2, 0);
}

void led_on()
{
	printk("Ending demand response \n");
	dev_led = device_get_binding("GPIO_0");
	gpio_pin_configure(dev_led, 2, GPIO_OUTPUT);
	gpio_pin_set(dev_led, 2, 1);
}

float meter_dataset[24]={4.02, 1.6, 8.94, 6.93, 5.53, 5.04, 
						8.81, 8.10, 4.60, 9.79, 2.04, 8.27,
						2.47, 8.77, 1.68, 5.31, 1.57, 6.04,
						7.74, 1.20, 5.30, 8.67, 3.10, 5.85};

float get_metering_data(int hour){
	if(hour>=0 && hour<24){
		return meter_dataset[hour];
	}
	else{
		return 0;
	}
}


void run_metering(int day, int hour){
	printk("running metering \n");
	int err;

	float metering_data;
	// get metering data
	metering_data=get_metering_data(hour);


	//put data in json object
	cJSON *root_obj = cJSON_CreateObject();
	//temperature=round(temperature);
	json_add_str(root_obj, "id", CONFIG_AWS_IOT_CLIENT_ID_STATIC);
	json_add_number(root_obj, "day", day);
	json_add_number(root_obj, "hour", hour);
	json_add_number(root_obj, "metering_data", metering_data);
	char *message;
	message=cJSON_Print(root_obj);

	//send data to aws
	if (message == NULL) {
		printk("cJSON_Print, error: returned NULL\n");
		err = -ENOMEM;
	}
	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic=metering_topic,
		.ptr = message,
		.len = strlen(message)
	};


	printk("Publishing: %s to AWS IoT broker\n", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}
	cJSON_FreeString(message);
	cJSON_Delete(root_obj);
}

void run_metering_2(){
	printk("running metering \n");
	int err;
	int day=2;
	float metering_data;
	// get metering data


	//put data in json object
	cJSON *root_obj = cJSON_CreateObject();
	//temperature=round(temperature);
	json_add_str(root_obj, "id", CONFIG_AWS_IOT_CLIENT_ID_STATIC);
	json_add_number(root_obj, "day", day);
	json_add_str(root_obj, "data", "3.50,8.80,4.50,6.80,2.20,5.60,2.60,3.90,5.20,3.00,9.70,6.00,6.50,9.40,9.00,8.00,8.10,8.50,2.90,6.20,7.10,4.10,5.90,9.3");
	char *message;
	message=cJSON_Print(root_obj);

	//send data to aws
	if (message == NULL) {
		printk("cJSON_Print, error: returned NULL\n");
		err = -ENOMEM;
	}
	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic=metering_topic,
		.ptr = message,
		.len = strlen(message)
	};


	printk("Publishing: %s to AWS IoT broker\n", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}
	cJSON_FreeString(message);
	cJSON_Delete(root_obj);
}

void ping_response(){
	int err;

	cJSON *root_obj = cJSON_CreateObject();
	json_add_str(root_obj, "id", CONFIG_AWS_IOT_CLIENT_ID_STATIC);
	json_add_number(root_obj, "request_id", 22);

	char *message;
	message=cJSON_Print(root_obj);
	//send data to aws
	if (message == NULL) {
		printk("cJSON_Print, error: returned NULL\n");
		err = -ENOMEM;
	}
	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic=ping_topic,
		.ptr = message,
		.len = strlen(message)
	};


	printk("Publishing: %s to AWS IoT broker\n", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}
	cJSON_FreeString(message);
	cJSON_Delete(root_obj);
}

static float read_voltage_sensor()
{
	int my_int=0;
	return 0.75;
}



BUILD_ASSERT(!IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT),
		"This sample does not support LTE auto-init and connect");

#define APP_TOPICS_COUNT CONFIG_AWS_IOT_APP_SUBSCRIPTION_LIST_COUNT

static struct k_work_delayable shadow_update_work;
static struct k_work_delayable connect_work;
static struct k_work shadow_update_version_work;

static bool cloud_connected;

K_SEM_DEFINE(lte_connected, 0, 1);

static int shadow_update(bool version_number_include){
	int err=aws_iot_ping();
	return err;
}


static void connect_work_fn(struct k_work *work)
{
	int err;

	if (cloud_connected) {
		return;
	}

	err = aws_iot_connect(NULL);
	if (err) {
		printk("aws_iot_connect, error: %d\n", err);
	}

	printk("Next connection retry in %d seconds\n",
	       CONFIG_CONNECTION_RETRY_TIMEOUT_SECONDS);

	k_work_schedule(&connect_work,
			K_SECONDS(CONFIG_CONNECTION_RETRY_TIMEOUT_SECONDS));
}

static void shadow_update_work_fn(struct k_work *work)
{
	int err;

	if (!cloud_connected) {
		return;
	}

	err = shadow_update(false);
	if (err) {
		printk("shadow_update, error: %d\n", err);
	}

	printk("Next data publication in %d seconds\n",
	       CONFIG_PUBLICATION_INTERVAL_SECONDS);

	k_work_schedule(&shadow_update_work,
			K_SECONDS(CONFIG_PUBLICATION_INTERVAL_SECONDS));
}

static void shadow_update_version_work_fn(struct k_work *work)
{
	int err;

	err = shadow_update(true);
	if (err) {
		printk("shadow_update, error: %d\n", err);
	}
}

static int send_temp()
{
	printk("running send_temp \n");
	int err;
	float temperature=-300;
	float voltage=0.9;

	//read voltage
	voltage=read_voltage_sensor();

	//calc temp
	temperature=(voltage-0.5)*100;

	//put data in json object
	cJSON *root_obj = cJSON_CreateObject();
	//temperature=round(temperature);
	json_add_str(root_obj, "id", CONFIG_AWS_IOT_CLIENT_ID_STATIC);
	json_add_number(root_obj, "temperature", temperature);
	char *message;
	message=cJSON_Print(root_obj);

	//send data to aws
	if (message == NULL) {
		printk("cJSON_Print, error: returned NULL\n");
		err = -ENOMEM;
		goto cleanup;
	}
	struct aws_iot_data tx_data = {
		.qos = MQTT_QOS_0_AT_MOST_ONCE,
		.topic=generalNbTemperature,
		.ptr = message,
		.len = strlen(message)
	};


	printk("Publishing: %s to AWS IoT broker\n", message);

	err = aws_iot_send(&tx_data);
	if (err) {
		printk("aws_iot_send, error: %d\n", err);
	}

	cJSON_FreeString(message);

cleanup:

	cJSON_Delete(root_obj);

	return err;
}

/*
static bool checkKey(cJSON this_json, const char *searchedKey) 
{
    cJSON *name=cJSON_GetObjectItem(this_json, searchedKey);
    if (name)
	{
		return true;
	} else {
		return false;
	}
	
} */

static int nb_request_handler(const char *buf, const char *topic, size_t topic_len)
{
	char *str = NULL;
	const cJSON *root_obj = NULL;

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		printk("cJSON Parse failure");
		return 0;
	}

	str = cJSON_Print(root_obj);
	if (str == NULL) {
		printk("Failed to print JSON object, JSON object = null");
		goto clean_exit;
	}
	
	const cJSON *rec_action = NULL;
	const cJSON *rec_data = NULL;
	rec_action=cJSON_GetObjectItemCaseSensitive(root_obj, "action");
	if (!cJSON_IsNumber(rec_action))
	{
		printk("nothing \n");
		return 1;
	}
	if (rec_action->valuedouble==1)
	{
		printk("action 1 \n");
		send_temp();
	}
	else if (rec_action->valuedouble==2)
	{
		printk("action 2 \n");
		led_off();
	}
	else if (rec_action->valuedouble==3)
	{
		printk("action 3 \n");
		led_on();
	}
	else if (rec_action->valuedouble==4)
	{
		printk("action 4 \n");
		const cJSON *rec_day = NULL;
		const cJSON *rec_hour = NULL;
		rec_day=cJSON_GetObjectItemCaseSensitive(root_obj, "day");
		rec_hour=cJSON_GetObjectItemCaseSensitive(root_obj, "hour");
		if(cJSON_IsNumber(rec_day) && cJSON_IsNumber(rec_hour)){
			run_metering(rec_day->valueint, rec_hour->valueint);
		}
		else{
			printk("bad request \n");
		}
	}
	else if (rec_action->valuedouble==5)
	{
		printk("action 5 \n");
		ping_response();
	}
	else if (rec_action->valuedouble==6){
		run_metering_2();
	}
	else
	{
		printk("invalid action \n");
	}  
	

	cJSON_FreeString(str);

clean_exit:
	//cJSON_Delete(root_obj);
	return 1;
}

static void print_received_data(const char *buf, const char *topic, size_t topic_len)
{
	char *str = NULL;
	cJSON *root_obj = NULL;

	root_obj = cJSON_Parse(buf);
	if (root_obj == NULL) {
		printk("cJSON Parse failure");
		return;
	}

	str = cJSON_Print(root_obj);
	if (str == NULL) {
		printk("Failed to print JSON object");
		goto clean_exit;
	}

	printf("Data received from AWS IoT console:\nTopic: %.*s\nMessage: %s\n",
	       topic_len, topic, str);

	cJSON_FreeString(str);

clean_exit:
	cJSON_Delete(root_obj);
}

void aws_iot_event_handler(const struct aws_iot_evt *const evt)
{
	switch (evt->type) {
	case AWS_IOT_EVT_CONNECTING:
		printk("AWS_IOT_EVT_CONNECTING\n");
		break;
	case AWS_IOT_EVT_CONNECTED:
		printk("AWS_IOT_EVT_CONNECTED\n");

		cloud_connected = true;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		(void)k_work_cancel_delayable(&connect_work);

		if (evt->data.persistent_session) {
			printk("Persistent session enabled\n");
		}

#if defined(CONFIG_NRF_MODEM_LIB)
		/** Successfully connected to AWS IoT broker, mark image as
		 *  working to avoid reverting to the former image upon reboot.
		 */
		boot_write_img_confirmed();
#endif

		/** Send version number to AWS IoT broker to verify that the
		 *  FOTA update worked.
		 */
		k_work_submit(&shadow_update_version_work);

		/** Start sequential shadow data updates.
		 */
		k_work_schedule(&shadow_update_work,
				K_SECONDS(CONFIG_PUBLICATION_INTERVAL_SECONDS));

#if defined(CONFIG_NRF_MODEM_LIB)
		int err = lte_lc_psm_req(true);
		if (err) {
			printk("Requesting PSM failed, error: %d\n", err);
		}
#endif
		break;
	case AWS_IOT_EVT_READY:
		printk("AWS_IOT_EVT_READY\n");
		break;
	case AWS_IOT_EVT_DISCONNECTED:
		printk("AWS_IOT_EVT_DISCONNECTED\n");
		cloud_connected = false;
		/* This may fail if the work item is already being processed,
		 * but in such case, the next time the work handler is executed,
		 * it will exit after checking the above flag and the work will
		 * not be scheduled again.
		 */
		(void)k_work_cancel_delayable(&shadow_update_work);
		k_work_schedule(&connect_work, K_NO_WAIT);
		break;
	case AWS_IOT_EVT_DATA_RECEIVED:
		printk("AWS_IOT_EVT_DATA_RECEIVED\n");
		print_received_data(evt->data.msg.ptr, evt->data.msg.topic.str, evt->data.msg.topic.len);
		if (1)
		{
			nb_request_handler(evt->data.msg.ptr, evt->data.msg.topic.str, evt->data.msg.topic.len);
			
		}
		break;
	case AWS_IOT_EVT_FOTA_START:
		printk("AWS_IOT_EVT_FOTA_START\n");
		break;
	case AWS_IOT_EVT_FOTA_ERASE_PENDING:
		printk("AWS_IOT_EVT_FOTA_ERASE_PENDING\n");
		printk("Disconnect LTE link or reboot\n");
#if defined(CONFIG_NRF_MODEM_LIB)
		err = lte_lc_offline();
		if (err) {
			printk("Error disconnecting from LTE\n");
		}
#endif
		break;
	case AWS_IOT_EVT_FOTA_ERASE_DONE:
		printk("AWS_FOTA_EVT_ERASE_DONE\n");
		printk("Reconnecting the LTE link");
#if defined(CONFIG_NRF_MODEM_LIB)
		err = lte_lc_connect();
		if (err) {
			printk("Error connecting to LTE\n");
		}
#endif
		break;
	case AWS_IOT_EVT_FOTA_DONE:
		printk("AWS_IOT_EVT_FOTA_DONE\n");
		printk("FOTA done, rebooting device\n");
		aws_iot_disconnect();
		sys_reboot(0);
		break;
	case AWS_IOT_EVT_FOTA_DL_PROGRESS:
		printk("AWS_IOT_EVT_FOTA_DL_PROGRESS, (%d%%)",
		       evt->data.fota_progress);
	case AWS_IOT_EVT_ERROR:
		printk("AWS_IOT_EVT_ERROR, %d\n", evt->data.err);
		break;
	case AWS_IOT_EVT_FOTA_ERROR:
		printk("AWS_IOT_EVT_FOTA_ERROR");
		break;
	default:
		printk("Unknown AWS IoT event type: %d\n", evt->type);
		break;
	}
}

static void work_init(void)
{
	k_work_init_delayable(&shadow_update_work, shadow_update_work_fn);
	k_work_init_delayable(&connect_work, connect_work_fn);
	k_work_init(&shadow_update_version_work, shadow_update_version_work_fn);
}

#if defined(CONFIG_NRF_MODEM_LIB)
static void lte_handler(const struct lte_lc_evt *const evt)
{
	switch (evt->type) {
	case LTE_LC_EVT_NW_REG_STATUS:
		if ((evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_HOME) &&
		     (evt->nw_reg_status != LTE_LC_NW_REG_REGISTERED_ROAMING)) {
			break;
		}

		printk("Network registration status: %s\n",
			evt->nw_reg_status == LTE_LC_NW_REG_REGISTERED_HOME ?
			"Connected - home network" : "Connected - roaming");

		k_sem_give(&lte_connected);
		break;
	case LTE_LC_EVT_PSM_UPDATE:
		printk("PSM parameter update: TAU: %d, Active time: %d\n",
			evt->psm_cfg.tau, evt->psm_cfg.active_time);
		break;
	case LTE_LC_EVT_EDRX_UPDATE: {
		char log_buf[60];
		ssize_t len;

		len = snprintf(log_buf, sizeof(log_buf),
			       "eDRX parameter update: eDRX: %f, PTW: %f",
			       evt->edrx_cfg.edrx, evt->edrx_cfg.ptw);
		if (len > 0) {
			printk("%s\n", log_buf);
		}
		break;
	}
	case LTE_LC_EVT_RRC_UPDATE:
		printk("RRC mode: %s\n",
			evt->rrc_mode == LTE_LC_RRC_MODE_CONNECTED ?
			"Connected" : "Idle");
		break;
	case LTE_LC_EVT_CELL_UPDATE:
		printk("LTE cell changed: Cell ID: %d, Tracking area: %d\n",
			evt->cell.id, evt->cell.tac);
		break;
	default:
		break;
	}
}

static void modem_configure(void)
{
	int err;

	if (IS_ENABLED(CONFIG_LTE_AUTO_INIT_AND_CONNECT)) {
		/* Do nothing, modem is already configured and LTE connected. */
	} else {
		err = lte_lc_init_and_connect_async(lte_handler);
		if (err) {
			printk("Modem could not be configured, error: %d\n",
				err);
			return;
		}
	}
}

static void nrf_modem_lib_dfu_handler(void)
{
	int err;

	err = nrf_modem_lib_get_init_ret();

	switch (err) {
	case MODEM_DFU_RESULT_OK:
		printk("Modem update suceeded, reboot\n");
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case MODEM_DFU_RESULT_UUID_ERROR:
	case MODEM_DFU_RESULT_AUTH_ERROR:
		printk("Modem update failed, error: %d\n", err);
		printk("Modem will use old firmware\n");
		sys_reboot(SYS_REBOOT_COLD);
		break;
	case MODEM_DFU_RESULT_HARDWARE_ERROR:
	case MODEM_DFU_RESULT_INTERNAL_ERROR:
		printk("Modem update malfunction, error: %d, reboot\n", err);
		sys_reboot(SYS_REBOOT_COLD);
		break;
	default:
		break;
	}
}
#endif

static int app_topics_subscribe(void)
{
	int err;
	static char custom_topic[75] = "nb/request";
	static char custom_topic_2[75] = "my-custom-topic/example_2";

	const struct aws_iot_topic_data topics_list[APP_TOPICS_COUNT] = {
		[0].str = custom_topic,
		[0].len = strlen(custom_topic),
		[1].str = custom_topic_2,
		[1].len = strlen(custom_topic_2)
	};

	err = aws_iot_subscription_topics_add(topics_list,
					      ARRAY_SIZE(topics_list));
	if (err) {
		printk("aws_iot_subscription_topics_add, error: %d\n", err);
	}

	return err;
}

static void date_time_event_handler(const struct date_time_evt *evt)
{
	switch (evt->type) {
	case DATE_TIME_OBTAINED_MODEM:
		printk("DATE_TIME_OBTAINED_MODEM\n");
		break;
	case DATE_TIME_OBTAINED_NTP:
		printk("DATE_TIME_OBTAINED_NTP\n");
		break;
	case DATE_TIME_OBTAINED_EXT:
		printk("DATE_TIME_OBTAINED_EXT\n");
		break;
	case DATE_TIME_NOT_OBTAINED:
		printk("DATE_TIME_NOT_OBTAINED\n");
		break;
	default:
		break;
	}
}


void main(void)
{

	int err;

	printk("modified AWS IoT sample started, version: %s\n", CONFIG_APP_VERSION);

	cJSON_Init();

#if defined(CONFIG_NRF_MODEM_LIB)
	nrf_modem_lib_dfu_handler();
#endif

	err = aws_iot_init(NULL, aws_iot_event_handler);
	if (err) {
		printk("AWS IoT library could not be initialized, error: %d\n",
		       err);
	}

	/** Subscribe to customizable non-shadow specific topics
	 *  to AWS IoT backend.
	 */
	//publish_data();
	err = app_topics_subscribe();
	if (err) {
		printk("Adding application specific topics failed, error: %d\n",
			err);
	}

	work_init();
#if defined(CONFIG_NRF_MODEM_LIB)
	modem_configure();

	err = modem_info_init();
	if (err) {
		printk("Failed initializing modem info module, error: %d\n",
			err);
	}

	k_sem_take(&lte_connected, K_FOREVER);
#endif


	date_time_update_async(date_time_event_handler);
	k_work_schedule(&connect_work, K_NO_WAIT);
}
