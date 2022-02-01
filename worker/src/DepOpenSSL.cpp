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
	MS_TRACE_STD();

	std::call_once(globalInitOnce, [] {
		MS_DEBUG_TAG(info, "openssl version: \"%s\"", OpenSSL_version(OPENSSL_VERSION));

		// Initialize some crypto stuff.
		RAND_poll();
	});
}


void DepOpenSSL::DetectAESNI()
{
	MS_TRACE_STD();

	MS_DEBUG_TAG_STD(info, "Intel CPU: %s AES-NI: %s", DepOpenSSL::HasIntelCpu() ? "true" : "false", DepOpenSSL::HasAESNI() ? "true" : "false");
}
