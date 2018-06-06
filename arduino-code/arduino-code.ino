#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

ESP8266WebServer server(80);

#include <BME280I2C.h>
#include <Wire.h>

BME280I2C bme;

const int ledPin = 13;
const int triggerPin = 2;
const int echoPin = 14;

int pressure() {
  float temperature(NAN), humidity(NAN), pressure(NAN);
  
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  bme.read(pressure, temperature, humidity, tempUnit, presUnit);

  return pressure;
}

float humidity() {
  float temperature(NAN), humidity(NAN), pressure(NAN);
  
  BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  bme.read(pressure, temperature, humidity, tempUnit, presUnit);

  return humidity;
}

long sample() {
  digitalWrite(triggerPin, LOW);
  
  delayMicroseconds(2);

  digitalWrite(triggerPin, HIGH);

  delayMicroseconds(10);

  digitalWrite(triggerPin, LOW);

  long duration = pulseIn(echoPin, HIGH);

  return duration;
}

long multisample(int numberSamples) {
  long durations[numberSamples];
  long totalDuration = 0;
  
  for (int i = 0; i <= numberSamples; i++) {
    long duration = sample();
    
    durations[i] = duration;
    totalDuration += duration;
  }

  long averageDuration = totalDuration / numberSamples;

  long deviations[numberSamples];
  long totalDeviation = 0;

  for (int i = 0; i <= numberSamples; i++) {
    long deviation = abs(durations[i] - averageDuration);

    deviations[i] = deviation;
    totalDeviation += deviation;
  }

  long averageDeviation = totalDeviation / numberSamples;

  int numberFiltered = 0;
  long totalFiltered = 0;

  for (int i = 0; i <= numberSamples; i++) {
    if (deviations[i] <= averageDeviation) {
      numberFiltered += 1;
      totalFiltered += durations[i];
    }
  }

  long filteredDuration = totalFiltered / numberFiltered;

  return filteredDuration;
}

void setup(void){
  pinMode(ledPin, OUTPUT);
  pinMode(triggerPin, OUTPUT);
  pinMode(echoPin, INPUT);
  
  digitalWrite(ledPin, 0);
  
  Serial.begin(9600);
  WiFi.begin();
  
  Serial.println("");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  
  Serial.println("Connected to wifi");
  
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  if (MDNS.begin("esp8266")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", [](){
    digitalWrite(ledPin, 1);
    
    //float distance = multisample(10) * 0.034 * 0.5;
    
    //Serial.print(distance);
    //Serial.println("cm");
    
    //server.send(200, "text/plain", String(distance) + "cm");

    server.send(200, "text/plain", String(pressure()) + " Pa, " + String(humidity()) + "%");
    
    digitalWrite(ledPin, 0);
  });

  server.on("/inline", [](){
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound([] () {
    digitalWrite(ledPin, 1);
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
      message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
    digitalWrite(ledPin, 0);
  });

  server.begin();
  Serial.println("HTTP server started");

  Wire.begin();

  while (!bme.begin()) {
    Serial.println("Looking for BME280...");
    delay(1000);
  }

  switch(bme.chipModel())
  {
     case BME280::ChipModel_BME280:
       Serial.println("Found BME280 sensor!");
       break;
     default:
       Serial.println("Cannot recognize sensor.");
  }
}

void loop(void){
  server.handleClient();
}
