#include <Arduino.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTTClient.h>
#include <ArduinoJson.h>

// WiFi Parameters are define in the "secrets" header file
#include "secrets.h"


// Button definitions
const char BUTTON_0 = 0;
const char BUTTON_1 = 35;

// display library instance
TFT_eSPI tft = TFT_eSPI();
char buff[512];

// Secure WiFi and MQTT client instances
WiFiClientSecure net = WiFiClientSecure();
MQTTClient client = MQTTClient(1024);

void displayMessage(String message){
  // Clear the display and configure graphics properties
  tft.setTextSize(2);
  tft.fillScreen(TFT_LIGHTGREY);
  tft.setCursor(0,0,2);

  // update the lcd with the message String
  tft.print(message);
}

void messageHandler(String &topic, String &payload){
  Serial.println("incoming: " + topic + " - " + payload);
  
  // Create a new temporary JSON object
  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload);

  //extract the message
  const char* message = doc["message"];

  // display the message on the LCD
  displayMessage(message);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Hello World");

  pinMode(BUTTON_0, INPUT_PULLUP);
  pinMode(BUTTON_1, INPUT_PULLUP);

  Serial.println("Initialise LCD");
  
  tft.init();
  tft.setRotation(3);
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(0, 0, 2);
  tft.setTextSize(2);
  tft.setTextColor(TFT_RED);
  tft.println(" Hello Amazon ");

  Serial.println("Connecting to WiFi");

  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED){
    delay(200);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi Connected!");

  tft.print(WiFi.localIP());

  Serial.println("Connecting to AWS");
  // Configure WiFiClientSecure to use the AWS IoT device credentials
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);

  // Start the MQTT client 
  client.begin(AWS_IOT_ENDPOINT, 8883, net);

  // define messageHandler as our call back function
  client.onMessage(messageHandler);

  while (!client.connect(CLIENT_NAME)) {
    Serial.print(".");
    delay(100);
  }

  if(!client.connected()){
    Serial.println("AWS IoT Timeout! :(");
    while(1);
  }
  else{
    Serial.println("AWS IoT Connected! :D");
  }

  // Subscribe to a topic
  client.subscribe(MESSAGE_TOPIC);

  sleep(1);

}

int counter = 0;

void publish(int count, String action ){
  StaticJsonDocument<200> doc;
  doc["count"] = count;
  doc["action"] = action;
  char jsonBuffer[512];
  serializeJson(doc, jsonBuffer);

  Serial.print("JSON Object: ");
  Serial.println(jsonBuffer);

  client.publish("counter", jsonBuffer);
}


void loop() {
  Serial.println("loop ");
  
  if(digitalRead(BUTTON_0) == false){
    Serial.println("Button 0 is pressed");
    counter++;
    publish(counter, "Increment");
  }

  if (digitalRead(BUTTON_1) == false){
      Serial.println("Button 1 is pressed");
      counter--;
      publish(counter, "Decrement");
  }

  Serial.print("counter value: ");
  Serial.println(counter);

  // Spin through the message handler
  client.loop();

  sleep(1);
}