# Amber FIDGET
### (<big>FI</big><small>re</small> <big>D</big><small>an</small><big>G</big><small>er but it's</small> <big>E</big><small>lec</small><big>T</big><small>ricity</small>)

## Usage
### Prerequisites
You have to install the following installed to use FIDGET:
- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (CLI for interfacing with the ESP32)
  - [ArduinoJSON](https://registry.platformio.org/libraries/bblanchon/ArduinoJson/installation)
  - [Adafruit NeoPixel](https://registry.platformio.org/libraries/adafruit/Adafruit%20NeoPixel)
- [HTTPClient](https://github.com/amcewen/HttpClient)

### Secrets
FIDGET uses the Amber API to fetch the current price descriptor.
In order to accomplish this, you must have your WiFi and API credentials stored in a file called `src/secrets.h`.
An example is provided below.

```c++
#define WIFI_SSID "[YOUR SSID]"
#define WIFI_PASSKEY "[YOUR PASSWORD]"
#define API_KEY "[YOUR API KEY]"
#define SITE_ID "[YOUR SITE ID]"
```

To find your Site ID, run the following command and look for the `id` key.

```sh
curl -H "Authorization: Bearer [YOUR API KEY]" https://api.amber.com.au/v1/sites
```
