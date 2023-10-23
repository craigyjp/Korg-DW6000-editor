# Korg-DW6000-editor
A built in sysex editor for the Korg DW6000 wavetable synthesizer

I picked up a broken Korg DW6000 for £100 a few months back, fixed it fairly quickly with a broken logic chip.

Back in the 80's I wrote an editor for the DW on my Amstrad CPC 6128, well computers have come a long way since then and a Teensy 4.1 probably has more power than the CPC6128 and certainly more RAM.

So I thought it would be good to build a hardware editor directly into the synth for each parameter and store them in memory. Also add an OLED display to show what is being edited.

![Synth](photos/synth.jpg)

Upgraded the DW6000 with the 128 waveform mod in banks of 8. Unfortunately the parameter 14 currently doesn't change over MIDI, developers say it was never really tested and it was done a while back. I'm in no position to fix it, so it sort of negates the 128 waveform expansion from the editor. You can still edit from the synth and maybe a fix will come along, who knows.

The editor board was a sandwich of proto board to get the spacing almost right to fit the pots where the numbers were for each parameter, everything is mounted on a single board and intercepts the incoming MIDI and adds usbMIDI support. Plus each parameter is now mapped to an incoming CC message for easier editing from a DAW etc.

* 999 Memories.
* USB to MIDI conversion.
* CC message control.
* Master control sends all params for each patch from memory to the DW.
* Slave mode, edits the DWs own patches.
* Reload of factory patches from the settings menu.
* Dump of current patch as a sysex file over MIDI to receiving device.
* Dump all patches over MIDI as sysex files (64) to receiving device.
* 128 waveform mod support with Param 14 (currently not working from pot, but param is stored).
* Receives a Sysex Patch over MIDI and loads it into the Synth and editor
* Load existing DW6000 patches in to patches 1-64 of the editor.

# Operation

* Send a Sysex patch to the DW6000 MIDI in and the editor will load the parameters into the editor and send them onto the DW6000, the patch name will be displayed as "Sysex Patch" and you can save it and rename it as you wish.
* Send the current patch, this sends the current patch from the DW6000 out over MIDI as a Sysex packet and follows the standard for the DW6000.
* Send all patches, this sends all 64 patches out over MIDI as 64 Sysex patches.
* Send Params (Master Mode), sends all the parameters of the recalled editor patch to the DW6000, the program number of the DW6000 does not change, This allows you to have full access to the 999 patches that can be held in the editor.
* Send Params (Off), send a program change to the DW6000 as you recall each patch, but does not send patch data on recall. It allows editing of existing patches in the DW6000 without over riding them.
* Load from DW, this will receive all your current patches from the DW6000 into the editor, useful for when you already have customized patches in your DW6000. To do this you need to connect a MIDI lead from the MIDI out of the DW6000 to the MIDI in of the DW6000, a loop effectively. Each patch is then requested from the DW6000 from 11-88 and stored in editor locations 1-64. The patches are named "Patch 1", "Patch 2" etc as the DW6000 holds no patch name information. Its a tedious process but you can save and rename each patch with something memorable in the editor.
* Load from Factory, a very dangerous option, but useful for some people, it loads all the factory sounds 11-88 into the DW6000 and the editor patches 1-64 with the same names as the factory patches.
* MIDI channel, sets the operation channel of incoming data for the editor.
* MIDI channel Out, sets the outgoing MIDI channel to the DW6000, really this can be left of channel 1. But if you do decide you want the DW6000 on a different transmit channel then you can set this MIDI out channel to match the DW6000 MIDI setting.
* Encoder Direction, simply sets the direction of the encoder movement. Can be reversed if neccessary.
  
# CC to Sysex conversion

* osc1_octave 10
* osc1_waveform 11
* osc1_level 12

* osc2_octave 13
* osc2_waveform 14
* osc2_level 15
* osc2_interval 16
* osc2_detune 17
* noise 18

* vcf_cutoff 74
* vcf_res 71
* vcf_kbdtrack 21
* vcf_polarity 22
* vcf_eg_intensity 23
* chorus 93

* vcf_attack 25
* vcf_decay 26
* vcf_breakpoint 27
* vcf_slope 28
* vcf_sustain 29
* vcf_release 30

* vca_attack 31
* vca_decay 32
* vca_breakpoint 33
* vca_slope 34
* vca_sustain 35
* vca_release 36

* mg_frequency 37
* mg_delay 38
* mg_osc 39
* mg_vcf 40

* bend_osc 41
* bend_vcf 42
* glide_time 43

* wave_bank 44

* poly1 47
* poly2 48
* unison 49

  


  
