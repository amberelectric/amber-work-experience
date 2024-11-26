#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HttpClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Secrets.h>
#include <ESP32_New_ISR_Servo.h>

#define LED_PIN 8
#define NUMPIXELS 1

#define SERVO_PIN 1

#define BUFFER_SIZE 1024
#define WIFI_ATTEMPTS 10
#define TIMER_DELAY 300000

// Servo angles
#define NO_DANGER           180
#define MODERATE_DANGER     157.5
#define HIGH_DANGER         112.5
#define EXTREME_DANGER       67.5
#define CATASTROPHIC_DANGER  22.5

int servoIndex = -1;

Adafruit_NeoPixel pixels(NUMPIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}


unsigned long lastRun = 0;
bool connect()
{
  Serial.print("Connecting to WiFi");
  int attempts = 0;

  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_ATTEMPTS)
  {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  Serial.println("");

  if (attempts == WIFI_ATTEMPTS)
  {
    Serial.println("Unable to connect to the WiFi");
    return false;
  }
  else
  {
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
    setClock();
    return true;
  }
}

String fetchDescriptor()
{
  WiFiClientSecure client;
  client.setInsecure();
  HttpClient http(client);
  int err = 0;

  String path = String("/v1/sites/") + String(SITE_ID) + String("/prices/current");

  http.beginRequest();
  err = http.startRequest("api.amber.com.au", 443, path.c_str(), HTTP_METHOD_GET, "ArduinoAmberLight");
  if (err != HTTP_SUCCESS)
  {
    switch (err)
    {
    case HTTP_ERROR_CONNECTION_FAILED:
      Serial.println("ERROR: Unable to connect to the API");
      break;
    case HTTP_ERROR_API:
      Serial.println("ERROR: Configuration issue");
      break;
    case HTTP_ERROR_TIMED_OUT:
      Serial.println("ERROR: Connection timed out");
      break;
    case HTTP_ERROR_INVALID_RESPONSE:
      Serial.println("ERROR: Invalid response received");
      break;
    }
    http.stop();
    return "";
  }

  String bearerToken = String("Bearer ") + String(API_KEY);
  http.sendHeader("Authorization", bearerToken.c_str());
  http.endRequest();

  err = http.responseStatusCode();
  if (err != 200)
  {
    Serial.printf("ERROR: Received Status Code: %d\n", err);
    http.stop();
    return String("");
  }

  err = http.skipResponseHeaders();
  if (err != HTTP_SUCCESS)
  {
    Serial.printf("ERROR: Unable to skip headers: %d\n", err);
    http.stop();
    return String("");
  }

  unsigned long timeoutStart = millis();
  char json[BUFFER_SIZE];
  memset((char *)&json, 0, BUFFER_SIZE);
  int i = 0;
  // Leave one character at the end, so the string is always terminated
  while (i < BUFFER_SIZE - 1 && (http.connected() || http.available()) && ((millis() - timeoutStart) < 30000))
  {
    if (http.available())
    {
      json[i++] = http.read();
      // Print out this character
      // We read something, reset the timeout counter
      timeoutStart = millis();
    }
    else
    {
      // We haven't got any data, so let's pause to allow some to
      // arrive
      delay(1000);
    }
  }

  http.stop();

  StaticJsonDocument<BUFFER_SIZE> doc;
  deserializeJson(doc, (char *)&json);
  if (!doc.is<JsonArray>())
  {
    Serial.println("Returned JSON object is not an array");
    return String("");
  }

  JsonArray prices = doc.as<JsonArray>();
  for (JsonVariant p : prices)
  {
    if (p.is<JsonObject>())
    {
      JsonObject price = p.as<JsonObject>();
      String channelType = price["channelType"].as<String>();
      if (channelType == "general")
      {
        return price["descriptor"].as<String>();
      }
    }
  }

  Serial.println("Error: General channel not found");
  return String("");
}

void setup()
{
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSKEY);

  // Set up the servo
  ESP32_ISR_Servos.useTimer(1);
  servoIndex = ESP32_ISR_Servos.setupServo(0, 500, 2500);
  if (servoIndex == -1) {
    Serial.println("Servo could not start!");
  }
  ESP32_ISR_Servos.setPosition(servoIndex, 0);

  // Force a run on startup
  lastRun = TIMER_DELAY;
  pixels.begin();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED && !connect())
  {
    Serial.println("WiFi connection attempts timed out. Will try again in a second.");
    delay(1000);
    return;
  }

  if ((millis() - lastRun) > TIMER_DELAY)
  {
    Serial.println("Checking the price");
    String descriptor = fetchDescriptor();

    pixels.clear();
    if (descriptor != String(""))
    {
      Serial.printf("Current Descriptor: %s\n", descriptor);

      if (descriptor == String("spike"))
      {
        Serial.printf("Price Spike!\n");
        ESP32_ISR_Servos.setPosition(servoIndex, CATASTROPHIC_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 0, 255));
      }
      else if (descriptor == String("high"))
      {
        Serial.printf("High prices\n");
        ESP32_ISR_Servos.setPosition(servoIndex, EXTREME_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      }
      else if (descriptor == String("neutral"))
      {
        Serial.printf("Average prices\n");
        ESP32_ISR_Servos.setPosition(servoIndex, HIGH_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 165, 0));
      }
      else if (descriptor == String("low"))
      {
        Serial.printf("Low prices\n");
        ESP32_ISR_Servos.setPosition(servoIndex, MODERATE_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 255, 0));
      }
      else if (descriptor == String("veryLow"))
      {
        Serial.printf("Very Low prices\n");
        ESP32_ISR_Servos.setPosition(servoIndex, NO_DANGER);
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));
      }
      else
      {
        Serial.printf("Extremely low prices\n");
        ESP32_ISR_Servos.setPosition(servoIndex, NO_DANGER);
        pixels.setPixelColor(0, pixels.Color(0, 255, 255));
      }
    }
    pixels.show();

    lastRun = millis();
  }
}
