#ifndef SONY_RECOVERED_ATRAC3_MDCT_H
#define SONY_RECOVERED_ATRAC3_MDCT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_atrac3_mdct_state {
    /* current time1024 -> four contiguous frequency bands of256 each;
       preceding frame1024; persistent folded overlap512. */
    float samples[2560];
    uint32_t gain_index[4];
} sony_atrac3_mdct_state;
typedef struct sony_atrac3_mdct_tables {
    uint32_t permutation[128];
    float butterfly[512];
    float fold[2048];
    float gain_scale[16];
    float final_scale[4];
} sony_atrac3_mdct_tables;
/* Recovered actual +0x460350 kernel, not a complete encoder.
   Binary32 round-to-nearest, separate multiply/add, no FMA. Tables contain
   numerical constants only and are supplied by the independent caller. */
int sony_recovered_atrac3_mdct(sony_atrac3_mdct_state* state,
                              const sony_atrac3_mdct_tables* tables);
const sony_atrac3_mdct_tables* sony_recovered_atrac3_mdct_default_tables(void);
#ifdef __cplusplus
}
#endif
#endif
