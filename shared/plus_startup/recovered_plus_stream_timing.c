#include "recovered_plus_stream_timing.h"
int sony_recovered_plus_stream_plan(uint64_t samples,uint32_t bytes,uint32_t pad_last,uint32_t mirror,sony_plus_stream_plan *out){
    if(!out||!bytes||bytes>2048||pad_last>1||mirror>1)return -1;
    sony_plus_stream_plan p={0};p.source_samples=samples;p.frame_bytes=bytes;p.pad_last=pad_last;p.mirror_sdk=mirror;
    if(samples){
        p.input_blocks=samples/2048+(samples%2048!=0);
        p.last_source_samples=(uint32_t)((samples-1)%2048)+1;
        p.last_valid_samples=pad_last?2048:p.last_source_samples;
        /* Original3d9610returns14336. Original3dbb70sets+1c:
         *floor((14336+valid+367)/2048)+1. The EOFcaller decrements
         *thiscounter BEFORE each valid0call, reaching0on the finalcall. */
        p.sdk_eof_zero_calls=8+(p.last_valid_samples>1680);
        p.sdk_analysis_calls=p.input_blocks+p.sdk_eof_zero_calls;
        p.minimal_analysis_calls=p.sdk_analysis_calls-5;
        p.output_frames=p.sdk_analysis_calls-7;
        if(p.output_frames>UINT64_MAX/bytes)return -2;
        p.core_bytes=p.output_frames*bytes;
        p.analysis_calls=mirror?p.sdk_analysis_calls:p.minimal_analysis_calls;
        p.eof_zero_calls=mirror?p.sdk_eof_zero_calls:p.sdk_eof_zero_calls-5;
        p.packed_packets=p.analysis_calls-2;
    }
    *out=p;return 0;
}
int sony_recovered_plus_stream_initialize(sony_plus_stream_timing *timing,const sony_plus_stream_plan *plan){
    if(!timing||!plan)return -1;
    sony_plus_stream_plan expected;
    int result=sony_recovered_plus_stream_plan(plan->source_samples,plan->frame_bytes,plan->pad_last,plan->mirror_sdk,&expected);
    if(result)return result;
    timing->plan=expected;timing->next_call=0;return 0;
}
int sony_recovered_plus_stream_next(sony_plus_stream_timing *timing,sony_plus_stream_action *out){
    if(!timing||!out)return -1;
    const sony_plus_stream_plan *p=&timing->plan;
    if(timing->next_call>=p->analysis_calls)return 1;
    const uint64_t call=timing->next_call++,blocks=p->input_blocks;
    sony_plus_stream_action a={0};a.call_number=call+1;a.packet_ordinal=a.output_packet_ordinal=UINT64_MAX;
    a.startup_before=call<7?(int32_t)(7-call):0;a.startup_after=call<7?(int32_t)(6-call):0;
    a.should_pack=call>=2;
    if(a.should_pack)a.packet_ordinal=call-2;
    if(call<blocks){
        a.source_offset=call*2048;a.source_samples=(call+1==blocks)?p->last_source_samples:2048;
        a.valid_samples=p->pad_last?2048:a.source_samples;a.flush_before=9;
        a.flush_after=(int32_t)(8+(a.valid_samples>1680));
    }else{
        const uint64_t eof_index=call-blocks;
        a.source_offset=p->source_samples;a.is_eof=1;
        a.flush_before=a.flush_after=(int32_t)(p->sdk_eof_zero_calls-1-eof_index);
    }
    a.should_emit=p->mirror_sdk?(call>=7):a.should_pack;
    if(a.should_emit)a.output_packet_ordinal=p->mirror_sdk?call-7:call-2;
    *out=a;return 0;
}
