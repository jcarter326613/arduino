To find what com port
```
arduino-cli.exe board list
```

To run
```
arduino-cli.exe compile -b arduino:renesas_uno:unor4wifi e:\Code\mycode\arduino\poll-sensors-and-post
arduino-cli.exe upload -t -p COM3 -b arduino:renesas_uno:unor4wifi e:\Code\mycode\arduino\poll-sensors-and-post
putty COM3 -serial -sercfg 9600,8,n,1,N
```


** Wiring Arduino Uno R4 Wifi **
1. 