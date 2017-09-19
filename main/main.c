/* i2c - Example

   For other examples please check:
https://github.com/espressif/esp-idf/tree/master/examples

This example code is in the Public Domain (or CC0 licensed, at your option.)

Unless required by applicable law or agreed to in writing, this
software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>

#include "sdkconfig.h"

#include "driver/i2c.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"


#include "i2c_routines.h"
#include "htu21d.h"
#include "MQTTClient.h"
#include "rmt_europace_fan.h"
#include "rmt_hvac_mitsubishi.h"

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
   */

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;
const int MQTT_READY_BIT = BIT0;

#define DELAY_TIME_BETWEEN_ITEMS_MS   10000 /*!< delay time between different test items */

#define MCP23017_ADDR   0x20

/* Constants that aren't configurable in menuconfig */
#define MQTT_SERVER "mqtt.iotcebu.com"
#define MQTT_USER "vergil"
#define MQTT_PASS "ab12cd34"
#define MQTT_PORT 8083
#define MQTT_CLIENTID "esp32"
#define MQTT_WEBSOCKET 1  // 0=no 1=yes

#define MQTT_TOPIC_LED "iotcebu/vergil/pwm/#"
#define MQTT_TOPIC_SWITCH "iotcebu/vergil/switch/#"
#define MQTT_TOPIC_FAN "iotcebu/vergil/fan/#"
#define MQTT_TOPIC_HVAC "iotcebu/vergil/hvac/#"

#define mainQUEUE_LENGTH 5 

#define DEFAULT_HVAC_TEMP 26

// Mutex to control access to resources from
// different task/threads
static xSemaphoreHandle print_mux;
static xSemaphoreHandle i2c_mux;
// The queue for passing data between threads
static QueueHandle_t xQueue = NULL;

/* FreeRTOS event group to signal when we are connected & ready to make a request */
EventGroupHandle_t wifi_event_group;
EventGroupHandle_t mqtt_event_group;

// Send and receive buffers for MQTT
#define MQTT_BUF_SIZE 512
static unsigned char mqtt_sendBuf[MQTT_BUF_SIZE];
static unsigned char mqtt_readBuf[MQTT_BUF_SIZE];

// GPIO ports for the LED drivers
static uint8_t led_pin[] = { 16, 17 };
static int led_cnt = sizeof(led_pin)/sizeof(led_pin[0]);

// Aircon Status
static uint8_t acstatus = 0, actemp = DEFAULT_HVAC_TEMP;

// Data types used for message passing (xQueue)
typedef struct SensorData {
    float temperature;
    float humidity;
}xSensorData;

typedef struct QueueMessage {
    char messageType;
    union {
        xSensorData sensorData;
        char data[20];
    }messagePayload;
}xQueueMessage;

typedef enum MessageType
{
    SENSOR_UPDATE = 0,
    SWITCH_UPDATE,
}xMessageType;

// Utilty functions
uint8_t str2uint8(uint8_t *out, const char *s) {
    char *end;
    if (s[0] == '\0')
        return 1;
    errno = 0;
    long l = strtol(s, &end, 10);
    if (l > 255)
        return 1;
    if (l < 0)
        return 1;
    if (*end != '\0')
        return 1;
    *out = (uint8_t)l;
    return 0;
}

void setLED(int gpio_num,uint8_t ledchan,uint8_t duty)
{

    if (duty>100) {
        duty=100;
    } 

    ledc_timer_config_t timer_conf;
    timer_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    timer_conf.bit_num    = LEDC_TIMER_10_BIT;
    timer_conf.timer_num  = LEDC_TIMER_0;
    timer_conf.freq_hz    = 1000;
    ledc_timer_config(&timer_conf);

    ledc_channel_config_t ledc_conf;
    ledc_conf.gpio_num   = gpio_num;
    ledc_conf.speed_mode = LEDC_HIGH_SPEED_MODE;
    ledc_conf.channel    = ledchan;
    ledc_conf.intr_type  = LEDC_INTR_DISABLE;
    ledc_conf.timer_sel  = LEDC_TIMER_0;
    ledc_conf.duty       = (0x03FF*(uint32_t)duty)/100;
    ledc_channel_config(&ledc_conf);
}

