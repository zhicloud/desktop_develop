#include <stdint.h>

#ifndef _H_TEST_MARSHALLERS

typedef struct {
    uint32_t data_size;
    uint64_t data[];
} SpiceMsgMainShortDataSubMarshall;

#endif /* _H_TEST_MARSHALLERS */

