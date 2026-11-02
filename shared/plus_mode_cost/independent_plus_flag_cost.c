#include "independent_plus_flag_cost.h"
int32_t plus_mode_choose_flags(plus_given_flag_vector* vector,int32_t count){
    if(!vector||count>16||(count>0&&!vector->values))return PLUS_PACK_INVALID;
    uint32_t sum=0;
    for(int32_t i=0;i<count;i++)sum+=vector->values[i];
    if(!sum){vector->present=0;vector->explicit_values=0;return 1;}
    vector->present=1;vector->explicit_values=sum!=(uint32_t)count;
    return vector->explicit_values?count+2:2;
}