static void mqtt_message_fan_handler(MessageData *md) 
{
    char subtopic[100];
    int topiclen;

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Topic received!: %.*s %.*s\n", md->topicName->lenstring.len, md->topicName->lenstring.data, md->message->payloadlen, (char*)md->message->payload);
    xSemaphoreGive(print_mux);

    // Get the root length of the topic
    topiclen = strlen(MQTT_TOPIC_FAN) - 1; // without the "#"
    sprintf(subtopic, "%.*s", md->topicName->lenstring.len - topiclen, md->topicName->lenstring.data + topiclen);

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Topic item : %s\n", subtopic);
    xSemaphoreGive(print_mux);

    if (strcmp(subtopic, "onoff") == 0)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Toggle Fan On/Off\n");
        xSemaphoreGive(print_mux);

        // Toggle the on off button on IR remote
        toggleFanOnOff();
    }

    if (strcmp(subtopic, "swing") == 0)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Toggle Fan Swing\n");
        xSemaphoreGive(print_mux);

        toggleFanOscillate();
    }

    if (strcmp(subtopic, "speed") == 0)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Toggle Fan Speed\n");
        xSemaphoreGive(print_mux);
        
        toggleFanSpeed();
    }
} 

static void mqtt_message_hvac_handler(MessageData *md) 
{
    char subtopic[100], val[100];
    uint8_t tmpval;
    int topiclen;

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Topic received!: %.*s %.*s\n", md->topicName->lenstring.len, md->topicName->lenstring.data, md->message->payloadlen, (char*)md->message->payload);
    xSemaphoreGive(print_mux);

    // Get the root length of the topic
    topiclen = strlen(MQTT_TOPIC_HVAC) - 1; // without the "#"
    sprintf(subtopic, "%.*s", md->topicName->lenstring.len - topiclen, md->topicName->lenstring.data + topiclen);

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Topic item : %s\n", subtopic);
    xSemaphoreGive(print_mux);
    
     // get the value
    sprintf(val,"%.*s",md->message->payloadlen, (char*)md->message->payload);
    tmpval = strtol(&val[0], NULL, 0);
   
    if (strcmp(subtopic, "onoff") == 0)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Setting AC to %s\n", (tmpval == 0) ? "Off":"On");
        xSemaphoreGive(print_mux);

        if (tmpval == 0) // Off
        {
            sendHvacCommand(HVAC_COLD, 30, FAN_SPEED_AUTO, WIDE_MIDDLE, 0); 
            acstatus = 0;
        }
        else
        {
            sendHvacCommand(HVAC_COLD, DEFAULT_HVAC_TEMP, FAN_SPEED_AUTO, WIDE_MIDDLE, 1);
            acstatus = 1;
            actemp = DEFAULT_HVAC_TEMP;
        }
    }

    if (strcmp(subtopic, "temp") == 0)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Setting AC Temperature to %d\n", tmpval);
        xSemaphoreGive(print_mux);

        sendHvacCommand(HVAC_COLD, tmpval, FAN_SPEED_AUTO, WIDE_MIDDLE, 1);

        acstatus = 1;
        actemp = tmpval;
    }

} 

static void mqtt_message_led_handler(MessageData *md) 
{
    uint8_t duty,gpio,ret;
    char gpoinum[2];
    char dutynum[255];

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Topic received!: %.*s %.*s\n", md->topicName->lenstring.len, md->topicName->lenstring.data, md->message->payloadlen, (char*)md->message->payload);
    xSemaphoreGive(print_mux);

    gpoinum[0]=*(md->topicName->lenstring.data+md->topicName->lenstring.len-1);
    gpoinum[1]='\0';

    sprintf(dutynum,"%.*s",md->message->payloadlen, (char*)md->message->payload);
    ret=str2uint8(&gpio,(const char *)gpoinum);
    if (ret!=0) {
        gpio=0;
    }
    if (gpio>=led_cnt) {
        gpio=led_cnt-1;
    }
    str2uint8(&duty,(const char *)dutynum);
    if (ret!=0) {
        duty=0;
    }

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("setLED!: %d %d %d %s %d\n", led_pin[gpio], gpio, duty,gpoinum,md->topicName->lenstring.len);
    xSemaphoreGive(print_mux);

    setLED(led_pin[gpio],gpio,duty);
}

