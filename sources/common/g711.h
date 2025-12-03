#ifndef __G711_H
#define __G711_H

#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <vector>
#include <cmath>

/*--------------------------------------------------------------------------------/
 * ULAW, ALAW and Linear PCM conversions
 *-------------------------------------------------------------------------------*/
#define SIGN_BIT   (0x80) /* Sign bit for a ALAW byte. */
#define QUANT_MASK (0xF)  /* Quantization field mask. */
#define NSEGS	   (8)	  /* Number of ALAW segments. */
#define SEG_SHIFT  (4)	  /* Left shift for segment number. */
#define SEG_MASK   (0x70) /* Segment field mask. */
#define BIAS	   (0x84) /* Bias for linear code. */

#define G711_RC_FAILURE (-1)
#define G711_RC_SUCCESS (1)

extern uint16_t PCM2ALaw(uint8_t *pcmBuffer, uint8_t *aLawBuffer, uint16_t pcmBufferSize);
extern uint16_t PCM2ULaw(uint8_t *pcmBuffer, uint8_t *uLawBuffer, uint16_t pcmBufferSize);
extern uint16_t aLaw2PCM(uint8_t *aLawBuffer, uint8_t *pcmBuffer, uint16_t aLawBufferSize);
extern uint16_t uLaw2PCM(uint8_t *uLawBuffer, uint8_t *pcmBuffer, uint16_t uLawBufferSize);

extern uint16_t aLawDecode(int16_t pcmBuffer[], const uint8_t aLawBuffer[], uint16_t aLawBufferSize);
extern uint16_t uLawDecode(int16_t pcmBuffer[], const uint8_t uLawBuffer[], uint16_t uLawBufferSize);
extern uint16_t aLawEncode(uint8_t aLawBuffer[], const int16_t pcmBuffer[], uint16_t pcmBufferSize);
extern uint16_t uLawEncode(uint8_t uLawBuffer[], const int16_t pcmBuffer[], uint16_t pcmBufferSize);

/*-----------------------------------------------------------------/
 * FORCE volume to be amplified
 * ---------------------------------------------------------------*/
extern std::vector<uint8_t> makePCMSoundLouderV1(uint8_t *pcmBuffer, uint32_t pcmBufferSize, float factor = 2.7);
extern std::vector<int16_t> makePCMSoundLouderV2(uint8_t *pcmBuffer, uint32_t pcmBufferSize, float factor = 2.7);

/*--------------------------------------------------------------------------------------/
 * Increase small volume and ignore big volume.
 * Reference: https://stackoverflow.com/questions/36355992/how-to-increase-volume-amplitude-on-raw-audio-bytes
 * ------------------------------------------------------------------------------------*/
extern std::vector<uint8_t> IncreaseDecibel(std::vector<uint8_t> pcmBuffer, float factor = 2.7);

#endif /* __G711_H */