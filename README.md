# LED Pedestal

This repository contains hardware, firmware and
3D-printable case for the LED Pedestal project.

## What is this project?

The name says it all - it's just a fancy-looking circuit
that can blink your precious (retro or not) LED that you most probably
still keep at the bottom of your component drawer. The circuit is heavily
over-complicated and the same thing could be done using 3 or 4 components total,
but it looks cool and has some nice features:

- PIC10F222 micro controller with sophisticated firmware (as far as 512 bytes 
firmware written in C with some assembly can be)
- Micro USB for charging the Li-Ion battery with all the (unnecessary)
protection circuits (overcharge, over-discharge and short-circuit protection) 
- small button for checking battery level and changing LED Blinking modes
- reaction to connecting charger and showing charging status
- can work while charging from USB

And on top of that cool-looking glass dome case 
to encapsulate the spirit of your LED! Example bellow:

XXX Image

## How do I make my own?

Before you start you have to:

- be able to solder small SMD components (TSSOP-8 is the smallest one used in
the design) and use hot-air or hot plate(or frying pan with some foil) to solder
Micro USB port (you could try to use soldering iron but it will be much more 
tedious) 
- be able to program PIC10F222 micro controller (even cheap PICKit2 can be used)
- calculate current-limiting resistor for your LED (assuming 3.3V drive voltage,
voltage drop and recommended current from LED datasheet. You can calculate it
using the following formula: `R = (V_battery - V_LEDdrop) / I_LED` or just slam
68 Ohm one and call it a day 

You can use available gerber zip archive and order the PCB from your favorite
(or the cheapest) PCB Manufacturer and buy the necessary components listed in
the Bill Of Materials (most of them should be cheap and easy to obtain even
from places like AliExpress)

If you are soldering the PCB by hand, begin with USB port and the button and
solder LED (Cathode to square pad) and JST socket or the Li-ion cell directly.
Don't forget to solder the jumper under the button. Jumper from middle pad to
pad marked with an arrow is variant with Random Seed Generator (Not implemented)
and the jumper from middle pad to the third one (the one without arrow mark) is
variant with battery measurement, so solder that one. 

If for whatever reason don't want to use SOICBite you can solder wires to the
pads directly and connect it to the programmer or program the PIC before
soldering it. Connection diagram can be found in the schematic. 
Example programming jig:

![Programming Jig](media/programming_jig.jpg)

After assembly and connecting the Li-Ion cell you have to connect it to the USB
to start the protection circuits (they go to fail-save if the battery was
disconnected) and you should have a nice and working pedestal!

XXX
To assembly the case you will need two 3D-printed parts (I recommend to print
them with some nice material and fine settings - they should look better than
your average print!), 8 small magnets and glass dome - you should be able to get
them on AliExpress or similar sites. Put the magnets into the holes in such
configuration that two case parts will attract each other. then put the PCB and
the battery into the bottom part, the glass dome into the upper one and they
should snap together. 

XXX IMAGE

## Battery life
On 35mAh Li-ion 1S battery it should work:

- about 20 hours when in random blink mode
- about 15 hours when in both breathing modes
- much longer when in shutdown mode (PIC has to wake up every ~2 seconds just to
go to sleep again)

It mostly depends on the LED used as the firmware of the micro controller does
everything to preserve battery life.

## Gallery can be found in `media` folder if you like photos and videos

### What else can/could be done:
The project is functional as-it-is but there is some room for improvements for
the software (hardware-wise it would be better to just make something simpler
and call it a day):

- Test other effects than RNG-BLINKENKIGHT, Quadwave and Triwave fading - simple
test jig can be found in `effects-workdir` folder.
- Try to rewrite current `quadwave` effect as it's not well suited for PIC10

### Author ramblings about the design

This whole project started because of the one small LED found in old lab
equipment and most of the decisions weren't really well-thought - after all
it wasn't meant to be some mass-produced product (if you want thing like that
you could just buy electronic candle) and as always after the fact there are 
many better ideas just popping into one's head. Bellow are some of them:

- Using other SOT-23 micro controller would probably allow for more effects and
slightly better battery life - for example PIC0F322 or Padauk PMS150C-U06 
or ATiny10, but I had access only to PIC10F222 so I used those. If someone would
want to port this firmware to other micro it should be possible - it's (almost)
C after all.
- Whole battery protection circuit could be ditched as even small Li-ion cell
come with one or at least change FS8205A to SOT-23 version that is smaller 
and easier to solder.
- Some elements around the button pin could be probably removed (they were there
because at that time it was undecided if this pedestal should work only 
on button press or the button would control the function and now they function
as over-complicated debouncing circuit. 
- author thought that an idea to make random seed generator based on
fluctuations of RC time constant where instead of Resistor, a Thermistor would
be used. But as it turned out, 16-bit LSFR was sufficient and showing battery
level seemed more useful in this useless device. If the capacitor will be
charged and pin function would be switched to ADC, the voltage level would
depend only on R value that is dependent on temperature. But on 8-bit fixed
sampling time ADC the variation wouldn't be too big so multiple readings
would be needed to extract some entropy from it so it's just an idea

#### License
BSD 3-Clause