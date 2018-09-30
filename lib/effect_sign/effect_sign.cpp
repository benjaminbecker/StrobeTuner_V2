#include <Arduino.h>
#include "effect_sign.h"

#define ZERO 400

void AudioEffectSign::update(void)
{
  audio_block_t *block;
  block = receiveWritable();
  if (!block) return;

  int16_t i;
  for (i = 0; i < AUDIO_BLOCK_SAMPLES; i++) {
    block->data[i] = ((block->data[i] > ZERO) - (block->data[i] < -ZERO)) * 32767;
  }

  transmit(block);
  release(block);
}
