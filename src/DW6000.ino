/*
  DW6000 Editor - Firmware Rev 1.0

  Includes code by:
    Dave Benn - Handling MUXs, a few other bits and original inspiration  https://www.notesandvolts.com/2019/01/teensy-synth-part-10-hardware.html
    ElectroTechnique for general method of menus and updates.

  Arduino IDE
  Tools Settings:
  Board: "Teensy4,1"
  USB Type: "Serial + MIDI"
  CPU Speed: "600"
  Optimize: "Fastest"

  Performance Tests   CPU  Mem
  180Mhz Faster       81.6 44
  180Mhz Fastest      77.8 44
  180Mhz Fastest+PC   79.0 44
  180Mhz Fastest+LTO  76.7 44
  240MHz Fastest+LTO  55.9 44

  Additional libraries:
    Agileware CircularBuffer available in Arduino libraries manager
    Replacement files are in the Modified Libraries folder and need to be placed in the teensy Audio folder.
*/

#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <MIDI.h>
#include "MidiCC.h"
#include "Constants.h"
#include "Parameters.h"
#include "PatchMgr.h"
#include "HWControls.h"
#include "EepromMgr.h"

#define PARAMETER 0      //The main page for displaying the current patch and control (parameter) changes
#define RECALL 1         //Patches list
#define SAVE 2           //Save patch page
#define REINITIALISE 3   // Reinitialise message
#define PATCH 4          // Show current patch bypassing PARAMETER
#define PATCHNAMING 5    // Patch naming page
#define DELETE 6         //Delete patch page
#define DELETEMSG 7      //Delete patch message page
#define SETTINGS 8       //Settings page
#define SETTINGSVALUE 9  //Settings page

unsigned int state = PARAMETER;

#include "ST7735Display.h"

boolean cardStatus = false;

//MIDI 5 Pin DIN
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);

byte ccType = 2;        //(EEPROM)
byte updateParams = 0;  //(EEPROM)

#include "Settings.h"


int count = 0;  //For MIDI Clk Sync
int DelayForSH3 = 12;
int patchNo = 1;               //Current patch no
int voiceToReturn = -1;        //Initialise
long earliestTime = millis();  //For voice allocation - initialise to now

byte byteArray[8];
byte writeRequest[7];

void setup() {
  SPI.begin();
  setupDisplay();
  setUpSettings();
  setupHardware();

  byteArray[0] = 0xF0;  // Start of SysEx
  byteArray[1] = 0x42;  // Manufacturer ID (example value)
  byteArray[2] = 0x30;  // Data byte 1 (example value)
  byteArray[3] = 0x04;  // Data byte 2 (example value)
  byteArray[4] = 0x41;  // Parameter Change
  byteArray[5] = 0x00;
  byteArray[6] = 0x00;
  byteArray[7] = 0xF7;  // End of Exclusive

  writeRequest[0] = 0xF0;  // Start of SysEx
  writeRequest[1] = 0x42;  // Manufacturer ID (example value)
  writeRequest[2] = 0x30;  // Format ID 42H
  writeRequest[3] = 0x04;  // DW6000 ID
  writeRequest[4] = 0x11;  // Write request
  writeRequest[5] = 0x00;
  writeRequest[6] = 0xF7;  // End of Exclusive

  cardStatus = SD.begin(BUILTIN_SDCARD);
  if (cardStatus) {
    Serial.println("SD card is connected");
    //Get patch numbers and names from SD card
    loadPatches();
    if (patches.size() == 0) {
      //save an initialised patch to SD card
      savePatch("1", INITPATCH);
      loadPatches();
    }
  } else {
    Serial.println("SD card is not connected or unusable");
    reinitialiseToPanel();
    showPatchPage("No SD", "conn'd / usable");
  }

  //Read MIDI Channel from EEPROM
  midiChannel = getMIDIChannel();
  Serial.println("MIDI Ch:" + String(midiChannel) + " (0 is Omni On)");

  //Read UpdateParams type from EEPROM
  updateParams = getUpdateParams();

  //USB Client MIDI
  usbMIDI.setHandleControlChange(myConvertControlChange);
  usbMIDI.setHandleProgramChange(myProgramChange);
  usbMIDI.setHandleNoteOff(myNoteOff);
  usbMIDI.setHandleNoteOn(myNoteOn);
  Serial.println("USB Client MIDI Listening");

  //MIDI 5 Pin DIN
  MIDI.begin();
  MIDI.setHandleControlChange(myConvertControlChange);
  MIDI.setHandleProgramChange(myProgramChange);
  MIDI.setHandleNoteOn(myNoteOn);
  MIDI.setHandleNoteOff(myNoteOff);
  Serial.println("MIDI In DIN Listening");

  //Read Encoder Direction from EEPROM
  encCW = getEncoderDir();
  //Read MIDI Out Channel from EEPROM
  midiOutCh = getMIDIOutCh();

  recallPatch(patchNo);  //Load first patch
}

void myNoteOn(byte channel, byte note, byte velocity) {
  MIDI.sendNoteOn(note, velocity, channel);
}

void myNoteOff(byte channel, byte note, byte velocity) {
  MIDI.sendNoteOff(note, velocity, channel);
}

void myConvertControlChange(byte channel, byte number, byte value) {
  switch (number) {
    case 1:
      MIDI.sendControlChange(number, value, channel);
      break;

    case 2:
      MIDI.sendControlChange(number, value, channel);
      break;

    case 7:
      MIDI.sendControlChange(number, value, channel);
      break;

    case 64:
      MIDI.sendControlChange(number, value, channel);
      break;

    case 65:
      MIDI.sendControlChange(number, value, channel);
      break;

    default:
      int newvalue = value;
      myControlChange(channel, number, newvalue);
      break;
  }
}

void myPitchBend(byte channel, int bend) {
  MIDI.sendPitchBend(bend, channel);
}

void allNotesOff() {
}

void updateosc1_octave() {
  if (!recallPatchFlag) {
    switch (osc1_octave) {
      case 2:
        showCurrentParameterPage("Osc1 Octave", String("4 Foot"));
        break;
      case 1:
        showCurrentParameterPage("Osc1 Octave", String("8 Foot"));
        break;
      case 0:
        showCurrentParameterPage("Osc1 Octave", String("16 Foot"));
        break;
    }
  }
  midiCCOut(CCosc1_octave, 19, osc1_octave);
}

