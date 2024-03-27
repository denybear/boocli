#!/bin/bash
sleep 10
export DISPLAY=:0
export HOME=/home/pi

# get connected devices
headphones=$( aplay -L | grep plughw:CARD=Headphones | awk -F ',' '{print $1}')
io2=$( aplay -L | grep plughw:CARD=io2 | awk -F ',' '{print $1}')
matribox=$( aplay -L | grep plughw:CARD=II | awk -F ',' '{print $1}')
hwdevice=$( aplay -L | grep plughw:CARD=Device | awk -F ',' '{print $1}')
istore=$( aplay -L | grep plughw:CARD=Audio | awk -F ',' '{print $1}')

echo "headphones: $headphones"
echo "io2: $io2"
echo "matribox II: $matribox"
echo "USB hardware sound device 7.1: $hwdevice"
echo "USB simple sound device: $istore"

if [ -n "$io2" ]; then
	soundcard=$io2
	device="hw:io2"
elif [ -n "$matribox" ]; then
	soundcard=$matribox
	device="hw:II"
elif [ -n "$hwdevice" ]; then
	soundcard=$hwdevice
	device="hw:Device"
elif [ -n "$istore" ]; then
	soundcard=$istore
	device="hw:Audio"
elif [ -n "$headphones" ]; then
	soundcard=$headphones
	device="hw:Headphones"
else
	echo "NO SOUNDCARD FOUND !!!"
fi

# launch boocli
export JACK_NO_AUDIO_RESERVATION=1
jackd --realtime --realtime-priority 70 --port-max 30 --silent -d alsa --device $device --nperiods 3 --rate 48000 --period 128 &
sleep 5
a2jmidid -ue &
sleep 5
/home/pi/boocli/boocli.a /home/pi/boocli/boocli.cfg
