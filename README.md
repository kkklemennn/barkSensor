# barkSensor
A quick, simple yet fully functional afternoon project in an attempt to stop our dog from barking when he sees the neighbours cat through the window. So far, I could say that the sensor serves its purpose pretty well as there is a noticable improvement of dog barking less then it used to :)

## A short explanation of the backend
In principle it is just a simple queue of 4 elements that stores the input of max value measured on a microphone during 10 second interval (yes, i really did measure, the dog barks on average once a little less then every 10 seconds or so). If there are more then 3 values in the queue that are above certain threshold, the dog is almost certainly barking and "bark" message is published on channel "bark/dog" via MQTT. Then when all values in queue drop below the given threshold, the dog almost certainly stopped barking and "silence" message is published on channel "bark/dog" via MQTT.

## Implementation
In my case, sensor is plugged into the USB port of an access point (but it should in theory draw very little current and could be battery powered aswell) which is in the center of the living room i.e. where the windows from where the dog sees the cat are. The messages are read in Home Assistant in a Node-RED flow, where the received messages trigger to close or open the shades accordingly.
### Node-RED flow
![Node-RED flow](https://i.imgur.com/EgsQtjC.png)

## Sketch of the hardware
![Node-RED flow](https://i.imgur.com/zFz1xEV.png)
## Interacting with the sensor

**Enabling/disabling debug info messages:**
`bark/debug: "on"`
`bark/debug: "off"`
By default debug over MQTT is disabled in order to use less energy. Enabling debug over MQTT, the structured JSON object containing current peak and threshold is published every 10 seconds on channel "bark/dog".

Example debug message:
```
{
	"peak": 89,
	"threshold": 145
}
```

**Setting threshold**
`bark/setthreshold: int`