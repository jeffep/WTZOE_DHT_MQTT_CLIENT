/*************************************************************************
 * This is the file that contains variables that need to be set for
 * each specific sensor.  Change all these settings for your specific needs.
 */

#ifndef USER_SETTINGS_H
#define USER_SETTINGS_H

#include <string.h>

#define MYYEAR 2024
#define MYMONTH 7
#define MYDATE 1
#define MYHOURS 12
#define MYMINUTES 0
#define MYSECONDS 0

#define DHT_PIN 14
#define MAX_TIMINGS = 85
#define LED_PIN 15

#define DHT_OK              0
#define DHT_ERR_CHECKSUM    1
#define DHT_ERR_NAN         2

// Try not to go over 200 chars here
#define MQTT_CLIENT_ID "rpi-pico106"
#define MQTT_USERNAME "mqtt"
#define MQTT_PASSWORD "1234"
#define MQTT_PUBLISH_TOPIC "homeassistant/home/workroom/temperature"
#define MQTT_PUBLISH_PAYLOAD "Temperature and Humidity"
#define PORT_MQTT 1883

#define MQTT_PUBLISH_PERIOD (1000 * 10) // 10 seconds
#define MQTT_KEEP_ALIVE 60              // 60 milliseconds
#define DATA_BUF_SIZE 2048
#define ETHERNET_BUF_MAX_SIZE (1024 * 2)
#define DEFAULT_TIMEOUT 2000 // 1 second (1000 changed to 2000)

#define SOCKET_MQTT 2
#define BROKER_ADDRESS "192.168.87.100" // Replace with your broker's address
#define BROKER_PORT 1883              // Standard MQTT port
#define KEEP_ALIVE 60                 // Keep-alive time in seconds
#define TOPIC "homeassistant/home/workroom/temperature"  // Topic to publish data
#define MAX_CONNECT_ATTEMPTS 5        // Maximum connection attempts
#define SLEEP_TIME_MS 1000            // Sleep time between MQTT retries (milliseconds)
#define CONNECT_TIMEOUT_MS 10000      // 10 second connection timeout

static char will_topic[] = "homeassistant/home/workroom/"; 
static char will_message[] = "Wiznet rpi-pico106 Disconnected";
static char username[] = "mqtt";
static char password[] = "1234";
static char client_id[] = "rpi-pico106"; 

//Here is your MQTT Broker IP address
static uint8_t *mqtt_broker_ip = (uint8_t[]){192, 168, 87, 100};

// Here is the settings for your WIZNET board

static wiz_NetInfo g_net_info =
    {
        .mac = {0x00, 0x08, 0xDC, 0x12, 0x34, 0x69}, // MAC address
        .ip = {192, 168, 87, 106},                     // IP address
        .sn = {255, 255, 255, 0},                    // Subnet Mask
        .gw = {192, 168, 87, 1},                     // Gateway
        .dns = {8, 8, 8, 8},                         // DNS server
//        .dhcp = NETINFO_STATIC                       // DHCP enable/disable
};


/* MQTT */
static uint8_t g_mqtt_send_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_mqtt_recv_buf[ETHERNET_BUF_MAX_SIZE] = {
    0,
};
static uint8_t g_mqtt_broker_ip[4] = {192, 168, 87, 100};

/*
static Network g_mqtt_network;
static MQTTClient g_mqtt_client;
static MQTTPacket_connectData g_mqtt_packet_connect_data = MQTTPacket_connectData_initializer;
static MQTTMessage g_mqtt_message;
*/
#endif