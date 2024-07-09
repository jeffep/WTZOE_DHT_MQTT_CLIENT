/**
 ******************************************************************************
 * @file    WZTOE/WTZOE_DHT_MQTT_CLIENT/main.c
 * @author  J. Lane
 * @brief   Main program body
 ******************************************************************************
 * Designed for the Wiznet W7500 Surf board with TCP/IP Offload Engine (TOE).
 * Initial testing showed the board to be underclocked for communication with
 * the DHT22.  Overclocking by a factor of 4 - by changing PLL_Value in 7500x.h
 * allowed for the PICO to gather several samples for each bit the DHT trans-
 * mitted.
 *
 * PLEASE SEE userSettings.h for configuration
 *
 * Steps
 *
 * 1. System, UART, and DualTimer initialization
 * 2. Set up network connection
 * 3. Set up MQTT
 * 4. Send a Will notice
 * 5. Start the main loop
 * 6. Interract with the DHT temperature sensor
 * 7. Create a publish packet for the info
 * 8. Publish the info to the Broker
 * 9. Repeat from #6
 **/
#include <stdio.h>
#include <stdbool.h>
#include "w7500x.h"
#include "wizchip_conf.h"
#include "userSettings.h"
#include "dualtimer.h"
#include "socket.h"
#include <inttypes.h>
#include <math.h>
#include "mqtt_interface.h"
#include "MQTTClient.h"


static void UART_Config(void);
void Network_Config();
static __IO uint32_t TimingDelay; // needed for w7500x_it.c
static void DUALTIMER_Config(void);
static void DUALTIMER2_Config(uint32_t countdown_seconds);
void dht(uint8_t *temperature_whole, uint8_t *temperature_decimal,
         uint8_t *humidity_whole, uint8_t *humidity_decimal);
char* create_data_packet(uint8_t temperature_whole, uint8_t temperature_decimal,
                        uint8_t humidity_whole, uint8_t humidity_decimal);


int main(void)
{
    SystemInit();
    UART_Config();
    DUALTIMER_Config();
    

    printf("W7500x Standard Peripheral Library version : %d.%d.%d\r\n", __W7500X_STDPERIPH_VERSION_MAIN, __W7500X_STDPERIPH_VERSION_SUB1, __W7500X_STDPERIPH_VERSION_SUB2);

    printf("SourceClock : %d\r\n", (int)GetSourceClock());
    printf("SystemClock : %d\r\n", (int)GetSystemClock());

#ifdef W7500
    printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_15, GPIO_Pin_14) == SET ? "Success" : "Fail");
#elif defined(W7500P)
    printf("PHY Init : %s\r\n", PHY_Init(GPIOB, GPIO_Pin_14, GPIO_Pin_15) == SET ? "Success" : "Fail");
