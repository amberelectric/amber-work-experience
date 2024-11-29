![Amber FIDGET - Fire Danger but it's Electricity](resources/images/header/header.png)

# Intro / Say What Now?!

### What is it an what does it do?
The Amber FIDGET is a miniature Fire Danger Rating sign that displays the current power price descriptor for your house.

### How does it do it?
The FIDGET connects to your Wi-Fi network
and uses the [Amber API](https://app.amber.com.au/developers/documentation/)
to fetch the current price descriptor for your property.
It then translates this into a fire danger rating, and tells a servo motor to point at that rating, with 'tweening'.

# Build Guide

## Parts

### Owned
These parts are only needed to **make** the FIDGET. They are not required once it's running
- Laptop (preferably should turn on)
- Hot Glue Gun
- 2D Printer
- 3D Printer (Not necessary. You can just tape or hot glue the components together.)
- Box Cutter

### Purchased
Links are provided for Core Electronics, a Newcastle-based electronics provider with generally pretty good prices.
- [ESP32-C3 WROOM Dev Board](https://core-electronics.com.au/esp32-c3-wroom-development-board.html) (Or any other ESP32 board)
- [9g Micro Servo](https://core-electronics.com.au/feetech-fs90-1-5kgcm-micro-servo-9g.html) (Or any 9g servo with half-decent torque)
- [Male to Female Jumper Wires](https://core-electronics.com.au/jumper-wires-premium-12-m-f-pack-of-10.html) (Any wire will work but jumpers are nicer)
- [USB Micro-B Cable](https://core-electronics.com.au/usb-cable-type-a-to-micro-b-1m.html) (Once the FIDGET is running, it's possible to power it off a battery pack)

### Printed In Two Dimensions
The front of the sign has to be printed (or you can draw it, I guess).
If you have a colour printer, use `resources/images/sign/sign colour.png`.
Otherwise, use `resources/images/sign/sign bw.png` and colour in the sections with a pen or highlighter.

### Printed In Three Dimensions
Note: These parts are not strictly necessary, but they make putting together and pulling apart the FIDGET much easier.

The files you need to print are in the `resources/objects` folder.
`fire danger sign.f3d` is the Fusion 360 model for the entire FIDGET, and `fire danger sign.stl` is the whole model.
You only need to print `arrow.stl`, `servo bottom.stl`, `servo top.stl` and `esp32.stl`.

When printing, bigger layer heights are reccomended to increase strength.

### Materials
- Cardboard box of regular thickness
- Hot glue
- Regular glue (tape will work too)
- Rubber band (a cable tie will work too)

## Make

Servo
The servo motor has to be connected to the ESP32 in the following way:

| ESP32 | Servo                |
|-------|----------------------|
| GND   | - (Brown wire)       |
| 5V    | + (Red wire)         |
| 0     | Signal (Orange wire) |

Note that you can use any of the GND or 5V pins on the ESP32.

# (Quick) Start

Prerequisites
The only prerequisite for FIDGET is [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), a CLI for interfacing with the ESP32.

Secrets
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

If you are using the debug server, you must add the following line to `secrets.h` as well.

```c++
#define DEBUG_HOST "[YOUR IP ADDRESS OR HOSTNAME]"
```

# Usage

# Troubleshooting

Debug Server
To use the debug server, run `resources/debugServer.sh`, then enter your credentials when prompted.
This script will copy real-time data from the Amber API into `resources/v1/sites/[YOUR SITE ID]/prices/current`,
then start a local web server on port 8000 with Python's `http.server`.

On the ESP32 side, all you need to do is uncomment this line near the start of the file:
```c++
#define USE_DEBUG_SERVER
```

Logging
The following table shows the status messages displayed on the ESP32 LED and in the Serial Monitor.
A description of the state is in the `Status` column,
the LED colour and Serial output are shown in the next two columns,
and the final column shows the minimum required log level to show the state.

| Status                         | LED Colour        | Print Statement                              | Level   |
|--------------------------------|-------------------|----------------------------------------------|---------|
| Connecting to Wi-Fi            | Yellow            | Connecting to Wi-Fi...                       | Info    |
| Setting Clock                  | Green             | Setting time...                              | Info    |
| Fetching Descriptor            | Cyan              | Fetching descriptor...                       | Info    |
| Fetching Debug Descriptor      | Blue              | Fetching descriptor from debug server...     | Info    |
| Moving Servo                   | Purple            | Updating servo...                            | Debug   |
| Idle                           | White             | Done!                                        | Info    |
| **Errors**                     | **Red-...**       |                                              |         |
| Could not connect to Wi-Fi     | Red-Red           | Could not connect to Wi-Fi                   | Warning |
| Could not connect to the API   | Red-Yellow-Red    | Could not connect to API                     | Error   |
| Could not configure API        | Red-Yellow-Yellow | Could not configure API                      | Error   |
| API timed out                  | Red-Yellow-Green  | API connection timed out                     | Error   |
| API gave invalid response      | Red-Yellow-Cyan   | API returned invalid response                | Error   |
| HTTP status code error         | Red-Green-Red     | API returned error code: [CODE]              | Error   |
| HTTP could not skip headers    | Red-Green-Yellow  | Could not skip HTTP headers: [CODE]          | Error   |
| Could not parse JSON           | Red-Cyan-Red      | Could not parse returned JSON                | Error   |
| Could not find general channel | Red-Cyan-Yellow   | Could not find general channel in JSON       | Error   |
| Servo could not start          | Red-Blue          | Could not start servo                        | Fatal   |
| Debug API not provided         | Red-Purple        | Debug API URL not provided. Using Amber API. | Warning |


Notes
1. Where more than one colour is listed in the `LED Colour` column,
   the LED will flash the colours in sequence with a 300ms delay.
2. The LED will show the appropriate colour no matter what the log level is set to.
3. If the `Level` is `Fatal`, the program will stop and the LED will flash the pattern repeatedly with a one second break.
