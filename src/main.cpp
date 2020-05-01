#if 0
#define HARDWARE_TEST
#define F_TEST 466.0
# endif

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include "effect_sign.h"

// GUItool: begin automatically generated code
#ifdef HARDWARE_TEST
AudioSynthWaveformSine  adc1;
#else
AudioInputAnalog         adc1;           //xy=78,342
#endif
AudioAnalyzeRMS          rms1;           //xy=391,147
AudioSynthWaveformDc     dc1;
AudioEffectSign          waveshape1;     //xy=249,359
AudioAnalyzeNoteFrequency notefreq1;      //xy=372,244
AudioFilterStateVariable filter1;        //xy=403,345
AudioMixer4              mixer1;         //xy=549,312
AudioOutputAnalog        dac1;           //xy=690,304
//AudioOutputUSB           usb2;
AudioConnection          patchCord1(adc1, notefreq1);
AudioConnection          patchCord7(adc1, rms1);
// AudioConnection          patchCord2(adc1, 0, waveshape1, 0);
// AudioConnection          patchCord3(waveshape1, 0, filter1, 0);
// AudioConnection          patchCord4(filter1, 1, mixer1, 0);
AudioConnection          patchCord2(adc1, 0, filter1, 0);
AudioConnection          patchCord3(filter1, 1, waveshape1, 0);
AudioConnection          patchCord4(waveshape1, 0, mixer1, 0);
//AudioConnection          patchCord4(waveshape1, 0, dac1, 0);
AudioConnection          patchCord5(dc1, 0, mixer1, 1);
AudioConnection          patchCord6(mixer1, dac1);
//AudioConnection          patchCord8(adc1, 0, usb2, 0);
//AudioConnection          patchCord9(mixer1, 0, usb2, 1);
// GUItool: end automatically generated code

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include "Adafruit_LEDBackpack.h"
#include <shiftRegisterSPI.h>
#include <StringDetection.h>
#include "Outside.h"
#include "DigitsSmall.h"

Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();

//const float stringFrequencies[6] = {82.41,110.0,146.83,196,246.94,329.63};
int idTone = 4;
bool precisionMode = true;

// timer for shift register
IntervalTimer shiftTimer;
// shift register controller
// Teensy 3.6 or 3.2
ShiftRegisterSPI shiftRegisters(9, 10);

float frequencyEstimate;

unsigned int loopCount = 0;
unsigned int rowCount = 0;
unsigned char rowIndexDeviation = 0;

unsigned char DIGITS_Trans[DIGITS_LEN][8];

// states for finite state machine
// tuner
const unsigned char STATE_STROBE_TUNER = 1;
// settings main menu
const unsigned char STATE_MAIN_MENU = 2;
// settings tunings
const unsigned char STATE_SETTINGS_TUNINGS = 3;
// settings f0
const unsigned char STATE_SETTING_FREQUENCY_BASE = 4;

void strobeCallback(){
  if ((yCoordinate[loopCount]>1) & (yCoordinate[loopCount]<8)){
    shiftRegisters.shiftOutSPI(
      (xByte[loopCount] //+ DIGITS[idTone][9 - yCoordinate[loopCount]]
      ),
      ~yByte[loopCount]
    );
  }
  else {
    shiftRegisters.shiftOutSPI(
      (xByte[loopCount]),
      ~(yByte[loopCount]) // + DIGITS_Trans[idTone][9 - xCoordinate[loopCount]])
    );
  }

  // shiftRegisters.shiftOutSPI(254);
  // shiftRegisters.shiftOutSPI(1);
  if (++loopCount >= N_LED){
    loopCount = 0;
  }
}

void letterCallback(){
  float deviationCents = getDeviationCents();
  if (deviationCents > 30){
    rowIndexDeviation = 0;
  }
  else if (deviationCents > 15){
    rowIndexDeviation = 1;
  }
  else if (deviationCents > 7){
    rowIndexDeviation = 2;
  }
  else if (deviationCents > 0){
    rowIndexDeviation = 3;
  }
  else if (deviationCents > -7){
    rowIndexDeviation = 4;
  }
  else if (deviationCents > -15){
    rowIndexDeviation = 5;
  }
  else if (deviationCents > -30){
    rowIndexDeviation = 5;
  }
  else {
    rowIndexDeviation = 7;
  }

  shiftRegisters.shiftOutSPI(
    ~(DIGITS[idTone][rowCount] + ((rowIndexDeviation==rowCount) ? 1 : 0)),
      0b10000000>>rowCount);
  if (++rowCount >= 8){
    rowCount = 0;
  }
}

