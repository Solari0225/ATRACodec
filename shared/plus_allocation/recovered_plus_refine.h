#ifndef SONY_RECOVERED_PLUS_REFINE_H
#define SONY_RECOVERED_PLUS_REFINE_H
#include "recovered_plus_energy.h"
#include "../plus_sf_wl/recovered_plus_sf_choice.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_refine_config {
    sony_plus_sf_choice_config sf;
    sony_plus_energy_noise_config noise;
    uint32_t units,subbands,profile,simd;uint16_t stereo_units[32];
} sony_plus_refine_config;
typedef struct sony_plus_refine_channel {
    sony_plus_sf_choice_channel sf;
    sony_plus_energy_noise_channel noise;
    float spectrum[2048],peakiness[32];int16_t quantized[2048];uint32_t noise_enabled;
} sony_plus_refine_channel;
typedef struct sony_plus_refine_shared {int32_t enabled;int16_t sf_bits,overhead,total;} sony_plus_refine_shared;
/* Complete original3e8270, including scalar/SIMD energy arithmetic, original
 * deterministic noise, coarse-index refinements, SF mode selection and full
 * budget rollback. All upstream spectra/gain/metrics are typed caller inputs.
 * Changes only coarse SF indices and SF preparation/metadata plus cost totals. */
int sony_recovered_plus_refine_scales(const sony_plus_refine_config *config,
    sony_plus_refine_channel channels[2],sony_plus_refine_shared *shared,
    int32_t allowed_extra_bits);
#ifdef __cplusplus
}
#endif
#endif
