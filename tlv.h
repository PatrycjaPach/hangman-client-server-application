#ifndef TLV_H
#define TLV_H

#include <stdint.h>
#include "protocol.h"


int sendtlv(int desc2, uint16_t type, const void *data, uint16_t len);

int recv_tlv(int desc2, uint16_t *type, void *buf, uint16_t bufsize);

#endif