void updateosc1_waveform() {
  if (!recallPatchFlag) {
    switch (osc1_waveform) {
      case 0:
        showCurrentParameterPage("Osc1 Wave", String("Brass/Strings"));
        break;

      case 1:
        showCurrentParameterPage("Osc1 Wave", String("Violin"));
        break;

      case 2:
        showCurrentParameterPage("Osc1 Wave", String("A Piano"));
        break;

      case 3:
        showCurrentParameterPage("Osc1 Wave", String("E Piano"));
        break;

      case 4:
        showCurrentParameterPage("Osc1 Wave", String("Synth Bass"));
        break;

      case 5:
        showCurrentParameterPage("Osc1 Wave", String("Saxaphone"));
        break;

      case 6:
        showCurrentParameterPage("Osc1 Wave", String("Clavi"));
        break;

      case 7:
        showCurrentParameterPage("Osc1 Wave", String("Bell & Gong"));
        break;
    }
  }
  midiCCOut(CCosc1_waveform, 24, osc1_waveform);
}

void updateosc1_level() {
  if (!recallPatchFlag) {
    if (osc1_level == 0) {
      showCurrentParameterPage("Osc1 Level", String("Off"));
    } else {
      showCurrentParameterPage("Osc1 Level", String(osc1_level));
    }
  }
  midiCCOut(CCosc1_level, 2, osc1_level);
}

void updateosc2_octave() {
  if (!recallPatchFlag) {
    switch (osc2_octave) {
      case 2:
        showCurrentParameterPage("Osc2 Octave", String("4 Foot"));
        break;
      case 1:
        showCurrentParameterPage("Osc2 Octave", String("8 Foot"));
        break;
      case 0:
        showCurrentParameterPage("Osc2 Octave", String("16 Foot"));
        break;
    }
  }
  midiCCOut(CCosc2_octave, 20, osc2_octave);
}

void updateosc2_waveform() {
  if (!recallPatchFlag) {
    switch (osc2_waveform) {
      case 0:
        showCurrentParameterPage("Osc2 Wave", String("Brass/Strings"));
        break;

      case 1:
        showCurrentParameterPage("Osc2 Wave", String("Violin"));
        break;

      case 2:
        showCurrentParameterPage("Osc2 Wave", String("A Piano"));
        break;

      case 3:
        showCurrentParameterPage("Osc2 Wave", String("E Piano"));
        break;

      case 4:
        showCurrentParameterPage("Osc2 Wave", String("Synth Bass"));
        break;

      case 5:
        showCurrentParameterPage("Osc2 Wave", String("Saxaphone"));
        break;

      case 6:
        showCurrentParameterPage("Osc2 Wave", String("Clavi"));
        break;

      case 7:
        showCurrentParameterPage("Osc2 Wave", String("Bell & Gong"));
        break;
    }
  }
  midiCCOut(CCosc2_waveform, 24, osc2_waveform);
}

void updateosc2_level() {
  if (!recallPatchFlag) {
    if (osc2_level == 0) {
      showCurrentParameterPage("Osc2 Level", String("Off"));
    } else {
      showCurrentParameterPage("Osc2 Level", String(osc2_level));
    }
  }
  midiCCOut(CCosc2_level, 3, osc2_level);
}

void updateosc2_interval() {
  if (!recallPatchFlag) {
    switch (osc2_interval) {
      case 0:
        showCurrentParameterPage("Osc2 Interval", String("Off"));
        break;
      case 1:
        showCurrentParameterPage("Osc2 Interval", String("-3"));
        break;
      case 2:
        showCurrentParameterPage("Osc2 Interval", String("+3"));
        break;
      case 3:
        showCurrentParameterPage("Osc2 Interval", String("+4"));
        break;
      case 4:
        showCurrentParameterPage("Osc2 Interval", String("+5"));
        break;
    }
  }
  midiCCOut(CCosc2_interval, 25, osc2_interval);
}

void updateosc2_detune() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("Osc2 Detune", String(osc2_detune));
  }
  midiCCOut(CCosc2_detune, 25, osc2_detune);
}

void updatenoise() {
  if (!recallPatchFlag) {
    if (noise == 0) {
      showCurrentParameterPage("Noise Level", String("Off"));
    } else {
      showCurrentParameterPage("Noise Level", String(noise));
    }
  }
  midiCCOut(CCnoise, 4, noise);
}

void updatevcf_cutoff() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Cutoff", String(vcf_cutoff));
  }
  midiCCOut(CCvcf_cutoff, 5, vcf_cutoff);
}

void updatevcf_res() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Res", String(vcf_res));
  }
  midiCCOut(CCvcf_res, 6, vcf_res);
}

void updatevcf_kbdtrack() {
  if (!recallPatchFlag) {
    switch (vcf_kbdtrack) {
      case 0:
        showCurrentParameterPage("KBD Track", String("Off"));
        break;
      case 1:
        showCurrentParameterPage("KBD Track", String("Half"));
        break;
      case 2:
        showCurrentParameterPage("KBD Track", String("Full"));
        break;
    }
  }
  midiCCOut(CCvcf_kbdtrack, 21, vcf_kbdtrack);
}

void updatevcf_polarity() {
  if (!recallPatchFlag) {
    switch (vcf_polarity) {
      case 0:
        showCurrentParameterPage("EG Polarity", String("Positive"));
        break;
      case 1:
        showCurrentParameterPage("EG Polarity", String("Negative"));
        break;
    }
  }
  midiCCOut(CCvcf_polarity, 22, vcf_polarity);
}

void updatevcf_eg_intensity() {
  if (!recallPatchFlag) {
    if (vcf_eg_intensity == 0) {
      showCurrentParameterPage("VCF EG level", String("Off"));
    } else {
      showCurrentParameterPage("VCF EG level", String(vcf_eg_intensity));
    }
  }
  midiCCOut(CCvcf_eg_intensity, 7, vcf_eg_intensity);
}

