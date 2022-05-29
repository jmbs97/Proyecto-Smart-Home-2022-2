
//author: 
// Juan Martin Bustos Suarez


//BIBLIOTECAS ASOCIADAS PARA LECTURA DE WIFI, SUBSCRIPCION DE TARJETA ESP8266, SERVO MOTOR Y TIMER
#include <Servo.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Time.h>

WiFiClient espClient;                 //suscripción a los datos asociados
PubSubClient client(espClient);
long lastMenssage = 0;                //variable de mensajes
char menssage[50];
int value = 0;
//Variables asociadas y asignación de pines
const int gpio2 = 2; //GPIO 2
String EncProg;
String ApaProg;
const int gpio16 = 16;   //GPIO 16
const int gpio13 = 13;   //GPIO 13
const int gpio15 = 15;   //GPIO 15
Servo servo; 
const int servomotor = 1 ; //GPIO 1

const char* ssid = "IZZI-D0F7";         //conexion a red 
const char* password = "9CC8FC7AD0F7";  //contraseña asociada 
const char* mqtt_server = "192.168.0.8";    //dirección del servidor mqtt mosquitto


WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP,"0.debian.pool.ntp.org",-21600,6000); //hora del servidor en horario de México


//configuración del dispositivo y conexion con servidor 
void setup() {
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);  //puesto de vinculación
  client.setCallback(callback);

  //inicialización de focos
  pinMode(gpio2, OUTPUT);
  pinMode(gpio13, OUTPUT);
  pinMode(gpio15, OUTPUT);

  //inicialización de puerta con el puerto 14
  servo.attach(servomotor);
  servo.write(0); 
  timeClient.begin(); 
}


 void setup_wifi() {      //verificación de comunicación con raspberry pi a traves de la terminal serial del puerto
    delay(10);
    Serial.println();
    Serial.print("Conectando a ");    //conexión
    Serial.println(ssid); 
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi conectado");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }

//funcion para entender los topics de la raspberry pi mediante mqtt
void callback(char* topic, byte* payload, unsigned int length) {        //función para reconocer los topics del protocolo mqtt
    String PAYLOAD;
    Serial.print("Mensaje Recibido en el topic [");
    Serial.print(topic);
    Serial.print("] ");
    Serial.print("Mensaje: ");
    for (int i = 0; i < length; i++) {
      Serial.print((char)payload[i]);
      PAYLOAD += (char)payload[i];
    }
    Serial.println();

//Asignación de Topic para prender y apagar foco
  if(String(topic) == "casa/SwitchFoco"){
    Serial.print("Estado del foco: ");
    if(PAYLOAD == "ON"){
      Serial.println("ON");
      digitalWrite(gpio2, HIGH);
    }else if(PAYLOAD == "OFF"){
      Serial.println("OFF");
      digitalWrite(gpio2, LOW);
    }
  }

//Asignación de Topic para modular con pwm  

  if(String(topic) == "casa/AtenuacionFoco"){
    Serial.print("Cambio del foco ");
    Serial.println("PAYLOAD");
    analogWrite(gpio16, PAYLOAD.toInt());
  }
//Asignación de Topic para abrir y cerrar puerta
  if(String(topic) == "casa/Puerta"){
    if(PAYLOAD == "PUERTA_ABIERTA"){
      Serial.println("Puerta Abierta");
      servo.write(0);
    }else if (PAYLOAD == "PUERTA_CERRADA"){
      Serial.println("Puerta Cerrada");
      servo.write(180);
    }
  }
//--------------- Timer -------------------------
  if(String(topic) == "casa/EncProg"){
    EncProg = String(PAYLOAD); 
  }   
  if(String(topic) == "casa/ApaProg"){
    ApaProg = String(PAYLOAD);
  }
} 


//--------------- Busqueda de conexion mqtt --------------
void reconnect(){
  while(!client.connected()){
    Serial.print("Realizando conexion MQTT");
    if(client.connect("ESP8266Client")){
      Serial.println("conectado");
      client.subscribe("casa/SwitchFoco"); //topic ON/OFF
      client.subscribe("casa/AtenuacionFoco");    //topic atenuación
      client.subscribe("casa/Puerta");     //topic puerta
      client.subscribe("casa/ApaProg");     //topic Programación de tiempo de encendido
      client.subscribe("casa/EncProg");     //topic Programación de tiempo de apagado
      
    }else{
      Serial.print("Fallo, rc=");
      Serial.print(client.state()); 
      Serial.println("Validación de conexion en 10s");
      delay(10000);
    }
  }
}
//---------------- Publicador -------------------
void loop() {
  timeClient.update();
  Serial.println(timeClient.getFormattedTime());
  digitalWrite(gpio13, HIGH);
  digitalWrite(gpio15, LOW);
  if(!client.connected()){
    reconnect();
  }
  client.loop();

  long now = millis();
  if(now - lastMenssage > 1000){ //definición del tiempo de muestreo
    lastMenssage = now;
    ++value;
    snprintf (menssage, 75, "Conexión exitosa #%ld", value);
    Serial.print("Publish message: ");
    Serial.println(menssage);
    client.publish("casa/#", menssage); //publica desde la terminal  
  }

  //Función para apagar o prender foco a partir de un cierto tiempo asigando por el usuario 

  if(timeClient.getFormattedTime()== EncProg){
    digitalWrite(2, HIGH); 
  }else if(timeClient.getFormattedTime()== ApaProg){
    digitalWrite(2, LOW);
  }
}
 
