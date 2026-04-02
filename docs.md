AdVisual is an AdLib/OPL2 Chiptune DAW.

AdVisual has two main panels where you will do your work. The one you start off in is the Composer, here you will Create and edit the Tracks notes and events.
The Insmaker panel allows you to create and edit instruments, and manage your bnk files. Click the saxophone icon in the toolbar to switch to it.

## Compiling
It can be compiled with g++, after downloading got to the main project directory and paste this snippet 
`g++ -o ./bin/COMPOSER2 -g ./src/*.cpp ./src/*.c -I ./include/ -I ./src/ -L ./lib/ `wx-config --cxxflags` `wx-config --libs` -ladplug -llibbinio && g  
db --quiet -ex 'run' ./bin/COMPOSER2`
## Files.
 The .rol file format is the only song type supported, it contains the tracks note and event info. You can press load track to open one of the sample rol files, or start editing from scratch.
The .bnk file format contains Instrument data, which can be edited in the Insmaker panel.
## Composer
The grid is made up one dashed vertical line for every beat, and one solid vertical line for every measure, you may change the frequency of ticks and beats in track options in the toolbar. Every dashed horizontal line represents a sharp note, if a note isn't directly on the line then it is a flat note. You can also use the piano guide on the left side as a more familiar reference, click on it to sample a note.

Left click to create a note, hold it to change the length. You can enable Audio Feedback in the toolbar to hear what new notes will sound like.

Right click to place the cursor, hold it to select a range of notes, backspace to delete. Press x, c, v, to cut, copy and paste selection respectively.

Editing the event header, which is placed between the toolbar and the grid, allows you to change the tempo, instrument, pitch, and amplitude dynamically during playback!
## Playback
Press the play button in the toolbar to listen to your track. The OPL2 can support up to 7 melodic (carrier * modulator) channels, and 4 percussion (modulator * modulator) channels. Or you can disable percussion entirely in track settings and get 9 melodic channels as a tradeoff.
Shift Left click a channel to mute it during playback.
## Insmaker
Select the instrument you want to edit in the text edit control in the toolbar.  Melodic instruments have two operators, the Carrier and the Modulator, whereas percussion instruments just use one, the Modulator.
### FM / AM
you can switch between FM (Frequency Modulation) and AM (Ampltude Modulation). In FM mode, FM is the default and is usually preffered. FM is represented by the multiplication symbol(x), in this mode the frequency of the carrier is controlled by the modulator. AM represented by the addition symbol(+) , it adds the output of the carrier and modulator together, affecting amplitude. 