// WiFi & HTTP Post
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
const char *ssid = "TESLA LIMITADA";
const char *password = "sp2PxwdwQ3mx";
const int wifitimeout = 2000;
/* API YD3GYAGK2SG2ZDKY / KNDQOPULOB532M03 */
String apiKey = "453OP6IXES3VEVOS";
const char *serverName = "https://iotesla.herokuapp.com/datos/"; /*"http://api.thingspeak.com/update"*/

//-----------------------------------------------------------------------------------------------------

int k = 0;
int u = 0;

// MQTT
extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/semphr.h"
}

#include <AsyncMQTT_ESP32.h>

#define _ASYNC_MQTT_LOGLEVEL_ 1
#define MQTT_HOST "p53a1ee6.ala.us-east-1.emqxsl.com"
#define ASYNC_TCP_SSL_ENABLED true

#if ASYNC_TCP_SSL_ENABLED

#define MQTT_SECURE true

const uint8_t MQTT_SERVER_FINGERPRINT[] = {0x41, 0x31, 0x0D, 0x31, 0x37, 0x06, 0x8F, 0xAC, 0xC9, 0xE3, 0xE6, 0xFC, 0xF1, 0x88, 0x89, 0x71, 0xC1, 0x69, 0xF5, 0xB3};
const char *PubTopic = "iotesla/modulo6/receiver"; // Topic to publish

#define MQTT_PORT 8883

#else

const char *PubTopic = "async-mqtt/ESP32_Pub"; // Topic to publish

#define MQTT_PORT 1883

#endif

const char *username_mqtt = "iotesla";
const char *password_mqtt = "tesla640";
const uint16_t keepalive_mqtt = 15;

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
static SemaphoreHandle_t bin_sem;
static SemaphoreHandle_t sd_sem;

// RTC
#include "RTClib.h"
#include "NTPClient.h"
RTC_DS1307 rtc;
const long utcOffsetInSeconds = -4 * 3600;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

// SD
#include <SPI.h>
#include <SD.h>
String filename = "";
const char *path = "";
String datalog = "";

// Humidity Sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#define DHTPIN 15
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float hum;
float tempamb;

// ADS1015/Current Sensor
#include <ADS1X15.h>

// DS18B20 Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
static float temp[5];
//
OneWire oneWire1(13);
DallasTemperature sensor1(&oneWire1);
//
OneWire oneWire2(14);
DallasTemperature sensor2(&oneWire2);
//
OneWire oneWire3(27);
DallasTemperature sensor3(&oneWire3);
//
OneWire oneWire4(26);
DallasTemperature sensor4(&oneWire4);
//
OneWire oneWire5(25);
DallasTemperature sensor5(&oneWire5);
//

// Current
int sum = 0;
int current = 0;
bool currenton = false;
bool currentact = false;
int factor1 = 0;
int factor2 = 0;
int factor3 = 0;
String Irms1;
int I1[1000];
String Irms2;
int I2[1000];
String Irms3;
int I3[1000];

// Instancias
//ADS1015 ADS1(0x48);
//ADS1015 ADS2(0x49);

// Funciones
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void openFile(fs::FS &fs, const char *path);
void connectToWifi();
/*
void connectToMqtt();
void WiFiEvent(WiFiEvent_t event);
*/
void printSeparationLine();
/*
void onMqttConnect(bool sessionPresent);
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(const uint16_t &packetId, const uint8_t &qos);
void onMqttUnsubscribe(const uint16_t &packetId);
void onMqttMessage(char *topic, char *payload, const AsyncMqttClientMessageProperties &properties,
                   const size_t &len, const size_t &index, const size_t &total);
void onMqttPublish(const uint16_t &packetId);
*/
void isr();

float battery;
int period = 0;
unsigned long time_now = 0;

// Entradas
//const int in_Pin1 = 35;
//const int in_Pin2 = 32;
int in_State1 = 0;
int in_State2 = 0;

// RTC Memory
RTC_DATA_ATTR int updt_day = 0;
RTC_DATA_ATTR int int_state = 1;

// OTA

