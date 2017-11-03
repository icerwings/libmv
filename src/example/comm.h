
#ifndef __common_h__
#define __common_h__

#include <stdint.h>
#include "protocol.h"

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

struct MqBody : public MsgBase {
    Type            type;
    Code            retcode;
    string          topic;
    string          content;

    CLASS_ENCODE(s << type << retcode << topic << content)
    CLASS_DECODE(s >> type >> retcode >> topic >> content)
};

#endif
