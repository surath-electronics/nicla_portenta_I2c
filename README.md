#Nicla stream at 333Hz
#Portenta streams >250Hz

To Stream the portenta data in the Terminal use:

mosquitto_sub -h 192.168.0.212 -t "nicla/data" -v