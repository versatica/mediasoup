#ifndef MS_COMMON_HPP
#define MS_COMMON_HPP

#include <memory> // std::addressof()
#include <algorithm> // std::transform(), std::find(), std::min(), std::max()
#include <cstddef> // size_t
#include <cstdint> // uint8_t, etc
#include <cinttypes> // PRIu64, etc
#include <sys/socket.h> // struct sockaddr, struct sockaddr_storage, AF_INET, AF_INET6
#include <netinet/in.h> // sockaddr_in, sockaddr_in6
#include <arpa/inet.h> // htonl(), htons(), ntohl(), ntohs()

#define MS_APP_NAME "mediasoup"
#define MS_PROCESS_NAME "mediasoup-worker"

#endif
