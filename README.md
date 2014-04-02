# EasyPay

## About EasyPay
|         |                                                               |
| ------- | ------------------------------------------------------------- |
| Author  | Nnoduka Eruchalu                                              |
| Date    | 05/16/2013                                                    |
| Website | [strivinglink.com/easypay/](http://strivinglink.com/easypay/) |

EasyPay takes the pain out of paying for utilities and parking in Nigeria with customized hardware and secure NFC smartcards. 

In summary, EasyPay will change the lives of Nigerians. To learn more about it visit the [website](http://strivinglink.com/easypay/)

This repo contains the code running the hardware's embedded system.


## Software Technologies
* C
* Microcontrollers (MCUs)
* NFC ([MIFARE](http://en.wikipedia.org/wiki/MIFARE))


## Software Modules Description
| Module      | Description                                                    |
| ----------- | -------------------------------------------------------------- |
| `data`      | Functions for communications between MCU and the HTTP Server   |
| `delay`     | Functions for implementing timed delays in the MCU             |
| `eventproc` | Functions for handling actions defined in `interface`'s FSM    |
| `interface` | System's Finite State Machine (FSM) tables and LCD display contents for various UI states |
| `interrupts` | Functions for initializing MCU interrupts                     |
| `keypad`   | Functions for interfacing the MCU with a 4x4 matrix keypad      |
| `lcd`      | Functions for interfacing the MCU with a 20x4 character LCD     |
| `main`     | Main loop of embedded system                                    |
| `mifare.c` | Functions for initializing communications with a MIFARE DESFire smartcard |
| `mifare/`  | Functions for full implementation of MIFARE DESFire communication protocols. |
| `queue`    | Functions for implementing a circular FIFO array with one empty slot |
| `serial`   | Functions for interfacing with the MCU's USART module           |
| `sim5218`  | Functions for interfacing with the 3G Module [Sim5218A]         |
| `smartcard` | Functions for Detecting and initializing communications with a SmartCard |
| `test/`    | E2E test framework for all modules                              |


## User Interface
#### Keypad:
```
+---------------------------+
| 1       2       3       A |
|                           |
| 4       5       6       B |
|                           |
| 7       8       9       C |
|                           |
| *       0       #       D |
+---------------------------+
```

#### Display:
###### Welcome Screen:
```
+--------------------+
|-Welcome to EasyPay-|
|                    |
| Tap Card to Start  |
|                    |
+--------------------+
```

###### On Card Tap --> Enter Pin Screen:
```
+--------------------+           +--------------------+ 
|-    Enter Pin     -|           |-    Enter Pin     -|
|        _           |  <0..9>   |        ****        |
|*Exit*           → C|   ==>     |*Undo*           → C|
|*Done*           → D|           |*Done*           → D| 
+--------------------+           +--------------------+
```


###### On Correct Pin --> Home Screen:
Press buttons indicated on right to navigate
```
+--------------------+ 
|EasyAccount      → A|
|Parking          → B|
|Mobile Recharge  → C|
|Utility Bills    → D|
+--------------------+
```

###### EasyAccount Screen:
```
+--------------------+ 
|-   EasyAccount    -|
|Balance: N50,000.00 |
|Recharge         → C|
|*Exit*           → D|
+--------------------+
```

###### Parking Screen:
```
+--------------------+           +--------------------+ 
|- Pay for Parking  -|           |- Pay for Parking  -|
|Enter Space: 010_   |    <D>    |Enter Time: 0:30_   |
|                    |    ==>    |                    |
|*Done*           → D|           |*Done*           → D|
+--------------------+           +--------------------+ 
```

###### Mobile Recharge:
```
+--------------------+           +--------------------+ 
|- Mobile Recharge  -|           |*Back*           → A|
|MTN              → B|    <D>    |Airtel           → B|
|GLO              → C|    ==>    |Etisalat         → C|
|*More*           → D|           |*Exit*           → D|
+--------------------+           +--------------------+ 
```

###### Utility Bills:
```
+--------------------+           +--------------------+   
|-  Utility Bills   -|           |- Utility: Power   -|
|Power            → B|    <B>    |Amount: N10,000.00_ | 
|Water            → C|    ==>    |                    | 
|*Exit*           → D|           |*Done*           → D|
+--------------------+           +--------------------+
```


#### Smart Card Procedures:
Tap card/Enter Pin on welcome, to save smartcard info locally.
After each transaction, tap card again to confirm.
Reason for this is to ensure it's same user and to sync card info with smart


## Demo
Click on the video below to see  a demo of the prototype module in action:

[![Demo Video](http://img.youtube.com/vi/BhdTa_jyTG4/0.jpg)](https://www.youtube.com/watch?v=BhdTa_jyTG4)

## Todo:
* [x] Perform tests on PIC8.
* [ ] Finish full MIFARE Hardware tests. Need to migrate to an AVR32UC for this.


## Resources:
`mifare/` module based on the following sources:

* [libfreefare](https://code.google.com/p/libfreefare/)
* [Draft ISO/IEC FCD 14443-4](http://www.waazaa.org/download/fcd-14443-4.pdf) 
* [Ridrix's Blog post on a MIFARE Desfire communication example](https://bitbucket.org/nceruchalu/ray_tracer/)
* MIFARE DESFire datasheets