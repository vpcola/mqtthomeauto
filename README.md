# ESP32 MQTT Home Automation Example


* This example will show you how to use I2C module by running tasks on i2c bus:
 
    * read external i2c sensor, here we use a HTU21D humidity and temperature sensor.
    * write to an I2C port expander IC (MCP23017) to interfacing with relays
    * Use I2C port (master mode) in ESP32.
 
* Pin assignment:
 
    * I2C master :
        * GPIO22 is assigned as the SCL (clock) signal of i2c master port
        * GPIO21 is assigned as the SDA (data)  signal of i2c master port
 
    *  LED PWM Dimmer
        * GPIO16 is assigned as a PWM output using the ledc peripheral of the ESP32
        * GPIO17 is assigned as a PWM output using the ledc peripheral of the ESP32
        
* Test items:
 
    * Connect to Wifi.
    * Conned to MQTT Broker securely (using secure web sockets)
    * Subscribe to topics for the dimmer and the relays/switch connected to the port expander
    * Periodically read temperature and humidity data (as soon as connected to MQTT), and publish to MQTT
    * Set port expander data (MCP23017) if data from subscribed topic arrives
    * Set led pwm data if subscribed topic arrives.

TODO:
    * Connect an IR Transmitter to control HVAC (air conditioner) and Fan
