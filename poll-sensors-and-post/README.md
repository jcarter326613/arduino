To find what com port
```
arduino-cli.exe board list
```

To run
```
arduino-cli compile -b arduino:renesas_uno:unor4wifi e:\Code\mycode\arduino\poll-sensors-and-post
arduino-cli compile -b esp32:esp32:uPesy_wroom

arduino-cli.exe upload -t -p COM3 -b arduino:renesas_uno:unor4wifi e:\Code\mycode\arduino\poll-sensors-and-post
arduino-cli upload -t -p /dev/cu.usbserial-0001 -b esp32:esp32:uPesy_wroom

putty COM3 -serial -sercfg 9600,8,n,1,N
putty /dev/cu.usbserial-0001 -serial -sercfg 9600,8,n,1,N
```


** Wiring Arduino Uno R4 Wifi **
1. 