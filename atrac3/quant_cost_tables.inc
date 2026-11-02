/* Exact numerical tables from capture009; see quant-cost-table-provenance.json. */
static const uint32_t scale_words[22] = {UINT32_C(0x00000000),UINT32_C(0x41f1e7a4),UINT32_C(0x421863f8),UINT32_C(0x42400000),UINT32_C(0x4249965e),UINT32_C(0x427dfbf2),UINT32_C(0x42a00000),UINT32_C(0x428d1c75),UINT32_C(0x42b1c9f7),UINT32_C(0x42e00000),UINT32_C(0x42b56dbb),UINT32_C(0x42e495f4),UINT32_C(0x43100000),UINT32_C(0x431730c6),UINT32_C(0x433e7cf6),UINT32_C(0x43700000),UINT32_C(0x439c3aef),UINT32_C(0x43c4d676),UINT32_C(0x43f80000),UINT32_C(0x441ec003),UINT32_C(0x44480335),UINT32_C(0x447c0000)};
static const uint32_t quant_masks[8] = {UINT32_C(0x00000000),UINT32_C(0x4b400000),UINT32_C(0x00000007),UINT32_C(0x00000007),UINT32_C(0x0000000f),UINT32_C(0x0000000f),UINT32_C(0x0000001f),UINT32_C(0x0000003f)};
static const uint32_t nonzero_lengths[16] = {2,5,4,6,5,8,7,9,4,7,6,8,6,9,8,10};
static const uint32_t vlc_lengths[8][64] = {
 {0},
 {0},
 {1,3,3,1,1,1,3,3},
 {1,3,4,4,1,4,4,3},
 {1,3,4,5,5,1,1,1,1,1,1,1,5,5,4,3},
 {2,3,4,4,5,6,6,4,2,4,6,6,5,4,4,3},
 {3,4,4,4,5,5,5,6,6,6,6,7,7,7,7,4,3,4,7,7,7,7,6,6,6,6,5,5,5,4,4,4},
 {3,5,5,5,5,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,4,3,4,8,8,8,8,8,8,8,8,8,8,7,7,7,7,7,7,7,6,6,6,6,6,6,6,6,5,5,5,5,5}
};
