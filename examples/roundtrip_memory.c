/* Minimal C11 application integration. Buffers are owned/freed by ATRACodec. */
#include "atracodec.h"
#include <stdio.h>
#include <string.h>
int main(void){
    const int16_t input[4]={123,-456,-32768,32767};
    uint8_t *oma=NULL;size_t bytes=0;int16_t *decoded=NULL;
    atracodec_audio_info info;
    if(atracodec_encode_pcm16(input,2,ATRACODEC_AALP,352000,&oma,&bytes)){
        fprintf(stderr,"Encode: %s\n",atracodec_last_error());return 1;
    }
    const int status=atracodec_decode_pcm16(oma,bytes,&decoded,&info);
    atracodec_free(oma);
    if(status){fprintf(stderr,"Decode: %s\n",atracodec_last_error());return 1;}
    const int correct=info.sample_frames==2&&info.channels==2&&
        info.sample_rate==44100&&info.sample_bits==16&&
        memcmp(input,decoded,sizeof(input))==0;
    atracodec_free(decoded);
    if(!correct){fprintf(stderr,"Lossless roundtrip mismatch\n");return 1;}
    printf("%s: lossless roundtrip OK, %zu OMA bytes\n",atracodec_version(),bytes);
    return 0;
}
