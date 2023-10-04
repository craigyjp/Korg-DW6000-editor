# Korg-DW6000-editor
A built in sysex editor for the Korg DW6000 wavetable synthesizer

I picked up a broken Korg DW6000 for £100 a few months back, fixed it fairly quickly with a broken logic chip.

Back in the 80's I wrote an editor for the DW on my Amstrad CPC 6128, well computers have come a long way since then and a Teensy 4.1 probably has more power than the CPC6128 and certainly more RAM.

So I thought it would be good to build a hardware editor directly into the synth for each parameter and store them in memory. Also add an OLED display to show what is being edited.

Upgraded the DW6000 with the 128 waveform mod in banks of 8. Unfortunately the parameter 14 currently doesnt change over MIDI, developers say it was never really tested and it was done a while back. I'm in no position to fix it, so it sort of negates the 128 waveform expansion from the editor. Yo ucan still edit from the synth and maybe a fix will come along, who knows.

The editor board was a sandwich of proto board to get the spacing almost right to fit the pots where the numbers were for each parameter, everything is mounted on a single board and intercepts the incoming MIDI and adds usbMIDI support. Plus each parameter is now mapped to an incoming CC message for easier editing from a DAW etc.

* 999 Memories.
* USB to MIDI conversion.
* CC message control.
* Master control sends all params for each patch from memory to the DW.
* slave mode, edits the DWs own patches.
* Reload of factory patches from the settings menu.
* Dump of current patch as a sysex file.
* Dump all patches over MIDI as sysex files (64).
* 128 waveform mod support with Param 14 (currently not working from pot, but param is stored).

* Still to do, fix slave/master mode.
* Accept sysex patches over MIDI would be good.

  
