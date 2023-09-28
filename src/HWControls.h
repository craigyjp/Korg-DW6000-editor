// This optional setting causes Encoder to use more optimized code,
// It must be defined before Encoder.h is included.
#define ENCODER_OPTIMIZE_INTERRUPTS
#include <Encoder.h>
#include <Bounce.h>
#include "TButton.h"
#include <ADC.h>
#include <ADC_util.h>

ADC *adc = new ADC();

//Teensy 4.1 - Mux Pins
#define MUX_0 29
#define MUX_1 32
#define MUX_2 30
#define MUX_3 31

#define MUX1_S A2  // ADC1
#define MUX2_S A0  // ADC1
#define MUX3_S A1  // ADC1


//Mux 1 Connections
#define MUX1_osc1_octave 0
#define MUX1_osc1_waveform 1
#define MUX1_osc1_level 2
#define MUX1_osc2_octave 3
#define MUX1_osc2_waveform 4
#define MUX1_osc2_level 5
#define MUX1_osc2_interval 6
#define MUX1_osc2_detune 7
#define MUX1_noise 8
#define MUX1_vcf_cutoff 9
#define MUX1_vcf_res 10
#define MUX1_vcf_kbdtrack 11
#define MUX1_vcf_eg_intensity 13

//Mux 2 Connections
#define MUX2_vcf_attack 0
#define MUX2_vcf_decay 1
#define MUX2_vcf_breakpoint 2
#define MUX2_vcf_slope 3
#define MUX2_vcf_sustain 4
#define MUX2_vcf_release 5
#define MUX2_vca_attack 6
#define MUX2_vca_decay 7
#define MUX2_vca_breakpoint 8
#define MUX2_vca_slope 9
#define MUX2_vca_sustain 10
#define MUX2_vca_release 11
#define MUX2_mg_frequency 12
#define MUX2_mg_delay 13
#define MUX2_mg_osc 14
#define MUX2_mg_vcf 15

//Mux 3 Connections
#define MUX3_bend_osc 0
#define MUX3_glide_time 2
#define MUX3_wave_bank 3

#define POLY1_SW 33
#define POLY2_SW 34
#define UNISON_SW 35
#define CHORUS_SW 40
#define BEND_VCF_SW 18
#define KBDTRACK_SW 19
#define POLARITY_SW 7

#define POLY1_LED 36
#define POLY2_LED 37
#define UNISON_LED 38
#define SAVE_LED 39
#define CHORUS_LED 28
#define BEND_VCF_LED 21
#define KBDTRACK_RED_LED 22
#define KBDTRACK_GREEN_LED 20
#define VCF_POLARITY_LED 8

//Teensy 4.1 Pins

#define RECALL_SW 17
#define SAVE_SW 41
#define SETTINGS_SW 12
#define BACK_SW 10

#define ENCODER_PINA 5
#define ENCODER_PINB 4

#define MUXCHANNELS 16
#define QUANTISE_FACTOR 31

#define DEBOUNCE 30

static byte muxInput = 0;
static int mux1ValuesPrev[MUXCHANNELS] = {};
static int mux2ValuesPrev[MUXCHANNELS] = {};
static int mux3ValuesPrev[MUXCHANNELS] = {};


static int mux1Read = 0;
static int mux2Read = 0;
static int mux3Read = 0;

static long encPrevious = 0;

//These are pushbuttons and require debouncing

TButton saveButton{ SAVE_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton settingsButton{ SETTINGS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton backButton{ BACK_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton recallButton{ RECALL_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };  //On encoder

TButton poly1Button{ POLY1_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton poly2Button{ POLY2_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton unisonButton{ UNISON_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };  //On encoder

TButton chorusButton{ CHORUS_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton bendvcfButton{ BEND_VCF_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };
TButton kbdtrackButton{ KBDTRACK_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };  //On encoder
TButton polarityButton{ POLARITY_SW, LOW, HOLD_DURATION, DEBOUNCE, CLICK_DURATION };  //On encoder

Encoder encoder(ENCODER_PINB, ENCODER_PINA);  //This often needs the pins swapping depending on the encoder

void setupHardware() {
  //Volume Pot is on ADC0
  adc->adc0->setAveraging(32);                                          // set number of averages 0, 4, 8, 16 or 32.
  adc->adc0->setResolution(12);                                         // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc0->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);  // change the conversion speed
  adc->adc0->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  //MUXs on ADC1
  adc->adc1->setAveraging(32);                                          // set number of averages 0, 4, 8, 16 or 32.
  adc->adc1->setResolution(12);                                         // set bits of resolution  8, 10, 12 or 16 bits.
  adc->adc1->setConversionSpeed(ADC_CONVERSION_SPEED::VERY_LOW_SPEED);  // change the conversion speed
  adc->adc1->setSamplingSpeed(ADC_SAMPLING_SPEED::MED_SPEED);           // change the sampling speed

  //Mux address pins

  pinMode(MUX_0, OUTPUT);
  pinMode(MUX_1, OUTPUT);
  pinMode(MUX_2, OUTPUT);
  pinMode(MUX_3, OUTPUT);

  digitalWrite(MUX_0, LOW);
  digitalWrite(MUX_1, LOW);
  digitalWrite(MUX_2, LOW);
  digitalWrite(MUX_3, LOW);


  //Mux ADC
  pinMode(MUX1_S, INPUT_DISABLE);
  pinMode(MUX2_S, INPUT_DISABLE);
  pinMode(MUX3_S, INPUT_DISABLE);

  //Switches
  pinMode(RECALL_SW, INPUT_PULLUP);  //On encoder
  pinMode(SAVE_SW, INPUT_PULLUP);
  pinMode(SETTINGS_SW, INPUT_PULLUP);
  pinMode(BACK_SW, INPUT_PULLUP);

  pinMode(POLY1_SW, INPUT_PULLUP);
  pinMode(POLY2_SW, INPUT_PULLUP);
  pinMode(UNISON_SW, INPUT_PULLUP);

  pinMode(CHORUS_SW, INPUT_PULLUP);
  pinMode(BEND_VCF_SW, INPUT_PULLUP);
  pinMode(KBDTRACK_SW, INPUT_PULLUP);
  pinMode(POLARITY_SW, INPUT_PULLUP);

  pinMode(POLY1_LED, OUTPUT);
  pinMode(POLY2_LED, OUTPUT);
  pinMode(UNISON_LED, OUTPUT);
  pinMode(SAVE_LED, OUTPUT);

  pinMode(CHORUS_LED, OUTPUT);
  pinMode(BEND_VCF_LED, OUTPUT);
  pinMode(KBDTRACK_RED_LED, OUTPUT);
  pinMode(KBDTRACK_GREEN_LED, OUTPUT);
  pinMode(VCF_POLARITY_LED, OUTPUT);
}
