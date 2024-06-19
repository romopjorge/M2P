/**
 * @file      MqttsBuiltlnAWS.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xin Yuan Electronic Technology Co., Ltd
 * @date      2023-11-28
 * @note
 * * * Example is suitable for A7670X/A7608X/SIM7672 series
 * * Connect MQTT Broker as https://aws.amazon.com/campaigns/IoT
 * * Example uses a forked TinyGSM <https://github.com/lewisxhe/TinyGSM>, which will not compile successfully using the mainline TinyGSM.
 * *!!!! When using ESP to connect to AWS, the AWS IOT HUB policy must be set to all devices, otherwise the connection cannot be made.
 * *!!!! When using ESP to connect to AWS, the AWS IOT HUB policy must be set to all devices, otherwise the connection cannot be made.
 * *!!!! When using ESP to connect to AWS, the AWS IOT HUB policy must be set to all devices, otherwise the connection cannot be made.
 * *!!!! When using ESP to connect to AWS, the AWS IOT HUB policy must be set to all devices, otherwise the connection cannot be made.
 * *Youtube : https://youtu.be/am-rTDzm4lQ
 * */
#define TINY_GSM_RX_BUFFER          1024 // Set RX buffer to 1Kb
#define USE_SSL
#define TINY_GSM_SSL_CLIENT_AUTHENTICATION


// See all AT commands, if wanted
#define DUMP_AT_COMMANDS

#define TINY_GSM_MODEM_A7670
#define LILYGO_T_A7670

#include "utilities.h"
#include <TinyGsmClient.h>
#include "certs/EmqxRootCA.h"

//-----------------------
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

const char* ssid = "M2P_Test";
const char* password = "tesla640";
AsyncWebServer server(80);
//-----------------------

#ifdef DUMP_AT_COMMANDS  // if enabled it requires the streamDebugger lib
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, Serial);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

// MQTT details
// The URL of xxxxxxx-ats.iot.ap-southeast-2.amazonaws.com can be found in the settings for endpoint in your AWS IOT Core account
const char *broker = "p53a1ee6.ala.us-east-1.emqxsl.com";
const uint16_t broker_port = 8883;
const char *broker_username = "iotesla";
const char *broker_password = "tesla640";
const char *clien_id = "A76XX";

// Replace the theme you want to publish
const char *publish_topic = "iotesla/modulo5/receiver";

// Current connection index, range 0~1
const uint8_t mqtt_client_id = 0;
uint32_t check_connect_millis = 0;

void mqtt_callback(const char *topic, const uint8_t *payload, uint32_t len)
{
    Serial.println();
    Serial.println("======mqtt_callback======");
    Serial.print("Topic:"); Serial.println(topic);
    Serial.println("Payload:");
    for (int i = 0; i < len; ++i) {
        Serial.print(payload[i], HEX); Serial.print(",");
    }
    Serial.println();
    Serial.println("=========================");
}

bool mqtt_connect()
{
    Serial.print("Connecting to ");
    Serial.print(broker);

    bool ret = modem.mqtt_connect(mqtt_client_id, broker, broker_port, clien_id, broker_username, broker_password);
    if (!ret) {
        Serial.println("Failed!"); return false;
    }
    Serial.println("successed.");

    if (modem.mqtt_connected()) {
        Serial.println("MQTT has connected!");
    } else {
        return false;
    }
    // Set MQTT processing callback
    modem.mqtt_set_callback(mqtt_callback);

    return true;
}


