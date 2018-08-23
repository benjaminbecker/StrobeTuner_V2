#if 0
#define HARDWARE_TEST
#define F_TEST 82.3
# endif

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <effect_sign.h>

// GUItool: begin automatically generated code
#ifdef HARDWARE_TEST
AudioSynthWaveformSine  adc1;
#else
AudioInputAnalog         adc1;           //xy=78,342
#endif
AudioSynthWaveformDc     dc1;
AudioEffectSign          waveshape1;     //xy=249,359
AudioAnalyzeNoteFrequency notefreq1;      //xy=372,244
AudioFilterStateVariable filter1;        //xy=403,345
AudioMixer4              mixer1;         //xy=549,312
AudioOutputAnalog        dac1;           //xy=690,304
// AudioOutputUSB           usb2;
AudioConnection          patchCord1(adc1, notefreq1);
AudioConnection          patchCord2(adc1, 0, waveshape1, 0);
AudioConnection          patchCord3(waveshape1, 0, filter1, 0);
AudioConnection          patchCord4(filter1, 1, mixer1, 0);
//AudioConnection          patchCord4(waveshape1, 0, dac1, 0);
AudioConnection          patchCord5(dc1, 0, mixer1, 1);
AudioConnection          patchCord6(mixer1, dac1);
// AudioConnection          patchCord7(mixer1, 0, usb2, 0);
// AudioConnection          patchCord8(mixer1, 0, usb2, 1);
// GUItool: end automatically generated code

#include <Arduino.h>
//#include <shiftRegister.h>
#include <shiftRegisterSPI.h>
#include <StringDetection.h>
#include "Outside.h"
#include "DigitsSmall.h"

const float stringFrequencies[6] = {82.41,110.0,146.83,196,246.94,329.63};
int idString = 4;
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

void strobeCallback(){
  if ((yCoordinate[loopCount]>1) & (yCoordinate[loopCount]<8)){
    shiftRegisters.shiftOutSPI(
      (xByte[loopCount] //+ DIGITS[idString][9 - yCoordinate[loopCount]]
      ),
      ~yByte[loopCount]
    );
  }
  else {
    shiftRegisters.shiftOutSPI(
      (xByte[loopCount]),
      ~(yByte[loopCount]) // + DIGITS_Trans[idString][9 - xCoordinate[loopCount]])
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
    ~(DIGITS[idString][rowCount] + ((rowIndexDeviation==rowCount) ? 1 : 0)),
      0b10000000>>rowCount);
  if (++rowCount >= 8){
    rowCount = 0;
  }
}

void setString(int idString){
  float fSet = stringFrequencies[idString];
  float Q = 0.7;
  shiftTimer.end();
  if (precisionMode){
    // strobe mode
    mixer1.gain(0, 2.0);
    mixer1.gain(1, 0.0);
    filter1.frequency(fSet);
    filter1.resonance(Q);
    shiftTimer.begin(strobeCallback, (long)1/fSet/N_LED*1000000);
  }
  else{
    // only show digit of string
    mixer1.gain(0, 0.0);
    mixer1.gain(1, 1.0);
    shiftTimer.begin(letterCallback, (long)1/1000.*1000000);
  }
}

int main(void) {
  // start serial connection
  // Serial.begin(9600);
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, HIGH);
  // setup for Audio sketch
  AudioMemory(30); // was 20
  #ifdef HARDWARE_TEST
  adc1.frequency(F_TEST);
  adc1.amplitude(0.5);
  #endif
  dc1.amplitude(-1.0);
  mixer1.gain(0, 2.0);
  mixer1.gain(1, 0.0);
  notefreq1.begin(0.2);
  // set analog reference to 3.3 V
  dac1.analogReference(EXTERNAL);
  // setup initial string
  setString(idString);
  // load transposed Digits
  transposeDigits(DIGITS_Trans);
  // bit bitOrder
  if (~precisionMode){
    shiftRegisters.setBitOrder(LSBFIRST);
  }
  // loop
  while (true){
    if (notefreq1.available()){
      frequencyEstimate = notefreq1.read();
      idString = estimateString(frequencyEstimate);
      if (stringHasChanged()){
        // change value for shiftTimer
        setString(idString);
        Serial.println(idString);
      }
    }
    else {
      //Serial.println("Nothing detected...");
    }
  }
}