void setup()
{

  //-Serial----------------------------------------------------------------------------

  Serial.begin(115200);
  delay(1);

  //-WiFi------------------------------------------------------------------------------

  connectToWifi();

  //-Entradas--------------------------------------------------------------------------

  //attachInterrupt(32, isr, CHANGE);
  //attachInterrupt(35, isr, CHANGE);

  //-----------------------------------------------------------------------------------

  SD.begin(5);
  delay(1);

  Wire.begin();
  Wire.setClock(400000);
  delay(1);

  /*
  ADS1.begin();
  ADS1.setGain(4);
  ADS1.setDataRate(7);
  delay(1);


  ADS2.begin();
  ADS2.setGain(4);
  ADS2.setDataRate(7);
  delay(1);
  */

  sensor1.begin();delay(1);
  sensor2.begin();delay(1);
  sensor3.begin();delay(1);
  sensor4.begin();delay(1);
  sensor5.begin();delay(1);
  //
  sensor1.requestTemperatures();delay(1);
  sensor2.requestTemperatures();delay(1);
  sensor3.requestTemperatures();delay(1);
  sensor4.requestTemperatures();delay(1);
  sensor5.requestTemperatures();delay(1);
  //
  delay(1);

  dht.begin();
  delay(1);

  rtc.begin();
  delay(1);

  pinMode(LED_BUILTIN, OUTPUT);
  delay(1);

  //pinMode(in_Pin1, INPUT);
  //delay(1);

  //pinMode(in_Pin2, INPUT);
  //delay(1);

  bin_sem = xSemaphoreCreateBinary();
  sd_sem = xSemaphoreCreateMutex();

  /*
  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void *)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  mqttClient.setCredentials(username_mqtt, password_mqtt);
  mqttClient.setKeepAlive(keepalive_mqtt);

#if ASYNC_TCP_SSL_ENABLED
  mqttClient.setSecure(MQTT_SECURE);

  if (MQTT_SECURE)
  {
    // mqttClient.addServerFingerprint((const uint8_t[])MQTT_SERVER_FINGERPRINT);
    mqttClient.addServerFingerprint((const uint8_t *)MQTT_SERVER_FINGERPRINT);
  }

#endif
*/
}

