led=4
button=3
gpio.mode(led, gpio.OUTPUT)
gpio.write(led, gpio.LOW)
gpio.mode(button, gpio.INT)

gpio.trig(button, "both", function(level)
  if level==0 then
    gpio.write(led, gpio.HIGH)
  else
    gpio.write(led, gpio.LOW)
  end
end)

