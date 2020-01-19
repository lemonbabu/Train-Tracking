#include<SoftwareSerial.h>
#include <FirebaseArduino.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

int flg = 0, k = 0;
char lat[9], lng[9], loc[19];

// Set these to run example.
#define FIREBASE_HOST "train-tracking-14fc1.firebaseio.com"
#define FIREBASE_AUTH "k9viYDIzEWRvgH5NefhGyNcnFhxrDVI67YjxHalK"
#define WIFI_SSID "X-OR"
#define WIFI_PASSWORD "xor.duet"

void setup() {
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("connected: ");
  Serial.println(WiFi.localIP());

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
}



void loop() {
  Serial.readBytes(loc,19); 
  Serial.println(loc); //Print data on Serial Monitor

  for(int i = 0; i < 19; i++){
    if(loc[i] == ','){
      flg = 1;
      k = 0;
    }
    else{
      if(flg == 0)
        lat[k++] = loc[i];
      else
        lng[k++] = loc[i];
    }
  }
  
  if(lat != "" && lng != ""){
  
    Firebase.setString("Train/lat", lat);
    // handle error
    if (Firebase.failed()) {
        Serial.print("setting /number failed:");
        Serial.println(Firebase.error());  
        return;
    }
    delay(1000);
    
    Firebase.setString("Train/lng", lng);
    // handle error
    if (Firebase.failed()) {
        Serial.print("setting /number failed:");
        Serial.println(Firebase.error());  
        return;
    }
  }
  
} 
