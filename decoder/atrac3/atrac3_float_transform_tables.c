#include "atrac3_float_transform.h"
static const atracodec_atrac3_float_transform_tables tables =
#include "atrac3_float_transform_tables.inc"
;
const atracodec_atrac3_float_transform_tables *
atracodec_atrac3_float_transform_default_tables(void)
{
    return &tables;
}
