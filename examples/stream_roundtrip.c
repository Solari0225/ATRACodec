/* C11 bounded-memory loopback. Feed one encoder's output into a separate
 * decoder context, checking PCM without storing either complete stream. */
#include "atracodec_stream.h"
#include <stdio.h>
typedef struct Example {
    atracodec_stream_decoder *decoder;
    uint64_t decoded,oma_bytes;
    int mismatch;
} Example;
static int16_t sample(uint64_t frame,unsigned channel){
    return (int16_t)((int32_t)((frame*71+channel*233)%8192)-4096);
}
static int receive_pcm(void *user,const int16_t *pcm,size_t frames){
    Example *e=(Example*)user;
    for(size_t i=0;i<frames;++i)for(unsigned ch=0;ch<2;++ch)
        if(pcm[i*2+ch]!=sample(e->decoded+i,ch)){e->mismatch=1;return 1;}
    e->decoded+=frames;return 0;
}
static int receive_oma(void *user,const uint8_t *data,size_t bytes){
    Example *e=(Example*)user;e->oma_bytes+=bytes;
    /* Calling another context from a callback is allowed. Reentering the
     *same context is rejected by the API. The callback must consume bytes. */
    return atracodec_decoder_write(e->decoder,data,bytes);
}
int main(void){
    Example e={0};atracodec_stream_encoder *encoder=NULL;const uint64_t frames=8193;
    if(atracodec_decoder_create(receive_pcm,&e,&e.decoder))goto failed;
    if(atracodec_encoder_create(ATRACODEC_AALP,352000,frames,receive_oma,&e,&encoder))goto failed;
    for(uint64_t at=0;at<frames;){
        int16_t pcm[514];const size_t n=(size_t)(frames-at>257?257:frames-at);
        for(size_t i=0;i<n;++i)for(unsigned ch=0;ch<2;++ch)pcm[i*2+ch]=sample(at+i,ch);
        if(atracodec_encoder_write(encoder,pcm,n))goto failed;at+=n;
    }
    if(atracodec_encoder_finish(encoder)||atracodec_decoder_finish(e.decoder))goto failed;
    if(e.mismatch||e.decoded!=frames)goto failed;
    atracodec_encoder_destroy(encoder);atracodec_decoder_destroy(e.decoder);
    printf("%s: streaming lossless roundtrip OK, %llu frames, %llu OMA bytes\n",atracodec_version(),(unsigned long long)e.decoded,(unsigned long long)e.oma_bytes);return 0;
failed:
    fprintf(stderr,"Streaming failure: %s\n",atracodec_stream_last_error());
    atracodec_encoder_destroy(encoder);atracodec_decoder_destroy(e.decoder);return 1;
}
