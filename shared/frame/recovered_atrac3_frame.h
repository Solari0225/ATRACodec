#ifndef SONY_RECOVERED_ATRAC3_FRAME_H
#define SONY_RECOVERED_ATRAC3_FRAME_H
#include "../sideinfo/recovered_atrac3_sideinfo.h"
#include "../tonal_analysis/recovered_atrac3_tonal_analysis.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Compose the recovered side-information and spectral writer. All input
 * encoding decisions and transformed spectra must already be supplied.
 * This is not a PCM encoder and must not be advertised as one.
 * Returns0, or the child writer's negative error. */
int sony_recovered_atrac3_write_channel(const sony_atrac3_sideinfo *info,
    const float spectrum[1024],uint8_t *output,uint32_t capacity,
    uint32_t byte_offset,uint32_t *end_bit);
/* Original independent-stereo132kbps framing: two192-byte sound units.
 * output capacity must be at least387, including word-store padding.
 * Only the first384 output bytes belong to the frame. The function clears
 * these387 bytes before writing. extended_header must be0 for this profile.
 * end_bits[2] are absolute offsets within output. -4 means unit budget exceeded.
 * On any failure discard the output. This function does not handle priming,
 * flush, OMA/AAL containers, PCM analysis or encoding-parameter selection. */
int sony_recovered_atrac3_write_stereo132(const sony_atrac3_sideinfo info[2],
    const float spectra[2][1024],uint8_t *output,uint32_t capacity,
    uint32_t end_bits[2]);
/* Same original independent-stereo layout with152-byte (105) or192-byte
 * (132) units. capacity must include3word-store padding bytes. No joint mode. */
int sony_recovered_atrac3_write_independent_stereo(const sony_atrac3_sideinfo info[2],
    const float spectra[2][1024],uint32_t unit_bytes,uint8_t *output,
    uint32_t capacity,uint32_t end_bits[2]);
/* Convert independently computed analysis decisions into the typed writer
 * inputs. Gain points and optional extended-header fields are upstream inputs;
 * they are not inferred from an encoded stream. Caller owns all output arrays
 * and keeps them alive through packing. Arrays must not overlap. Returns the
 * number of exported components, or-1 invalid metadata. */
int32_t sony_recovered_atrac3_sideinfo_from_analysis(
    const sony_atrac3_tonal_analysis_state *state,
    const sony_atrac3_gain_band gains[4],uint32_t extended_header,
    const uint32_t header_fields[5],sony_atrac3_sideinfo *info,
    sony_atrac3_tonal_group groups[2],sony_atrac3_tonal_component components[64]);
#ifdef __cplusplus
}
#endif
#endif
