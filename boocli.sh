#!/bin/bash

sleep 10
export DISPLAY=:0
export HOME=/home/pi
export JACK_NO_AUDIO_RESERVATION=1
jackd --realtime --realtime-priority 70 --port-max 20 --silent -d alsa --device hw:1 --nperiods 3 --rate 48000 --period 64 &
sleep 5
a2jmidid -ue &
sleep 5
/home/pi/boocli/boocli.a /home/pi/boocli/boocli.cfg