void updatechorus() {
  if (!recallPatchFlag) {
    switch (chorus) {
      case 0:
        showCurrentParameterPage("Chorus", String("Off"));
        break;
      case 1:
        showCurrentParameterPage("Chorus", String("On"));
        break;
    }
  }
  midiCCOut(CCchorus, 23, chorus);
}

void updatevcf_attack() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Attack", String(vcf_attack));
  }
  midiCCOut(CCvcf_attack, 8, vcf_attack);
}

void updatevcf_decay() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Decay", String(vcf_decay));
  }
  midiCCOut(CCvcf_decay, 9, vcf_decay);
}

void updatevcf_breakpoint() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF B.Point", String(vcf_breakpoint));
  }
  midiCCOut(CCvcf_breakpoint, 10, vcf_breakpoint);
}

void updatevcf_slope() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Slope", String(vcf_slope));
  }
  midiCCOut(CCvcf_slope, 11, vcf_slope);
}

void updatevcf_sustain() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Sustain", String(vcf_sustain));
  }
  midiCCOut(CCvcf_sustain, 12, vcf_sustain);
}

void updatevcf_release() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCF Release", String(vcf_release));
  }
  midiCCOut(CCvcf_release, 13, vcf_release);
}

void updatevca_attack() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCA Attack", String(vca_attack));
  }
  midiCCOut(CCvca_attack, 14, vca_attack);
}

void updatevca_decay() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCA Decay", String(vca_decay));
  }
  midiCCOut(CCvca_decay, 15, vca_decay);
}

void updatevca_breakpoint() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCA B.Point", String(vca_breakpoint));
  }
  midiCCOut(CCvca_breakpoint, 16, vca_breakpoint);
}

void updatevca_slope() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCA Slope", String(vca_slope));
  }
  midiCCOut(CCvca_slope, 17, vca_slope);
}

void updatevca_sustain() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCA Sustain", String(vca_sustain));
  }
  midiCCOut(CCvca_sustain, 18, vca_sustain);
}

void updatevca_release() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("VCA Release", String(vca_release));
  }
  midiCCOut(CCvca_release, 19, vca_release);
}

void updatemg_frequency() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("MG Frequency", String(mg_frequency));
  }
  midiCCOut(CCmg_frequency, 20, mg_frequency);
}

void updatemg_delay() {
  if (!recallPatchFlag) {
    showCurrentParameterPage("MG Delay", String(mg_delay));
  }
  midiCCOut(CCmg_delay, 21, mg_delay);
}

void updatemg_osc() {
  if (!recallPatchFlag) {
    if (mg_osc == 0) {
      showCurrentParameterPage("MG to Osc", String("Off"));
    } else {
      showCurrentParameterPage("MG to Osc", String(mg_osc));
    }
  }
  midiCCOut(CCmg_osc, 22, mg_osc);
}

void updatemg_vcf() {
  if (!recallPatchFlag) {
    if (mg_vcf == 0) {
      showCurrentParameterPage("MG to VCF", String("Off"));
    } else {
      showCurrentParameterPage("MG to VCF", String(mg_vcf));
    }
  }
  midiCCOut(CCmg_vcf, 23, mg_vcf);
}

void updatebend_osc() {
  if (!recallPatchFlag) {
    if (bend_osc == 0) {
      showCurrentParameterPage("Bend Range", String("Off"));
    } else {
      showCurrentParameterPage("Bend Range", String(bend_osc));
    }
  }
  midiCCOut(CCbend_osc, 0, bend_osc);
}

void updatebend_vcf() {
  if (!recallPatchFlag) {
    switch (bend_vcf) {
      case 0:
        showCurrentParameterPage("Bend to VCF", String("Off"));
        break;
      case 1:
        showCurrentParameterPage("Bend to VCF", String("On"));
        break;
    }
  }
  midiCCOut(CCbend_vcf, 18, bend_vcf);
}

void updateglide_time() {
  if (!recallPatchFlag) {
    if (glide_time == 0) {
      showCurrentParameterPage("Portamento", String("Off"));
    } else {
      showCurrentParameterPage("Portamento", String(glide_time));
    }
  }
  midiCCOut(CCglide_time, 1, glide_time);
}

void updatePoly1() {
  if (poly1 == 1) {
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly1 Mode", String("On"));
    }
    digitalWrite(POLY1_LED, HIGH);
    digitalWrite(POLY2_LED, LOW);
    digitalWrite(UNISON_LED, LOW);
    poly1 = 1;
    poly2 = 0;
    unison = 0;
    polymode = 0;
    midiCCOut(CCpoly1, 0, poly1);
  }
}

void updatePoly2() {
  if (poly2 == 1) {
    if (!recallPatchFlag) {
      showCurrentParameterPage("Poly2 Mode", String("On"));
    }
    digitalWrite(POLY1_LED, LOW);
    digitalWrite(POLY2_LED, HIGH);
    digitalWrite(UNISON_LED, LOW);
    poly1 = 0;
    poly2 = 1;
    unison = 0;
    polymode = 1;
    midiCCOut(CCpoly2, 0, poly2);
  }
}

void updateUnison() {
  if (unison == 1) {
    if (!recallPatchFlag) {
      showCurrentParameterPage("Unison Mode", String("On"));
    }
    digitalWrite(POLY1_LED, LOW);
    digitalWrite(POLY2_LED, LOW);
    digitalWrite(UNISON_LED, HIGH);
    poly1 = 0;
    poly2 = 0;
    unison = 1;
    polymode = 2;
    midiCCOut(CCunison, 0, unison);
  }
}

void updatePatchname() {
  showPatchPage(String(patchNo), patchName);
}

