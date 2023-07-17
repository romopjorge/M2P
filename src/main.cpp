#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include "RTClib.h"

RTC_DS1307 rtc;

/*
#include "ESP32_MailClient.h"
SMTPData datosSMTP;
int correo(String alerttext);
void alertmail();


//MQ135
#include <MQUnifiedsensor.h>
#define Voltage_Resolution 3.3
double CO2;
MQUnifiedsensor MQ135("ESP-32", Voltage_Resolution, 12, 34, "MQ-135");
*/

// SD
#include <SPI.h>
#include <SD.h>
#include <ESP32Time.h>

// Humidity Sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#define DHTPIN 25
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
String h = "";
String tamp = "";
int humidity = 1;

// ADS1015/Current Sensor
#include <ADS1X15.h>

// DS18B20 Sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// SH1106 OLED Screen
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
Adafruit_SH1106 display(21, 22);

#define LED_BUILTIN 2

// Funciones
void serialandwrite();
void wifiload();
void serialEvent();
void tempload();
void screen(float temp1);
void currentsensor();
void humiditysensor();
void currentload();
void httppos();
void rtctime();
void airquality();
void writeFile(fs::FS &fs, const char *path, const char *message);
void appendFile(fs::FS &fs, const char *path, const char *message);
void openFile(fs::FS &fs, const char *path);
uint8_t findDevices();
void leertemperatura(float &tempsensor1, float &tempsensor2, float &tempsensor3, float &tempsensor4, float &tempsensor5);
void pressure();

// OLED Screen Variables
String temps1;
String temps2;
String temps3;
String temps4;
String temps5;
float temp1;
float tempmax;
float tempmin;
bool screenon = false;

// WiFi Variables
bool wifisearch = false;
bool wifion = false;
 
String ssid = "";
String password = "";
const int wifiTimeout = 2000;

// RTC
bool rtc_flag = false;

// SD Name File
String filename = "";
const char *path = "";
String datalog = "";
bool datalogon = false;

// Data Variables
bool stringComplete = false;
String inputString = "";
String httpRequestData;

// DS18B20 Sensor Variables
bool tempsearch = false;
String tempsensors[6];
String day = "";

float tempsensor1;
float tempsensor2;
float tempsensor3;
float tempsensor4;
float tempsensor5;

uint8_t sensor1[8] = {0};
uint8_t sensor2[8] = {0};
uint8_t sensor3[8] = {0};
uint8_t sensor4[8] = {0};
uint8_t sensor5[8] = {0};

uint8_t countt = {};

// Current Sensor Variables
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

// Presion
#define SDA_1 21
#define SCL_1 22
#define SENSOR_ADDRESS 0x6D
#define SENSOR_REG 0x06
float fadc;
float druck;
bool pressureon = false;

/* API YD3GYAGK2SG2ZDKY / KNDQOPULOB532M03 */
String apiKey = "453OP6IXES3VEVOS";
const char *serverName = "https://iotesla.herokuapp.com/datos/"; /*"http://api.thingspeak.com/update"*/

// Instancias
Preferences preferences;
OneWire oneWire(33);
DallasTemperature sensors(&oneWire);
ADS1015 ADS1(0x48);
ADS1015 ADS2(0x49);

// Tesla LTDA Logo
static const unsigned char PROGMEM tesla[] = {
    0xff, 0x6c, 0xff, 0x6c, 0x00, 0x6c, 0xff, 0x6c, 0xff, 0x6c, 0x00, 0x6c, 0xd8, 0x6c, 0xd8, 0x6c,
    0xd8, 0x00, 0xdb, 0xfc, 0xdb, 0xfc, 0xd8, 0x00, 0xdb, 0xfc, 0xdb, 0xfc};

// OLED Text
String texto = "";

// OLED Mode
int mode = 0;

// Time Variables
int lastrefresh = 0;
int lastsensor = 0;
int lastpressure = 0;
int lastcurrent = 0;
int lastdatalog = 0;
// int lasttext1 = 0;
// int lasttext2 = 10000;
// int lasttext3 = 15000;
// const int timerDelay = 20000;
int timerrefresh = 20000;
int sensorrefresh = 10000;
// int pressurerefresh = 10000;
int currentrefresh = 5000;
int datalogrefresh = 15000;
int minutes = 5;