void loop()
{

  if (millis() - time_now > period)
  {
    time_now = millis();
    period = 180000;

    ///////////////////////////////////////////// FUNCIONES ////////////////////////////////////////////////////

    //-Input--------------------------------------------------------------------------

    //in_State1 = digitalRead(in_Pin1);
    //in_State2 = digitalRead(in_Pin2);

    //-Temperatura--------------------------------------------------------------------

    sensor1.requestTemperatures();delay(1);
    sensor2.requestTemperatures();delay(1);
    sensor3.requestTemperatures();delay(1);
    sensor4.requestTemperatures();delay(1);
    sensor5.requestTemperatures();delay(1);

    temp[0] = sensor1.getTempCByIndex(0);delay(1);
    temp[1] = sensor2.getTempCByIndex(0);delay(1);
    temp[2] = sensor3.getTempCByIndex(0);delay(1);
    temp[3] = sensor4.getTempCByIndex(0);delay(1);
    temp[4] = sensor5.getTempCByIndex(0);delay(1);

    //-Humedad--------------------------------------------------------------------------

    hum = dht.readHumidity();
    tempamb = dht.readTemperature();

    //-Corriente------------------------------------------------------------------------
/*
    if (current >= 1)
    {
      for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
      {
        I1[j] = ADS1.readADC_Differential_2_3(); // set element at location i
      }
      if (current >= 2)
      {
        for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
        {
          I2[j] = ADS1.readADC_Differential_0_1(); // set element at location i
        }
        if (current >= 3)
        {
          for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
          {
            I3[j] = ADS2.readADC_Differential_0_1(); // set element at location i
          }
        }
        else
        {
          Irms3 = "nan";
        }
      }
      else
      {
        Irms2 = "nan";
        Irms3 = "nan";
      }
    }
    else
    {
      Irms1 = "nan";
      Irms2 = "nan";
      Irms3 = "nan";
    }

    if (current >= 1)
    {
      for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
      {
        I1[j] *= I1[j]; // set element at location i
        sum += I1[j];
      }
      Irms1 = String(0.0005 * factor1 * sqrt(sum / 1000));
      sum = 0;
      if (current >= 2)
      {
        for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
        {
          I2[j] *= I2[j]; // set element at location i
          sum += I2[j];
        }
        Irms2 = String(0.0005 * factor2 * sqrt(sum / 1000));
        sum = 0;
        if (current >= 3)
        {
          for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
          {
            I3[j] *= I3[j]; // set element at location i
            sum += I3[j];
          }
          Irms3 = String(0.0005 * factor3 * sqrt(sum / 1000));
          sum = 0;
        }
      }
    }
*/
    //-RTC----------------------------------------------------------------------------

    DateTime time = rtc.now();
    delay(1);
/*
    int now_day = time.day();
    if (WiFi.status() == WL_CONNECTED && updt_day != now_day)
    {
      timeClient.begin();
      timeClient.update();
      time_t epochTime = timeClient.getEpochTime();
      struct tm *ptm = gmtime((time_t *)&epochTime);
      int monthDay = ptm->tm_mday;
      int currentMonth = ptm->tm_mon + 1;
      int currentYear = ptm->tm_year + 1900;
      rtc.adjust(DateTime(currentYear, currentMonth, monthDay, (timeClient.getHours()), (timeClient.getMinutes()), (timeClient.getSeconds())));
      updt_day = now_day;
    }
*/

    //-SD------------------------------------------------------------------------

    xSemaphoreTake(sd_sem, portMAX_DELAY);
    digitalWrite(LED_BUILTIN, HIGH);
    time = rtc.now();
    String allsensor = String(temp[0]) + "," + String(temp[1]) + "," + String(tempamb) + "," + String(hum) + "," + String(Irms1) + "," + String(Irms2) + "," + String(in_State1) + "," + String(in_State2);
    datalog = time.timestamp() + "," + allsensor + "\r\n";
    filename = "/" + time.timestamp(DateTime::TIMESTAMP_DATE) + ".csv";
    path = filename.c_str();
    openFile(SD, path);
    appendFile(SD, path, datalog.c_str());
    digitalWrite(LED_BUILTIN, LOW);
    xSemaphoreGive(sd_sem);


    //-MQTT-----------------------------------------------------------------------

    /*
    const int start_http = millis();
    xSemaphoreTake(bin_sem, portMAX_DELAY);
    if (WiFi.status() == WL_CONNECTED)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      String jsondata = "{\"modulo\":\"M2P_005\",\"timestamp\":\"" + time.timestamp() + "\",\"sensor1\":\"" + String(tempsensor1) + "\",\"id1\":\"1\",\"sensor2\":\"" + String(tempsensor2) + "\",\"id2\":\"2\",\"sensor3\":\"" + String(tempsensor3) + "\",\"id3\":\"3\",\"sensor4\":\"" + String(tempsensor4) + "\",\"id4\":\"4\",\"sensor5\":\"" + String(tempsensor5) + "\",\"id5\":\"5\",\"sensor6\":\"" + String(tempamb) + "\",\"id6\":\"6\",\"sensor7\":\"" + String(hum) + "\",\"id7\":\"7\",\"sensor8\":\"" + String(Irms1) + "\",\"id8\":\"8\",\"sensor9\":\"" + String(Irms2) + "\",\"id9\":\"9\",\"sensor10\":\"" + String(Irms3) + "\",\"id10\":\"10\",\"sensor11\":\"" + String(in_State1) + "\",\"id11\":\"11\",\"sensor12\":\"" + String(in_State2) + "\",\"id12\":\"12\"}";
      uint16_t packetIdPub1 = mqttClient.publish(PubTopic, 1, true, jsondata.c_str());
      Serial.println(packetIdPub1);
      delay(10);
      digitalWrite(LED_BUILTIN, LOW);
    }
    else
    {
      connectToWifi();
    }
    */

    //-HTTP-----------------------------------------------------------------------

    if (WiFi.status() == WL_CONNECTED)
    {
      digitalWrite(LED_BUILTIN, HIGH);
      HTTPClient http;
      http.begin("http://192.168.4.1:80/gateway?");
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      String postData = "parametro1=modulo6&parametro2={\"modulo\":\"M2P_006\",\"timestamp\":\"" + time.timestamp() + "\",\"sensor1\":\"" + String(temp[0]) + "\",\"id1\":\"1\",\"sensor2\":\"" + String(temp[1]) + "\",\"id2\":\"2\",\"sensor3\":\"" + String(temp[2]) + "\",\"id3\":\"3\",\"sensor4\":\"" + String(temp[3]) + "\",\"id4\":\"4\",\"sensor5\":\"" + String(temp[4]) + "\",\"id5\":\"5\",\"sensor6\":\"" + String(tempamb) + "\",\"id6\":\"6\",\"sensor7\":\"" + String(hum) + "\",\"id7\":\"7\",\"sensor8\":\"" + String(Irms1) + "\",\"id8\":\"8\",\"sensor9\":\"" + String(Irms2) + "\",\"id9\":\"9\",\"sensor10\":\"" + String(Irms3) + "\",\"id10\":\"10\",\"sensor11\":\"" + String(in_State1) + "\",\"id11\":\"11\",\"sensor12\":\"" + String(in_State2) + "\",\"id12\":\"12\"}"; // Datos que deseas enviar en el POST
      int httpResponseCode = http.POST(postData);
      delay(10);
      digitalWrite(LED_BUILTIN, LOW);
    }
    else
    {
      connectToWifi();
    }

  }
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  // Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    // Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    // Serial.println("File written");
  }
  else
  {
    // Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  // Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    return;
  }
  if (file.print(message))
  {
    // Serial.println("Message appended");
  }
  else
  {
    // Serial.println("Append failed");
  }
  file.close();
}

