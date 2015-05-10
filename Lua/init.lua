-- ESPToy Startup Demo
-- This requires file 'esptoy.htm' to exist on the ESP8266 module. Upload the file if it doesn't exist

-- Set up GPIO pins
r=0
g=0
b=0
pin_red=2
pin_green=1
pin_but=3
pin_blue=4
gpio.mode(pin_but, gpio.INPUT, gpio.PULLUP)
pwm.setup(pin_red, 100, 512)
pwm.setup(pin_green,100,512)
pwm.setup(pin_blue,100, 0)
pwm.start(pin_red)
pwm.start(pin_green)
pwm.start(pin_blue)
tmr.delay(200000)
pwm.setduty(pin_red,0)
pwm.setduty(pin_green,0)
pwm.setduty(pin_blue,0)

-- Set up WiFi
wifi.setmode(wifi.SOFTAP)
cfg={}
cfg.ssid="ESPToy" .. (string.sub(wifi.ap.getmac(), 15,17))
cfg.pwd="opendoor"
wifi.ap.config(cfg)

if srv then
  srv:close()
end
srv=net.createServer(net.TCP, 3)
srv:listen(80,function(conn) 
  conn:on("receive", function(conn,payload)
    local isOpen=false
    conn:on("sent", function(conn)
      if not isOpen then
        isOpen=true
        file.open(fileName, 'r')
      end
      local data=file.read(1024)
      if data then
        conn:send(data)
      else
        file.close()
        conn:close()
        conn=nil
      end
    end)
    
    if string.sub(payload, 1, 6) == 'GET / ' then
      fileName = 'esptoy.htm'
      conn:send("HTTP/1.1 200 OK\r\n")
      conn:send("Content-type: text/html\r\n")
      conn:send("Connection: close\r\n\r\n")
    elseif string.sub(payload, 1, 8) == 'GET /ja ' then
      conn:send("HTTP/1.1 200 OK\r\n")
      conn:send("Content-type: application/json\r\n")
      conn:send("Connection: close\r\n\r\n")
      conn:send("{\"adc\":" .. adc.read(0) .. ",\"but\":" .. (1-gpio.read(pin_but)) .. ",\"red\":" .. r .. ",\"green\":" .. g .. ",\"blue\":" .. b .. "}")
      conn:close()
      conn=nil
    elseif string.sub(payload, 1, 8) == 'GET /cc?' then
      conn:close()
      conn=nil

      local val=string.match(payload,"r=(%d+)")
      if val then
        r=tonumber(val)
      end
      local val=string.match(payload,"g=(%d+)")
      if val then
        g=tonumber(val)
      end
      local val=string.match(payload,"b=(%d+)")
      if val then
        b=tonumber(val)
      end
      pwm.setduty(pin_red, r)
      pwm.setduty(pin_green, g)
      pwm.setduty(pin_blue, b)
    else
      conn:close()
    end
  end) 
end)