// HTTP
bool httpon = false;
String allsensor = "";

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

  /*
  ADS2.begin();
  ADS2.setGain(4);
  ADS2.setDataRate(7);
  delay(1);


  display.begin(SH1106_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(1);

  display.clearDisplay();
  display.setTextColor(WHITE);
  delay(1);

  // logo tesla
  display.drawBitmap(0, 50, tesla, 14, 64, 1);
  display.setTextSize(1);
  display.setCursor(16, 57);
  display.print("Tesla");
  display.setTextSize(1);
  display.setCursor(16, 27);
  display.print("Inicializando");
  display.display();
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

  wifiload();
  delay(1);

  /*
    delay(100);
    WiFi.begin(ssid, password);
    delay(100);

    //Set math model to calculate the PPM concentration and the value of constants
    MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b
    MQ135.init();
    float calcR0 = 0;
    for(int i = 1; i<=10; i ++)
    {
      MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin
      calcR0 += MQ135.calibrate(3.6);
    }
    MQ135.setR0(calcR0/10);
   // MQ CAlibration
    MQ135.serialDebug(false);
    */
}

void loop()
{

  if (stringComplete == true)
  {
    inputString.trim();
    serialandwrite();
    inputString = "";
    stringComplete = false;
  }

  /*if (wifisearch == false)
  {
    wificonnect();
    wifisearch = true;
  }*/

  if (tempsearch == false)
  {
    tempload();
    tempsearch = true;
  }

  if (currentact == false)
  {
    currentload();
    currentact = true;
  }

  /*
    if ((WiFi.status() == WL_CONNECTED) && (rtchour == false))
    {
      rtctime();
      rtchour = true;
      filename = "/" + time.timestamp() + ".csv";
      path = filename.c_str();
      openFile(SD, path);
    }
    */

    if ((millis() - lastcurrent) >= currentrefresh)
    {
      lastcurrent = millis();
      currentsensor();
      if (currenton == false)
      {
        currentrefresh = 60000 * minutes;
        currenton = true;
      }
    }

    /*
    if ((millis() - lastpressure) >= pressurerefresh)
    {
      lastpressure = millis();
      pressure();
      if (pressureon == false)
      {
        pressurerefresh = 30000;
        pressureon = true;
      }
    }
  */

  if ((millis() - lastsensor) >= sensorrefresh)
  {
    lastsensor = millis();
    leertemperatura(tempsensor1, tempsensor2, tempsensor3, tempsensor4, tempsensor5);
    delay(1);
    humiditysensor();
    delay(1);
    // airquality();
    temps1 = String(tempsensor1);
    temps2 = String(tempsensor2);
    temps3 = String(tempsensor3);
    temps4 = String(tempsensor4);
    temps5 = String(tempsensor5);
    if (screenon == false)
    {
      // tempmin = tempsensor1;
      sensorrefresh = 60000 * minutes;
      screenon = true;
    }
    delay(1);

    /*
    if (day != rtc.getTime("%d") && rtchour == true)
      {
        day = rtc.getTime("%d");
        tempmax = tempsensor1;
        tempmin = tempsensor1;
      }
    alertmail();
    */
  }

  if ((millis() - lastrefresh) >= timerrefresh)
  {
    lastrefresh = millis();
    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED && millis() - lastrefresh < wifiTimeout)
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
    if (httpon == false)
    {
      timerrefresh = 60000 * minutes;
      httpon = true;
    }
  }

  if ((millis() - lastdatalog) >= datalogrefresh) // && rtc_flag == true)
  {
    lastdatalog = millis();
    digitalWrite(LED_BUILTIN, HIGH);
    DateTime time = rtc.now();
    allsensor = temps1 + "," + temps2 + "," + temps3 + "," + temps4 + "," + temps5 + "," + tamp + "," + h;
    datalog = time.timestamp() + "," + allsensor + "\r\n";
    filename = "/" + time.timestamp(DateTime::TIMESTAMP_DATE) + ".csv";
    path = filename.c_str();
    openFile(SD, path);
    appendFile(SD, path, datalog.c_str());
    if (datalogon == false)
    {
      datalogrefresh = 60000 * minutes;
      datalogon = true;
    }
    digitalWrite(LED_BUILTIN, LOW);
  }

  /*
  if ((millis() - lasttext1) >= timerDelay && mode == 0)
  {
    lasttext1 = millis();
    display.fillRect(0, 0, 128, 50, BLACK);
    display.fillRect(14, 50, 114, 7, BLACK);
    display.display();
    texto = "ACTUAL";
    temp1 = temp;
    screen(temp1);
    mode = 1;
  }

  if ((millis() - lasttext1) >= lasttext2 && mode == 1)
  {
    display.fillRect(0, 0, 128, 50, BLACK);
    display.fillRect(14, 50, 114, 7, BLACK);
    display.display();
    texto = "MAXIMO";
    if (temp > tempmax)
    {
      tempmax = temp;
    }
    temp1 = tempmax;
    screen(temp1);
    mode = 2;
  }

  if ((millis() - lasttext1) >= lasttext3 && mode == 2)
  {
    display.fillRect(0, 0, 128, 50, BLACK);
    display.fillRect(14, 50, 114, 7, BLACK);
    display.display();
    texto = "MINIMO";
    if (temp < tempmin && temp != -127)
    {
      tempmin = temp;
    }
    temp1 = tempmin;
    screen(temp1);
    mode = 0;
  } */
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