void myControlChange(byte channel, byte control, int value) {
  switch (control) {

    case CCosc1_octave:
      osc1_octave = value;
      osc1_octave = map(osc1_octave, 0, 127, 0, 2);
      updateosc1_octave();
      break;

    case CCosc1_waveform:
      osc1_waveform = value;
      osc1_waveform = map(osc1_waveform, 0, 127, 0, 7);
      updateosc1_waveform();
      break;

    case CCosc1_level:
      osc1_level = value;
      osc1_level = map(osc1_level, 0, 127, 0, 31);
      updateosc1_level();
      break;

    case CCosc2_octave:
      osc2_octave = value;
      osc2_octave = map(osc2_octave, 0, 127, 0, 2);
      updateosc2_octave();
      break;

    case CCosc2_waveform:
      osc2_waveform = value;
      osc2_waveform = map(osc2_waveform, 0, 127, 0, 7);
      updateosc2_waveform();
      break;

    case CCosc2_level:
      osc2_level = value;
      osc2_level = map(osc2_level, 0, 127, 0, 31);
      updateosc2_level();
      break;

    case CCosc2_interval:
      osc2_interval = value;
      osc2_interval = map(osc2_interval, 0, 127, 0, 4);
      updateosc2_interval();
      break;

    case CCosc2_detune:
      osc2_detune = value;
      osc2_detune = map(osc2_detune, 0, 127, 0, 6);
      updateosc2_detune();
      break;

    case CCnoise:
      noise = value;
      noise = map(noise, 0, 127, 0, 31);
      updatenoise();
      break;

    case CCvcf_cutoff:
      vcf_cutoff = value;
      vcf_cutoff = map(vcf_cutoff, 0, 127, 0, 63);
      updatevcf_cutoff();
      break;

    case CCvcf_res:
      vcf_res = value;
      vcf_res = map(vcf_res, 0, 127, 0, 31);
      updatevcf_res();
      break;

    case CCvcf_kbdtrack:
      vcf_kbdtrack = value;
      vcf_kbdtrack = map(vcf_kbdtrack, 0, 127, 0, 2);
      updatevcf_kbdtrack();
      break;

    case CCvcf_polarity:
      vcf_polarity = value;
      vcf_polarity = map(vcf_polarity, 0, 127, 0, 1);
      updatevcf_polarity();
      break;

    case CCvcf_eg_intensity:
      vcf_eg_intensity = value;
      vcf_eg_intensity = map(vcf_eg_intensity, 0, 127, 0, 31);
      updatevcf_eg_intensity();
      break;

    case CCchorus:
      chorus = value;
      chorus = map(chorus, 0, 127, 0, 1);
      updatechorus();
      break;

    case CCvcf_attack:
      vcf_attack = value;
      vcf_attack = map(vcf_attack, 0, 127, 0, 31);
      updatevcf_attack();
      break;

    case CCvcf_decay:
      vcf_decay = value;
      vcf_decay = map(vcf_decay, 0, 127, 0, 31);
      updatevcf_decay();
      break;

    case CCvcf_breakpoint:
      vcf_breakpoint = value;
      vcf_breakpoint = map(vcf_breakpoint, 0, 127, 0, 31);
      updatevcf_breakpoint();
      break;

    case CCvcf_slope:
      vcf_slope = value;
      vcf_slope = map(vcf_slope, 0, 127, 0, 31);
      updatevcf_slope();
      break;

    case CCvcf_sustain:
      vcf_sustain = value;
      vcf_sustain = map(vcf_sustain, 0, 127, 0, 31);
      updatevcf_sustain();
      break;

    case CCvcf_release:
      vcf_release = value;
      vcf_release = map(vcf_release, 0, 127, 0, 31);
      updatevcf_release();
      break;

    case CCvca_attack:
      vca_attack = value;
      vca_attack = map(vca_attack, 0, 127, 0, 31);
      updatevca_attack();
      break;

    case CCvca_decay:
      vca_decay = value;
      vca_decay = map(vca_decay, 0, 127, 0, 31);
      updatevca_decay();
      break;

    case CCvca_breakpoint:
      vca_breakpoint = value;
      vca_breakpoint = map(vca_breakpoint, 0, 127, 0, 31);
      updatevca_breakpoint();
      break;

    case CCvca_slope:
      vca_slope = value;
      vca_slope = map(vca_slope, 0, 127, 0, 31);
      updatevca_slope();
      break;

    case CCvca_sustain:
      vca_sustain = value;
      vca_sustain = map(vca_sustain, 0, 127, 0, 31);
      updatevca_sustain();
      break;

    case CCvca_release:
      vca_release = value;
      vca_release = map(vca_release, 0, 127, 0, 31);
      updatevca_release();
      break;

    case CCmg_frequency:
      mg_frequency = value;
      mg_frequency = map(mg_frequency, 0, 127, 0, 31);
      updatemg_frequency();
      break;

    case CCmg_delay:
      mg_delay = value;
      mg_delay = map(mg_delay, 0, 127, 0, 31);
      updatemg_delay();
      break;

    case CCmg_osc:
      mg_osc = value;
      mg_osc = map(mg_osc, 0, 127, 0, 31);
      updatemg_osc();
      break;

    case CCmg_vcf:
      mg_vcf = value;
      mg_vcf = map(mg_vcf, 0, 127, 0, 31);
      updatemg_vcf();
      break;

    case CCbend_osc:
      bend_osc = value;
      bend_osc = map(bend_osc, 0, 127, 0, 12);
      updatebend_osc();
      break;

    case CCbend_vcf:
      bend_vcf = value;
      bend_vcf = map(bend_vcf, 0, 127, 0, 1);
      updatebend_vcf();
      break;

    case CCglide_time:
      glide_time = value;
      glide_time = map(glide_time, 0, 127, 0, 31);
      updateglide_time();
      break;

    case CCpoly1:
      //value > 0 ? poly1 = 1 : poly1 = 0;
      updatePoly1();
      break;

    case CCpoly2:
      //value > 0 ? poly2 = 1 : poly2 = 0;
      updatePoly2();
      break;

    case CCunison:
      //value > 0 ? unison = 1 : unison = 0;
      updateUnison();
      break;

    case CCallnotesoff:
      allNotesOff();
      break;
  }
}

void myProgramChange(byte channel, byte program) {
  state = PATCH;
  patchNo = program + 1;
  recallPatch(patchNo);
  Serial.print("MIDI Pgm Change:");
  Serial.println(patchNo);
  state = PARAMETER;
}

