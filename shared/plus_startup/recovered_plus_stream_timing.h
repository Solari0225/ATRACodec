#ifndef SONY_RECOVERED_PLUS_STREAM_TIMING_H
#define SONY_RECOVERED_PLUS_STREAM_TIMING_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
enum {SONY_PLUS_FRAME_SAMPLES=2048,SONY_PLUS_ANALYSIS_PRIMING_CALLS=2,
    SONY_PLUS_PACKET_FIFO_DELAY=5,SONY_PLUS_PUBLIC_STARTUP_CALLS=7};
/* pad_last=1 reproduces observed official pure CLI submission: user data's
 * last short block is zero-filled, then submitted as2048valid samples.
 * pad_last=0 models the original SDK API given the true last block length.
 * mirror_sdk=1 includes the5packetFIFO delay;0 emits currentpackets directly
 * and stops5tailmathcalls earlier. Both have independently derived length. */
typedef struct sony_plus_stream_plan {
    uint64_t source_samples,input_blocks,output_frames,core_bytes;
    uint64_t sdk_analysis_calls,minimal_analysis_calls,analysis_calls,packed_packets;
    uint32_t frame_bytes,pad_last,mirror_sdk,eof_zero_calls;
    uint32_t sdk_eof_zero_calls,last_source_samples,last_valid_samples,reserved;
} sony_plus_stream_plan;
typedef struct sony_plus_stream_timing {
    sony_plus_stream_plan plan;
    uint64_t next_call;
} sony_plus_stream_timing;
typedef struct sony_plus_stream_action {
    uint64_t call_number,source_offset,packet_ordinal,output_packet_ordinal;
    uint32_t source_samples,valid_samples,is_eof,should_pack,should_emit;
    int32_t startup_before,startup_after,flush_before,flush_after;
    uint32_t reserved;
} sony_plus_stream_action;
int sony_recovered_plus_stream_plan(uint64_t source_samples,uint32_t frame_bytes,
    uint32_t pad_last,uint32_t mirror_sdk,sony_plus_stream_plan *plan);
int sony_recovered_plus_stream_initialize(sony_plus_stream_timing *timing,
    const sony_plus_stream_plan *plan);
/*0=oneaction;1=exhausted;-1=invalid. Every action must run PCM analysis,
 * timefeature, spectral preparation and allocation, INCLUDING first2calls.
 * Only should_pack gates bitstream serialization. No calibration probe.
 * should_emit inSDKmirror mode consumes packet_ordinal-5from a6slotFIFO;
 * inminimal mode it consumes this call'scurrentpacket without aFIFO. */
int sony_recovered_plus_stream_next(sony_plus_stream_timing *timing,
    sony_plus_stream_action *action);
#ifdef __cplusplus
}
#endif
#endif