void wifiload()
{
  preferences.begin("credentials", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  preferences.end();
}

void serialandwrite()
{
  if (inputString.startsWith("$W"))
  {
    digitalWrite(LED_BUILTIN, HIGH);
    ssid = inputString.substring(2, 34);
    ssid.trim();
    password = inputString.substring(34);
    preferences.begin("credentials", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (inputString.startsWith("$ST"))
  {
    findDevices();
    Serial.println("");
    Serial.flush();
  }
  if (inputString.startsWith("$T"))
  {
    int StringCount = 0;

    for (int i = 0; i < 6; i++)
    {
      tempsensors[i] = "0";
    }

    while (inputString.length() > 0)
    {
      int index = inputString.indexOf('/');
      if (index == -1) // No space found
      {
        tempsensors[StringCount++] = inputString;
        break;
      }
      else
      {
        tempsensors[StringCount++] = inputString.substring(0, index);
        inputString = inputString.substring(index + 1);
      }
    }

    preferences.begin("sensor", false);
    preferences.putString("tempsensor1", tempsensors[1]);
    preferences.putString("tempsensor2", tempsensors[2]);
    preferences.putString("tempsensor3", tempsensors[3]);
    preferences.putString("tempsensor4", tempsensors[4]);
    preferences.putString("tempsensor5", tempsensors[5]);
    preferences.end();
    tempsearch = false;
  }
  if (inputString.startsWith("$C"))
  {
    current = (inputString.substring(2, 3)).toInt();
    factor1 = (inputString.substring(3, 5)).toInt();
    factor2 = (inputString.substring(5, 7)).toInt();
    factor3 = (inputString.substring(7, 9)).toInt();
    preferences.begin("current", false);
    preferences.putInt("current", current);
    preferences.putInt("factor1", factor1);
    preferences.putInt("factor2", factor2);
    preferences.putInt("factor3", factor3);
    preferences.end();
    currentact = false;
  }
}

void tempload()
{
  preferences.begin("sensor", false);
  temps1 = preferences.getString("tempsensor1", "0");
  temps2 = preferences.getString("tempsensor2", "0");
  temps3 = preferences.getString("tempsensor3", "0");
  temps4 = preferences.getString("tempsensor4", "0");
  temps5 = preferences.getString("tempsensor5", "0");
  countt = preferences.getUInt("count", 0);
  preferences.end();

  int i;
  int addrv[8];

  const char *temp1 = temps1.c_str();
  sscanf(temp1, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor1[i] = (__typeof__(sensor1[0]))addrv[i];
  }

  const char *temp2 = temps2.c_str();
  sscanf(temp2, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor2[i] = (__typeof__(sensor2[0]))addrv[i];
  }

  const char *temp3 = temps3.c_str();
  sscanf(temp3, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor3[i] = (__typeof__(sensor3[0]))addrv[i];
  }

  const char *temp4 = temps4.c_str();
  sscanf(temp4, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor4[i] = (__typeof__(sensor4[0]))addrv[i];
  }

  const char *temp5 = temps5.c_str();
  sscanf(temp5, "%x,%x,%x,%x,%x,%x,%x,%x", &addrv[0], &addrv[1], &addrv[2], &addrv[3], &addrv[4], &addrv[5], &addrv[6], &addrv[7]); // parse the 8 ascii hex bytes in 8 ints
  for (i = 0; i < 8; i++)
  {
    sensor5[i] = (__typeof__(sensor5[0]))addrv[i];
  }

  tempsensor1 = 0;
  tempsensor2 = 0;
  tempsensor3 = 0;
  tempsensor4 = 0;
  tempsensor5 = 0;
}

uint8_t findDevices()
{

  uint8_t address[8];
  countt = 0;

  if (oneWire.search(address))
  {
    do
    {
      if (countt > 0)
        Serial.print("/");
      countt++;
      for (uint8_t i = 0; i < 8; i++)
      {
        Serial.print("0x");
        if (address[i] < 0x10)
          Serial.print("0");
        Serial.print(address[i], HEX);
        if (i < 7)
          Serial.print(", ");
      }
    } while (oneWire.search(address));
  }
  preferences.begin("sensor", false);
  preferences.putUInt("count", countt);
  preferences.end();
  return countt;
}

void serialEvent()
{
  while (Serial.available())
  {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n')
    {
      stringComplete = true;
    }
  }
}

void leertemperatura(float &tempsensor1, float &tempsensor2, float &tempsensor3, float &tempsensor4, float &tempsensor5)
{
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
}

void screen(float temp1)
{
  String L;
  display.setTextSize(2);
  display.setCursor(29, 4);
  display.print(texto);
  display.setTextSize(3);
  L = String(temp1);
  switch (L.length())
  {
  case 4:
    display.setCursor(36, 24);
    break;
  case 5:
    display.setCursor(18, 24);
    break;
  case 6:
    display.setCursor(0, 24);
    break;
  }
  display.print(temp1);
  display.drawCircle(111, 34, 2, 1);
  display.setCursor(116, 32);
  display.setTextSize(2);
  display.print("C");
  display.display();
}

void currentload()
{
  preferences.begin("current", false);
  current = preferences.getInt("current", {});
  factor1 = preferences.getInt("factor1", {});
  factor2 = preferences.getInt("factor2", {});
  factor3 = preferences.getInt("factor3", {});
  preferences.end();
}

void currentsensor()
{
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
}

void humiditysensor()
{
  if (humidity == 1)
  {
    h = String(dht.readHumidity());
    tamp = String(dht.readTemperature());
  }
  else
  {
    h = "null";
    tamp = "null";
  }
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
    String httpRequestData = "api_key=" + apiKey + "&field1=" + temps1 + "&field2=" + temps2 + "&field3=" + temps3 + "&field4=" + temps4 + "&field5=" + tamp + "&field6=" + h;
    int httpResponseCode = http.POST(httpRequestData);
    Serial.println(httpResponseCode);
    */

    http.begin(serverName);

    http.addHeader("Content-Type", "application/json");

    String Modulo = "M2P_001_23";
    String httpRequestData = "{\"dato\":{\"modulo\":\"" + Modulo + "\",\"sensor1\":\"" + temps1 + "\",\"sensor2\":\"" + temps2 + "\",\"sensor3\":\"" + temps3 + "\",\"sensor4\":\"" + temps4 + "\",\"sensor5\":\"" + temps5 + "\",\"sensor6\":\"" + tamp + "\",\"sensor7\":\"" + h + "\"}}";
    int httpResponseCode = http.POST(httpRequestData);

  }
  else
  {
    Serial.println("WiFi Disconnected");
  }
}

/*
void rtctime()
{
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    rtc.setTimeStruct(timeinfo);
    rtchour = true;
  }
  else
  {
    return;
  }
}
*/

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
    writeFile(SD, path, "Hour, Temp1, Temp2, Temp3, Temp4, Temp5, TempAmb, Humidity \r\n");
    delay(1);
    return;
  }
  file.close();
}