void setString(int idTone){
  float fSet;
  switch (getTunerMode()){
    case GUITAR:
      fSet = stringFrequencies[idTone];
      break;
    case CHROMATIC:
      fSet = toneFrequencies[idTone];
      break;
  }

  float Q = 0.7;
  shiftTimer.end();
  if (precisionMode){
    // strobe mode
    mixer1.gain(0, 2.0);
    mixer1.gain(1, 0.0);
    filter1.frequency(fSet);
    filter1.resonance(Q);
    // alphanumeric display
    switch (getTunerMode()){
      case GUITAR:
        alpha4.writeDigitAscii(0, stringNames[idTone][0]);
        alpha4.writeDigitAscii(1, stringNames[idTone][1]);
        break;
      case CHROMATIC:
        alpha4.writeDigitAscii(0, noteNames[idTone][0]);
        alpha4.writeDigitAscii(1, noteNames[idTone][1]);
        break;
    }

    alpha4.writeDigitAscii(2, ' ');
    alpha4.writeDigitAscii(3, ' ');
    alpha4.writeDisplay();
    shiftTimer.begin(strobeCallback, (long)1/fSet/N_LED*1000000);
  }
  else{
    // only show digit of string
    mixer1.gain(0, 0.0);
    mixer1.gain(1, 1.0);
    shiftTimer.begin(letterCallback, (long)1/1000.*1000000);
  }
}

void switchOffMatrix(){
  mixer1.gain(0, 0.0);
  mixer1.gain(1, 1.0);
}

void switchOnMatrix(){
  mixer1.gain(0, 2.0);
  mixer1.gain(1, 0.0);
}

float averageFrequency(float frequency[], float probability[]){
  // assumes that arrays are size 10
  float sumFreq = 0, sumProb = 0;
  const int N = 10;
  for (int id=0; id < N; id++){
    sumFreq += frequency[id]*probability[id];
    sumProb += probability[id];
  }
  return sumFreq/sumProb;
}

int main(void) {
  // setup for Audio sketch
  AudioMemory(60); // was 30
  #ifdef HARDWARE_TEST
  adc1.frequency(F_TEST);
  adc1.amplitude(0.5);
  #endif
  dc1.amplitude(1.0);
  mixer1.gain(0, 2.0);
  mixer1.gain(1, 0.0);
  notefreq1.begin(0.15);
  // init alphanumeric display
  alpha4.begin(0x70);
  // display every character,
  alpha4.writeDigitAscii(0, 'O');
  alpha4.writeDigitAscii(1, 'p');
  alpha4.writeDigitAscii(2, ' ');
  alpha4.writeDigitAscii(3, 'G');
  alpha4.writeDisplay();
  //delay(10000);
  // set analog reference to 3.3 V
  dac1.analogReference(EXTERNAL);
  // setup tuner mode
  //setTunerMode(GUITAR);
  setTunerMode(CHROMATIC);
  // setup initial string
  setString(idTone);
  // load transposed Digits
  transposeDigits(DIGITS_Trans);
  // bit bitOrder
  if (~precisionMode){
    shiftRegisters.setBitOrder(LSBFIRST);
  }
  float minSignalRms = 0.02;
  // loop
  while (true){
    if (rms1.available()){
      if (rms1.read() > minSignalRms) switchOnMatrix();
      else switchOffMatrix();
    }
    if (notefreq1.available()){
      if (notefreq1.probability() > 0.98){
        frequencyEstimate = notefreq1.read();
        idTone = estimateTone(frequencyEstimate);
        if (stringHasChanged()){
          // change value for shiftTimer
          setString(idTone);
        }
      }
    }
  }
}
