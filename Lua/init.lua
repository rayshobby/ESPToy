r=0
g=0
b=0
pin_red=2
pin_green=1
pin_but=3
pin_blue=4
p=0
gpio.mode(pin_but, gpio.INPUT, gpio.PULLUP)
pwm.setup(pin_red, 100, 512)
pwm.setup(pin_green,100,512)
pwm.setup(pin_blue,100, 0)
pwm.start(pin_red)
pwm.start(pin_green)
pwm.start(pin_blue)
tmr.delay(500000)
pwm.setduty(pin_red,0)
pwm.setduty(pin_green,0)
pwm.setduty(pin_blue,0)
wifi.setmode(wifi.SOFTAP)
cfg={}
cfg.ssid="ESPToy" .. (string.sub(wifi.ap.getmac(), 15,17))
cfg.pwd="opendoor"
wifi.ap.config(cfg)

srv=net.createServer(net.TCP, 1)
srv:listen(80,function(conn) 
     conn:on("receive",function(conn,payload) 
          if string.match(payload, " / ") then
               p=1
               print("Serve homepage")
               conn:send("<!DOCTYPE html><html> <head> <title>ESPToy Demo</title> <meta name='viewport' content='width=device-width, initial-scale=1'> </head> <body> <div> <h3>ESPToy Demo</h3> <hr> <div> <table cellpadding=2> <script>function w(s){document.write(s);}function id(s){return document.getElementById(s);}</script> <tr><td><b>Analog</b>:</td><td><label id='lbl_adc'></label></td></tr><tr><td><b>Button</b>:</td><td><label id='lbl_but'></label></td></tr><tr><td><b>Red</b>:</td><td><input type='range' id='s_r' value=0 min=0 max=1000></input></td></tr><tr><td><b>Green</b>:</td><td><input type='range' id='s_g' value=0 min=0 max=1000></input></td></tr><tr><td><b>Blue</b>:</td><td><input type='range' id='s_b' value=0 min=0 max=1000></input></td></tr></table> <div> <button id='btn_ref' onclick='loadValues()'>Refresh Values</button> <button id='btn_sub' onclick='setColor()'>Set Color</button> </div></div></div><script>var devip='http://192.168.4.1';")
          end
          if string.match(payload, " /ja ") then
               p=2
               conn:send("{\"adc\":" .. adc.read(0) .. ",\"but\":" .. (1-gpio.read(pin_but)) .. ",\"red\":" .. r .. ",\"green\":" .. g .. ",\"blue\":" .. b .. "}")
          end
          if string.match(payload, " /cc?") then
               print("Received request to change color")
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
               print("Heap size: " .. node.heap())          
               pwm.setduty(pin_red, r)
               pwm.setduty(pin_green, g)
               pwm.setduty(pin_blue, b)
               
          end
     end) 
     conn:on("sent",function(conn) 
          if p==1 then
               p=2
               conn:send(" function loadValues(){var xmlhttp; if(window.XMLHttpRequest) xmlhttp=new XMLHttpRequest(); else xmlhttp=new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.onreadystatechange=function(){if(xmlhttp.readyState==4 && xmlhttp.status==200){var jd=JSON.parse(xmlhttp.responseText); id('lbl_adc').innerHTML=jd.adc; id('lbl_but').innerHTML=(jd.but)?'pressed':'released'; id('s_r').value=jd.red; id('s_g').value=jd.green; id('s_b').value=jd.blue;}}; xmlhttp.open('GET',devip+'/ja',true); xmlhttp.send();}function setColor(){var xmlhttp; if(window.XMLHttpRequest) xmlhttp=new XMLHttpRequest(); else xmlhttp=new ActiveXObject('Microsoft.XMLHTTP'); xmlhttp.open('GET',devip+'/cc?'+'r='+id('s_r').value+'&g='+id('s_g').value+'&b='+id('s_b').value,true); xmlhttp.send();}setTimeout(loadValues, 500); </script> </body></html>")
          else
               p=0
               conn:close()
          end
     end)
end)

