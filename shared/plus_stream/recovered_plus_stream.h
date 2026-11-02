#ifndef SONY_RECOVERED_PLUS_STREAM_H
#define SONY_RECOVERED_PLUS_STREAM_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Whole-memory signed16 interleaved stereo44100 -> original physical plus
 * cores. bitrate is one of48000/64000/96000/128000/160000/192000/256000/
 * 320000/352000. pad_last1 mirrors pure-OMA CLI zero-padding to2048 valid
 * frames; pad_last0 uses actual last valid frames for AALP scheduling.
 * Output includes the original stream priming/flush cores and5-frameFIFO.
 * Success returns0 and a malloc-owned buffer. Free it only with the paired
 * sony_recovered_plus_free_cores. No Sony runtime, files, goldens or GUI.
 * Failure returns a negative result and resets output/bytes/frame_count.
 * This declaration is an integration contract, not a completed validation. */
int sony_recovered_plus_encode_cores(const int16_t *pcm,uint32_t sample_frames,
    uint32_t bitrate,uint32_t pad_last,uint8_t **cores,size_t *bytes,
    uint32_t *frame_count);
void sony_recovered_plus_free_cores(void *cores);

#ifdef __cplusplus
}
#endif
#endif