void setup()
{
    Serial.begin(115200); // Set console baud rate

    Serial.println("Start Sketch");

    WiFi.softAP(ssid, password);
    IPAddress ip = WiFi.softAPIP();
    delay(10);

    SerialAT.begin(115200, SERIAL_8N1, MODEM_RX_PIN, MODEM_TX_PIN);

#ifdef BOARD_POWERON_PIN
    pinMode(BOARD_POWERON_PIN, OUTPUT);
    digitalWrite(BOARD_POWERON_PIN, HIGH);
#endif

    // Set modem reset pin ,reset modem
    pinMode(MODEM_RESET_PIN, OUTPUT);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL); delay(100);
    digitalWrite(MODEM_RESET_PIN, MODEM_RESET_LEVEL); delay(2600);
    digitalWrite(MODEM_RESET_PIN, !MODEM_RESET_LEVEL);

    pinMode(BOARD_PWRKEY_PIN, OUTPUT);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, HIGH);
    delay(100);
    digitalWrite(BOARD_PWRKEY_PIN, LOW);

    // Check if the modem is online
    Serial.println("Start modem...");

    int retry = 0;
    while (!modem.testAT(1000)) {
        Serial.println(".");
        if (retry++ > 10) {
            digitalWrite(BOARD_PWRKEY_PIN, LOW);
            delay(100);
            digitalWrite(BOARD_PWRKEY_PIN, HIGH);
            delay(1000);
            digitalWrite(BOARD_PWRKEY_PIN, LOW);
            retry = 0;
        }
    }
    Serial.println();

    // Check if SIM card is online
    SimStatus sim = SIM_ERROR;
    while (sim != SIM_READY) {
        sim = modem.getSimStatus();
        switch (sim) {
        case SIM_READY:
            Serial.println("SIM card online");
            break;
        case SIM_LOCKED:
            Serial.println("The SIM card is locked. Please unlock the SIM card first.");
            // const char *SIMCARD_PIN_CODE = "123456";
            // modem.simUnlock(SIMCARD_PIN_CODE);
            break;
        default:
            break;
        }
        delay(1000);
    }

    //SIM7672G Can't set network mode
#ifndef TINY_GSM_MODEM_SIM7672
    if (!modem.setNetworkMode(MODEM_NETWORK_AUTO)) {
        Serial.println("Set network mode failed!");
    }
    String mode = modem.getNetworkModes();
    Serial.print("Current network mode : ");
    Serial.println(mode);
#endif

    // Check network registration status and network signal status
    int16_t sq ;
    Serial.print("Wait for the modem to register with the network.");
    RegStatus status = REG_NO_RESULT;
    while (status == REG_NO_RESULT || status == REG_SEARCHING || status == REG_UNREGISTERED) {
        status = modem.getRegistrationStatus();
        switch (status) {
        case REG_UNREGISTERED:
        case REG_SEARCHING:
            sq = modem.getSignalQuality();
            Serial.printf("[%lu] Signal Quality:%d", millis() / 1000, sq);
            delay(1000);
            break;
        case REG_DENIED:
            Serial.println("Network registration was rejected, please check if the APN is correct");
            return ;
        case REG_OK_HOME:
            Serial.println("Online registration successful");
            break;
        case REG_OK_ROAMING:
            Serial.println("Network registration successful, currently in roaming mode");
            break;
        default:
            Serial.printf("Registration Status:%d\n", status);
            delay(1000);
            break;
        }
    }
    Serial.println();


    Serial.printf("Registration Status:%d\n", status);
    delay(1000);

    String ueInfo;
    if (modem.getSystemInformation(ueInfo)) {
        Serial.print("Inquiring UE system information:");
        Serial.println(ueInfo);
    }

    if (!modem.enableNetwork()) {
        Serial.println("Enable network failed!");
    }

    delay(5000);

    String ipAddress = modem.getLocalIP();
    Serial.print("Network IP:"); Serial.println(ipAddress);

    // Initialize MQTT, use SSL, use SNI
    modem.mqtt_begin(true, true);

    // Set Amazon Certificate
    modem.mqtt_set_certificate(EmqxRootCA);
    
    // Connecting to AWS IOT Core
    if (!mqtt_connect()) {
        return ;
    }


    server.on("/gateway", HTTP_POST, [](AsyncWebServerRequest *request){
      Serial.println("Recibiendo solicitud HTTP POST en /gateway");

      if (!modem.mqtt_connected()) {
                mqtt_connect();
            } else {

                // If connected, send information once every ten seconds
                AsyncWebParameter* p = request->getParam(1);

                modem.mqtt_publish(0, publish_topic, p->value().c_str());
            }
      
      modem.mqtt_handle();
      delay(5);

      // Responder a la solicitud
      request->send(200, "text/plain", "Solicitud recibida con éxito");
    });

    server.begin();
}

void loop()
{
    // Debug AT
    if (SerialAT.available()) {
        Serial.write(SerialAT.read());
    }
    if (Serial.available()) {
        SerialAT.write(Serial.read());
    }
    delay(1);

    modem.mqtt_handle();
    delay(5);
}
