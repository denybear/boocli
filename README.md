# boocli
boocli is a screen-less ("headless") audio looper, based on Jack audio, synchronized with an external MIDI clock signal.  
  
boocli (pronounced "book - Lee") comes from "boucle", the french word for "loop". And this is exactly what boocli is: an audio looper.
One of the challenge of any musician (keyboard, guitar, bass) playing along with a beatbox/groovebox is to create audio loops that are fully in sync with the beat. Any slight desync is audible, and, after some bars, result in drifting between the audio loop (keyboard, guitar, bass, etc) and the groovebox.  
  
boocli aims at being a looper that is always in sync with the groovebox. As a musician, you can record and play multiple tracks, they will always be played in sync with the main tempo, ie. the main clock of your groovebox.  
  
Furthermore, as boocli has been thought with live performance in mind, it is intended to:
- work with Jack audio, alongside other software (a2jmidid)
- work on a limited piece of hardware, ie. Raspberry PI 3B "headless" at minimum
- work without any "on screen" UI. You control the looper through a midi launchpad-type of device
- work with a master midi clock coming from the groovebox
- work with a soundcard connected to the Raspberry PI that captures audio from your guitar/bass/keyboard/microphone etc
- be configurable, ie. you can edit a config file so your own midi launchpad is supported as the UI device  
  
As for hardware, what you need is:
- Raspberry PI with Jack, boocli, and a2jmidid (for midi support of ALSA hardware in Jack)
- a groovebox connected to MIDI CLOCK port of boocli (check out the Jack graph view): this is your master clock, everything will be sync'ed to it! For that one, you would prefer USB MIDI connection over to usual DIN5 serial MIDI. Time-wise, USB is much more stable and accurate than DIN5, which usually causes drifting/desync. 
- a launchpad connected to MIDI IN and MIDI OUT ports of boocli (again... graph view in Jack!), to be used as UI device
- a soundcard connected to audio IN and OUT of boocli: to capture your play  
  
Once you have worked the boocli.cfg config file, you are good to go and will never be out of sync anymore!  
Hope you'll like it!
