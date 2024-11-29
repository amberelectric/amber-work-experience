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
- Computer (preferably should turn on)
- Hot Glue Gun
- 2D Printer
- 3D Printer (Not necessary. You can just tape or hot glue the components together.)
- Box Cutter
- Hair Dryer (I'll explain I promise)

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

When printing, bigger layer heights are recommended to increase strength.

### Materials
- Cardboard box of regular thickness
- Hot glue
- Regular glue (tape will work too)
- Rubber band (a cable tie will work too)

## Make

### 1. Cut the Sign Cardboard
Use the box cutter to cut a piece of blank cardboard about half a centimeter bigger than the sign on all sides,
as shown in the picture below:

![Cardboard cut about half a centimeter bigger than the sign on all sides](resources/images/instructions/cut%20cardboard.png)

Then, align the sign in the center of the cardboard and glue it on.
Regular (non-hot) glue is preferable as it allows the paper to lie flush on the cardboard.

### 2. Wiring
The servo motor has to be connected to the ESP32 as follows:

| ESP32 Pin | Servo Pin            |
|-----------|----------------------|
| GND       | - (Brown wire)       |
| 5V        | + (Red wire)         |
| 0         | Signal (Orange wire) |

Note that you can use any of the GND or 5V pins on the ESP32.

### 3. Mount the Servo
To mount the servo on the sign you need to do a few things:
1. Use something sharp and pointy (pen, box cutter, scissors, drill, hole punch, etc) to punch a hole in the front of the sign where the servo will stick out.
2. If you 3D printed `servo top.stl` and `servo bottom.stl`:
   1. Place the servo inside `servo bottom`.
   2. Place `servo top` on top.
   3. Make sure the servo wire is out the side of the case, and the front of the servo protrudes from the front of the case.
   4. Carefully hot glue or melt the two halves of the servo case together, making sure not to get glue on the servo within.
3. Glue the servo case on to the back of the sign, so that the wires point to the left and the front of the servo goes through the hole in the sign.

The attached servo should look like the below image:

![Mounted servo motor](resources/images/instructions/mount%20servo.png)

### 4. Attach the Arrow
Turning around to the front of the sign, you can now attach the arrow to the servo motor.
In the bag of hardware that comes with the servo motor, there are five "horns" (pieces of plastic that attach to the gear on the front of the servo), one small screw and two large screws.

1. Make sure the servo is at 0˚. (Use the code at the end of this step)
2. Attach the **shortest** servo horn to the servo gear, pointing directly to the right. Note that it doesn't have to be pointing exactly right. You can adjust the angles it points to easily at the top of `main.cpp`.  
3. Screw the horn in place with the small screw.
4. Hot glue the arrow onto the horn.

To set the servo to 0˚, use the following code:
```c++
#include <ESP32_New_ISR_Servo.h>

void setup() {
  ESP32_ISR_Servos.useTimer(0);  // Use timer 0
  int servoIndex = ESP32_ISR_Servos.setupServo(0, 500, 2500);  // Start the servo on pin 0
  ESP32_ISR_Servos.setPosition(servoIndex, 0);  // Set the servo to 0˚
}

void loop() {}
```

The attached arrow should look like the below images:

![Attached arrow](resources/images/instructions/attach%20arrow%201.png)
![Attached arrow](resources/images/instructions/attach%20arrow%202.png)

### 5. Attach the ESP23
Attaching the ESP32 is fairly simple.
If you printed the ESP32 mount, hot glue it on the back of the FIDGET to the left, and clip in the ESP32.
If you didn't simply glue or tape the ESP32 in place.

Note:
The 3D printed mount may not fit perfectly.
Test it before you hot glue it in place and then bend the arms while they're hot from the glue.

The attached ESP32 should look like the below image:

![Mounted ESP32](resources/images/instructions/mount%20esp32.png)

### 6. Finishing touches
Use a rubber band to hold the wires together neatly.
I feel like you shouldn't need an image for this one...

Then, cut out some more cardboard and hot glue it into a stand shape, like this:

![FIDGET Stand](resources/images/instructions/make%20stand.png)

The design is primarily up to your imagination/engineering, but you should add a cross-beam to stop the cardboard from bending.

### 7. You did it!!
Congratulations!
You built the Amber FIDGET without blowing anything up!
Behold, the final product.
(The paperclip is because I had glue issues.)

![Finished FIDGET](resources/images/instructions/finished%201.png)
![Finished FIDGET](resources/images/instructions/finished%202.png)

# (Quick) Start

### Prerequisites
The only prerequisite for FIDGET is [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html), a CLI for interfacing with the ESP32.

### Secrets file
FIDGET uses the Amber API to fetch the current price descriptor.
In order to accomplish this, you must have your WiFi and API credentials stored in `src/secrets.h`.
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

Note: If you are using the debug server, you must add the following line to `secrets.h` as well.

```c++
#define DEBUG_HOST "[YOUR IP ADDRESS OR HOSTNAME]"
```

### Quick Run
To quickly upload the code to the FIDGET, connect the ESP32 to your computer, then run `resources/scripts/upload.sh`.

# Usage

## Running the Code
Multiple scripts are provided in `resources/scripts`.
Each one serves a specific purpose:
- `compile.sh` checks that your code can be compiled for the ESP32 but does not upload the code.
- `upload.sh` compiles and uploads your code to the ESP32, then opens a serial monitor in your terminal.
- `debug.sh` starts the debugging server on your device.

Note:
Both `compile.sh` and `upload.sh` automatically install all required libraries via [the PlatformIO registry](https://registry.platformio.org).

## Troubleshooting

### Debug Server
To use the debug server, run `resources/debugServer.sh`, then enter your credentials when prompted.
This script will copy real-time data from the Amber API into `resources/v1/sites/[YOUR SITE ID]/prices/current`,
then start a local web server on port 8000 with Python's `http.server`.

On the ESP32 side,
all you need to do is define `DEBUG_HOST` in `secrets.h` (see "Secrets File")
and uncomment this line near the start of `main.cpp`:
```c++
#define USE_DEBUG_SERVER
```

## Logging
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
