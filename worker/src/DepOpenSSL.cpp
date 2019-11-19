#define MS_CLASS "DepOpenSSL"
// #define MS_LOG_DEV_LEVEL 3

#include "DepOpenSSL.hpp"
#include "Logger.hpp"
#include <openssl/crypto.h>
#include <openssl/rand.h>

/* Static methods. */

void DepOpenSSL::ClassInit()
{
	MS_TRACE();

	MS_DEBUG_TAG(info, "openssl version: \"%s\"", OpenSSL_version(OPENSSL_VERSION));

	// Initialize some crypto stuff.
	RAND_poll();
}
