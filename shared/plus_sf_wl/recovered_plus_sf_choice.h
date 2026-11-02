#ifndef SONY_RECOVERED_PLUS_SF_CHOICE_H
#define SONY_RECOVERED_PLUS_SF_CHOICE_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct sony_plus_sf_choice_config {uint32_t channels,units,groups,full_table;} sony_plus_sf_choice_config;
typedef struct sony_plus_sf_choice_channel {
    uint32_t channel_index;int32_t word_length[32],coarse[32];
    /* Raw original words at1c73c..1c758: mode,prefix,width,base,Huffman table,
     * weight,shape and first anchor. These include unused intermediates. */
    int32_t metadata[8],prepared[4][32];
    /* Original1078,107c,1080 and1084 vector for codebook-description cost. */
    int32_t book_metadata[3],book_class[32];
} sony_plus_sf_choice_channel;
/* Complete original3eada0/3eae50 SF mode selection and mathematical closure.
 * Exact original six-bit SF, weighted and group-shape preparation, four-mode
 * comparison and first-candidate tie behavior. Return0/-1, output bit cost.
 * units0 is original no-op. For active units, groups1..10 is original cfg+b8.
 * All32 supplied coarse values must be0..63, including padded group members. */
int sony_recovered_plus_choose_scale_factors(const sony_plus_sf_choice_config *config,
    sony_plus_sf_choice_channel channels[2],int32_t enabled,int32_t *bits);
/* Complete original3e6bf0->403a90. This is spectral-codebook classification
 * cost, not SF cost. full_table0/1 selects original2/3bit descriptor widths. */
int sony_recovered_plus_codebook_class_cost(const sony_plus_sf_choice_config *config,
    sony_plus_sf_choice_channel channels[2],int32_t *bits);
#ifdef __cplusplus
}
#endif
#endif
