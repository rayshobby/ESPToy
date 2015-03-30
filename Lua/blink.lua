led=2
gpio.mode(led, gpio.OUTPUT)
gpio.write(led, gpio.HIGH)
tmr.delay(1000000)
gpio.write(led, gpio.LOW)
