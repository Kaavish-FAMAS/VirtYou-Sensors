#include <DHT.h> // include the library for the DHT22 sensor
#include <WiFi.h> // include the library to connect ESP32 to the internet
#include <HTTPClient.h> // include the library to send HTTP requests to MongoDB
#include <ArduinoJson.h> // include the library to compile data in JSON format

#include "time.h" // for accurate timestamps
#include "secrets.h" // includes the password and ssid of internet

#define LDR_PIN 34 // LDR sensor is connected to digital pin 34
#define PIR_PIN 19 // PIR sensor is connected to digital pin 19
#define LED_PIN 2 // Onboard LED of ESP32 for testing
#define DHTPIN 27 // define the digital pin the DHT22 is connected to - D27 on ESP32
#define DHTTYPE DHT22 // define the type of DHT sensor

int pirState = LOW; // Assume that the output of PIR sensor is 0 initially
// instantiate the variables to return data into
int ldrReading, pirReading;
float temp, hum; 
unsigned long epochTime; // The epoch time
unsigned long dataMillis = 0;

// The Json document we will be uploading to MongoDB Database
StaticJsonDocument<500> doc;

DHT dht(DHTPIN, DHTTYPE); // create a DHT object

// Function to initialize the WiFi of the ESP32
void initWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi ..");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println('.');
    delay(1000);
  }
  Serial.println(WiFi.localIP());
}

void setup() {
  pinMode(LDR_PIN, INPUT); // Set LDR pin as an input
  pinMode(DHTPIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);
  dht.begin(); // initialize the DHT sensor
  initWiFi();

  Serial.print("RRSI: ");
  Serial.println(WiFi.RSSI()); // Print the strength of the WiFi connected
  configTime(0, 0, ntpServer);
}

// Function to read the digital value from the LDR sensor
void readLDR() {
  int ldrValue = digitalRead(LDR_PIN); // Read the LDR value

  if (ldrValue == LOW) { // If the light level is low
    // Do something, such as turning on an LED
    Serial.println("There is sufficient lighting");
    digitalWrite(LED_PIN, LOW);
  } else { // If the light level is high
    // Do something else, such as sending a notification
    Serial.println("The light level was low so I turned on the light.");
    digitalWrite(LED_PIN, HIGH);
  }
  ldrReading = ldrValue;
}

// Function to read the digital value from the PIR sensor
void readPIR() {
  int pirValue = digitalRead(PIR_PIN); // Read the PIR value

  if (pirValue == HIGH){            
    // digitalWrite(LED_PIN, HIGH); // If sensor value is HIGH, turn on the LED
    if (pirState == LOW){ // If the previous state of sensor was LOW, print "Motion detected" on Serial
      Serial.println("Motion detected!"); 
      pirState = HIGH; // Change sensor previous state to HIGH
    }
  } else{
    // digitalWrite(LED_PIN, LOW); // If sensor value is LOW, turn off the LED
    if (pirState == HIGH){ // If the previous state of sensor was HIGH, print "Motion detection ended" on Serial
      Serial.println("Motion detection ended!");  
      pirState = LOW; // Change sensor previous state to LOW
    }
  }
  pirReading = pirValue;
}

// Function to read the humidity and temperature
void readTemperatureHumidity() {
  float humidity = dht.readHumidity(); // read the humidity
  float temperature = dht.readTemperature(); // read the temperature

  // check if the read was successful, and print the data to the serial monitor
  if (!isnan(humidity) && !isnan(temperature)) {
    temp = temperature;
    hum = humidity;
  }
}

// Function to get the timestamp
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return(0);
  }
  time(&now);
  return now;
}

// Function to send an HTTP POST request to the MongoDB Atlas server
void POSTData()
{
  if(WiFi.status()== WL_CONNECTED){
    HTTPClient http;

    http.begin(serverName);
    http.addHeader("Content-Type", "application/json");

    String json;
    serializeJson(doc, json);

    Serial.println(json);
    int httpResponseCode = http.POST(json);
    Serial.println(httpResponseCode);

    if (httpResponseCode == 200) {
      Serial.println("Data uploaded.");
      delay(200);
    } else {
      Serial.println("ERROR: Couldn't upload data.");
      delay(200);
    }

  }
}

void loop() {

  if (millis() - dataMillis > 15000 || dataMillis == 0) {
    
    dataMillis = millis();

    epochTime = getTime();
    Serial.print("Epoch Time: ");
    Serial.println(epochTime);

    readLDR();
    readPIR();
    readTemperatureHumidity();

    Serial.print("Temperature: ");
    Serial.print(String(temp));
    Serial.print(" C\nHumidity: ");
    Serial.print(String(hum));
    Serial.print("\nMoisture: ");
    Serial.println("\n");

    // Construct the JSON object to send to MongoDB

    doc["sensor-values"]["temperature"] = temp;
    doc["sensor-values"]["humidity"] = hum;
    doc["sensor-values"]["motion"] = pirReading;
    doc["sensor-values"]["light"] = ldrReading;
    doc["sensor-values"]["timestamp"] = epochTime;

    Serial.println("Uploading data... "); 
    POSTData();
  }
}