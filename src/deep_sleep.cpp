#define LED_BUILTIN 2

//WiFi & HTTP Post
#include <WiFi.h>
#include <HTTPClient.h>
const char* ssid = "";
const char* password = "";
const int wifitimeout = 2000;
/* API YD3GYAGK2SG2ZDKY / KNDQOPULOB532M03 */
String apiKey = "453OP6IXES3VEVOS";
const char *serverName = "http://api.thingspeak.com/update"; /*"https://iotesla.herokuapp.com/datos/"*/

//RTC
#include "RTClib.h"
RTC_DS1307 rtc;

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

// ADS1015/Current Sensor
#include <ADS1X15.h>

// DS18B20 Sensor
#include <OneWire.h>
#include <DallasTemperature.h>
uint8_t sensor1[8] = {0};
uint8_t sensor2[8] = {0};
uint8_t sensor3[8] = {0};
uint8_t sensor4[8] = {0};
uint8_t sensor5[8] = {0};

// Current
int sum = 0;
int current = 1;
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
OneWire oneWire(33);
DallasTemperature sensors(&oneWire);
ADS1015 ADS1(0x48);
ADS1015 ADS2(0x49);

//Funciones
void httppos();
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void openFile(fs::FS &fs, const char *path);

void setup()
{
  Serial.begin(115200);
  delay(1);

  SD.begin(5);
  delay(1);

  Wire.begin();
  Wire.setClock(400000);
  delay(1);

  ADS1.begin();
  ADS1.setGain(4);
  ADS1.setDataRate(7);
  delay(1);

  ADS2.begin();
  ADS2.setGain(4);
  ADS2.setDataRate(7);
  delay(1);

  sensors.begin();
  delay(1);

  dht.begin();
  delay(1);

  rtc.begin();
  delay(1);

  pinMode(LED_BUILTIN, OUTPUT);
  delay(1);

///////////////////////////////////////////// FUNCIONES ////////////////////////////////////////////////////
  
  //-Temperatura--------------------------------------------------------------------

  sensors.requestTemperatures();

  if (sensor1[0] != 0 && sensor1[1] != 0)
  {
    float tempsensor1 = sensors.getTempC(sensor1);
  }
  else
  {
    float tempsensor1 = -127;
  }

  if (sensor2[0] != 0 && sensor2[1] != 0)
  {
    float tempsensor2 = sensors.getTempC(sensor2);
  }
  else
  {
    float tempsensor2 = -127;
  }

  if (sensor3[0] != 0 && sensor3[1] != 0)
  {
    float tempsensor3 = sensors.getTempC(sensor3);
  }
  else
  {
    float tempsensor3 = -127;
  }

  if (sensor4[0] != 0 && sensor4[1] != 0)
  {
    float tempsensor4 = sensors.getTempC(sensor4);
  }
  else
  {
    float tempsensor4 = -127;
  }

  if (sensor5[0] != 0 && sensor5[1] != 0)
  {
    float tempsensor5 = sensors.getTempC(sensor5);
  }
  else
  {
    float tempsensor5 = -127;
  }

  //-Humedad--------------------------------------------------------------------------

  float hum = dht.readHumidity();
  float tempamb = dht.readTemperature();

  //-Corriente------------------------------------------------------------------------

  if (current >= 1)
  {
    for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
    {
      I1[j] = ADS1.readADC_Differential_0_1(); // set element at location i
    }
    if (current >= 2)
    {
      for (int j = 0; j < 1000; ++j) // initialize elements of array n to 0
      {
        I2[j] = ADS1.readADC_Differential_2_3(); // set element at location i
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

  //-SD------------------------------------------------------------------------

  digitalWrite(LED_BUILTIN, HIGH);
  DateTime time = rtc.now();
  //allsensor = temps1 + "," + temps2 + "," + temps3 + "," + temps4 + "," + tamp + "," + h;
  //datalog = time.timestamp() + "," + allsensor + "\r\n";
  filename = "/" + time.timestamp(DateTime::TIMESTAMP_DATE) + ".csv";
  path = filename.c_str();
  openFile(SD, path);
  appendFile(SD, path, datalog.c_str());
  digitalWrite(LED_BUILTIN, LOW);

  //-HTTP-----------------------------------------------------------------------
  
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

    http.begin(client, serverName);

    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    //String httpRequestData = "api_key=" + apiKey + "&field1=" + temps1 + "&field2=" + temps2 + "&field3=" + temps3 + "&field4=" + temps4 + "&field5=" + tamp + "&field6=" + h;
    //int httpResponseCode = http.POST(httpRequestData);

    /*
    http.begin(serverName);

    http.addHeader("Content-Type", "application/json");

    String Modulo = "M2P_001_23";
    String httpRequestData = "{\"dato\":{\"modulo\":\"" + Modulo + "\",\"sensor1\":\"" + temps1 + "\",\"sensor2\":\"" + temps2 + "\",\"sensor3\":\"" + temps3 + "\",\"sensor4\":\"" + temps4 + "\",\"sensor6\":\"" + tamp + "\",\"sensor7\":\"" + h + "\"}}";
    int httpResponseCode = http.POST(httpRequestData);
    */

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
    writeFile(SD, path, "Hour, Temp1, Temp2, Temp3, Temp4, TempAmb, Humidity \r\n");
    delay(1);
    return;
  }
  file.close();
}