#endif
    printf("Link : %s\r\n", PHY_GetLinkStatus() == PHY_LINK_ON ? "On" : "Off");


    static Network g_mqtt_network;
    static MQTTClient g_mqtt_client;
    static MQTTPacket_connectData g_mqtt_packet_connect_data = MQTTPacket_connectData_initializer;
    static MQTTMessage g_mqtt_message;
    int32_t loopcount = 0;
    int retval=0;

    //network_initialize(g_net_info);  //old style
    Network_Config();
    DEBUG_PRINT("Wait until Link turns on");
    while (PHY_GetLinkStatus() != PHY_LINK_ON)
        ;
    DEBUG_PRINT("ON!");

    NewNetwork(&g_mqtt_network, SOCKET_MQTT);
    
    retval = ConnectNetwork(&g_mqtt_network, g_mqtt_broker_ip, PORT_MQTT);

    if (retval != 1)
    {
        printf(" Network connect failed\n");

        while (1)
            ;
    }

   /* Initialize MQTT client */
    MQTTClientInit(&g_mqtt_client, &g_mqtt_network, DEFAULT_TIMEOUT, g_mqtt_send_buf, ETHERNET_BUF_MAX_SIZE, g_mqtt_recv_buf, ETHERNET_BUF_MAX_SIZE);

    /* Connect to the MQTT broker */
    g_mqtt_packet_connect_data.MQTTVersion = 3;
    g_mqtt_packet_connect_data.cleansession = 1;
    g_mqtt_packet_connect_data.willFlag = 0;
    g_mqtt_packet_connect_data.keepAliveInterval = MQTT_KEEP_ALIVE;
    g_mqtt_packet_connect_data.clientID.cstring = MQTT_CLIENT_ID;
    g_mqtt_packet_connect_data.username.cstring = MQTT_USERNAME;
    g_mqtt_packet_connect_data.password.cstring = MQTT_PASSWORD;

    retval = MQTTConnect(&g_mqtt_client, &g_mqtt_packet_connect_data);

    if (retval < 0)
    {
        printf(" MQTT connect failed : %d\n", retval);

        while (1)
            ;
    }

    printf(" MQTT connected\n");
    g_mqtt_message.qos = QOS0;
    g_mqtt_message.retained = 0;
    g_mqtt_message.dup = 0;
    g_mqtt_message.payload = MQTT_PUBLISH_PAYLOAD;

    printf("System Loop Start\r\n");

    //Publish a will message first
    DEBUG_PRINT("Sending Will message.");
    g_mqtt_message.payload = "{\"disconnected\": true}";
    g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);
    retval = MQTTPublish(&g_mqtt_client, MQTT_PUBLISH_TOPIC, &g_mqtt_message);
    if (retval < 0) {
        printf(" Publish failed : %d\n", retval);
    }

    while (1)
    {
        // Variables for the DHT data
        uint8_t humidity_whole = 0, humidity_decimal=0;
        uint8_t temperature_whole = 0, temperature_decimal = 0;
        
        dht(&temperature_whole, &temperature_decimal, &humidity_whole, &humidity_decimal);
        g_mqtt_message.payload = create_data_packet(temperature_whole, temperature_decimal,
                                                    humidity_whole, humidity_decimal);
        g_mqtt_message.payloadlen = strlen(g_mqtt_message.payload);

        /* Publish */
        retval = MQTTPublish(&g_mqtt_client, MQTT_PUBLISH_TOPIC, &g_mqtt_message);
        if (retval < 0) {
            printf(" Publish failed : %d\n", retval);
            while (1)
                    ;
        }

        printf(" Published\n");
        /*
        for (int i=0; i<4000; i++) wait_s(5);
        printf("Loop again.");
        */
        DUALTIMER2_Config(1);
        DUALTIMER_ClearIT(DUALTIMER1_0);
        DUALTIMER_ITConfig(DUALTIMER1_0, ENABLE);
        DUALTIMER_Cmd(DUALTIMER1_0, ENABLE);
        DUALTIMER_ClearIT(DUALTIMER1_0);
        uint32_t timeout_us = 90000000; // 5 second timeout
        while ((DUALTIMER_GetITStatus(DUALTIMER1_0) != SET) && (timeout_us > 0)) { 
            //DEBUG_PRINT("Dualtimer not done. timeout=%lu",timeout_us);
            timeout_us -= 1;
        }
        DEBUG_PRINT("Timer fired, or timeout=%d",timeout_us);
        DUALTIMER_ClearIT(DUALTIMER1_0);

        if ((retval = MQTTYield(&g_mqtt_client, g_mqtt_packet_connect_data.keepAliveInterval)) < 0)
        {
            printf(" Yield error : %d\n", retval);

            while (1)
                ;
        }
    }
}

static void UART_Config(void)
{
    UART_InitTypeDef UART_InitStructure;

    UART_StructInit(&UART_InitStructure);

#if defined(USE_WIZWIKI_W7500_EVAL)
    UART_Init(UART1, &UART_InitStructure);
    UART_Cmd(UART1, ENABLE);
#else
    S_UART_Init(115200);
    S_UART_Cmd(ENABLE);
#endif
}

