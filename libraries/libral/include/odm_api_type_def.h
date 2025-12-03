#ifndef ODM_API_ROI_H
#define ODM_API_ROI_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Coordinates of a moving object in the main stream
 */
typedef struct {
    int x;
    int y;
} fca_point_t;

/**
 * A structure of ROI
 */
typedef struct {
    char roi_grid[8];
} fca_roi_t;


#ifdef __cplusplus
}
#endif

#endif /* ODM_API_ROI_H */
