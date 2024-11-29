#include <Arduino.h>
#include <HttpClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <Secrets.h>
#include <ESP32_New_ISR_Servo.h>
#include <Tween.h>

//#define USE_DEBUG_SERVER  // Uncomment this to use the local debugging API
#define LOG_LEVEL LOGGING_INFO

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

// Log levels
#define LOGGING_NONE    0
#define LOGGING_FATAL   1
#define LOGGING_ERROR   2
#define LOGGING_WARNING 3
#define LOGGING_INFO    4
#define LOGGING_DEBUG   5
#define LOGGING_VERBOSE 6

// LED colours
#define LED_RED    {255,   0,   0}
#define LED_YELLOW {255, 255,   0}
#define LED_GREEN  {  0, 255,   0}
#define LED_CYAN   {  0, 255, 255}
#define LED_BLUE   {  0,   0, 255}
#define LED_PURPLE {255,   0  255}
#define LED_WHITE  {255, 255, 255}

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


void log(String text, int level) {
  if (LOG_LEVEL >= level) {
    switch (level) {
      case LOGGING_FATAL:
        Serial.print("💥 Fatal Error: "); break;
      case LOGGING_ERROR:
        Serial.print("❗️ Error: "); break;
      case LOGGING_WARNING:
        Serial.print("⚠️ Warning: "); break;
      case LOGGING_INFO:
        Serial.print("ℹ️ Info: "); break;
      case LOGGING_DEBUG:
        Serial.print("🪲 Debug: "); break;
      case LOGGING_VERBOSE:
        Serial.print("Verbose: "); break;
    }
    Serial.println(text);
  }
  // Will add LED functionality here soon
  // !!! Remember to use pixels.clear() and pixels.show(), as well as pixels.setPixelColor(0, pixels.Color(r, g, b));
}

void setClock() {
  configTime(0, 0, "pool.ntp.org");

  log(F("Setting time..."), LOGGING_INFO);
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    yield();
    nowSecs = time(nullptr);
  }

  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  log(String(F("Current time: ")) + asctime(&timeinfo), LOGGING_VERBOSE);
}


unsigned long lastRun = 0;
bool connect()
{
  log(F("Connecting to Wi-Fi…"), LOGGING_INFO);

  if (WiFi.status() != WL_CONNECTED) {
    log(String(F("Could not connect to Wi-Fi")) + String(WiFi.status()), LOGGING_WARNING);
    return false;
  }
  else
  {
    log(String(F("Connected to WiFi network with IP Address: ")) + WiFi.localIP(), LOGGING_VERBOSE);
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
      log(F("Fetching descriptor from debug server..."), LOGGING_INFO);
      server_name = DEBUG_HOST;
      port = 8000;
    #else
      log(F("Debug API URL not provided. Using Amber API."), LOGGING_WARNING);
    #endif
  #endif

  if (server_name == "api.amber.com.au") {log(F("Fetching descriptor..."), LOGGING_INFO);}

  String path = String("/v1/sites/") + String(SITE_ID) + String("/prices/current");

  http.beginRequest();
  err = http.startRequest(server_name, port, path.c_str(), HTTP_METHOD_GET, "AmberFidget");
  if (err != HTTP_SUCCESS)
  {
    switch (err)
    {
    case HTTP_ERROR_CONNECTION_FAILED:
      log(F("Could not connect to API"), LOGGING_ERROR);
      break;
    case HTTP_ERROR_API:
      log(F("Could not configure API"), LOGGING_ERROR);
      break;
    case HTTP_ERROR_TIMED_OUT:
      log(F("API connection timed out"), LOGGING_ERROR);
      break;
    case HTTP_ERROR_INVALID_RESPONSE:
      log(F("API returned invalid response"), LOGGING_ERROR);
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
    log(String(F("API returned error code: ")) + String(err), LOGGING_ERROR);
    http.stop();
    return String("");
  }

  err = http.skipResponseHeaders();
  if (err != HTTP_SUCCESS)
  {
    log(String(F("Could not skip HTTP headers: ")) + String(err), LOGGING_ERROR);
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
    log(F("Could not parse returned JSON"), LOGGING_ERROR);
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

  log(F("Could not find general channel in JSON"), LOGGING_ERROR);
  return String("");
}

void sequence_motions(float newTarget) {
  if (target == newTarget) {
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
  ESP32_ISR_Servos.useTimer(0);
  servoIndex = ESP32_ISR_Servos.setupServo(0, 500, 2500);
  if (servoIndex == -1) {
    log(F("Could not start servo"), LOGGING_FATAL);
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
    String descriptor = fetchDescriptor();

    if (descriptor != String(""))
    {
      log(String(F("Current Price Descriptor: ")) + descriptor, LOGGING_VERBOSE);

      if (descriptor == String("spike"))
      {
        log(F("Price Spike!"), LOGGING_VERBOSE);
        sequence_motions(CATASTROPHIC_DANGER);
      }
      else if (descriptor == String("high"))
      {
        log(F("High prices"), LOGGING_VERBOSE);
        sequence_motions(CATASTROPHIC_DANGER);
      }
      else if (descriptor == String("neutral"))
      {
        log(F("Average prices"), LOGGING_VERBOSE);
        sequence_motions(EXTREME_DANGER);
      }
      else if (descriptor == String("low"))
      {
        log(F("Low prices"), LOGGING_VERBOSE);
        sequence_motions(HIGH_DANGER);
      }
      else if (descriptor == String("veryLow"))
      {
        log(F("Very Low prices"), LOGGING_VERBOSE);
        sequence_motions(MODERATE_DANGER);
      }
      else
      {
        log(F("Extremely low prices"), LOGGING_VERBOSE);
        sequence_motions(MODERATE_DANGER);
      }
    }

    lastRun = millis();
    log(F("Done!"), LOGGING_INFO);
  }
}
