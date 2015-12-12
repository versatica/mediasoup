#ifndef MS_COMMON_H
#define MS_COMMON_H

#include <cstddef>  // size_t
#include <cstdint>  // uint8_t, etc
#include <sys/socket.h>  // struct sockaddr, struct sockaddr_storage, AF_INET, AF_INET6
#include <netinet/in.h>  // sockaddr_in, sockaddr_in6
#include <arpa/inet.h>  // htonl(), htons(), ntohl(), ntohs()

#define MS_APP_NAME "mediasoup"
#define MS_PROCESS_NAME "mediasoup-worker"
#define MS_PROCESS_MIN_NAME "worker"

#define MS_LOG_LEVEL_DEBUG 3
#define MS_LOG_LEVEL_INFO  2
#define MS_LOG_LEVEL_WARN  1
#define MS_LOG_LEVEL_ERROR 0

#define MS_BYTE   uint8_t
#define MS_2BYTES uint16_t
#define MS_4BYTES uint32_t
#define MS_8BYTES uint64_t
#define MS_PORT   uint16_t

// Detect Little-Endian or Big-Endian CPU.
// NOTE: If the macro MS_LITTLE_ENDIAN or MS_BIG_ENDIAN is already defined
// then don't autodetect it.
#if !defined(MS_LITTLE_ENDIAN) && !defined(MS_BIG_ENDIAN)
	#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	    #define MS_LITTLE_ENDIAN
	#elif defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	    #define MS_BIG_ENDIAN
	#else
		#error "cannot determine whether the processor is Little-Endian or Big-Endian"
	#endif
#endif

#endif
