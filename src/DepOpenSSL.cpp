#define MS_CLASS "DepOpenSSL"

#include "DepOpenSSL.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <openssl/err.h>
#include <openssl/engine.h>  // ENGINE_cleanup()

/* Static attributes. */

// This array will store all of the mutex available for OpenSSL.
pthread_mutex_t* DepOpenSSL::mutexes = nullptr;
int DepOpenSSL::numMutexes = 0;

/* Static methods. */

void DepOpenSSL::ClassInit()
{
	MS_TRACE();

	MS_DEBUG("loaded openssl version: %s", SSLeay_version(SSLEAY_VERSION));

	// First initialize OpenSSL stuff.
	SSL_load_error_strings();
	SSL_library_init();
	RAND_poll();

	// Make OpenSSL thread-safe.
	DepOpenSSL::mutexes = new pthread_mutex_t[CRYPTO_num_locks()];
	if (!DepOpenSSL::mutexes)
		MS_THROW_ERROR("allocation of mutexes failed");

	DepOpenSSL::numMutexes = CRYPTO_num_locks();

	for (int i = 0; i < DepOpenSSL::numMutexes; i++)
	{
		int err = pthread_mutex_init(&DepOpenSSL::mutexes[i], nullptr);
		if (err)
			MS_THROW_ERROR("pthread_mutex_init() failed with return code %d\n", err);
	}

	CRYPTO_THREADID_set_callback(DepOpenSSL::SetThreadId);
	CRYPTO_set_locking_callback(DepOpenSSL::LockingFunction);
	CRYPTO_set_dynlock_create_callback(DepOpenSSL::DynCreateFunction);
	CRYPTO_set_dynlock_lock_callback(DepOpenSSL::DynLockFunction);
	CRYPTO_set_dynlock_destroy_callback(DepOpenSSL::DynDestroyFunction);
}

void DepOpenSSL::ClassDestroy()
{
	MS_TRACE();

	// FAQ: https://www.openssl.org/support/faq.html#PROG13

	// Thread-local cleanup functions.
	ERR_remove_thread_state(nullptr);

	// Application-global cleanup functions that are aware of usage (and
	// therefore thread-safe).
	ENGINE_cleanup();

	// "Brutal" (thread-unsafe) Application-global cleanup functions.
	ERR_free_strings();
	EVP_cleanup();  // Removes all ciphers and digests.
	CRYPTO_cleanup_all_ex_data();

	// https://bugs.launchpad.net/percona-server/+bug/1341067.
	sk_SSL_COMP_free(SSL_COMP_get_compression_methods());

	// Free mutexes.
	for (int i = 0; i < DepOpenSSL::numMutexes; i++)
	{
		int err = pthread_mutex_destroy(&DepOpenSSL::mutexes[i]);
		if (err)
			MS_ERROR("pthread_mutex_destroy() failed with return code %d\n", err);
	}
	if (DepOpenSSL::mutexes)
		delete[] DepOpenSSL::mutexes;

	// Reset callbacks.
	CRYPTO_THREADID_set_callback(nullptr);
	CRYPTO_set_locking_callback(nullptr);
	CRYPTO_set_dynlock_create_callback(nullptr);
	CRYPTO_set_dynlock_lock_callback(nullptr);
	CRYPTO_set_dynlock_destroy_callback(nullptr);
}

void DepOpenSSL::SetThreadId(CRYPTO_THREADID* id)
{
	// MS_TRACE();

	CRYPTO_THREADID_set_numeric(id, (unsigned long)pthread_self());
}

void DepOpenSSL::LockingFunction(int mode, int n, const char *file, int line)
{
	// MS_TRACE();

	/**
	 * - mode:  bitfield describing what should be done with the lock:
	 *     CRYPTO_LOCK     0x01
	 *     CRYPTO_UNLOCK   0x02
	 *     CRYPTO_READ     0x04
	 *     CRYPTO_WRITE    0x08
	 * - n:  the number of the mutex.
	 * - file:  source file calling this function.
	 * - line:  line in the source file calling this function.
	 */

	// MS_DEBUG("[mode: %s+%s, mutex id: %d, file: %s, line: %d]", mode & CRYPTO_LOCK ? "LOCK" : "UNLOCK", mode & CRYPTO_READ ? "READ" : "WRITE", n, file, line);

	if (mode & CRYPTO_LOCK)
		pthread_mutex_lock(&DepOpenSSL::mutexes[n]);
	else
		pthread_mutex_unlock(&DepOpenSSL::mutexes[n]);
}

CRYPTO_dynlock_value* DepOpenSSL::DynCreateFunction(const char* file, int line)
{
	// MS_TRACE();

	CRYPTO_dynlock_value* value = new CRYPTO_dynlock_value;
	if (!value)
	{
		MS_ABORT("new CRYPTO_dynlock_value failed");
		return nullptr;
	}

	pthread_mutex_init(&value->mutex, nullptr);
	return value;
}

void DepOpenSSL::DynLockFunction(int mode, CRYPTO_dynlock_value* v, const char* file, int line)
{
	// MS_TRACE();

	if (mode & CRYPTO_LOCK)
		pthread_mutex_lock(&v->mutex);
	else
		pthread_mutex_unlock(&v->mutex);
}

void DepOpenSSL::DynDestroyFunction(CRYPTO_dynlock_value* v, const char* file, int line)
{
	// MS_TRACE();

	pthread_mutex_destroy(&v->mutex);
	delete v;
}
