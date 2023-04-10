#include<WiFi.h> 
#include<Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define WIFI_SSID "Try"
#define WIFI_PASSWORD "Du833a321"
#define API_KEY "AIzaSyASlnbXgOTeoMSeBtvpSlyHf7UF21EGYX4"
#define DATABASE_URL "https://esp32andfirebase-934c4-default-rtdb.asia-southeast1.firebasedatabase.app/"
#define FIREBASE_PROJECT_ID "esp32andfirebase-934c4"
 
// #define mq7Pin 34
// #define dustPin 35
// #define ledPower 32

// #define samplingTime 280
// #define deltaTime 40
// #define sleepTime 9680

int mq7Pin = 34; // The analog pin connected to the MQ-7 sensor
int dustPin = 35; // The digital pin connected to the dust sensor
int ledPower = 32;

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

float dustVal = 0;
float voltage = 0;
float dustDensity = 0;
float ratio = 0;  
float sensor_volt = 0;
float sensorVal = 0;
float RS_gas = 0;
float R0 = 7200.0;

FirebaseData fbdo;  
FirebaseAuth auth;
FirebaseConfig config;  

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;

void setup() {
  Serial.begin(115200); // Initialize the serial communication
  pinMode(ledPower, OUTPUT); // Set the dust sensor pin as an ouput

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;
  // config.signer.tokens.legacy_token = DATABASE_SECRET;

 if(Firebase.signUp(&config, &auth, "","")){
    Serial.println("signUp OK");
    signupOK = true;
   } else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
   }

   config.token_status_callback = tokenStatusCallback;
   Firebase.begin(&config, &auth);
   Firebase.reconnectWiFi(true);
}

void loop() {
  if(Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

  digitalWrite(ledPower, LOW);
  delayMicroseconds(samplingTime);

  dustVal = analogRead(dustPin); // Read dust sensor value
  
  delayMicroseconds(deltaTime);
  digitalWrite(ledPower, HIGH);
  delayMicroseconds(sleepTime);

  voltage = dustVal * (5.0 / 1024.0); // Convert dust sensor value to voltage
  dustDensity = 0.17 * voltage - 0.1; // Convert voltage to dust density (mg/m3)
  Serial.print("Dust Density: ");
  Serial.print(dustDensity);
  Serial.println(" mg/m3");
  delay(1000);
  
  // Read data from the MQ-7 sensor
  sensorVal = analogRead(mq7Pin);
  sensor_volt = sensorVal/1024*5.0;
   RS_gas = (5.0-sensor_volt)/sensor_volt;
   ratio = RS_gas/R0; //Replace R0 with the value found using the sketch above
   float x = 1538.46 * ratio;
   float ppm = pow(x,-1.709);
   Serial.print("PPM: ");
   Serial.println(ppm);
   delay(1000);

  FirebaseJson content;
  String documentPath = "dust_sensor/2I92Vl9tKQFLJwZ1n6ZY";

  content.set("fields/value/doubleValue",String(dustDensity).c_str());
//     content.set("fields/time/doubleValue","1681070452238"); 
     
  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw(),"value"))
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
  else
    Serial.println(fbdo.errorReason());
  if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, documentPath.c_str(), content.raw()))
    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
  else
    Serial.println(fbdo.errorReason());

    String secondPath = "co_sensor/LXp82yLO5Pz3101mL0aX";

    content.set("fields/value/doubleValue",String(ppm).c_str());
//     content.set("fields/time/doubleValue","1681070452238"); 
     
     if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, secondPath.c_str(), content.raw(),"value"))
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
     else
      Serial.println(fbdo.errorReason());
    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */, secondPath.c_str(), content.raw()))
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
     else
      Serial.println(fbdo.errorReason()); 
  }
}