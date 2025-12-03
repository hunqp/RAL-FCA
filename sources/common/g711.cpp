#include "g711.h"

using namespace std;

static int16_t segmentEnd[8] = {0xFF, 0x1FF, 0x3FF, 0x7FF, 0xFFF, 0x1FFF, 0x3FFF, 0x7FFF};

static uint16_t search(uint16_t val, int16_t *table, uint16_t size);
static int16_t aLawToLinear(uint8_t aByte);
static int16_t uLawToLinear(uint8_t uByte);
static uint8_t linearToULaw(int16_t pcm);
static uint8_t linearToALaw(int16_t pcm);
static int16_t GetHighestAbsoluteSample(const std::vector<uint8_t> &audioBuffer);

uint16_t PCM2ALaw(uint8_t *pcmBuffer, uint8_t *aLawBuffer, uint16_t pcmBufferSize) {
	if (pcmBuffer == NULL && aLawBuffer == NULL && pcmBufferSize == 0) {
		return G711_RC_FAILURE;
	}

	return aLawEncode((uint8_t *)aLawBuffer, (int16_t *)pcmBuffer, pcmBufferSize / 2);
}

uint16_t PCM2ULaw(uint8_t *pcmBuffer, uint8_t *uLawBuffer, uint16_t pcmBufferSize) {
	if (pcmBuffer == NULL && uLawBuffer == NULL && pcmBufferSize == 0) {
		return G711_RC_FAILURE;
	}

	return uLawEncode((uint8_t *)uLawBuffer, (int16_t *)pcmBuffer, pcmBufferSize / 2);
	;
}

uint16_t aLaw2PCM(uint8_t *aLawBuffer, uint8_t *pcmBuffer, uint16_t aLawBufferSize) {
	if (pcmBuffer == NULL && aLawBuffer == NULL && aLawBufferSize == 0) {
		return G711_RC_FAILURE;
	}

	return aLawDecode((int16_t *)pcmBuffer, (uint8_t *)aLawBuffer, aLawBufferSize);
}

uint16_t uLaw2PCM(uint8_t *uLawBuffer, uint8_t *pcmBuffer, uint16_t uLawBufferSize) {
	if (pcmBuffer == NULL && uLawBuffer == NULL && uLawBufferSize == 0) {
		return G711_RC_FAILURE;
	}

	return uLawDecode((int16_t *)pcmBuffer, (uint8_t *)uLawBuffer, uLawBufferSize);
}

uint8_t linearToALaw(int16_t pcm) {
	int16_t mask, seg;
	uint8_t aLaw;

	if (pcm >= 0) {
		mask = 0xD5;
	}
	else {
		mask = 0x55;
		pcm	 = -pcm - 8;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm, segmentEnd, 8);

	/* Combine the sign, segment, and quantization bits. */
	if (seg >= 8) {
		return (0x7F ^ mask);
	}

	aLaw = seg << SEG_SHIFT;
	if (seg < 2) {
		aLaw |= (pcm >> 4) & QUANT_MASK;
	}
	else {
		aLaw |= (pcm >> (seg + 3)) & QUANT_MASK;
	}

	return (aLaw ^ mask);
}

uint8_t linearToULaw(int16_t pcm) {
	int16_t mask, seg;
	uint8_t uLaw;

	if (pcm < 0) {
		pcm	 = BIAS - pcm;
		mask = 0x7F;
	}
	else {
		pcm += BIAS;
		mask = 0xFF;
	}

	/* Convert the scaled magnitude to segment number. */
	seg = search(pcm, segmentEnd, 8);

	/* Combine the sign, segment, quantization bits and complement the code word. */
	if (seg >= 8) {
		return (0x7F ^ mask);
	}

	uLaw = (seg << 4) | ((pcm >> (seg + 3)) & 0xF);

	return (uLaw ^ mask);
}

uint16_t aLawDecode(int16_t pcmBuffer[], const uint8_t aLawBuffer[], uint16_t aLawBufferSize) {
	uint16_t i;
	uint16_t samples;
	uint8_t code;
	uint16_t sl;

	for (samples = i = 0;;) {
		if (i >= aLawBufferSize) {
			break;
		}
		code = aLawBuffer[i++];

		sl = aLawToLinear(code);

		pcmBuffer[samples++] = (int16_t)sl;
	}
	return (samples * 2);
}

uint16_t uLawDecode(int16_t pcmBuffer[], const uint8_t uLawBuffer[], uint16_t uLawBufferSize) {
	uint16_t i;
	uint16_t samples;
	uint8_t code;
	uint16_t sl;

	for (samples = i = 0;;) {
		if (i >= uLawBufferSize) {
			break;
		}
		code = uLawBuffer[i++];

		sl = uLawToLinear(code);

		pcmBuffer[samples++] = (int16_t)sl;
	}
	return samples * 2;
}

uint16_t aLawEncode(uint8_t aLawBuffer[], const int16_t pcmBuffer[], uint16_t pcmBufferSize) {
	for (uint16_t id = 0; id < pcmBufferSize; ++id) {
		aLawBuffer[id] = linearToALaw(pcmBuffer[id]);
	}

	return pcmBufferSize;
}

uint16_t uLawEncode(uint8_t uLawBuffer[], const int16_t pcmBuffer[], uint16_t pcmBufferSize) {
	for (uint16_t id = 0; id < pcmBufferSize; ++id) {
		uLawBuffer[id] = linearToULaw(pcmBuffer[id]);
	}

	return pcmBufferSize;
}

uint16_t search(uint16_t val, int16_t *table, uint16_t size) {
	uint16_t id;

	for (id = 0; id < size; id++) {
		if (val <= *table++) {
			return (id);
		}
	}

	return (size);
}