/*

int correo(String alerttext){
delay(1);
//Configuración del servidor de correo electrónico SMTP, host, puerto, cuenta y contraseña
datosSMTP.setLogin("smtp.gmail.com", 465, "iotesla.cesfam@gmail.com", "kgmnapevifgpdder");
// Establecer el nombre del remitente y el correo electrónico
datosSMTP.setSender("IoTesla", "iotesla.cesfam@gmail.com");
// Establezca la prioridad o importancia del correo electrónico High, Normal, Low o 1 a 5 (1 es el más alto)
datosSMTP.setPriority(1);
// Establecer el asunto
datosSMTP.setSubject("Temperatura/s fuera de rango");
// Establece el mensaje de correo electrónico en formato de texto (sin formato)
datosSMTP.setMessage(alerttext, true);
// Agregar destinatarios, se puede agregar más de un destinatario
datosSMTP.addRecipient("romojorge20@gmail.com");
 //Comience a enviar correo electrónico.
if (!MailClient.sendMail(datosSMTP))
//Serial.println("Error enviando el correo, " + MailClient.smtpErrorReason());
//Borrar todos los datos del objeto datosSMTP para liberar memoria
datosSMTP.empty();
delay(1);
return 0;
}

void alertmail(){
String alerttext = "";
Serial.println(ssid);
alerttext = "";
  if((tempsensor1 >= 8 && tempsensor1 < 85) || (tempsensor1 <= 2 && tempsensor1 > -127)){
    alerttext += "<p>La temperatura en el Refrigerador 1 es de " + temps1 + "°C</p>";
  }
  if((tempsensor2 >= 8 && tempsensor2 < 85) || (tempsensor2 <= 2 && tempsensor2 > -127)){
    alerttext += "<p>La temperatura en el Refrigerador 2 es de " + temps2 + "°C</p>";
  }
  if((tempsensor3 >= 8 && tempsensor3 < 85) || (tempsensor3 <= 2 && tempsensor3 > -127)){
    alerttext += "<p>La temperatura en el Refrigerador 3 es de " + temps3 + "°C</p>";
  }
  if((tempsensor4 >= 8 && tempsensor4 < 85) || (tempsensor4 <= 2 && tempsensor4 > -127)){
    alerttext += "<p>La temperatura en el Refrigerador 4 es de " + temps4 + "°C</p>";
  }
  if((tempsensor5 >= 8 && tempsensor5 < 85) || (tempsensor5 <= 2 && tempsensor5 > -127)){
    alerttext += "<p>La temperatura en el Refrigerador 5 es de " + temps5 + "°C</p>";
  }
  Serial.println(alerttext);
  if(alerttext.length()!=0){
    correo(alerttext);
  }
}

void pressure(){
  uint32_t low, med, high;
  uint32_t wert;
  Wire.beginTransmission(SENSOR_ADDRESS);
  Wire.write(SENSOR_REG);
  Wire.endTransmission(false);
  Wire.requestFrom(SENSOR_ADDRESS, 3);
  if(Wire.available() == 3 )
  {
    high = Wire.read();
    med = Wire.read();
    low = Wire.read();

    wert = (high<<16) | (med<<8) | low;
  }
  Wire.endTransmission();

  if(wert & 0x800000)
  {
  fadc = wert - 16777216.0;
  }
  else
  {
  fadc = wert;
  }
  druck = 20 * (((3.3 * wert / 8388608.0)-0.5) / 2.0) * 14.503773773;
}

void airquality(){
  MQ135.update();
  MQ135.setA(110.47); MQ135.setB(-2.862);
  CO2 = analogRead(34);
  int co2ppm = map(CO2,250,1023,850,6000);
  Serial.println(co2ppm);
}

*/