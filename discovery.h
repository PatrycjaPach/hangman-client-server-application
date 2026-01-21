#ifndef DISCOVERY_H
#define DISCOVERY_H

#include <netinet/in.h>


int discover_server(struct sockaddr_in6 *servaddr);

#endif