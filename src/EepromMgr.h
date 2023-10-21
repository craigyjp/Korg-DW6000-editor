#include <EEPROM.h>

#define EEPROM_MIDI_CH 0
#define EEPROM_ENCODER_DIR 1
#define EEPROM_LAST_PATCH 2
#define EEPROM_MIDI_OUT_CH 3
#define EEPROM_LOAD_FACTORY 4
#define EEPROM_UPDATE_PARAMS 5
#define EEPROM_SAVE_CURRENT 6
#define EEPROM_SAVE_ALL 7
#define EEPROM_LOAD_FROM_DW 8

int getMIDIChannel() {
  byte midiChannel = EEPROM.read(EEPROM_MIDI_CH);
  if (midiChannel < 0 || midiChannel > 16) midiChannel = MIDI_CHANNEL_OMNI;//If EEPROM has no MIDI channel stored
  return midiChannel;
}

void storeMidiChannel(byte channel)
{
  EEPROM.update(EEPROM_MIDI_CH, channel);
}

boolean getEncoderDir() {
  byte ed = EEPROM.read(EEPROM_ENCODER_DIR); 
  if (ed < 0 || ed > 1)return true; //If EEPROM has no encoder direction stored
  return ed == 1 ? true : false;
}

void storeEncoderDir(byte encoderDir)
{
  EEPROM.update(EEPROM_ENCODER_DIR, encoderDir);
}

boolean getUpdateParams() {
  byte params = EEPROM.read(EEPROM_UPDATE_PARAMS); 
  if (params < 0 || params > 1)return true; //If EEPROM has no encoder direction stored
  return params == 1 ? true : false;
}

void storeUpdateParams(byte updateParameters)
{
  EEPROM.update(EEPROM_UPDATE_PARAMS, updateParameters);
}

int getLastPatch() {
  int lastPatchNumber = EEPROM.read(EEPROM_LAST_PATCH);
  if (lastPatchNumber < 1 || lastPatchNumber > 999) lastPatchNumber = 1;
  return lastPatchNumber;
}

void storeLastPatch(int lastPatchNumber)
{
  EEPROM.update(EEPROM_LAST_PATCH, lastPatchNumber);
}

int getMIDIOutCh() {
  byte mc = EEPROM.read(EEPROM_MIDI_OUT_CH);
  if (mc < 0 || midiOutCh > 16) mc = 0;//If EEPROM has no MIDI channel stored
  return mc;
}

void storeMidiOutCh(byte midiOutCh){
  EEPROM.update(EEPROM_MIDI_OUT_CH, midiOutCh);
}

boolean getLoadFromDW() {
  byte lfd = EEPROM.read(EEPROM_LOAD_FROM_DW); 
  if (lfd < 0 || lfd > 1)return true;
  return lfd ? true : false;
}

void storeLoadFromDW(byte lfdupdate)
{
  EEPROM.update(EEPROM_LOAD_FROM_DW, lfdupdate);
}

boolean getLoadFactory() {
  byte lf = EEPROM.read(EEPROM_LOAD_FACTORY); 
  if (lf < 0 || lf > 1)return true;
  return lf ? true : false;
}

void storeLoadFactory(byte lfupdate)
{
  EEPROM.update(EEPROM_LOAD_FACTORY, lfupdate);
}

boolean getSaveCurrent() {
  byte sc = EEPROM.read(EEPROM_SAVE_CURRENT); 
  if (sc < 0 || sc > 1)return true;
  return sc ? true : false;
}

void storeSaveCurrent(byte scupdate)
{
  EEPROM.update(EEPROM_SAVE_CURRENT, scupdate);
}

boolean getSaveAll() {
  byte sa = EEPROM.read(EEPROM_SAVE_ALL); 
  if (sa < 0 || sa > 1)return true;
  return sa ? true : false;
}

void storeSaveAll(byte saupdate)
{
  EEPROM.update(EEPROM_SAVE_ALL, saupdate);
}
