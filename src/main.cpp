#define LED_BUILTIN 2

//WiFi & HTTP Post
#include <WiFi.h>
#include <WiFiUdp.h>
#include <HTTPClient.h>
const char* ssid = "TESLA LIMITADA";
const char* password = "sp2PxwdwQ3mx";
const int wifitimeout = 2000;
/* API YD3GYAGK2SG2ZDKY / KNDQOPULOB532M03 */
String apiKey = "453OP6IXES3VEVOS";
const char *serverName = "https://iotesla.herokuapp.com/datos/"; /*"http://api.thingspeak.com/update"*/

//MQTT
extern "C" {
  #include "freertos/FreeRTOS.h"
  #include "freertos/timers.h"
  #include "freertos/semphr.h"
}
#include <AsyncMqttClient.h>

#define MQTT_HOST IPAddress(146, 190, 57, 0)
#define MQTT_PORT 1883
const char* username_mqtt = "iotesla";
const char* password_mqtt = "t3s1a2o2e";
const uint16_t keepalive_mqtt = 15;

const char *PubTopic  = "iotesla/modulo1/receiver";

AsyncMqttClient mqttClient;
TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;
static SemaphoreHandle_t bin_sem; 
static SemaphoreHandle_t sd_sem; 

//RTC
#include "RTClib.h"
#include "NTPClient.h"
RTC_DS1307 rtc;
const long utcOffsetInSeconds = -3 * 3600;
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
#define DHTPIN 25
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
float hum;
float tempamb;

// ADS1015/Current Sensor
#include <ADS1X15.h>

// DS18B20 Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
uint8_t sensor1[8] = {0x28, 0x51, 0x33, 0x15, 0x00, 0x00, 0x00, 0x74};
uint8_t sensor2[8] = {0x28, 0x4D, 0x03, 0x15, 0x00, 0x00, 0x00, 0xD6};
uint8_t sensor3[8] = {0x28, 0x1C, 0x06, 0x15, 0x00, 0x00, 0x00, 0x1D};
uint8_t sensor4[8] = {0};
uint8_t sensor5[8] = {0};
float tempsensor1;
float tempsensor2;
float tempsensor3;
float tempsensor4;
float tempsensor5;

// Current
int sum = 0;
int current = 2;
bool currenton = false;
bool currentact = false;
int factor1 = 50;
int factor2 = 50;
int factor3 = 0;
String Irms1;
int I1[1000];
String Irms2;
int I2[1000];
String Irms3;
int I3[1000];

// Instancias
OneWire oneWire(33);
DallasTemperature sensors(&oneWire);
ADS1015 ADS1(0x48);
ADS1015 ADS2(0x49);

//Funciones
void httppos();
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void openFile(fs::FS &fs, const char *path);
void onMqttConnect(bool sessionPresent);
void connectToWifi();
void connectToMqtt();
void WiFiEvent(WiFiEvent_t event);
void printSeparationLine();
void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
void onMqttSubscribe(const uint16_t& packetId, const uint8_t& qos);
void onMqttUnsubscribe(const uint16_t& packetId);
void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total);
void onMqttPublish(const uint16_t& packetId);


float battery;
int minutes = 1;

//Entradas
const int in_Pin = 32; 
int in_State = 0;

//RTC Memory
RTC_DATA_ATTR int updt_day = 0;