static void mqtt_message_switch_handler(MessageData *md) 
{
    uint8_t onoff,switchport;
    uint8_t data[2] = { 0, 0};
    char switchnum[2];
    char val[255];

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Topic received!: %.*s %.*s\n", md->topicName->lenstring.len, md->topicName->lenstring.data, md->message->payloadlen, (char*)md->message->payload);
    xSemaphoreGive(print_mux);

    // convert the switch to a string (must be a single digit), we take the leftmost digit
    switchnum[0]=*(md->topicName->lenstring.data+md->topicName->lenstring.len-1);
    switchnum[1]='\0';
    str2uint8(&switchport,(const char *)switchnum);
    if (switchport > 8) // We only have 4 switches! (0 - 3)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Swwitch range error [%d]\n", switchport);
        xSemaphoreGive(print_mux);
        return;
    } 

    // get the value, must be either '1' or '0' (1 = on, 0 = off)
    sprintf(val,"%.*s",md->message->payloadlen, (char*)md->message->payload);
    onoff = (strcmp(val, "1") == 0 ) ? 1 : 0;
    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Setting port switch [%d] with [%s]\n", switchport, onoff ? "On" : "Off");
    xSemaphoreGive(print_mux);

    xSemaphoreTake(i2c_mux, portMAX_DELAY);
    // first get the value of the latched data on the mcp23017
    if ( i2c_master_read_slave_reg(I2C_MASTER_NUM, MCP23017_ADDR, 0x14, &data[0], 1) == ESP_OK)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Current switch value [%x]\n", data[0]);
        xSemaphoreGive(print_mux);

        // Set the value (on or off) based on switch position
        if (onoff) // On the switch
        {
            data[1] = data[0] | (1 << switchport);
        }
        else // Off the switch
        {
            data[1] = data[0] & ~(1 << switchport);
        }
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("New switch value [%x]\n", data[1]);
        xSemaphoreGive(print_mux);

        data[0] = 0x12; // Output port
        // Finally write the values to mcp23017
        i2c_master_write_slave(I2C_MASTER_NUM, MCP23017_ADDR, &data[0], 2);
    }
    xSemaphoreGive(i2c_mux);
}



/**
 * @brief Initialize the MCP23017 port expander
 *        First set the register 0x00 (IODIRA) for output (0x00),
 *        then set the register 0x01 (IODIRB) for input (0xFF)
 **/
static void mcp23017_setup(i2c_port_t i2cnum)
{
    uint8_t data[2];

    // Set IODIRA as output
    data[0] = 0x00; // IODIRA
    data[1] = 0x00; // All outputs

    i2c_master_write_slave(i2cnum, MCP23017_ADDR, &data[0], 2);

    // Set IODIRB as input
    data[0] = 0x01; // IODIRB
    data[1] = 0xFF; // All inputs

    i2c_master_write_slave(i2cnum, MCP23017_ADDR, &data[0], 2);

    // Set GPPUB (pull up) on port B as 1 (all pins has internal 100K pullup resistor)
    data[0] = 0x0D; // GPPUB
    data[1] = 0xFF; // pull up resistor on all pins
    i2c_master_write_slave(i2cnum, MCP23017_ADDR, &data[0], 2);


    // Set Port A all to 0
    data[0] = 0x12;
    data[1] = 0x00;
    i2c_master_write_slave(i2cnum, MCP23017_ADDR, &data[0], 2);
}


/**
 * @brief test function to show buffer
 */
static void disp_buf(uint8_t* buf, int len)
{
    int i;
    for (i = 0; i < len; i++) {
        printf("%02x ", buf[i]);
        if (( i + 1 ) % 16 == 0) {
            printf("\n");
        }
    }
    printf("\n");
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}
//            .ssid = CONFIG_WIFI_SSID,
//            .password = CONFIG_WIFI_PASSWORD,

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    xSemaphoreGive(print_mux);

    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}


