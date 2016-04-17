# Smarthome Controller
Control Philips Hue (and eventually other stuff) from the command line.

### Setup
Clone this repo:
```
$ git clone https://github.com/hunterforsyth/smarthomecontroller.git
```
Edit ```config.h``` to pair Hue Bridge, instructions inside.

Run:
```
$ sudo make install
```

### Usage
Retrieve a list of all connected lights by their lightnum:
```
$ home listlights
```


To control a single light:
```
$ home light [lightnum] [r],[g],[b]
```
for example:
```
$ home light 1 255,0,255
```
To control multiple lights:
```
$ home lights 1,2,3 255,0,255
```


More examples:
```
$ home lights all 255,255,255
$ home light 1 red
$ home lights all off
```


Run ```$ home help``` for more info.