int16_t aLawToLinear(uint8_t aByte) {
	int t;
	int seg;

	aByte ^= 0x55;

	t	= (aByte & QUANT_MASK) << 4;
	seg = ((unsigned)aByte & SEG_MASK) >> SEG_SHIFT;

	switch (seg) {
	case 0:
		t += 8;
		break;

	case 1:
		t += 0x108;
		break;

	default: {
		t += 0x108;
		t <<= seg - 1;
	} break;
	}

	return ((aByte & SIGN_BIT) ? t : -t);
}

int16_t uLawToLinear(uint8_t uByte) {
	int t;

	uByte = ~uByte;
	t	  = ((uByte & QUANT_MASK) << 3) + BIAS;
	t <<= ((unsigned)uByte & SEG_MASK) >> SEG_SHIFT;

	return ((uByte & SIGN_BIT) ? (BIAS - t) : (t - BIAS));
}

std::vector<uint8_t> makePCMSoundLouderV1(uint8_t *pcmBuffer, uint32_t pcmBufferSize, float factor) {
	std::vector<uint8_t> amplifiedPCMBuffer(pcmBufferSize);

	{
		int16_t highestValue			= GetHighestAbsoluteSample(amplifiedPCMBuffer);
		float highestPossibleMultiplier = (static_cast<float>(INT16_MAX) / highestValue);
		if (factor > highestPossibleMultiplier) {
			factor = highestPossibleMultiplier;
		}

		// printf("> Amplification factor : %0.2f\r\n", factor);
	}

	for (uint32_t id = 0; id < pcmBufferSize; ++id) {
		int16_t pcmSample		= static_cast<int16_t>((pcmBuffer[id + 1] << 8) | pcmBuffer[id]);
		int32_t amplifiedSample = static_cast<int32_t>(pcmSample) * factor;

		if (amplifiedSample > INT16_MAX) {
			amplifiedSample = INT16_MAX;
		}
		else if (amplifiedSample < INT16_MIN) {
			amplifiedSample = INT16_MIN;
		}

		amplifiedPCMBuffer[id]	 = static_cast<uint8_t>(amplifiedSample & 0xFF);
		amplifiedPCMBuffer[++id] = static_cast<uint8_t>((amplifiedSample >> 8) & 0xFF);
	}

	return amplifiedPCMBuffer;
}

std::vector<int16_t> makePCMSoundLouderV2(uint8_t *pcmBuffer, uint32_t pcmBufferSize, float factor) {
	std::vector<int16_t> amplifiedPCMBuffer(pcmBufferSize / 2);

	for (uint32_t id = 0; id < pcmBufferSize; id += 2) {
		int16_t pcmSample		= static_cast<int16_t>((pcmBuffer[id + 1] << 8) | pcmBuffer[id]);
		int32_t amplifiedSample = static_cast<int32_t>(pcmSample) * factor;

		if (amplifiedSample > INT16_MAX) {
			amplifiedSample = INT16_MAX;
		}
		else if (amplifiedSample < INT16_MIN) {
			amplifiedSample = INT16_MIN;
		}

		amplifiedPCMBuffer[id / 2] = static_cast<int16_t>(amplifiedSample);
	}

	return amplifiedPCMBuffer;
}

/*--------------------------------------------------------------------------------------/
 * MARK: Increase volume with avoid overflow/to much increase
 * ------------------------------------------------------------------------------------*/
static std::vector<uint8_t> GetLittleEndianBytesFromShort(int16_t data) {
	std::vector<uint8_t> b(2);
	b[0] = static_cast<uint8_t>(data);
	b[1] = static_cast<uint8_t>((data >> 8) & 0xFF);
	return b;
}

/* Helper function to get the highest absolute sample from the audio buffer */
int16_t GetHighestAbsoluteSample(const std::vector<uint8_t> &audioBuffer) {
	int16_t highestAbsoluteValue = 0;
	for (size_t i = 0; i < audioBuffer.size() - 1; i += 2) {
		int16_t sample = static_cast<int16_t>((audioBuffer[i + 1] << 8) | audioBuffer[i]);

		/* Prevent std::abs overflow exception */
		if (sample == INT16_MIN) {
			sample += 1;
		}
		int16_t absoluteValue = abs(sample);

		if (absoluteValue > highestAbsoluteValue) {
			highestAbsoluteValue = absoluteValue;
		}
	}
	return highestAbsoluteValue;
}

/* Function to increase the decibel level of the audio buffer */
std::vector<uint8_t> IncreaseDecibel(std::vector<uint8_t> pcmBuffer, float factor) {
	/* Max range -32768 and 32767 */
	int16_t highestValue			= GetHighestAbsoluteSample(pcmBuffer);
	float highestPossibleMultiplier = (static_cast<float>(INT16_MAX) / highestValue); /* INT16_MAX = 32767 */

	if (factor > highestPossibleMultiplier) {
		factor = highestPossibleMultiplier;
	}

	printf("Multiple factor = %0.2f\r\n", factor);

	for (size_t id = 0; id < pcmBuffer.size(); id += 2) {
		int16_t sample = static_cast<int16_t>((pcmBuffer[id + 1] << 8) | pcmBuffer[id]);
		sample		   = static_cast<int16_t>(sample * factor);

		std::vector<uint8_t> sampleBytes = GetLittleEndianBytesFromShort(sample);
		pcmBuffer[id]					 = sampleBytes[0];
		pcmBuffer[id + 1]				 = sampleBytes[1];
	}

	return pcmBuffer;
}