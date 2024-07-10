Hello!

## Intro
This is my project for having a temperature sensor driven by one connection: POE.
The Wiznet company has done a fantastic job in creating Pico boards with
ethernet ports attached.  With the 5500 board, we were provided MQTT libraries
so that the board could be a client or even a server for MQTT.  The 6100 board
was equally impressive with it's IP6 ability.  Now the 7500 Surf5 board has
added POE ability.

## This Project

Successfully attached a DHT22 temperature sensor to the GPIO pins on the
7500P Surf5 board.  The code reads from the temperature sensor and creates
a data packet to put in the payload of an MQTT message.  The MQTT message
is for reporting to a topic on a Broker.  In my case I ust Mosquito broker
running on a Home Assistant Green server in my house.  The data packed that
is created has to report in a json/token-key format.
The 7500 board must be overclocked by a factor of 4 to be able to read
correctly from the DHT22.

## Why it’s useful

The 7500 board continues the abilities of their boards with GPIO pins.  At
different protocol levels these pins can be used for popular communiction
to attached devices.  This includes I2c, UART, etc.  But withe the POE addition
this project allows me to run the entire device with DHT22 attachment with
only an ethernet cable - driven by a POE switch, of course.

## How users can get started
At this point in time I have provided code that will allow someone to download
and compile and with the MQTT libraries, that I will link to - or provide
at some point.  You must have VS Code setup on your computer and have purchased
the DHT22 and the 7500 board.  Finally, you obviously must have a switch, with
POE capablities (but optional if using USB power) and access to an MQTT Broker.

## Where to find help
Email me for help - I'll try.  elpasojeff@gmail.com

## Who maintains the project
At this point, I am the only one who maintains it.

## Information Links
1. Manual for the 7500 Surf5 Pico board
2. Manual for the DHT22
3. [Pinout List - because the Manual (#1) is confusing.](PinoutList.png)
4. [Screenshot of successful output.](SampleDebug.png)