void recallPatch(int patchNo) {
  allNotesOff();
  usbMIDI.sendProgramChange(patchNo - 1, midiOutCh);
  MIDI.sendProgramChange(patchNo - 1, midiOutCh);
  delay(50);
  recallPatchFlag = true;
  File patchFile = SD.open(String(patchNo).c_str());
  if (!patchFile) {
    Serial.println("File not found");
  } else {
    String data[NO_OF_PARAMS];  //Array of data read in
    recallPatchData(patchFile, data);
    setCurrentPatchData(data);
    patchFile.close();
  }
  recallPatchFlag = false;
}

void setCurrentPatchData(String data[]) {
  patchName = data[0];
  osc1_octave = data[1].toInt();
  osc1_waveform = data[2].toInt();
  osc1_level = data[3].toInt();
  osc2_octave = data[4].toInt();
  osc2_waveform = data[5].toInt();
  osc2_level = data[6].toInt();
  osc2_interval = data[7].toInt();
  osc2_detune = data[8].toInt();
  noise = data[9].toInt();
  vcf_cutoff = data[10].toInt();
  vcf_res = data[11].toInt();
  vcf_kbdtrack = data[12].toInt();
  vcf_polarity = data[13].toInt();
  vcf_eg_intensity = data[14].toInt();
  chorus = data[15].toInt();
  vcf_attack = data[16].toInt();
  vcf_decay = data[17].toInt();
  vcf_breakpoint = data[18].toInt();
  vcf_slope = data[19].toInt();
  vcf_sustain = data[20].toInt();
  vcf_release = data[21].toInt();
  vca_attack = data[22].toInt();
  vca_decay = data[23].toInt();
  vca_breakpoint = data[24].toInt();
  vca_slope = data[25].toInt();
  vca_sustain = data[26].toInt();
  vca_release = data[27].toInt();
  mg_frequency = data[28].toInt();
  mg_delay = data[29].toInt();
  mg_osc = data[30].toInt();
  mg_vcf = data[31].toInt();
  bend_osc = data[32].toInt();
  bend_vcf = data[33].toInt();
  glide_time = data[34].toInt();
  poly1 = data[35].toInt();
  poly2 = data[36].toInt();
  unison = data[37].toInt();

  updatePoly1();
  updatePoly2();
  updateUnison();

  //Patchname
  updatePatchname();

  Serial.print("Set Patch: ");
  Serial.println(patchName);
}

void sendToSynth(int row) {

  updateosc1_octave();
  updateosc1_waveform();
  updateosc1_level();
  updateosc2_octave();
  updateosc2_waveform();
  updateosc2_level();
  updateosc2_interval();
  updateosc2_detune();
  updatenoise();
  updatevcf_cutoff();
  updatevcf_res();
  updatevcf_kbdtrack();
  updatevcf_polarity();
  updatevcf_eg_intensity();
  updatechorus();
  updatevcf_attack();
  updatevcf_decay();
  updatevcf_breakpoint();
  updatevcf_slope();
  updatevcf_sustain();
  updatevcf_release();
  updatevca_attack();
  updatevca_decay();
  updatevca_breakpoint();
  updatevca_slope();
  updatevca_sustain();
  updatevca_release();
  updatemg_frequency();
  updatemg_delay();
  updatemg_osc();
  updatemg_vcf();
  updatebend_osc();
  updatebend_vcf();
  updateglide_time();
  updatePoly1();
  updatePoly2();
  updateUnison();

  delay(2);
  writeRequest[5] = row;
  MIDI.sendSysEx(sizeof(writeRequest), writeRequest);
}

String getCurrentPatchData() {
  return patchName + "," + String(osc1_octave) + "," + String(osc1_waveform) + "," + String(osc1_level)
         + "," + String(osc2_octave) + "," + String(osc2_waveform) + "," + String(osc2_level) + "," + String(osc2_interval) + "," + String(osc2_detune) + "," + String(noise)
         + "," + String(vcf_cutoff) + "," + String(vcf_res) + "," + String(vcf_kbdtrack) + "," + String(vcf_polarity) + "," + String(vcf_eg_intensity) + "," + String(chorus)
         + "," + String(vcf_attack) + "," + String(vcf_decay) + "," + String(vcf_breakpoint) + "," + String(vcf_slope) + "," + String(vcf_sustain) + "," + String(vcf_release)
         + "," + String(vca_attack) + "," + String(vca_decay) + "," + String(vca_breakpoint) + "," + String(vca_slope) + "," + String(vca_sustain) + "," + String(vca_release)
         + "," + String(mg_frequency) + "," + String(mg_delay) + "," + String(mg_osc) + "," + String(mg_vcf)
         + "," + String(bend_osc) + "," + String(bend_vcf) + "," + String(glide_time)
         + "," + String(poly1) + "," + String(poly2) + "," + String(unison);
}

