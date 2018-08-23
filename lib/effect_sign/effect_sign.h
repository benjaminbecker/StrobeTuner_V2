#ifndef effect_sign_h_
#define effect_sign_h_
#include "Arduino.h"
#include "AudioStream.h"

class AudioEffectSign : public AudioStream
{
public:
	AudioEffectSign() : AudioStream(1, inputQueueArray) { }
	virtual void update(void);
private:
	audio_block_t *inputQueueArray[1];
};

#endif
