#define MS_CLASS "DepOpenSSL"
// #define MS_LOG_DEV_LEVEL 3

#include "DepOpenSSL.hpp"
#include "Logger.hpp"
#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <mutex>

/* Static. */

static std::once_flag globalInitOnce;

/* Static methods. */

void DepOpenSSL::ClassInit()
{
	MS_TRACE();

	std::call_once(
	  globalInitOnce,
	  []
	  {
		  MS_DEBUG_TAG(info, "openssl version: \"%s\"", OpenSSL_version(OPENSSL_VERSION));

		  // Initialize some crypto stuff.
		  RAND_poll();
	  });
}
