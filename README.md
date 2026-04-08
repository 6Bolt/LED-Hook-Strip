# LED-Hook-Strip

This C Code is for the Raspberry Pi Pico 2 and Pico 2W micro-controllers. The code control addressable LEDs using the NeoPixel interface. Which is WS2811, WS2812, and SK6812 addressable LEDs. The software that controls the micro-controller, is Hook of the Reaper. With Hook of the Reaper, it can do 4 display functions. The first is Display Range, where it uses the ALED strip to display info to the player. For example, it can be used as how much ammo is left. As you shoot, the ammo goes down, a long with the ALED strip. The other 3 display functions are Flash, Random Flash, and Sequential. This can be used with data from a game, using Hook of the Reaper. For example, the player takes damage, so then the ALED can flash red. 


## Licensing 

Like Hook of the Reaper, LED Hook: Strip, is under the GPL 3.0 license. This means, is a person uses this code, even one line of code, they agree to the GPL 3.0 license. Meaning their project and code, needs to be open sourced. This also goes for copying the code too. Saying you did it from scratch, while studying the code, means you are still bound by the licenses. 

Also, if using this code in a commercial project, you also have to give credit back to this project. This has to be done on your product information page, by citing this open sourced project. With at least 12pt font, and a link back to this GitHub page, and cannot be hidden using colors. If you cannot accept this, then don’t use the code.


## Compile Changes

There is a couple of things that people can change, but needs a recompile. 

The first is using just RGB LEDs. If you have RGBW LEDs, they can be used. But you cannot mix RGB and RGBW on one controller. To switch to RGBW LEDs, is in the globals.h file, at or around line 36. The ‘#define IS_RGBW’ can be switched to true, instead o false.

The second thing is the output pins that control the addressable LED. The are also defined in the globals.h file, at or around lines 46 to 49. The first pin is defined as ‘WS2812_PIN_0’ and set to ‘2,’ which is GPIO2, and not pin 2. All the pins can be change, by changing the WS2812_PIN_[0:3]. Currently, they are set to 2, 3, 4, and 5. Again, that is GPIO, and not the pin number. 


## Included Firmware

There are 3 devices that I will compile firmware for. That is the Raspberry Pi Pico 2, Raspberry Pi Pico 2W, and the Seed Pico 2. If you need a different Pico 2 or Pico 2W device, then you much re-compile the code. 


## Hook Up the Micro-Controller to the Addressable LEDs

Here is a picture of how you hook up the addressable LEDs to the Pico 2 or Pico 2W. It is a pretty simple connection, as a 200-400 Ohm resistor is needed on the line. 

What about a level shifter? Looking at the WS2812B datasheet, the VIH is 0.7V. That means 0.7V or higher is high, or a logic 1. With the VIH set that low, a 1.8V micro-controller can drive the signal, but not recommended.

So, what causes the flickering or bad LED coloring. Looking at the specs, I am guessing it is the VIL, which is 0.3V. Meaning 0.3V and lower, is a low, or a logic 0. The specs says, it hit hysteresis at 0.35V. That is a low ground floor, as a common ground is needed. Must likely, the ground voltage can spike up higher than 0.35V, and be recorded as high, which starts the interface, and does at least 1 LED. 

So if you are having flickering or bad LED coloring, it is best to look at the ground, and remove ground noise. Also, any EMI that the wire travels by. Also, use good wire, and at lease 22awg or smaller. If you are using 30awg with a long run, you are just asking for trouble. You could use a buffer, like the 74HCT125 or 74HCT245. But this is just covering up the problem, and as an engineer, the problem should be fixed, and not masked. But I know people would just use the buffer, hence, why I am saying all this.

Link to the WS2812B Datasheet, from Adafruit, who did the NeoPixel Interface
https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf

Also, the interface is not running that fast. It is clock at 800KHz, which is a period of 1.25us. It is a single end data pin, that is a clock finder. Both logic 0 and 1, has a raising and failing edge. If no data is on the line, then it remains at logic 0. But if it was a differential signal, it would need the buffer at all, but would need 2 pins though, but can see why Adafruit designed it this way. 
