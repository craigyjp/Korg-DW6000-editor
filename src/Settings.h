#include "SettingsService.h"

void settingsMIDICh();
void settingsMIDIOutCh();
void settingsEncoderDir();
void settingsUpdateParams();
void settingsLoadFactory();
void settingsSaveCurrent();

int currentIndexMIDICh();
int currentIndexMIDIOutCh();
int currentIndexEncoderDir();
int currentIndexUpdateParams();
int currentIndexLoadFactory();
int currentIndexSaveCurrent();

void settingsMIDICh(int index, const char *value) {
  if (strcmp(value, "ALL") == 0) {
    midiChannel = MIDI_CHANNEL_OMNI;
  } else {
    midiChannel = atoi(value);
  }
  storeMidiChannel(midiChannel);
}

void settingsMIDIOutCh(int index, const char *value) {
  if (strcmp(value, "Off") == 0) {
    midiOutCh = 0;
  } else {
    midiOutCh = atoi(value);
  }
  storeMidiOutCh(midiOutCh);
}

void settingsEncoderDir(int index, const char *value) {
  if (strcmp(value, "Type 1") == 0) {
    encCW = true;
  } else {
    encCW =  false;
  }
  storeEncoderDir(encCW ? 1 : 0);
}

void settingsUpdateParams(int index, const char *value) {
  if (strcmp(value, "Send Params") == 0) {
    updateParams = true;
  } else {
    updateParams =  false;
  }
  storeUpdateParams(updateParams ? 1 : 0);
}

void settingsLoadFactory(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    loadFactory = true;
  } else {
    loadFactory =  false;
  }
  storeLoadFactory(loadFactory);
}

void settingsSaveCurrent(int index, const char *value) {
  if (strcmp(value, "Yes") == 0) {
    saveCurrent = true;
  } else {
    saveCurrent =  false;
  }
  storeSaveCurrent(saveCurrent);
}

int currentIndexMIDICh() {
  return getMIDIChannel();
}

int currentIndexMIDIOutCh() {
  return getMIDIOutCh();
}

int currentIndexEncoderDir() {
  return getEncoderDir() ? 0 : 1;
}

int currentIndexUpdateParams() {
  return getUpdateParams() ? 0 : 1;
}

int currentIndexLoadFactory() {
  return getLoadFactory();
}

int currentIndexSaveCurrent() {
  return getSaveCurrent();
}

// add settings to the circular buffer
void setUpSettings() {
  settings::append(settings::SettingsOption{"MIDI Ch.", {"All", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0"}, settingsMIDICh, currentIndexMIDICh});
  settings::append(settings::SettingsOption{"MIDI Out Ch.", {"Off", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "\0"}, settingsMIDIOutCh, currentIndexMIDIOutCh});
  settings::append(settings::SettingsOption{"Encoder", {"Type 1", "Type 2", "\0"}, settingsEncoderDir, currentIndexEncoderDir});
  settings::append(settings::SettingsOption{"MIDI Params", {"Off", "Send Params", "\0"}, settingsUpdateParams, currentIndexUpdateParams});
  settings::append(settings::SettingsOption{"Load factory", {"No", "Yes", "\0"}, settingsLoadFactory, currentIndexLoadFactory});
  settings::append(settings::SettingsOption{"Save Current", {"No", "Yes", "\0"}, settingsSaveCurrent, currentIndexSaveCurrent});
}
