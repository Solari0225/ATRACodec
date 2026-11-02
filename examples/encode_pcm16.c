/* Minimal public C API caller. Input is raw signed16 little-endian stereo44100. */
#include "atracodec.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main(int argc,char **argv){
    if(argc!=3){fprintf(stderr,"Usage: atracodec-c-example INPUT.pcm OUTPUT.oma\n");return 2;}
    FILE *in=fopen(argv[1],"rb");if(!in){perror("Input");return 1;}
    if(fseek(in,0,SEEK_END)){fclose(in);return 1;}
    const long length=ftell(in);
    if(length<=0||length%4||(uint64_t)length/4>UINT32_MAX||fseek(in,0,SEEK_SET)){fclose(in);return 1;}
    const uint32_t frames=(uint32_t)((uint64_t)length/4);
    int16_t *pcm=(int16_t*)malloc((size_t)length);if(!pcm){fclose(in);return 1;}
    for(size_t i=0;i<(size_t)frames*2;++i){
        const int lo=fgetc(in),hi=fgetc(in);
        if(lo==EOF||hi==EOF){free(pcm);fclose(in);return 1;}
        pcm[i]=(int16_t)((uint32_t)lo|((uint32_t)hi<<8));
    }
    fclose(in);uint8_t *encoded=NULL;size_t bytes=0;
    const int status=atracodec_encode_pcm16(pcm,frames,
        ATRACODEC_AALP,352000,&encoded,&bytes);
    free(pcm);
    if(status){fprintf(stderr,"%s\n",atracodec_last_error());return 1;}
    FILE *out=fopen(argv[2],"wb");
    if(!out){atracodec_free(encoded);perror("Output");return 1;}
    const int written=fwrite(encoded,1,bytes,out)==bytes;
    const int closed=fclose(out)==0;
    atracodec_free(encoded);
    if(!written||!closed)return 1;
    printf("%zu bytes\n",bytes);return 0;
}
