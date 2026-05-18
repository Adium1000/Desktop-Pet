# Desktop Pet V0.1

![T](.github/Cat/T.png)


![overview](.github/Cat/!OV.png)

![d](.github/desktop.png)

- A simple desktop pet that makes your desk more intreasting  :D

![APS](.github/Cat/!APS.png)

You can have a demo at the desktop pet's firmware [(Here)](https://adium1000.github.io/Desktop-Pet/)

![Media Controller](.github/Cat/media.png)
- This is a Media Controller app for your device that supplyes power to the Desktop Pet
- Tap the touch sensor to navigate and hold to execute the selected action
- It uses Adafruit Tiny USB to do that

![media](.github/APS/media.png)

![Pong](.github/Cat/pong.png)

- Pong is an game for your Desktop Pet
- To play just tap on the touch sensor to move the paddle
- When you lose (ai scores) you can long press to exit or tap to play again

![pong_game](.github/APS/pong.png)![pong_lose](.github/APS/pong_lost.png)

![alt text](.github/Cat/x&0.png)

- X&0 is also a game for your Desktop Pet
- To play you have to tap on the touch sensor and hold where you want to place x (you always play as x)
- You can exit by holding on the touch sensor one the match is finnished and tap to play again

![x&0](.github/APS/x&0.png)

![alt text](.github/Cat/timer.png)

- Timer is a small app that can count time when you start it
- To start the timer tap on the touch sensor, and to stop tap again
- To exit, stop the timer and hold the touch sersor, if you hold while timer is going you will reset the timer

![timer](.github/APS/timer.png)

![alt text](.github/Cat/sensors.png)

- Sesors app lets you acces the BM180 Sensor's data like temperature and atmosferic presure
- Hold the touch sensor to exit

![sensors](.github/APS/sensors.png)

![SS](.github/Cat/ss.png)
- Screensavers are a way to bring more fun to the pet, here you can pick from a large library of them and play them!
- While on screensver tap the touch sensor to exit or hold to exit to the screensaver library

![ss](.github/APS/screensaver.png)

![settings](.github/Cat/settings.png)
- The settings app lets you turn off the buzzer or turn on automatic switch which will cycle between FACE-TEMP-AP
- Go to the exit button and hold the touch to exit

![Settings](.github/APS/settings.png)

![off](.github/Cat/off.png)

- When oppened from launcher it turns off the oled
- Tap the touch sensor to turn oled back on

![off](.github/APS/off.png)


![CAD](.github/Cat/!CAD.png)

- Here you will find all relevant stuff about this project and how it was made

![Schematics](.github/Cat/!SC.png)

- The schematics for this project were made in Fritzing 

![schematics](CAD/Schematics/Schematics.png)

This project uses an 0.9 Inch OLED with a WS RP2040 Microcontroller, a buzzer, a touch sensor and a BM180 sensor

![Firmware](.github/Cat/!FR.png)

- The firmware for this desktop pet is located in `CAD/Firmware` and it is ment for RP2040 Microcontrollers 

![CASE](.github/Cat/!CA.png)

- The case was originaly created by Thingiverse user visakhmv, and is licensed under Creative Commons - Attribution - Share Alike, remixed by Adium1000
- All components are installed in the case using hot glue


![case](CAD/Case/3d.png)

![BOM](.github/Cat/BOM.png)

| # | Component | Price | Note |
|---|---|---|---|
| 1 | Waveshare RP2040-Zero | ~$4.80 | LCSC/AliExpress |
| 2 | Passive Buzzer Module | ~$0.50 |  AliExpress |
| 3 | BMP180 Sensor Module (GY-68) | ~$0.60 | AliExpress |
| 4 | Touch Sensor (TTP223B) | ~$0.40 | AliExpress |
| 5 | Fire (Dupont wires, set) | ~$1.00 | M-M or M-F |
| 6 | ~19g filament PLA | ~$0.48 | ~$0.025/g × 19g |
| | **TOTAL** | **~$7.78** | |