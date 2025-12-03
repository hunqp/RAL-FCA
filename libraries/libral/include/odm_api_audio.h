/**
 * @file fca_audio.h
 * @brief FPT Camera Agent Audio API Interface
 * 
 * This header file defines all audio-related APIs for the FPT Camera Agent.
 * It includes functions for audio input/output initialization, configuration,
 * and control operations.
 */

#ifndef FCA_AUDIO_H
#define FCA_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio codec types
 */
typedef enum {
    FCA_AUDIO_CODEC_AAC,          /**< AAC audio codec */
    FCA_AUDIO_CODEC_G711_ALAW,    /**< G.711 A-law codec */
    FCA_AUDIO_CODEC_G711_ULAW,    /**< G.711 μ-law codec */
    FCA_AUDIO_CODEC_PCM           /**< PCM raw audio */
} FCA_AUDIO_CODEC;

/**
 * @brief Audio sample rates
 */
typedef enum {
    FCA_AUDIO_SAMPLE_RATE_8_KHZ = 8000,     /**< 8 kHz sample rate */
    FCA_AUDIO_SAMPLE_RATE_16_KHZ = 16000,   /**< 16 kHz sample rate */
    FCA_AUDIO_SAMPLE_RATE_32_KHZ = 32000,   /**< 32 kHz sample rate */
    FCA_AUDIO_SAMPLE_RATE_44_1_KHZ = 44100, /**< 44.1 kHz sample rate */
    FCA_AUDIO_SAMPLE_RATE_48_KHZ = 48000    /**< 48 kHz sample rate */
} FCA_AUDIO_SAMPLE_RATE;

/**
 * @brief Audio configuration structure
 */
typedef struct {
    FCA_AUDIO_CODEC codec;        /**< Audio codec type */
    FCA_AUDIO_SAMPLE_RATE rate;   /**< Sample rate */
    int channel;                  /**< Channel count (1:mono) */
    int format;                   /**< Bit width (16: 16-bit samples) */
} fca_audio_config_t;

/**
 * @brief Audio frame structure
 */
typedef struct {
    unsigned char* data;          /**< Audio frame data */
    unsigned int size;            /**< Audio frame size in bytes */
    unsigned long long timestamp; /**< Timestamp of the frame */
    unsigned long sequence;       /**< Sequence number of the frame */
} fca_audio_frame_t;

/**
 * @brief Audio input callback function type
 */
typedef int (*FCA_AUDIO_IN_CALLBACK)(const fca_audio_frame_t* audio_frame);

/* Audio Input Functions */

/**
 * @brief Initialize the audio input module
 * @return 0 on success, negative error code on failure
 */
int fca_audio_in_init(void);

/**
 * @brief Set configuration for audio input
 * @param index Channel index
 * @param config Audio configuration to set
 * @return 0 on success, negative error code on failure
 */
/*
Codec:
The PCM codec supports sampling rates of 8 kHz, 16 kHz, 32 kHz, 44.1 kHz, and 48 kHz.
The AAC and G711A/U codecs only support a sampling rate of 8 kHz or 16 kHz; other sampling rates are not supported at this time.
*/
int fca_audio_in_set_config(int index, fca_audio_config_t* config);

/**
 * @brief Get configuration for audio input
 * @param index Channel index
 * @param config Audio configuration to get
 * @return 0 on success, negative error code on failure
 */
int fca_audio_in_get_config(int index, fca_audio_config_t* config);

/**
 * @brief Uninitialize the audio input module
 * @return 0 on success, negative error code on failure
 */
int fca_audio_in_uninit(void);

/**
 * @brief Get the current audio input volume
 * @param volume Pointer to store volume value (0-100)
 * @return 0 on success, negative error code on failure
 */
int fca_get_audio_in_volume(int* volume);

/**
 * @brief Set the audio input volume
 * @param volume Volume value to set (0-100)
 * @return 0 on success, negative error code on failure
 */
int fca_set_audio_in_volume(int volume);

/**
 * @brief Start audio input callback
 * @param index Channel index
 * @param cb Callback function to receive audio frames
 * @return 0 on success, negative error code on failure
 */
int fca_audio_in_start_callback(int index, FCA_AUDIO_IN_CALLBACK cb);

/**
 * @brief Stop audio input callback
 * @param index Channel index
 * @return 0 on success, negative error code on failure
 */
int fca_audio_in_stop_callback(int index);

/**
 * @brief Turn on/off decibel detection
 * @param onoff 1 to enable, 0 to disable
 * @return 0 on success, negative error code on failure
 */
int fca_audio_in_decibel_detection_set_onoff(int onoff);

/**
 * @brief Set decibel detection sensitivity
 * @param level Sensitivity level
 * @return 0 on success, negative error code on failure
 */
int fca_audio_in_decibel_detection_set_sensitivity(int level);

/**
 * @brief Get current decibel detection status
 * @return Decibel detection status
 */
int fca_audio_in_get_decibel_status(void);

/* Audio Output Functions */

/**
 * @brief Initialize the audio output module
 * @return 0 on success, negative error code on failure
 */
int fca_audio_out_init(void);

/**
 * @brief Set configuration for audio output
 * @param config Audio configuration to set
 * @return 0 on success, negative error code on failure
 */
/*
Codec:
The PCM codec supports sampling rates of 8 kHz, 16 kHz, 32 kHz, 44.1 kHz, and 48 kHz.
The AAC and G711A/U codecs only support a sampling rate of 8 kHz or 16 kHz; other sampling rates are not supported at this time.
*/
int fca_audio_out_set_config(fca_audio_config_t* config);

/**
 * @brief Get configuration for audio output
 * @param config Audio configuration to get
 * @return 0 on success, negative error code on failure
 */
int fca_audio_out_get_config(fca_audio_config_t* config);

/**
 * @brief Uninitialize the audio output module
 * @return 0 on success, negative error code on failure
 */
int fca_audio_out_uninit(void);

/**
 * @brief Set audio output volume
 * @param volume Volume value to set (0-100)
 * @return 0 on success, negative error code on failure
 */
int fca_audio_out_set_volume(int volume);

/**
 * @brief Get audio output volume
 * @param volume Pointer to store volume value (0-100)
 * @return 0 on success, negative error code on failure
 */
int fca_audio_out_get_volume(int* volume);

/**
 * @brief Play a PCM file (blocking)
 * @param file_path Path to the audio file
 * @return 0 on success, negative error code on failure
 */
/*
    Only PCM or ALAW formats are supported. 
    The audio file format is 16-bit bit depth, single-channel, 
    with an 8 kHz sampling rate.
*/
int fca_audio_out_play_file_block(const char* file_path);

/**
 * @brief Play an audio frame (non-blocking)
 * @param frame_data Audio frame data
 * @param frame_len Length of audio frame
 * @return 0 on success, negative error code on failure
 */
int fca_audio_out_play_frame(unsigned char* frame_data, int frame_len);

/**
 * @brief Stop audio playback immediately
 * @return 0 on success, negative error code on failure
 */
int fca_audio_out_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* FCA_AUDIO_H */
