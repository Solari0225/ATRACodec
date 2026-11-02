#ifndef SONY_INDEPENDENT_PLUS_WINDOW_H
#define SONY_INDEPENDENT_PLUS_WINDOW_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_window_config {
    /* max_qu stores profileID(settings+1e8); low_band_limit is settings+bc. */
    int32_t mode,policy_force,low_band_limit,max_qu;
    uint32_t evolving_flags;
} sony_plus_window_config;
/* These are only the three actual fields selected by40a4c0. They map to
 * window_flag+0x3c,low_band_scale+0x54,peak_ratio+0x64in the native0x98record.
 * Other fields of that larger gain-analysis record are deliberately absent. */
typedef struct sony_plus_window_result {
    uint32_t window_flag;
    float low_band_scale,peak_ratio;
} sony_plus_window_result;
/* input[band] points to exactly128binary32samples corresponding to the
 * native band-buffer positions64..191. Prior gain counts are only read in
 * the forced evolving-mask branch. result.window_flag is an in/out field;
 * the real enclosing caller clears all records before normal analysis. */
int sony_recovered_plus_select_windows(const sony_plus_window_config *config,
    const float *const input[16],uint32_t bands,const int32_t prior_gain_count[16],
    sony_plus_window_result result[16],uint32_t output_flags[16]);
#ifdef __cplusplus
}
#endif
#endif