// Network Init Function
void Network_Config(void)
{
    wiz_NetInfo gWIZNETINFO;

    // Here is the settings for your WIZNET board
    //  Defined in the userSettings file
        uint8_t mac_addr[6] = { 0x00, 0x08, 0xDC, 0x01, 0x02, 0x03 };
        uint8_t ip_addr[4] = { 192, 168, 87, 106 };
        uint8_t gw_addr[4] = { 192, 168, 87, 1 };
        uint8_t sub_addr[4] = { 255, 255, 255, 0 };
        uint8_t dns_addr[4] = { 8, 8, 8, 8 };
    

    memcpy(gWIZNETINFO.mac, mac_addr, 6);
    memcpy(gWIZNETINFO.ip, ip_addr, 4);
    memcpy(gWIZNETINFO.gw, gw_addr, 4);
    memcpy(gWIZNETINFO.sn, sub_addr, 4);
    memcpy(gWIZNETINFO.dns, dns_addr, 4);

    ctlnetwork(CN_SET_NETINFO, (void *)&gWIZNETINFO);

    printf("MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n", gWIZNETINFO.mac[0], gWIZNETINFO.mac[1], gWIZNETINFO.mac[2], gWIZNETINFO.mac[3], gWIZNETINFO.mac[4], gWIZNETINFO.mac[5]);
    printf("IP: %d.%d.%d.%d\r\n", gWIZNETINFO.ip[0], gWIZNETINFO.ip[1], gWIZNETINFO.ip[2], gWIZNETINFO.ip[3]);
    printf("GW: %d.%d.%d.%d\r\n", gWIZNETINFO.gw[0], gWIZNETINFO.gw[1], gWIZNETINFO.gw[2], gWIZNETINFO.gw[3]);
    printf("SN: %d.%d.%d.%d\r\n", gWIZNETINFO.sn[0], gWIZNETINFO.sn[1], gWIZNETINFO.sn[2], gWIZNETINFO.sn[3]);
    printf("DNS: %d.%d.%d.%d\r\n", gWIZNETINFO.dns[0], gWIZNETINFO.dns[1], gWIZNETINFO.dns[2], gWIZNETINFO.dns[3]);
}

static void DUALTIMER_Config()
{
    DUALTIMER_InitTypDef DUALTIMER_InitStructure;
    //NVIC_InitTypeDef NVIC_InitStructure;
    //printf("In dualtimer_configA");

    //According to manual, these are the steps.
    // 1. Enable Dualtimer
    DUALTIMER_Cmd(DUALTIMER0_0, ENABLE);
    DUALTIMER_ITConfig(DUALTIMER0_0, ENABLE);
    // 2. Set the Load register
    DUALTIMER_InitStructure.Timer_Load = GetSystemClock() / 100000; //10us
    // 3. Set the timer size
    DUALTIMER_InitStructure.Timer_Size = DUALTIMER_Size_32;
    // 4. Set the Prescale
    DUALTIMER_InitStructure.Timer_Prescaler = DUALTIMER_Prescaler_1;
    // 5. Set the Interrupt Enable
    DUALTIMER_ITConfig(DUALTIMER0_0, ENABLE);
    // 6. Set the Repetition Mode
    DUALTIMER_InitStructure.Timer_Repetition = DUALTIMER_Wrapping;  // continuously countdown
    // 7. Shoud Wrapping Mode be Free Running or Periodic?
    DUALTIMER_InitStructure.Timer_Wrapping = DUALTIMER_Periodic; // reload initial time

    DUALTIMER_Init(DUALTIMER0_0, &DUALTIMER_InitStructure);
}

static void DUALTIMER2_Config(uint32_t countdown_seconds)
{
    DUALTIMER_InitTypDef DUALTIMER2_InitStructure;

    //According to manual, these are the steps.
    // 1. Enable Dualtimer
    DUALTIMER_Cmd(DUALTIMER0_1, ENABLE);
    DUALTIMER_ITConfig(DUALTIMER0_1, ENABLE);
    // 2. Set the Load register
    DEBUG_PRINT("Setting DUALTIMER2, Load=%lu",GetSystemClock() * (countdown_seconds * 3));
    DUALTIMER2_InitStructure.Timer_Load = GetSystemClock() * (countdown_seconds * 3); //1s * countdown
    // 3. Set the timer size
    DUALTIMER2_InitStructure.Timer_Size = DUALTIMER_Size_32;
    // 4. Set the Prescale
    DUALTIMER2_InitStructure.Timer_Prescaler = DUALTIMER_Prescaler_1;
    // 5. Set the Interrupt Enable
    DUALTIMER_ITConfig(DUALTIMER0_1, ENABLE);
    // 6. Set the Repetition Mode
    DUALTIMER2_InitStructure.Timer_Repetition = DUALTIMER_OneShot;  // continuously countdown
    // 7. Shoud Wrapping Mode be Free Running or Periodic?
    DUALTIMER2_InitStructure.Timer_Wrapping = DUALTIMER_Periodic; // reload initial time

    DUALTIMER_Init(DUALTIMER0_1, &DUALTIMER2_InitStructure);
}

/**
 * @brief  Decrements the TimingDelay variable.  Used in w7500x_it.c
 * @param  None
 * @retval None
 */
void TimingDelay_Decrement(void)
{
    if (TimingDelay != 0x00) {
        TimingDelay--;
    }
}

