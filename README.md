# Amber FIDGET
### (<big>FI</big><small>re</small> <big>D</big><small>an</small><big>G</big><small>er but it's</small> <big>E</big><small>lec</small><big>T</big><small>ricity</small>)

## Usage
### Parts
For this project,
an [ESP32-C3 WROOM Dev Board](https://core-electronics.com.au/esp32-c3-wroom-development-board.html) and a [9g Micro Servo](https://core-electronics.com.au/feetech-fs90-1-5kgcm-micro-servo-9g.html) were used.
Theoretically, you could use any Arduino-enabled board and any servo motor.

### Prerequisites
The only prerequisite for FIDGET is [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), a CLI for interfacing with the ESP32.

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

If you are using the debug server, you must add the following line as well.

```c++
#define DEBUG_HOST "[YOUR IP ADDRESS OR HOSTNAME]"
```

### Servo
The servo motor has to be connected to the ESP32 in the following way:

| ESP32 | Servo                |
|-------|----------------------|
| GND   | - (Brown wire)       |
| 5V    | + (Red wire)         |
| 0     | Signal (Orange wire) |

Note that you can use any of the GND or 5V pins on the ESP32.


### Debug Server
To use the debug server, run `resources/debugServer.sh`, then enter your credentials when prompted.
This script will copy real-time data from the Amber API into `resources/v1/sites/[YOUR SITE ID]/prices/current`,
then start a local web server on port 8000 with Python's `http.server`.

On the ESP32 side, all you need to do is uncomment this line near the start of the file:
```c++
#define USE_DEBUG_SERVER
```
