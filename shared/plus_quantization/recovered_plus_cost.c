#include "recovered_plus_cost.h"
#include "../plus_fields/independent_plus_spectrum.h"
int32_t sony_recovered_plus_packed_cost(uint32_t index,const int16_t *symbols,uint32_t n){
    const plus_spectrum_descriptor *d=plus_spectrum_book(index);
    if(!d||!symbols||n>128||n%(4u*d->coefficient_group))return -1;
    if(d->symbol_group!=1&&d->symbol_group!=2&&d->symbol_group!=4)return -1;
    const uint32_t count=n/d->coefficient_group,group=d->symbol_group;
    int32_t bits=0;
    for(uint32_t first=0;first<count;first+=group){
        uint32_t present=0;
        for(uint32_t j=0;j<group;++j){
            const uint32_t symbol=(uint16_t)symbols[first+j];
            if(symbol>=d->row_count)return -1;present|=symbol;
        }
        if(group>1){++bits;if(!present)continue;}
        for(uint32_t j=0;j<group;++j){
            const int32_t width=plus_spectrum_symbol_width(index,(uint16_t)symbols[first+j]);
            if(width<0)return -1;bits+=width;
        }
    }
    return bits;
}
