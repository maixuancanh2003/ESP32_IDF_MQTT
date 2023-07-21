#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "driver/adc.h"
#include "esp_spi_flash.h"
#include "freertos/timers.h"


#define  EXAMPLE_ESP_WIFI_SSID "HELLO"
#define  EXAMPLE_ESP_WIFI_PASS "12345678"
#define MAX_RETRY 10
#define ESP_INTR_FLAG_DEFAULT 0
#define CONFIG_BUTTON_PIN 0


int realtime= 0; 
int demthoigian = 0 ; 
static int retry_cnt = 0;
uint32_t voltage;
int cnt = 0 ; 
uint32_t MQTT_CONNEECTED = 0;
static const char *TAG = "MQTT_EXAMPLE";
TaskHandle_t ISR = NULL;


void button_task(void *arg)
{
        while(1){  
        vTaskSuspend(NULL);
        cnt ++ ; // dem so lan an nut 
        printf ("BUTTON PRESS \r\n");
        }
}
// interrupt service routine, called when the button is pressed

void IRAM_ATTR button_isr_handler(void* arg) {
        xTaskResumeFromISR(ISR);
        //portYIELD_FROM_ISR(  );
        }
static void mqtt_app_start(void);
static esp_err_t wifi_event_handler(void *arg, esp_event_base_t event_base,
                                   int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_STA_START:
        esp_wifi_connect();
        ESP_LOGI(TAG, "Trying to connect with Wi-Fi\n");
        break;

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "Wi-Fi connected\n");
        break;

    case IP_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "got ip: startibg MQTT Client\n");
        mqtt_app_start();
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGI(TAG, "disconnected: Retrying Wi-Fi\n");
        if (retry_cnt++ < MAX_RETRY)
        {
            esp_wifi_connect();
        }
        else
        ESP_LOGI(TAG, "Max Retry Failed: Wi-Fi Connection\n");
        break;

    default:
        break;
    }
    return ESP_OK;
}

void wifi_init(void)
{
    esp_event_loop_create_default();
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    esp_netif_init();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
}


static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        MQTT_CONNEECTED=1;
        
        msg_id = esp_mqtt_client_subscribe(client, "/topic/test1", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "/topic/test2", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        MQTT_CONNEECTED=0;
        break;

    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_mqtt_client_handle_t client = NULL;
static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "STARTING MQTT");
    esp_mqtt_client_config_t mqttConfig = {
        .broker.address.uri = "mqtt://192.168.137.1:1883/"};
    
    client = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}
void Publisher_Task(void *params)
{

char snum[6];
char str[80];


  while (true)
  {
    if(MQTT_CONNEECTED)
    {
        voltage = adc1_get_raw(ADC1_CHANNEL_6); 
        sprintf(str, "[{time: %d s, Gia tri cua bien tro: %d}]", realtime, voltage);
        puts(str);
        // itoa(voltage, snum, 10);
        // printf("%d\n", voltage);
        esp_mqtt_client_publish(client, "/topic/test3", str, 0, 1,0 );
        vTaskDelay(5000/portTICK_PERIOD_MS) ;
    realtime+=5 ; 
    }
  }
}

void app_main(void)
{
 esp_rom_gpio_pad_select_gpio(CONFIG_BUTTON_PIN); 
  gpio_set_direction(CONFIG_BUTTON_PIN, GPIO_MODE_INPUT);
  gpio_set_intr_type(CONFIG_BUTTON_PIN, GPIO_INTR_NEGEDGE);
  gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
  gpio_isr_handler_add(CONFIG_BUTTON_PIN, button_isr_handler, NULL);
  xTaskCreate( button_task, "button_task", 4096, NULL , 10,&ISR );

 uint32_t voltage;

    // Configure ADC1 capture width
    // 12 bit decimal value from 0 to 4095
    adc1_config_width(ADC_WIDTH_BIT_12); 
    adc1_config_channel_atten(ADC1_CHANNEL_6, ADC_ATTEN_DB_11);
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    wifi_init();
    xTaskCreate(Publisher_Task, "Publisher_Task", 1024 * 5, NULL, 5, NULL);
}