void checkMux() {

  mux1Read = adc->adc1->analogRead(MUX1_S);

  if (mux1Read > (mux1ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux1Read < (mux1ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux1ValuesPrev[muxInput] = mux1Read;
    mux1Read = (mux1Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX1_osc1_octave:
        myControlChange(midiChannel, CCosc1_octave, mux1Read);
        break;
      case MUX1_osc1_waveform:
        myControlChange(midiChannel, CCosc1_waveform, mux1Read);
        break;
      case MUX1_osc1_level:
        myControlChange(midiChannel, CCosc1_level, mux1Read);
        break;
      case MUX1_osc2_octave:
        myControlChange(midiChannel, CCosc2_octave, mux1Read);
        break;
      case MUX1_osc2_waveform:
        myControlChange(midiChannel, CCosc2_waveform, mux1Read);
        break;
      case MUX1_osc2_level:
        myControlChange(midiChannel, CCosc2_level, mux1Read);
        break;
      case MUX1_osc2_interval:
        myControlChange(midiChannel, CCosc2_interval, mux1Read);
        break;
      case MUX1_osc2_detune:
        myControlChange(midiChannel, CCosc2_detune, mux1Read);
        break;
      case MUX1_noise:
        myControlChange(midiChannel, CCnoise, mux1Read);
        break;
      case MUX1_vcf_cutoff:
        myControlChange(midiChannel, CCvcf_cutoff, mux1Read);
        break;
      case MUX1_vcf_res:
        myControlChange(midiChannel, CCvcf_res, mux1Read);
        break;
      case MUX1_vcf_kbdtrack:
        myControlChange(midiChannel, CCvcf_kbdtrack, mux1Read);
        break;
      case MUX1_vcf_polarity:
        myControlChange(midiChannel, CCvcf_polarity, mux1Read);
        break;
      case MUX1_vcf_eg_intensity:
        myControlChange(midiChannel, CCvcf_eg_intensity, mux1Read);
        break;
      case MUX1_chorus:
        myControlChange(midiChannel, CCchorus, mux1Read);
        break;
    }
  }

  mux2Read = adc->adc1->analogRead(MUX2_S);

  if (mux2Read > (mux2ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux2Read < (mux2ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux2ValuesPrev[muxInput] = mux2Read;
    mux2Read = (mux2Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX2_vcf_attack:
        myControlChange(midiChannel, CCvcf_attack, mux2Read);
        break;
      case MUX2_vcf_decay:
        myControlChange(midiChannel, CCvcf_decay, mux2Read);
        break;
      case MUX2_vcf_breakpoint:
        myControlChange(midiChannel, CCvcf_breakpoint, mux2Read);
        break;
      case MUX2_vcf_slope:
        myControlChange(midiChannel, CCvcf_slope, mux2Read);
        break;
      case MUX2_vcf_sustain:
        myControlChange(midiChannel, CCvcf_sustain, mux2Read);
        break;
      case MUX2_vcf_release:
        myControlChange(midiChannel, CCvcf_release, mux2Read);
        break;
      case MUX2_vca_attack:
        myControlChange(midiChannel, CCvca_attack, mux2Read);
        break;
      case MUX2_vca_decay:
        myControlChange(midiChannel, CCvca_decay, mux2Read);
        break;
      case MUX2_vca_breakpoint:
        myControlChange(midiChannel, CCvca_breakpoint, mux2Read);
        break;
      case MUX2_vca_slope:
        myControlChange(midiChannel, CCvca_slope, mux2Read);
        break;
      case MUX2_vca_sustain:
        myControlChange(midiChannel, CCvca_sustain, mux2Read);
        break;
      case MUX2_vca_release:
        myControlChange(midiChannel, CCvca_release, mux2Read);
        break;
      case MUX2_mg_frequency:
        myControlChange(midiChannel, CCmg_frequency, mux2Read);
        break;
      case MUX2_mg_delay:
        myControlChange(midiChannel, CCmg_delay, mux2Read);
        break;
      case MUX2_mg_osc:
        myControlChange(midiChannel, CCmg_osc, mux2Read);
        break;
      case MUX2_mg_vcf:
        myControlChange(midiChannel, CCmg_vcf, mux2Read);
        break;
    }
  }


  mux3Read = adc->adc1->analogRead(MUX3_S);

  if (mux3Read > (mux3ValuesPrev[muxInput] + QUANTISE_FACTOR) || mux3Read < (mux3ValuesPrev[muxInput] - QUANTISE_FACTOR)) {
    mux3ValuesPrev[muxInput] = mux3Read;
    mux3Read = (mux3Read >> resolutionFrig);  // Change range to 0-127

    switch (muxInput) {
      case MUX3_bend_osc:
        myControlChange(midiChannel, CCbend_osc, mux3Read);
        break;
      case MUX3_bend_vcf:
        myControlChange(midiChannel, CCbend_vcf, mux3Read);
        break;
      case MUX3_glide_time:
        myControlChange(midiChannel, CCglide_time, mux3Read);
        break;
    }
  }

  muxInput++;
  if (muxInput >= MUXCHANNELS)
    muxInput = 0;

  digitalWriteFast(MUX_0, muxInput & B0001);
  digitalWriteFast(MUX_1, muxInput & B0010);
  digitalWriteFast(MUX_2, muxInput & B0100);
  digitalWriteFast(MUX_3, muxInput & B1000);
  delayMicroseconds(75);
}

void showSettingsPage() {
  showSettingsPage(settings::current_setting(), settings::current_setting_value(), state);
}

void midiCCOut(byte cc, int param_offset, byte value) {

  switch (param_offset) {
    case 0:
      value = (polymode << 4) | (bend_osc & 0x0F);
      break;

    case 18:
      value = (bend_vcf << 5) | (vca_sustain & 0x1F);
      break;

    case 19:
      value = (osc1_octave << 5) | (vca_release & 0x1F);
      break;

    case 20:
      value = (osc2_octave << 5) | (mg_frequency & 0x1F);
      break;

    case 21:
      value = (vcf_kbdtrack << 5) | (mg_delay & 0x1F);
      break;

    case 22:
      value = (vcf_polarity << 5) | (mg_osc & 0x1F);
      break;

    case 23:
      value = (chorus << 5) | (mg_vcf & 0x1F);
      break;

    case 24:
      value = (osc1_waveform << 3) | (osc2_waveform & 0x07);
      break;

    case 25:
      value = (osc2_interval << 3) | (osc2_detune & 0x07);
      break;
  }

  byteArray[5] = param_offset;
  byteArray[6] = value;

  switch (ccType) {
    case 0:
      if (midiOutCh > 0) {
        MIDI.sendControlChange(cc, value, midiOutCh);  //MIDI DIN is set to Out
      }
      break;

    case 1:
      if (midiOutCh > 0) {
        MIDI.sendControlChange(cc, value, midiOutCh);  //MIDI DIN is set to Out
      }
      break;

    case 2:
      if (midiOutCh > 0) {
        MIDI.sendSysEx(sizeof(byteArray), byteArray);
      }
      break;
  }
}

void checkSwitches() {

  poly1Button.update();
  if (poly1Button.numClicks() == 1) {
    poly1 = 1;
    myControlChange(midiChannel, CCpoly1, poly1);
  }

  poly2Button.update();
  if (poly2Button.numClicks() == 1) {
    poly2 = 1;
    myControlChange(midiChannel, CCpoly2, poly2);
  }

  unisonButton.update();
  if (unisonButton.numClicks() == 1) {
    unison = 1;
    myControlChange(midiChannel, CCunison, unison);
  }

  saveButton.update();
  if (saveButton.held()) {
    switch (state) {
      case PARAMETER:
      case PATCH:
        state = DELETE;
        break;
    }
  } else if (saveButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        if (patches.size() < PATCHES_LIMIT) {
          resetPatchesOrdering();  //Reset order of patches from first patch
          patches.push({ patches.size() + 1, INITPATCHNAME });
          state = SAVE;
        }
        break;
      case SAVE:
        //Save as new patch with INITIALPATCH name or overwrite existing keeping name - bypassing patch renaming
        patchName = patches.last().patchName;
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patches.last().patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() > 0) patchName = renamedPatch;  //Prevent empty strings
        state = PATCH;
        savePatch(String(patches.last().patchNo).c_str(), getCurrentPatchData());
        showPatchPage(patches.last().patchNo, patchName);
        patchNo = patches.last().patchNo;
        loadPatches();  //Get rid of pushed patch if it wasn't saved
        setPatchesOrdering(patchNo);
        renamedPatch = "";
        state = PARAMETER;
        break;
    }
  }

  settingsButton.update();
  if (settingsButton.held()) {
    //If recall held, set current patch to match current hardware state
    //Reinitialise all hardware values to force them to be re-read if different
    state = REINITIALISE;
    reinitialiseToPanel();
  } else if (settingsButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = SETTINGS;
        showSettingsPage();
        break;
      case SETTINGS:
        showSettingsPage();
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  backButton.update();
  if (backButton.held()) {
    //If Back button held, Panic - all notes off
  } else if (backButton.numClicks() == 1) {
    switch (state) {
      case RECALL:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        renamedPatch = "";
        state = PARAMETER;
        loadPatches();  //Remove patch that was to be saved
        setPatchesOrdering(patchNo);
        break;
      case PATCHNAMING:
        charIndex = 0;
        renamedPatch = "";
        state = SAVE;
        break;
      case DELETE:
        setPatchesOrdering(patchNo);
        state = PARAMETER;
        break;
      case SETTINGS:
        state = PARAMETER;
        break;
      case SETTINGSVALUE:
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }

  //Encoder switch
  recallButton.update();
  if (recallButton.held()) {
    //If Recall button held, return to current patch setting
    //which clears any changes made
    state = PATCH;
    //Recall the current patch
    patchNo = patches.first().patchNo;
    recallPatch(patchNo);
    state = PARAMETER;
  } else if (recallButton.numClicks() == 1) {
    switch (state) {
      case PARAMETER:
        state = RECALL;  //show patch list
        break;
      case RECALL:
        state = PATCH;
        //Recall the current patch
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case SAVE:
        showRenamingPage(patches.last().patchName);
        patchName = patches.last().patchName;
        state = PATCHNAMING;
        break;
      case PATCHNAMING:
        if (renamedPatch.length() < 12)  //actually 12 chars
        {
          renamedPatch.concat(String(currentCharacter));
          charIndex = 0;
          currentCharacter = CHARACTERS[charIndex];
          showRenamingPage(renamedPatch);
        }
        break;
      case DELETE:
        //Don't delete final patch
        if (patches.size() > 1) {
          state = DELETEMSG;
          patchNo = patches.first().patchNo;     //PatchNo to delete from SD card
          patches.shift();                       //Remove patch from circular buffer
          deletePatch(String(patchNo).c_str());  //Delete from SD card
          loadPatches();                         //Repopulate circular buffer to start from lowest Patch No
          renumberPatchesOnSD();
          loadPatches();                      //Repopulate circular buffer again after delete
          patchNo = patches.first().patchNo;  //Go back to 1
          recallPatch(patchNo);               //Load first patch
        }
        state = PARAMETER;
        break;
      case SETTINGS:
        state = SETTINGSVALUE;
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::save_current_value();
        state = SETTINGS;
        showSettingsPage();
        break;
    }
  }
}

void reinitialiseToPanel() {
  //This sets the current patch to be the same as the current hardware panel state - all the pots
  //The four button controls stay the same state
  //This reinialises the previous hardware values to force a re-read
  muxInput = 0;
  for (int i = 0; i < MUXCHANNELS; i++) {
    mux1ValuesPrev[i] = RE_READ;
    mux2ValuesPrev[i] = RE_READ;
    mux3ValuesPrev[i] = RE_READ;
  }
  patchName = INITPATCHNAME;
  showPatchPage("Initial", "Panel Settings");
}

void checkEncoder() {
  //Encoder works with relative inc and dec values
  //Detent encoder goes up in 4 steps, hence +/-3

  long encRead = encoder.read();
  if ((encCW && encRead > encPrevious + 3) || (!encCW && encRead < encPrevious - 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.push(patches.shift());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.push(patches.shift());
        break;
      case SAVE:
        patches.push(patches.shift());
        break;
      case PATCHNAMING:
        if (charIndex == TOTALCHARS) charIndex = 0;  //Wrap around
        currentCharacter = CHARACTERS[charIndex++];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.push(patches.shift());
        break;
      case SETTINGS:
        settings::increment_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::increment_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  } else if ((encCW && encRead < encPrevious - 3) || (!encCW && encRead > encPrevious + 3)) {
    switch (state) {
      case PARAMETER:
        state = PATCH;
        patches.unshift(patches.pop());
        patchNo = patches.first().patchNo;
        recallPatch(patchNo);
        state = PARAMETER;
        break;
      case RECALL:
        patches.unshift(patches.pop());
        break;
      case SAVE:
        patches.unshift(patches.pop());
        break;
      case PATCHNAMING:
        if (charIndex == -1)
          charIndex = TOTALCHARS - 1;
        currentCharacter = CHARACTERS[charIndex--];
        showRenamingPage(renamedPatch + currentCharacter);
        break;
      case DELETE:
        patches.unshift(patches.pop());
        break;
      case SETTINGS:
        settings::decrement_setting();
        showSettingsPage();
        break;
      case SETTINGSVALUE:
        settings::decrement_setting_value();
        showSettingsPage();
        break;
    }
    encPrevious = encRead;
  }
}

void checkLoadFactory() {
  loadFactory = getLoadFactory();
  if (loadFactory) {

    for (int row = 0; row < 64; row++) {
      String currentRow = factory[row];

      String values[38];   // Assuming you have 38 values per row
      int valueIndex = 0;  // Index for storing values
      for (int i = 0; i < currentRow.length(); i++) {
        char currentChar = currentRow.charAt(i);

        // Check for the delimiter (",") and move to the next value
        if (currentChar == ',') {
          valueIndex++;  // Move to the next value
          continue;      // Skip the delimiter
        }

        // Append the character to the current value
        values[valueIndex] += currentChar;
      }

      // Process the values
      int intValues[38];
      for (int i = 0; i < 38; i++) {  // Adjust the loop count based on the number of values per row
        switch (i) {

          case 0:
            patchName = values[i];
            break;

          case 1:
            intValues[i] = values[i].toInt();
            switch (intValues[i]) {
              case 16:
                osc1_octave = 0;
                break;
              case 8:
                osc1_octave = 1;
                break;
              case 4:
                osc1_octave = 2;
                break;
            }
            break;

          case 2:
            intValues[i] = values[i].toInt();
            osc1_waveform = (intValues[i] - 1);
            break;

          case 3:  // osc1_level
            intValues[i] = values[i].toInt();
            osc1_level = intValues[i];
            break;

          case 4:
            intValues[i] = values[i].toInt();
            switch (intValues[i]) {
              case 16:
                osc2_octave = 0;
                break;
              case 8:
                osc2_octave = 1;
                break;
              case 4:
                osc2_octave = 2;
                break;
            }
            break;

          case 5:
            intValues[i] = values[i].toInt();
            osc2_waveform = (intValues[i] - 1);
            break;

          case 6:  // osc2_level
            intValues[i] = values[i].toInt();
            osc2_level = intValues[i];
            break;

          case 7:
            intValues[i] = values[i].toInt();
            switch (intValues[i]) {
              case 1:
                osc2_interval = 0;
                break;
              case 3:
                osc2_interval = 1;
                break;
              case -3:
                osc2_interval = 2;
                break;
              case 4:
                osc2_interval = 3;
                break;
              case 5:
                osc2_interval = 4;
                break;
            }
            break;

          case 8:
            intValues[i] = values[i].toInt();
            osc2_detune = (intValues[i] - 1);
            break;

          case 9:  // moise_level
            intValues[i] = values[i].toInt();
            noise = intValues[i];
            break;

          case 10:  // cutoff
            intValues[i] = values[i].toInt();
            vcf_cutoff = intValues[i];
            break;

          case 11:  // res
            intValues[i] = values[i].toInt();
            vcf_res = intValues[i];
            ;
            break;

          case 12:  // kbdtrack
            intValues[i] = values[i].toInt();
            vcf_kbdtrack = intValues[i];
            break;

          case 13:  // polarity
            intValues[i] = values[i].toInt();
            vcf_polarity = (intValues[i] - 1);
            break;

          case 14:  // eg_intensity
            intValues[i] = values[i].toInt();
            vcf_eg_intensity = intValues[i];
            break;

          case 15:  // chorus
            intValues[i] = values[i].toInt();
            chorus = intValues[i];
            break;

          case 16:  // vcf_attack
            intValues[i] = values[i].toInt();
            vcf_attack = intValues[i];
            break;

          case 17:  // vcf_decay
            intValues[i] = values[i].toInt();
            vcf_decay = intValues[i];
            break;

          case 18:  // vcf_bp
            intValues[i] = values[i].toInt();
            vcf_breakpoint = intValues[i];
            break;

          case 19:  // vcf_slope
            intValues[i] = values[i].toInt();
            vcf_slope = intValues[i];
            break;

          case 20:  // vcf_sustain
            intValues[i] = values[i].toInt();
            vcf_sustain = intValues[i];
            break;

          case 21:  // vcf_release
            intValues[i] = values[i].toInt();
            vcf_release = intValues[i];
            break;

          case 22:  // vca_attack
            intValues[i] = values[i].toInt();
            vca_attack = intValues[i];
            break;

          case 23:  // vca_decay
            intValues[i] = values[i].toInt();
            vca_decay = intValues[i];
            break;

          case 24:  // vca_bp
            intValues[i] = values[i].toInt();
            vca_breakpoint = intValues[i];
            break;

          case 25:  // vca_slope
            intValues[i] = values[i].toInt();
            vca_slope = intValues[i];
            break;

          case 26:  // vca_sustain
            intValues[i] = values[i].toInt();
            vca_sustain = intValues[i];
            break;

          case 27:  // vca_release
            intValues[i] = values[i].toInt();
            vca_release = intValues[i];
            break;

          case 28:  // mg_freq
            intValues[i] = values[i].toInt();
            mg_frequency = intValues[i];
            break;

          case 29:  // mg_delay
            intValues[i] = values[i].toInt();
            mg_delay = intValues[i];
            break;

          case 30:  // mg_osc
            intValues[i] = values[i].toInt();
            mg_osc = intValues[i];
            break;

          case 31:  // mg_vcf
            intValues[i] = values[i].toInt();
            mg_vcf = intValues[i];
            break;

          case 32:  // bend_osc
            intValues[i] = values[i].toInt();
            bend_osc = intValues[i];
            break;

          case 33:  // bend_vcf
            intValues[i] = values[i].toInt();
            bend_vcf = intValues[i];
            ;
            break;

          case 34:  // glide
            intValues[i] = values[i].toInt();
            glide_time = intValues[i];
            break;

          case 35:  // poly1
            intValues[i] = values[i].toInt();
            poly1 = intValues[i];
            break;

          case 36:  // poly2
            intValues[i] = values[i].toInt();
            poly2 = intValues[i];
            break;

          case 37:  // unison
            intValues[i] = values[i].toInt();
            unison = intValues[i];
            break;
        }
      }
      // Add a newline to separate rows (optional)
      sprintf(buffer, "%d", row + 1);
      savePatch(buffer, getCurrentPatchData());
      updatePatchname();
      recallPatchFlag = true;
      sendToSynth(row);
      recallPatchFlag = false;
    }
    loadPatches();
    loadFactory = false;
    storeLoadFactory(loadFactory);
    state = PATCH;
    recallPatch(1);
  }
}

void loop() {
  checkMux();
  checkSwitches();
  checkEncoder();
  MIDI.read(midiChannel);
  usbMIDI.read(midiChannel);
  checkLoadFactory();
}