static void mqtt_task(void *arg)
{
    int ret;
    Network network;

    while(1) {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Wait for WiFi ...\n");
        xSemaphoreGive(print_mux);

        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
           */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                false, true, portMAX_DELAY);

        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Connected to AP\n");
        printf("Start MQTT Task ...\n");
        xSemaphoreGive(print_mux);

        MQTTClient client;
        NetworkInit(&network);
        network.websocket = MQTT_WEBSOCKET;

        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("NetworkConnect %s:%d ...\n",MQTT_SERVER,MQTT_PORT);
        xSemaphoreGive(print_mux);

        ret = NetworkConnect(&network, MQTT_SERVER, MQTT_PORT);
        if (ret != 0) {
            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("NetworkConnect not SUCCESS: %d\n", ret);
            xSemaphoreGive(print_mux);

            goto exit;
        }

        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("MQTTClientInit  ...\n");
        xSemaphoreGive(print_mux);

        MQTTClientInit(&client, &network,
                2000,            // command_timeout_ms
                mqtt_sendBuf,         //sendbuf,
                MQTT_BUF_SIZE, //sendbuf_size,
                mqtt_readBuf,         //readbuf,
                MQTT_BUF_SIZE  //readbuf_size
                );

        char buf[30];
        MQTTString clientId = MQTTString_initializer;
        sprintf(buf, MQTT_CLIENTID);
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("MQTTClientInit  %s\n",buf);
        xSemaphoreGive(print_mux);
        clientId.cstring = buf;

        MQTTString username = MQTTString_initializer;
        username.cstring = MQTT_USER;

        MQTTString password = MQTTString_initializer;
        password.cstring = MQTT_PASS;

        MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
        data.clientID          = clientId;
        data.willFlag          = 0;
        data.MQTTVersion       = 4; // 3 = 3.1 4 = 3.1.1
        data.keepAliveInterval = 5;
        data.cleansession      = 1;
        data.username          = username;
        data.password          = password;

        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("MQTTConnect  ...\n");
        xSemaphoreGive(print_mux);

        ret = MQTTConnect(&client, &data);
        if (ret != SUCCESS) {
            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("MQTTConnect FAILED: %d\n", ret);
            xSemaphoreGive(print_mux);
            goto exit;
        }

        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("MQTTSubscribe  ...\n");
        xSemaphoreGive(print_mux);

        ret = MQTTSubscribe(&client, MQTT_TOPIC_LED, QOS0, mqtt_message_led_handler);
        if (ret != SUCCESS) {
            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("MQTTSubscribe: %d\n", ret);
            xSemaphoreGive(print_mux);
            goto exit;
        }

        ret = MQTTSubscribe(&client, MQTT_TOPIC_SWITCH, QOS0, mqtt_message_switch_handler);
        if (ret != SUCCESS) {
            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("MQTTSubscribe: %d\n", ret);
            xSemaphoreGive(print_mux);
            goto exit;
        }

        ret = MQTTSubscribe(&client, MQTT_TOPIC_FAN, QOS0, mqtt_message_fan_handler);
        if (ret != SUCCESS) {
            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("MQTTSubscribe: %d\n", ret);
            xSemaphoreGive(print_mux);
            goto exit;
        }

        ret = MQTTSubscribe(&client, MQTT_TOPIC_HVAC, QOS0, mqtt_message_hvac_handler);
        if (ret != SUCCESS) {
            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("MQTTSubscribe: %d\n", ret);
            xSemaphoreGive(print_mux);
            goto exit;
        }

#if defined(MQTT_TASK)
        if ((ret = MQTTStartTask(&client)) != pdPASS)
        {
            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("Return code from start tasks is %d\n", ret);
            xSemaphoreGive(print_mux);
        }
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("MQTT Task started!\n");
        xSemaphoreGive(print_mux);
#endif

        // Mark MQTT listeners we are ready
        xEventGroupSetBits(mqtt_event_group, MQTT_READY_BIT);


        char msgbuf[200];
        xQueueMessage msg;
        while(1) {
#if !defined(MQTT_TASK)
            ret = MQTTYield(&client, (data.keepAliveInterval+1)*1000);
            if (ret != SUCCESS) {
                xSemaphoreTake(print_mux, portMAX_DELAY);
                printf("MQTTYield: %d", ret);
                xSemaphoreGive(print_mux);
                goto exit;
            }
#endif
            if (xQueueReceive(xQueue, &msg, portMAX_DELAY))
            {
                switch(msg.messageType)
                {
                    case SENSOR_UPDATE:
                        sprintf(msgbuf, "{\"temperature\":%.1f,\"humidity\":%.1f,\"acstatus\":%d,\"actemp\":%d}", 
                                msg.messagePayload.sensorData.temperature, 
                                msg.messagePayload.sensorData.humidity,
                                acstatus,
                                actemp);

                        xSemaphoreTake(print_mux, portMAX_DELAY);
                        printf("*******************\n");
                        printf("MQTT Task:  MQTT Publish\n");
                        printf("Sensor update payload [%s]\n", msgbuf);
                        printf("*******************\n");
                        xSemaphoreGive(print_mux);
                        break;
                    default:
                        xSemaphoreTake(print_mux, portMAX_DELAY);
                        printf("Invalid message Type!\n");
                        xSemaphoreGive(print_mux);
                        continue;
                }

                // Do something with data received
                MQTTMessage message;

                message.qos = QOS0;
                message.retained = false;
                message.dup = false;
                message.payload = (void*)msgbuf;
                message.payloadlen = strlen(msgbuf)+1;

                ret = MQTTPublish(&client, "iotcebu/vergil/weather", &message);
                if (ret != SUCCESS) {
                    xSemaphoreTake(print_mux, portMAX_DELAY);
                    printf("MQTTPublish FAILED: %d\n", ret);
                    xSemaphoreGive(print_mux);
                    goto exit;
                }
            }

        }
exit:
        MQTTDisconnect(&client);
        NetworkDisconnect(&network);
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("Starting again!");
        xSemaphoreGive(print_mux);
    }
    esp_task_wdt_delete();
    vTaskDelete(NULL);
}



