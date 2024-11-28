#include <Arduino.h>
#include <HttpClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Secrets.h>
#include <ESP32_New_ISR_Servo.h>
#include <Tween.h>

//#define USE_DEBUG_SERVER  // Uncomment this to use the local debugging API

// Use http when on the debug server
#ifdef USE_DEBUG_SERVER
  #include <WiFiClient.h>
  #include <WiFi.h>
#else
  #include <WiFiClientSecure.h>
#endif

#define LED_PIN 8
#define NUMPIXELS 1

#define SERVO_PIN 1

#define BUFFER_SIZE 1024
#define WIFI_ATTEMPTS 10
#define TIMER_DELAY 300000

// Servo angles
#define NO_DANGER           173
#define MODERATE_DANGER     150
#define HIGH_DANGER         108
#define SERVO_MIDPOINT       81
#define EXTREME_DANGER       58
#define CATASTROPHIC_DANGER  11

#define ANIMATION_SPEED 15  // Bigger numbers are slower

int servoIndex = -1;

Tween::Timeline timeline;
float target = 0.f;

int lastConnectionAttempt = 0;
#define CONNECT_DELAY 500

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

  if (WiFi.status() != WL_CONNECTED) {
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
  #ifdef USE_DEBUG_SERVER
    WiFiClient client;
  #else
    WiFiClientSecure client;
    client.setInsecure();
  #endif

  HttpClient http(client);
  int err = 0;

  const char* server_name = "api.amber.com.au";
  uint16_t port = 443;

  #ifdef USE_DEBUG_SERVER
    #ifdef DEBUG_HOST
      Serial.println("Using debug server");
      server_name = DEBUG_HOST;
      port = 8000;
    #else
      Serial.println("No debug URL provided! Using Amber API.");
    #endif
  #endif

  String path = String("/v1/sites/") + String(SITE_ID) + String("/prices/current");

  http.beginRequest();
  err = http.startRequest(server_name, port, path.c_str(), HTTP_METHOD_GET, "AmberFidget");
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

void sequence_motions(float newTarget) {
  if (target == newTarget) {  // TODO: make it nicer
    return;
  }
  timeline.clear();
  timeline.add(target);

  // Big complex decision tree
  // If the current and desired servo position are on the same side, AND the current position is closer to the
  // top (≈90˚), then bounce "down" (away from 90˚). OTHERWISE, just back in/out.
  if (target < SERVO_MIDPOINT) {
    if (newTarget < SERVO_MIDPOINT && target > newTarget) {
      timeline[target].then<Ease::BounceOut>(newTarget, (target - newTarget) * ANIMATION_SPEED);
    } else {
      timeline[target].then<Ease::BackInOut>(newTarget, abs(target - newTarget) * ANIMATION_SPEED * 2);
    }
  } else {
    if (newTarget > SERVO_MIDPOINT && target < newTarget) {
      timeline[target].then<Ease::BounceOut>(newTarget, (newTarget - target) * ANIMATION_SPEED);
    } else {
      timeline[target].then<Ease::BackInOut>(newTarget, abs(newTarget - target) * ANIMATION_SPEED * 2);
    }
  }
  timeline.start();
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
  timeline.add(target)
    .init(SERVO_MIDPOINT)
    .offset(1000)
    .then<Ease::BounceOut>(NO_DANGER, 2000);
  timeline.start();

  // Force a run 5000ms after startup (to allow servo animation to complete)
  lastRun = 5000 - TIMER_DELAY;

  pixels.begin();
}

void loop()
{
  // Update the servo position as set by the tweening library
  timeline.update();
  ESP32_ISR_Servos.setPosition(servoIndex, target);

  // Connect to WiFi
  if (WiFi.status() != WL_CONNECTED)  {
    if ((millis()-lastConnectionAttempt) < CONNECT_DELAY) {
      return;
    }
    if (!(((millis()-lastConnectionAttempt) > CONNECT_DELAY) && connect())) {
        Serial.println("WiFi connection not acquired yet. Trying again soon...");
        lastConnectionAttempt = millis();
        return;
    }
  }

  if ((millis() - lastRun) > TIMER_DELAY)  {
    Serial.println("Checking the price");
    String descriptor = fetchDescriptor();

    pixels.clear();
    if (descriptor != String(""))
    {
      Serial.printf("Current Descriptor: %s\n", descriptor);

      if (descriptor == String("spike"))
      {
        Serial.printf("Price Spike!\n");
        sequence_motions(CATASTROPHIC_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 0, 255));
      }
      else if (descriptor == String("high"))
      {
        Serial.printf("High prices\n");
        sequence_motions(CATASTROPHIC_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 0, 0));
      }
      else if (descriptor == String("neutral"))
      {
        Serial.printf("Average prices\n");
        sequence_motions(EXTREME_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 165, 0));
      }
      else if (descriptor == String("low"))
      {
        Serial.printf("Low prices\n");
        sequence_motions(HIGH_DANGER);
        pixels.setPixelColor(0, pixels.Color(255, 255, 0));
      }
      else if (descriptor == String("veryLow"))
      {
        Serial.printf("Very Low prices\n");
        sequence_motions(MODERATE_DANGER);
        pixels.setPixelColor(0, pixels.Color(0, 255, 0));
      }
      else
      {
        Serial.printf("Extremely low prices\n");
        sequence_motions(MODERATE_DANGER);
        pixels.setPixelColor(0, pixels.Color(0, 255, 255));
      }
    }
    pixels.show();

    lastRun = millis();
  }
}