void dht(uint8_t *temperature_whole, uint8_t *temperature_decimal,
         uint8_t *humidity_whole, uint8_t *humidity_decimal)
{

        GPIO_InitTypeDef DHT_Structure;
        int goodBitCounter=0, switches=0;
        uint8_t myDHTBuffer[700],myDHTResult[40], myDHTDecimal[5]={0, 0, 0, 0, 0};;

        //Variables for the DATA packet
        //char data[] = "Hello from MQTT client!";

        // Set up the GPIO Pin
        DHT_Structure.GPIO_Pin = GPIO_Pin_14; 
        DHT_Structure.GPIO_AF = PAD_AF1;
        DHT_Structure.GPIO_Direction = GPIO_Direction_OUT;
        GPIO_Init(GPIOC, &DHT_Structure);

        // Send a 0 and then a 1
        GPIO_ResetBits(GPIOC, GPIO_Pin_14);  // Reset=0
        wait_us(50);  // low for at least 1ms, but seems too long
        GPIO_SetBits(GPIOC, GPIO_Pin_14);  // Set=1
        wait_us(30); // raise line high for 20-40us
        DEBUG_PRINT("Sent 0, 1");

        // Reset the GPIO Pin for input
        DHT_Structure.GPIO_Direction = GPIO_Direction_IN;
        GPIO_Init(GPIOC, &DHT_Structure);

        // DHT Responds with 1s then 0, 1, 0, then all the data.
        while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14) != Bit_SET);
        while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14) != Bit_RESET);
        while (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14) != Bit_SET);

        DEBUG_PRINT("DHT SENDING");

        // READ ALL DATA
        // Should be valid data coming.  Read all data.
        // With overclocking we sample many times during
        // transmission.  0='11110000000', 1='111111110000000'
        for (switches=0; switches<700; switches++) {
              if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_14) == Bit_SET) {
                    DEBUG_PRINT("1");
                    myDHTBuffer[switches]=1;
               } else {
                    DEBUG_PRINT("0");
                    myDHTBuffer[switches]=0;
            }
        }

        // Works best to parse backwards to find the start of the data
        switches=699;
        goodBitCounter=39;
        while (myDHTBuffer[switches] == 1) switches--;
        // Now we've hit 0, so the last data bit

        // A zero bit is high (3.3v) for 28us then goes low.
        // Overclocked at 4 we get up to 4 reads at 3.3v for 30us.
        // The 5th read is consistently 0 for a zero bit, or 1 for a 1
        while (switches > 0) {
            while(myDHTBuffer[switches] == 0) switches--;
            myDHTResult[goodBitCounter] = (myDHTBuffer[switches] == 1 &&
                                            myDHTBuffer[switches - 4] == 1);
            //skip to the next set of 0s
            while(myDHTBuffer[switches]==1) switches--;
            goodBitCounter--;
        }

        DEBUG_PRINT("goodBitCounter= %d. Result array=", goodBitCounter);
        if (goodBitCounter == 0) {
            printf("Bad Read");
        }

        // Now convert 40bits to decimal 4 bytes
        for (int j = 0; j < 5; j++) {
            myDHTDecimal[j] = 0;
            for (int k = 0; k < 8; k++) {
                myDHTDecimal[j] += myDHTResult[j * 8 + k] * (1 << (7 - k));
            }
            DEBUG_PRINT(" MDD[%d]=%d ",j,myDHTDecimal[j]);
        }

        // Checksum
        if (myDHTDecimal[4] != ((myDHTDecimal[0] + 
                                myDHTDecimal[1] + 
                                myDHTDecimal[2] +  
                                myDHTDecimal[3]) & 0xFF)) {
            printf("Failed Checksum");
        }

        //DEBUG_PRINT("Pre Results: %.1f %% %.1f degrees",humidity,temperature);
        uint32_t mytemp = 0, myhum = 0, mytemp_faren = 0;
        uint32_t mytemp_whole = 0, mytemp_decimal = 0;
        uint32_t myhum_whole = 0, myhum_decimal = 0;
        uint32_t mybigtemp = 0, mybig_whole, mybig_decimal;
        myhum = ((myDHTDecimal[0] << 8) + myDHTDecimal[1]);
        mytemp = (((myDHTDecimal[2]& 0x7F) << 8) + myDHTDecimal[3]);
        DEBUG_PRINT("Before Calculation: %d+.%d %% %d+.%d",(myDHTDecimal[0] << 8), myDHTDecimal[1],
                                    ((myDHTDecimal[2]& 0x7F) << 8), myDHTDecimal[3]);
        DEBUG_PRINT("After Calculation: %lu/10 %% %lu/10 C",myhum,mytemp);

        // Can't work with Floats???
        //DEBUG_PRINT("or %.2f%% %.2fc",(float)(myhum/10),(float)(mytemp/10));
        myhum_whole = myhum/10;
        myhum_decimal = myhum % 10;
        mytemp_whole = mytemp/10;
        mytemp_decimal = mytemp % 10;
        printf("Humidity=%lu.%lu, Temperature=%lu.%lu",myhum_whole,myhum_decimal,mytemp_whole,mytemp_decimal);
 
        // Calculate Farenheit (can't work with floats!?)
        //mytemp = (mytemp * 9/5) + 32;  // Farenheit

        mybigtemp = (mytemp_whole * 100) + (mytemp_decimal);
        DEBUG_PRINT("mybigtemp = %lu", mybigtemp);
        mybigtemp = mybigtemp * 9/5;
        DEBUG_PRINT("*9/5=%lu",mybigtemp);
        mybig_whole = mybigtemp / 100;
        mybig_decimal = mybigtemp % 100;
        DEBUG_PRINT("back=%lu.%lu",mybig_whole,mybig_decimal);
        mytemp_faren = mybig_whole + 32;

        printf("Final Results: %lu.%lu %% %lu.%lu degrees F",myhum_whole, myhum_decimal, mytemp_faren,mybig_decimal);
        *temperature_whole=mytemp_faren;
        *temperature_decimal=mybig_decimal;
        *humidity_whole=myhum_whole;
        *humidity_decimal=myhum_decimal;
}