void app_main()
{
    int ret_temp, ret_humid;
    float temperature, humidity;

    print_mux = xSemaphoreCreateMutex();
    i2c_mux = xSemaphoreCreateMutex();

    nvs_flash_init();
    rmt_tx_init();
    // Initialize I2C and related peripherals connected to it
    xSemaphoreTake(i2c_mux, portMAX_DELAY);
    i2c_master_init();
    mcp23017_setup(I2C_MASTER_NUM);
    xSemaphoreGive(i2c_mux);
    // Initialize wifi
    initialise_wifi();

    // Create the queue
    xQueue = xQueueCreate( mainQUEUE_LENGTH, sizeof( xQueueMessage ) );
    mqtt_event_group = xEventGroupCreate();

    // Create the MQTT thread
    xTaskCreate(mqtt_task, "mqtt_task", 12288, NULL, 5, NULL);

    xSemaphoreTake(print_mux, portMAX_DELAY);
    printf("I2C Sensor task waiting for MQTT ready ...\n");
    xSemaphoreGive(print_mux);

    // Wait untill mqtt is ready 
    xEventGroupWaitBits(mqtt_event_group, MQTT_READY_BIT,
        false, true, portMAX_DELAY);


    while(1)
    {
        xSemaphoreTake(print_mux, portMAX_DELAY);
        printf("*******************\n");
        printf("Main Task: MASTER READ SENSOR( HTU21D )\n");
        // Start reading i2c for temperature and humidity
        xSemaphoreTake(i2c_mux, portMAX_DELAY);
        if ( (ret_temp = htu21d_temperature(&temperature)) == ESP_OK)
            printf("Sensor temperature: %.2f\n", temperature);
        if ( (ret_humid = htu21d_humidity(&humidity)) == ESP_OK)
            printf("Sensor humidity   : %.2f\n", humidity);
        xSemaphoreGive(i2c_mux);
        printf("*******************\n");
        xSemaphoreGive(print_mux);

        // Push the values into the queue
        if ((ret_temp == ESP_OK) && (ret_humid == ESP_OK))
        {
            xQueueMessage msg;

            xSemaphoreTake(print_mux, portMAX_DELAY);
            printf("Sending data to MQTT Task ...\n");
            xSemaphoreGive(print_mux);

            msg.messageType = SENSOR_UPDATE;
            msg.messagePayload.sensorData.temperature = temperature;
            msg.messagePayload.sensorData.humidity = humidity;
            // Send to the queue, don't block if the queue is already
            // full
            xQueueSend(xQueue, &msg, ( TickType_t ) 0);
        }
       
        size_t numBytes = xPortGetFreeHeapSize();
        xSemaphoreTake(print_mux, portMAX_DELAY);
        //printf("%sMain Task : Free Heap Size = %d%s\n", ANSI_COLOR_RED, numBytes, ANSI_COLOR_RESET);
        printf("Main Task : Free Heap Size = %d\n", numBytes);
        xSemaphoreGive(print_mux);

        vTaskDelay( DELAY_TIME_BETWEEN_ITEMS_MS / portTICK_RATE_MS);
    }
}

