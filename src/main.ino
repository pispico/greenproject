/*
 * Project Name: GrowSystem
 * Version     :  V1.1
 * 
 * 
 * Criado por: William Pispico
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Time.h>
#include <TimeAlarms.h>

#include <DHT.h>
#define DHT_DATA_PIN 2 
#define DHTTYPE DHT22
DHT dht(DHT_DATA_PIN, DHTTYPE);

const char* SSID          = "ssid_wifi";
const char* PASSWORD      = "mypass";

const char* MQTT_SERVER   = "mqtt_server_ip";
const int MQTT_PORT       =  1883;
const char* MQTT_USER     = "MQTT_USER";
const char* MQTT_PASSWORD = "MQTT_PASSWORD";
const char* MQTT_CLIENT   = "00001";

const char* TOPICO_ESCUTA  = "topicoEscuta";
const char* TOPICO_ENVIO   = "sensor/temperature";

const int PIN_RELE_1 = 14; // (D5) Ventilador
const int PIN_RELE_2 = 12; // (D6) Luz
const int PIN_RELE_3 = 13; // (D7) Bomba de agua

WiFiClient espClient;
PubSubClient MQTT(espClient);

int RELE1STATUS = 3; //AUTO STATUS
int RELE2STATUS = 6; //AUTO STATUS
int RELE3STATUS = 8; //AUTO STATUS


//Using digital pin to power the sensor. This will prevent corrosion of the sensor as it sits in the soil
int val = 0;         //value for storing moisture value 
int soilPin = A0;    //Declare a variable for the soil moisture sensor 
int soilPower = D2;  //Variable for Soil moisture Power


void reconnectWiFi() {
  if (WiFi.status() == WL_CONNECTED)
    return;
     
  Serial.println("Conectando-se na rede: " + String(SSID));
  Serial.println("Aguarde");
  WiFi.begin(SSID, PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Endereco IP: ");
  Serial.println(WiFi.localIP());
}


void callback(char* topic, byte* payload, unsigned int length) {
  String msg;

  for(int i = 0; i < length; i++){
    char c = (char)payload[i];
    msg += c;
  }


  if (msg.equals("1")) { 
    RELE1STATUS = 1;
  } 
  else if (msg.equals("2")){
    RELE1STATUS = 2;
  }
  else if (msg.equals("3")){
    RELE1STATUS = 3;
  }
  else if (msg.equals("4")){
    RELE2STATUS = 4;
  }
  else if (msg.equals("5")){
    RELE2STATUS = 5;
  }
  else if (msg.equals("6")){
    RELE2STATUS = 6;
  }
  else if (msg.equals("7")){
    RELE3STATUS = 7;
  }
  else if (msg.equals("8")){
    RELE3STATUS = 8;
  }

}

void initMQTT() {
  MQTT.setServer(MQTT_SERVER, MQTT_PORT);
  MQTT.setCallback(callback);
}


void reconnectMQTT() {
    while (!MQTT.connected()) {
      Serial.print("Tentado conectar ao broker MQTT: " + String(MQTT_SERVER) + ":" + String(MQTT_PORT) + " :");
      if (MQTT.connect(MQTT_CLIENT, MQTT_USER, MQTT_PASSWORD)) {
        Serial.println("conectado");
        MQTT.publish(TOPICO_ENVIO, "Cliente conectado ao broker");
        MQTT.subscribe(TOPICO_ESCUTA);
      } 
      else {
        Serial.print("falha, rc=");
        Serial.print(MQTT.state());
        Serial.println(" tenta novamente após 5 segundos");
        delay(5000);
      }
    }
}


void setup() {
  Serial.begin(115200);
  setTime(23,35,0,1,1,17); // set time to Saturday 8:30:00am Jan 1 2017
  Alarm.alarmRepeat(17,00,0,LightsOn);  // 17:00pm every day
  Alarm.alarmRepeat(23,40,0,LightsOff);  // 10:00am every day
  
  pinMode(PIN_RELE_1, OUTPUT);
  pinMode(PIN_RELE_2, OUTPUT);
  pinMode(PIN_RELE_3, OUTPUT);
  digitalWrite(PIN_RELE_1, HIGH);
  digitalWrite(PIN_RELE_2, HIGH);
  digitalWrite(PIN_RELE_3, HIGH);
  
  dht.begin();
  
  pinMode(soilPower, OUTPUT);
  digitalWrite(soilPower, LOW);
  
  reconnectWiFi();
  initMQTT();
}

//Thread time control
long threadmqtt = 0;

void loop() {
  reconnectWiFi();
  reconnectMQTT();
  MQTT.loop();
  
  
  if (RELE1STATUS == 1){
    digitalWrite(PIN_RELE_1, LOW);
  }
  if (RELE1STATUS == 2){
    digitalWrite(PIN_RELE_1, HIGH);
  }
  if (newTemp > 25 && RELE1STATUS == 3){
    digitalWrite(PIN_RELE_1, LOW);
  } 
  if (newTemp < 25 && RELE1STATUS == 3){
    digitalWrite(PIN_RELE_1, HIGH);
  }
  if(RELE2STATUS == 4){
    digitalWrite(PIN_RELE_2, LOW);
  }
  if(RELE2STATUS == 5){
    digitalWrite(PIN_RELE_2, HIGH);
  }
  if(RELE2STATUS == 6){
    //Auto Mode função LightsOn e LightsOff são chamadas por trigger de alarme
    //trigger Alarm.alarmRepeat
  }
  if(RELE3STATUS == 7){
    digitalWrite(PIN_RELE_3, LOW);
    delay(30000);
    digitalWrite(PIN_RELE_3, HIGH);
  }
  
  if(SoilMoisture < 20 && RELE3STATUS == 8){
    digitalWrite(PIN_RELE_2, LOW);
    delay(30000);
    digitalWrite(PIN_RELE_3, HIGH);
    }
  
  
  long now_mqttsend = millis();
  if (now_mqttsend - threadmqtt > 60000) {

    float newHum = readUmidity();
    float newTemp = readTemperature();
    int SoilMoisture = readSoil();
  
    threadmqtt = now_mqttsend;
    sendMqttMsg(newTemp,newHum,SoilMoisture);
  }


  
}



int readSoil() {

  digitalWrite(soilPower, HIGH);
  delay(10); 
  val = analogRead(soilPin); //Read the SIG value form sensor 
  digitalWrite(soilPower, LOW);

  int output_value = map(val,185,16,0,100);

    if (output_value > 100){
      output_value = 100;
    }

    if (output_value < 0){
      output_value = 0;
    }

  return output_value;

}


float readUmidity(){
  float output_value = dht.readHumidity();
  return output_value;
}

float readTemperature(){
  float output_value = dht.readTemperature();
  return output_value;
}

void LightsOn(){
  if(RELE2STATUS != 6){
  digitalWrite(PIN_RELE_2, LOW);
  }             
}

void LightsOff(){
  if(RELE2STATUS != 6){
  digitalWrite(PIN_RELE_2, HIGH);
  }           
}


void sendMqttMsg(float a, float b, int c){

  String msgMQTT = String(a).c_str();
  msgMQTT += ',';
  msgMQTT += String(b).c_str();
  msgMQTT += ',';
  msgMQTT += String(c).c_str();
  msgMQTT += ',';
  msgMQTT += String(MQTT_CLIENT).c_str();
      
  MQTT.publish(TOPICO_ENVIO, String(msgMQTT).c_str(), true);

}