char* create_data_packet(uint8_t temperature_whole, uint8_t temperature_decimal,
                        uint8_t humidity_whole, uint8_t humidity_decimal)
{
        uint8_t myhum_whole = humidity_whole;
        uint8_t myhum_decimal=humidity_decimal;
        uint8_t mytemp_faren = temperature_whole;
        uint8_t mybig_decimal=temperature_decimal;

        char payloadAll[105]="";
        char payload1[55]= "\"Temperature\": \"";
        char buffer1[10];
        char payload2[] = "\", \"Humidity\": \"";
        char buffer2[10];

        //data_packet[0]='\0';
        payloadAll[0]='\0';
        int retval=0;

        // Create the data packet (QoS 0 assumed)
        int position = 0;
        if ((mytemp_faren/100) != 0) {
            DEBUG_PRINT("first#=%d",mytemp_faren/100);
            buffer1[position++] = '0' + mytemp_faren/100;
        }
        buffer1[position++]= '0' + mytemp_faren/10;
        DEBUG_PRINT("buffer1[%d]=%c",position-1, '0' + mytemp_faren/10);
        buffer1[position++]= '0' + mytemp_faren % 10;
        DEBUG_PRINT("buffer1[%d]=%c",position-1, '0' + mytemp_faren % 10);
        buffer1[position++]='.';
        buffer1[position] = '\0';
        DEBUG_PRINT("Converting temp decimal: %d",mybig_decimal);
        DEBUG_PRINT("buffer1=%s", buffer1);
        sprintf(buffer1 + position, "%.2d",mybig_decimal);
        buffer1[position+3]='\0';

        DEBUG_PRINT("buffer1=");
        for (int i=0; i<position+2; i++) DEBUG_PRINT("%c",buffer1[i]);

        position = 0;
        if ((myhum_whole/100) != 0) buffer2[position++] = '0' + myhum_whole/100;
        buffer2[position++]= '0' + myhum_whole/10;
        buffer2[position++]= '0' + myhum_whole % 10;
        buffer2[position++]='.';
        buffer2[position] = '\0';
        if (myhum_decimal>9) {
            sprintf(buffer2 + position, "%.2d",myhum_decimal); 
            buffer2[position+3]='\0';
            position+=3;

        } else {
            sprintf(buffer2 + position, "%d",myhum_decimal);
            buffer2[position+2]='\0';
            position+=2;
        }

        DEBUG_PRINT("buffer2=");
        for (int i=0; i<position; i++) DEBUG_PRINT("%c",buffer2[i]);

        sprintf(payloadAll,"{\"location\": \"");
        strcat(payloadAll,"homeassistant/home/workroom/temperature");
        strcat(payloadAll,"\", ");

        strcat(payloadAll, payload1);
        strcat(payloadAll, buffer1);
        strcat(payloadAll, payload2);
        strcat(payloadAll, buffer2);
        strcat(payloadAll, "\"}");
        printf("Expected payload is: %s ", payloadAll);
        return(payloadAll);
}