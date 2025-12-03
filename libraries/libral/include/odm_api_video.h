/**
 * @file fca_video.h
 * @brief FPT Camera Agent Video API Definitions
 * 
 * This header file contains all definitions and function prototypes for 
 * video input handling in the FPT Camera Agent system.
 */

#ifndef FCA_VIDEO_H
#define FCA_VIDEO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
#define    defCapMaxLen             (2*1024*1024)
/**
 * @brief Video codec types enumeration
 */
typedef enum {
    FCA_CODEC_VIDEO_H264 = 0,  ///< H.264 video codec
    FCA_CODEC_VIDEO_H265,      ///< H.265 video codec
    FCA_CODEC_VIDEO_MJPEG,     ///< Motion JPEG codec
} FCA_VIDEO_IN_CODEC_E;

/**
 * @brief Video encoding modes enumeration
 */
typedef enum {
    FCA_VIDEO_ENCODING_MODE_CBR = 0,  ///< Constant Bitrate
    FCA_VIDEO_ENCODING_MODE_VBR,      ///< Variable Bitrate
    FCA_VIDEO_ENCODING_MODE_AVBR,     ///< Adaptive Variable Bitrate
} FCA_VIDEO_IN_ENCODING_MODE_E;

/**
 * @brief Video frame types enumeration
 */
typedef enum {
    FCA_VIDEO_IN_FRAME_TYPE_P = 0,  ///< P-frame
    FCA_VIDEO_IN_FRAME_TYPE_I,      ///< I-frame
    FCA_VIDEO_IN_FRAME_TYPE_PI,     ///< P and I frame
} FCA_VIDEO_IN_FRAME_TYPE_E;

/**
 * @brief Video input configuration structure
 */
typedef struct {
    FCA_VIDEO_IN_CODEC_E codec;            ///< Codec type
    FCA_VIDEO_IN_ENCODING_MODE_E encoding_mode; ///< CBR/AVBR/VBR
    int bitrate_target;                    ///< Target bitrate (kbps)
    int bitrate_max;                       ///< Maximum bitrate (kbps)
    int gop;                               ///< Group of pictures
    int fps;                               ///< Frame rate (1-25 fps)
    int width;                             ///< Frame width in pixels
    int height;                            ///< Frame height in pixels
    int max_qp;                            ///< Quantization parameter max
    int min_qp;                            ///< Quantization parameter min
} fca_video_in_config_t;

/**
 * @brief Video frame structure
 */
typedef struct {
    unsigned char *data;                   ///< Frame data buffer
    unsigned int size;                     ///< Frame size in bytes
    unsigned long long timestamp;          ///< Frame timestamp
    unsigned long sequence;                ///< Frame sequence number
    FCA_VIDEO_IN_FRAME_TYPE_E type;        ///< Frame type
} fca_video_in_frame_t;

/**
 * @brief Video frame callback function type
 */
typedef void (*FCA_VIDEO_IN_CALLBACK)(const fca_video_in_frame_t *video_frame);

/**
 * @brief QR code callback function type
 */
typedef void (*FCA_VIDEO_IN_QRCODE_CALLBACK)(char *qrcode_str, int qrcode_len);

/**
 * @brief Initialize the video input module
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_init(void);

/**
 * @brief Set configuration for the specified video stream
 * @param index Stream index (0: main, 1: sub/snapshot, 2: third)
 * @param config Pointer to video configuration structure
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_set_config(int index, fca_video_in_config_t *config);

/**
 * @brief Get configuration for the specified video stream
 * @param index Stream index (0: main, 1: sub/snapshot, 2: third)
 * @param config Pointer to video configuration structure to be filled
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_get_config(int index, fca_video_in_config_t *config);

/**
 * @brief Uninitialize the video input module
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_uninit(void);

/**
 * @brief Get a snapshot from the sub video stream (JPEG format)
 * @param snap_addr Pointer to buffer for snapshot data
 * @param snap_size Pointer to variable for snapshot size
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_get_snapshot(char *snap_addr, int *snap_size);

/**
 * @brief Start video frame callback for specified stream
 * @param index Stream index (0: main, 1: sub/snapshot, 2: third)
 * @param cb Callback function to receive video frames
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_start_callback(int index, FCA_VIDEO_IN_CALLBACK cb);

/**
 * @brief Stop video frame callback for specified stream
 * @param index Stream index (0: main, 1: sub/snapshot, 2: third)
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_stop_callback(int index);

/**
 * @brief Start QR code scanning on sub video stream
 * @param cb Callback function to receive QR code strings
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_qrcode_scan_start(FCA_VIDEO_IN_QRCODE_CALLBACK cb);

/**
 * @brief Stop QR code scanning
 * @return 0 on success, negative error code on failure
 */
int fca_video_in_qrcode_scan_stop();

#ifdef __cplusplus
}
#endif

#endif /* FCA_VIDEO_H */