void setup()
{
  Serial.begin(115200);
  delay(1);

  //SD.begin(5);
  delay(1);

  Wire.begin();
  Wire.setClock(400000);
  delay(1);

  ADS1.begin();
  ADS1.setGain(4);
  ADS1.setDataRate(7);
  delay(1);

  /*
  ADS2.begin();
  ADS2.setGain(4);
  ADS2.setDataRate(7);
  delay(1);
  */

  sensors.begin();
  delay(1);

  dht.begin();
  delay(1);

  rtc.begin();
  delay(1);

  pinMode(LED_BUILTIN, OUTPUT);
  delay(1);

  pinMode(in_Pin, INPUT);
  delay(1);

  bin_sem = xSemaphoreCreateBinary();
  sd_sem = xSemaphoreCreateMutex();

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
                                    reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0,
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

///////////////////////////////////////////// FUNCIONES ////////////////////////////////////////////////////

  //-WiFi---------------------------------------------------------------------------

  connectToWifi();

  //-Temperatura--------------------------------------------------------------------

  sensors.requestTemperatures();

  if (sensor1[0] != 0 && sensor1[1] != 0)
  {
    tempsensor1 = sensors.getTempC(sensor1);
  }
  else
  {
    tempsensor1 = -127;
  }

  if (sensor2[0] != 0 && sensor2[1] != 0)
  {
    tempsensor2 = sensors.getTempC(sensor2);
  }
  else
  {
    tempsensor2 = -127;
  }

  if (sensor3[0] != 0 && sensor3[1] != 0)
  {
    tempsensor3 = sensors.getTempC(sensor3);
  }
  else
  {
    tempsensor3 = -127;
  }

  if (sensor4[0] != 0 && sensor4[1] != 0)
  {
    tempsensor4 = sensors.getTempC(sensor4);
  }
  else
  {
    tempsensor4 = -127;
  }

  if (sensor5[0] != 0 && sensor5[1] != 0)
  {
    tempsensor5 = sensors.getTempC(sensor5);
  }
  else
  {
    tempsensor5 = -127;
  }

  //-Entradas-------------------------------------------------------------------------

  in_State = digitalRead(in_Pin);

  //-Humedad--------------------------------------------------------------------------

  hum = dht.readHumidity();
  tempamb = dht.readTemperature();

  //-Corriente------------------------------------------------------------------------

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
        Irms3 = "null";
      }
    }
    else
    {
      Irms2 = "null";
      Irms3 = "null";
    }
  }
  else
  {
    Irms1 = "null";
    Irms2 = "null";
    Irms3 = "null";
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

  //-RTC----------------------------------------------------------------------------



  //-SD------------------------------------------------------------------------
  
  xSemaphoreTake(sd_sem,portMAX_DELAY);
  digitalWrite(LED_BUILTIN, HIGH);
  DateTime time = rtc.now();
  String allsensor = String(tempsensor1) + "," + String(tempsensor2) + "," + String(tempsensor3) + "," + String(tempamb) + "," + String(hum) + "," + String(Irms1) + "," + String(Irms2) + "," + String(in_State);
  datalog = time.timestamp() + "," + allsensor + "\r\n";
  filename = "/" + time.timestamp(DateTime::TIMESTAMP_DATE) + ".csv";
  path = filename.c_str();
  openFile(SD, path);
  appendFile(SD, path, datalog.c_str());
  digitalWrite(LED_BUILTIN, LOW);
  xSemaphoreGive(sd_sem);
  
  //-MQTT-----------------------------------------------------------------------

  const int start_http = millis();
  xSemaphoreTake(bin_sem,portMAX_DELAY);
  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    String jsondata = "{\"modulo\":\"M2P_001\",\"sensor1\":\"" + String(tempsensor1) + "\",\"sensor2\":\"" + String(tempsensor2) + "\",\"sensor3\":\"" + String(tempsensor3) + "\",\"sensor6\":\"" + String(tempamb) + "\",\"sensor7\":\"" + String(hum) + "\",\"sensor8\":\"" + String(Irms1) + "\",\"sensor9\":\"" + String(Irms2) + "\"}";
    uint16_t packetIdPub1 = mqttClient.publish(PubTopic, 1, true, jsondata.c_str());  
    Serial.println(packetIdPub1);
    delay(10);
    mqttClient.disconnect();
    WiFi.disconnect();
    digitalWrite(LED_BUILTIN, LOW);
  }
  else
  {
    WiFi.disconnect();
  }

  //-HTTP-----------------------------------------------------------------------
  /*
  WiFi.begin(ssid, password);
  const int start_http = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start_http < wifitimeout)
  {
    delay(500);
  }
    delay(1);
  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    httppos();
    WiFi.disconnect();
    digitalWrite(LED_BUILTIN, LOW);
  }
  else
  {
    WiFi.disconnect();
  }
  */

int time_sleep = minutes * 30000;
int time_running = millis();
esp_sleep_enable_timer_wakeup((time_sleep-time_running)*1000);
esp_deep_sleep_start();

}

void loop()
{

}

void httppos()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClient client;
    HTTPClient http;

    /*
    http.begin(client, serverName);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "api_key=" + apiKey + "&field1=" + String(tempsensor1) + "&field2=" + String(tempamb) + "&field3=" + String(hum) + "&field4=" + String(Irms1) + "&field5=" + String(battery);
    int httpResponseCode = http.POST(httpRequestData);
    */
    
    http.begin(serverName);

    http.addHeader("Content-Type", "application/json");

    String Modulo = "M2P_001_23";
    String httpRequestData = "{\"dato\":{\"modulo\":\"" + Modulo + "\",\"sensor1\":\"" + String(tempsensor1) + "\",\"sensor2\":\"" + String(tempsensor2) + "\",\"sensor3\":\"" + String(tempsensor3) + "\",\"sensor6\":\"" + String(tempamb) + "\",\"sensor7\":\"" + String(hum) + "\",\"sensor8\":\"" + String(Irms1) + "\",\"sensor9\":\"" + String(Irms2) + "\",\"sensor14\":\"" + String(in_State) + "\"}}";
    int httpResponseCode = http.POST(httpRequestData);
    

  }
  else
  {
    Serial.println("WiFi Disconnected");
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
    writeFile(SD, path, "Hour, Temp1, Temp2, Temp3, TempAmb, Humidity, Current1, Current2, Switch \r\n");
    delay(1);
    return;
  }
  file.close();
}

void connectToWifi()
{
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin("#SCA_signal", "SCA@Modern2019");
  //WiFi.begin("TESLA LIMITADA", "sp2PxwdwQ3mx");
}

void connectToMqtt()
{
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
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

void printSeparationLine()
{
  Serial.println("************************************************");
}

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

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  
  Serial.printf("Disconnected from MQTT: %u\n", static_cast<std::underlying_type<AsyncMqttClientDisconnectReason>::type>(reason));

  if (WiFi.isConnected())
  {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(const uint16_t& packetId, const uint8_t& qos)
{
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(const uint16_t& packetId)
{
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void onMqttMessage(char* topic, char* payload, const AsyncMqttClientMessageProperties& properties,
                   const size_t& len, const size_t& index, const size_t& total)
{
  (void) payload;

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

void onMqttPublish(const uint16_t& packetId)
{
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}