void openFile(fs::FS &fs, const char *path)
{
  // Serial.printf("Open file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    writeFile(SD, path, "Hour, Temp1, Temp2, TempAmb, Humidity, Current1, Current2, In1, In2 \r\n");
    delay(1);
    return;
  }
  file.close();
}

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin("M2P_Test", "tesla640");
  //WiFi.begin("TESLA LIMITADA", "sp2PxwdwQ3mx");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    k++;
    if (k == 10)
    {
      esp_restart();
    }
  }
  Serial.println("");
}

/*
void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
  u++;
  if (u == 5)
  {
    esp_restart();
  }
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
#if USING_CORE_ESP32_CORE_V200_PLUS

  case ARDUINO_EVENT_WIFI_READY:
    Serial.println("WiFi ready");
    break;

  case ARDUINO_EVENT_WIFI_STA_START:
    Serial.println("WiFi STA starting");
    break;

  case ARDUINO_EVENT_WIFI_STA_CONNECTED:
    Serial.println("WiFi STA connected");
    break;

  case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
  case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    connectToMqtt();
    break;

  case ARDUINO_EVENT_WIFI_STA_LOST_IP:
    Serial.println("WiFi lost IP");
    break;

  case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    xTimerStart(wifiReconnectTimer, 0);
    break;
#else

  case SYSTEM_EVENT_STA_GOT_IP:
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    connectToMqtt();
    break;

  case SYSTEM_EVENT_STA_DISCONNECTED:
    Serial.println("WiFi lost connection");
    xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
    xTimerStart(wifiReconnectTimer, 0);
    break;
#endif

  default:
    break;
  }
}
*/

void printSeparationLine()
{
  Serial.println("************************************************");
}

/*
void onMqttConnect(bool sessionPresent)
{
  Serial.print("Connected to MQTT broker: ");
  Serial.print(MQTT_HOST);
  Serial.print(", port: ");
  Serial.println(MQTT_PORT);
  Serial.print("PubTopic: ");
  Serial.println(PubTopic);

  printSeparationLine();
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  xSemaphoreGive(bin_sem);
  return;
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason)
{

  Serial.printf("Disconnected from MQTT: %u\n", static_cast<std::underlying_type<AsyncMqttClientDisconnectReason>::type>(reason));

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(const uint16_t &packetId, const uint8_t &qos)
{
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(const uint16_t &packetId)
{
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char *topic, char *payload, const AsyncMqttClientMessageProperties &properties,
                   const size_t &len, const size_t &index, const size_t &total)
{
  (void)payload;

  Serial.println("Publish received.");
  Serial.print("  topic: ");
  Serial.println(topic);
  Serial.print("  qos: ");
  Serial.println(properties.qos);
  Serial.print("  dup: ");
  Serial.println(properties.dup);
  Serial.print("  retain: ");
  Serial.println(properties.retain);
  Serial.print("  len: ");
  Serial.println(len);
  Serial.print("  index: ");
  Serial.println(index);
  Serial.print("  total: ");
  Serial.println(total);
}

void onMqttPublish(const uint16_t &packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  xSemaphoreGive(bin_sem);
  return;
}
*/

void IRAM_ATTR isr() {
  period = 0;
}