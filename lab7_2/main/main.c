/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "sdkconfig.h"
#include "driver/i2c_master.h"

/* Constants that aren't configurable in menuconfig */
#define WEB_SERVER ""
#define WEB_PORT "8000"
#define WEB_PATH "/"

/* Weather Sensor Constants */
#define I2C_MASTER_SCL_IO           8       			/*!< GPIO number used for I2C master clock */
#define I2C_MASTER_SDA_IO           10       			/*!< GPIO number used for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUM_0                   /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          100000			/*!< I2C master clock frequency */
#define I2C_MASTER_TIMEOUT_MS       1000

#define SHTC3_SENSOR_ADDR	    0x70			/*!< Address of the SHTC3 sensor */
#define SHTC3_READ_TEMP		    0x7CA2			/*!< Read Temperature without clock stretching */
#define SHTC3_WAKEUP		    0x3517			/*!< Wakeup SHTC3*/
#define SHTC3_SLEEP		    0xB098			/*!< Put SHTC3 to sleep*/

static const char *TAG = "ESP32-Weather-Poster";

/* Temperature Functions Start */

/**
 * @brief i2c master initialization
 */

static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;

static esp_err_t i2c_master_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = SHTC3_SENSOR_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));
    return ESP_OK;
}

static bool check_crc(uint8_t *data, uint8_t len, uint8_t crc)
{
    uint8_t crc_calc = 0xFF;
    for (int i = 0; i < len; i++) {
        crc_calc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc_calc = (crc_calc & 0x80) ? ((crc_calc << 1) ^ 0x31) : (crc_calc << 1);
        }
    }
    return crc_calc == crc;
}

/*
 * @brief Send a sequence of bytes to the SHTC3 sensor to wake up SHTC3
 * */
static void SHTC3_wake()
{
    uint8_t wake_cmd[2] = {SHTC3_WAKEUP >> 8, (uint8_t)(SHTC3_WAKEUP & 0xFF)};
    i2c_master_transmit(dev_handle, wake_cmd, 2, I2C_MASTER_TIMEOUT_MS);
    vTaskDelay(pdMS_TO_TICKS(200));
}

/*
 * @brief Send a sequence of bytes to the SHTC3 sensor to put it in sleep mode
 * */
static void SHTC3_sleep()
{
    uint8_t sleep_cmd[2] = {SHTC3_SLEEP >> 8, SHTC3_SLEEP & 0xFF};
    i2c_master_transmit(dev_handle, sleep_cmd, 2, I2C_MASTER_TIMEOUT_MS);
}

/*
 * @brief Send a sequence of bytes to the SHTC3 sensor to measure temperature
 * */
static esp_err_t SHTC3_measure(uint16_t *raw_temp, uint16_t *raw_humid)
{
    /* Send data to the SHTC3 sensor */
    uint8_t measure_cmd[2] = {SHTC3_READ_TEMP >> 8, (uint8_t)(SHTC3_READ_TEMP & 0xFF)};
    ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, measure_cmd, sizeof(measure_cmd), I2C_MASTER_TIMEOUT_MS));

    /* Delay by 20 ms */
    vTaskDelay(pdMS_TO_TICKS(20));

    /* Recieve data and process it into raw_temp and raw_humid */
    uint8_t data[6];
    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, data, 6, I2C_MASTER_TIMEOUT_MS));

    if (!check_crc(data, 2, data[2])) {
    	ESP_LOGE(TAG, "Failed to read temperature");
	return ESP_FAIL;
    } else if (!check_crc(data + 3, 2, data[5])) {
    	ESP_LOGE(TAG, "Failed to read humidity");
	return ESP_FAIL;
    }

    *raw_temp = (data[0] << 8) | data[1];
    *raw_humid = (data[3] << 8) | data[4];
    return ESP_OK;
}

/* Temperature Functions End*/

static const char *POST_PAYLOAD = "Temperature: %d°C (%d°F), Humidity: %d%";
static const char *TEMPLATE = "POST " WEB_PATH " HTTP/1.0\r\n"
    "Host: "WEB_SERVER":"WEB_PORT"\r\n"
    "User-Agent: esp-idf/1.0 esp32\r\n"
    "Content-Type: text-plain\r\n"
    "Content-Length: %d\r\n"
    "\r\n"
    "%s";

static void http_get_task(void *pvParameters)
{
    const struct addrinfo hints = {
        .ai_family = AF_INET,
        .ai_socktype = SOCK_STREAM,
    };
    struct addrinfo *res;
    struct in_addr *addr;
    int s, r;
    char recv_buf[64];

    ESP_ERROR_CHECK(i2c_master_init());
    ESP_LOGI(TAG, "I2C initialized for getting weather");

    SHTC3_wake();
    uint16_t unconverted_temp, unconverted_humidity;
    SHTC3_measure(&unconverted_temp, &unconverted_humidity);

    float humidity = (100.0f * unconverted_humidity) / 65535.0f;
    float temp_celsius = ((175.0f * unconverted_temp) / 65535.0f) - 45.0f;
    float temp_fahrenheit = ((temp_celsius * 9) / 5) + 32;
    SHTC3_sleep();

    char payload[256];
    int ret = snprintf(payload, sizeof(payload), POST_PAYLOAD, (int)temp_celsius, (int)temp_fahrenheit, (int)humidity);
    if (ret < 0)
	    ESP_LOGE(TAG, "Failed to add temperature to payload!");

    while(1) {
        int err = getaddrinfo(WEB_SERVER, WEB_PORT, &hints, &res);

        if(err != 0 || res == NULL) {
            ESP_LOGE(TAG, "DNS lookup failed err=%d res=%p", err, res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }

        /* Code to print the resolved IP.

           Note: inet_ntoa is non-reentrant, look at ipaddr_ntoa_r for "real" code */
        addr = &((struct sockaddr_in *)res->ai_addr)->sin_addr;
        ESP_LOGI(TAG, "DNS lookup succeeded. IP=%s", inet_ntoa(*addr));

        s = socket(res->ai_family, res->ai_socktype, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.");
            freeaddrinfo(res);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket");

        if(connect(s, res->ai_addr, res->ai_addrlen) != 0) {
            ESP_LOGE(TAG, "... socket connect failed errno=%d", errno);
            close(s);
            freeaddrinfo(res);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }

        ESP_LOGI(TAG, "... connected");
        freeaddrinfo(res);

	char request[512];
	int request_len = snprintf(request, sizeof(request), TEMPLATE, strlen(payload), payload);

        if (write(s, request, request_len) < 0) {
            ESP_LOGE(TAG, "... socket send failed");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket send success");

        struct timeval receiving_timeout;
        receiving_timeout.tv_sec = 5;
        receiving_timeout.tv_usec = 0;
        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                sizeof(receiving_timeout)) < 0) {
            ESP_LOGE(TAG, "... failed to set socket receiving timeout");
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... set socket receiving timeout success");

        /* Read HTTP response */
        do {
            bzero(recv_buf, sizeof(recv_buf));
            r = read(s, recv_buf, sizeof(recv_buf)-1);
            for(int i = 0; i < r; i++) {
                putchar(recv_buf[i]);
            }
        } while(r > 0);

        ESP_LOGI(TAG, "... done reading from socket. Last read return=%d errno=%d.", r, errno);
        close(s);
        for(int countdown = 10; countdown >= 0; countdown--) {
            ESP_LOGI(TAG, "%d... ", countdown);
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        ESP_LOGI(TAG, "Starting again!");
    }
}

void app_main(void)
{

    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
