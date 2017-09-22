#ifndef __common_h__
#define __common_h__

#include <stdint.h>

enum class Type {
    SUBSCRIBE,
    PUBLISH,
    PUBLISH_ACK,
    MESSAGE
};
enum class Code {
    SUCCESS,
    ERROR,
    NO_CONSUMER
};

struct MqHead {
    uint32_t        bodySize;
    int             version;
};

struct MqBody {
    Type            type;
    Code            retcode;
    char            topic[64];
    char            content[0];
};

#endif
