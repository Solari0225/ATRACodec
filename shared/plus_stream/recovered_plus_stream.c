#include "recovered_plus_stream.h"
#include "../plus_startup/recovered_plus_stream_timing.h"
#include "../plus_pcm/recovered_plus_pcm_core.h"
#include <stdlib.h>
#include <string.h>

void sony_recovered_plus_free_cores(void *cores){free(cores);}

int sony_recovered_plus_encode_cores(const int16_t *pcm,uint32_t samples,
    uint32_t bitrate,uint32_t pad_last,uint8_t **cores,size_t *bytes,uint32_t *count){
    if(!cores||!bytes||!count)return -1;
    *cores=NULL;*bytes=0;*count=0;
    if((samples&&!pcm)||pad_last>1)return -1;
    sony_plus_spectrum_config config;
    if(sony_recovered_plus_spectrum_default(bitrate,&config))return -1;
    sony_plus_stream_plan plan;
    int result=sony_recovered_plus_stream_plan(samples,config.frame.frame_bytes,pad_last,1,&plan);
    if(result)return result;
    if(plan.core_bytes>SIZE_MAX||plan.output_frames>UINT32_MAX||
       (uint64_t)samples>SIZE_MAX/(2u*sizeof(int16_t)))return -2;
    if(!samples)return 0;
    uint8_t *output=(uint8_t*)malloc((size_t)plan.core_bytes);
    sony_plus_pcm_core_state *state=(sony_plus_pcm_core_state*)malloc(sizeof(*state));
    if(!output||!state){free(output);free(state);return -3;}
    if(sony_recovered_plus_pcm_core_initialize(bitrate,config.allocation.initial.request_mode,state)){
        free(output);free(state);return -4;
    }
    sony_plus_stream_timing timing;
    if(sony_recovered_plus_stream_initialize(&timing,&plan)){
        free(output);free(state);return -4;
    }
    uint8_t fifo[6][2048];uint64_t fifo_id[6];
    for(uint32_t i=0;i<6;++i)fifo_id[i]=UINT64_MAX;
    int16_t block[4096];size_t written=0;uint32_t frames=0;
    sony_plus_stream_action action;
    while((result=sony_recovered_plus_stream_next(&timing,&action))==0){
        memset(block,0,sizeof(block));
        if(action.source_samples){
            const size_t offset=(size_t)action.source_offset*2u;
            memcpy(block,pcm+offset,(size_t)action.source_samples*2u*sizeof(int16_t));
        }
        if(sony_recovered_plus_pcm_core_analyze(state,block)<0){result=-4;break;}
        if(action.should_pack){
            const uint32_t slot=(uint32_t)(action.packet_ordinal%6u);
            if(sony_recovered_plus_pcm_core_pack(state,fifo[slot],sizeof(fifo[slot]))!=(int32_t)plan.frame_bytes){result=-4;break;}
            fifo_id[slot]=action.packet_ordinal;
        }
        if(action.should_emit){
            const uint32_t slot=(uint32_t)(action.output_packet_ordinal%6u);
            if(fifo_id[slot]!=action.output_packet_ordinal||
               written>(size_t)plan.core_bytes-plan.frame_bytes||frames>=plan.output_frames){result=-5;break;}
            memcpy(output+written,fifo[slot],plan.frame_bytes);
            written+=plan.frame_bytes;++frames;
        }
    }
    free(state);
    if(result!=1||written!=(size_t)plan.core_bytes||frames!=plan.output_frames){free(output);return result<0?result:-5;}
    *cores=output;*bytes=written;*count=frames;return 0;
}
