#include <Arduino.h>

#include <HTTPClient.h>

#include <iostream>

#include "DHTesp.h"

#include <WiFi.h>

#include <LiquidCrystal_I2C.h>

using namespace std;

#include <ArduinoJson.h>

const char* ssid = "Wokwi-GUEST";

const char* password = "";

const int deviceId=4;





#define I2C_ADDR 0x27

#define LCD_COLUMNS 16

#define LCD_LINES 2



const int DHT_PIN = 15;

bool rangesExist=false;
double maxTemp;
double minTemp;
double maxHum;
double minHum;
int plantId;

DHTesp dhtSensor;



LiquidCrystal_I2C lcd(I2C_ADDR, LCD_COLUMNS, LCD_LINES);



void setup() {

  



  Serial.begin(115200);



 WiFi.begin(ssid,password);

  while(WiFi.status()!=WL_CONNECTED){

  delay(500);

  Serial.println("Conectandote a Wifi...");

  }

  Serial.println("Conectado a la red wifi...");

  
 dhtSensor.setup(DHT_PIN, DHTesp::DHT22);

 lcd.init();

 lcd.backlight();



}



void loop() {

 TempAndHumidity data = dhtSensor.getTempAndHumidity();

  Serial.println("Temp: " + String(data.temperature, 1) + "°C");

  Serial.println("Humidity: " + String(data.humidity, 1) + "%");

  Serial.println("---");
 lcd.setCursor(0, 0);

 lcd.print(" Temp: " + String(data.temperature, 1) + "\xDF"+"C ");

 lcd.setCursor(0, 1);

 lcd.print(" Humidity: " + String(data.humidity, 1) + "% ");
 

 lcd.print("Wokwi Online IoT");
  

  bool active=false;
  bool activeNotification=false;
  bool activeRealTimeData=false;
  string cropName;
  double planTemperature;
  int accountId;
  int cropId;
  HTTPClient http1;
  http1.begin("http://nifty-jet-404014.rj.r.appspot.com/api/v1/devices/"+String(deviceId));
  int httpResponseCode1=http1.GET();
  if (httpResponseCode1 > 0) {
      String response = http1.getString();
      Serial.print("HTTP GET exitoso del device, respuesta del servidor:");
      Serial.println(response);
      DynamicJsonDocument deviceData(512);
      deserializeJson(deviceData, response);
      active=deviceData["active"];
      activeNotification=deviceData["activeNotification"];
      activeRealTimeData=deviceData["activeRealTimeData"];
      accountId=deviceData["farmerId"];
      cropId=deviceData["cropId"];
      const char* cropNameCStr = deviceData["cropName"];
      cropName = std::string(cropNameCStr);
      //---------------------
      if(!rangesExist){
        HTTPClient http0;
        http0.begin("http://nifty-jet-404014.rj.r.appspot.com/api/v1/crops/getCrop/"+String(cropId));
        int httpResponseCode0=http0.GET();
        if(httpResponseCode0>0){
          String response = http0.getString();
          
          Serial.print("HTTP GET exitoso obtener el crop, respuesta del servidor:");
          Serial.println(response);
          DynamicJsonDocument cropData(512);
          deserializeJson(cropData, response);
          plantId=cropData["plantId"];
          http0.begin("https://nifty-jet-404014.rj.r.appspot.com/api/v1/plant/ranges/"+String(plantId));
          int newResponseCode=http0.GET();
          if(newResponseCode){
            String response=http0.getString();
            DynamicJsonDocument rangesData(512);
            deserializeJson(rangesData, response);
            maxTemp=rangesData["maxTemperature"];
            minTemp=rangesData["minTemperature"];
            maxHum=rangesData["maxHumidity"];
            minHum=rangesData["minHumidity"];
            rangesExist=true;
          }
          else{
            Serial.print("HTTP GET fracasó al obtener el crop, respuesta del servidor:");
            Serial.println(newResponseCode);
          }
          
        }
      else{
        Serial.print("La solicitud fracasó, respuesta del servidor: ");
        Serial.println(httpResponseCode0);
      }
  }
    } else {
      Serial.print("Error en la solicitud HTTP GET, código de respuesta: ");
      Serial.println(httpResponseCode1);
    }

    http1.end();
  /*antes de enviar periodicamente info, if(GetActiveRealTomeByDeviceId()) para evitar enviar info a cada rato */
if(activeRealTimeData){
  HTTPClient http2;
  http2.begin("http://nifty-jet-404014.rj.r.appspot.com/api/v1/devices/temperature");
  StaticJsonDocument<200> sendTemperature;
  sendTemperature["deviceId"]=deviceId;
  sendTemperature["temperature"]=data.temperature;
  sendTemperature["humidity"]=data.humidity;

  String send;
  serializeJson(sendTemperature,send);
  Serial.println(send);
  http2.addHeader("Content-Type", "application/json");
  int httpResponseCode2=http2.POST(send);
  if(httpResponseCode2>0){
    Serial.print("HTTP POST exitoso, temperatura y humedad enviadas");
    Serial.println(httpResponseCode2);
  }
  else{
    Serial.print("Error en la solicitud HTTP POST a temperatura y humedad, código de respuesta: ");
      Serial.println(httpResponseCode1);
  }
  http2.end();
}

  if ((data.temperature < minTemp || data.temperature > maxTemp || data.humidity<minHum||data.humidity>maxHum)&&activeNotification&&rangesExist) {
    
 StaticJsonDocument<200> jsonDoc;

std::string message = cropName + " temperature or humidity out of range";


 jsonDoc["message"] =message; // Puedes personalizar el mensaje

 jsonDoc["imageUrl"] = "https://us.123rf.com/450wm/alonastep/alonastep1510/alonastep151000068/47434084-advertencia-de-peligro-muestra-de-la-atenci%C3%B3n-icono-en-un-tri%C3%A1ngulo-amarillo-con-el-s%C3%ADmbolo-de.jpg?ver=6"; // Puedes ajustar la URL de la imagen

 jsonDoc["notificationType"] = "crop"; // Puedes especificar el tipo de notificación

 jsonDoc["date"] = "03/11/2023 21:14:49"; // Puedes establecer la fecha

 jsonDoc["toAccountId"] = accountId; // Puedes ajustar el destinatario de la notificación

 jsonDoc["plantId"] = plantId; // Puedes establecer el ID de la planta

 jsonDoc["fromAccountId"] = 0; 

  

  String jsonString;

 serializeJson(jsonDoc, jsonString);

  if (WiFi.status() == WL_CONNECTED) {

 HTTPClient http;

 http.begin("http://nifty-jet-404014.rj.r.appspot.com/api/v1/notifications");

 http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(jsonString);
  String response = http.getString();
  if (httpResponseCode > 0) {

  Serial.println("Respuesta del servidor:");

  Serial.println(response);

  } else {

  Serial.print("Error en la solicitud HTTP POST, código de respuesta: ");

  Serial.println(httpResponseCode);

  }
  }

  Serial.println("Saliéndose de los rangos, peligro");

  }

  delay(2000);

  }
