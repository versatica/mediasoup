#ifndef MS_DEP_OPENSSL_H
#define	MS_DEP_OPENSSL_H

#include "common.h"
#include <openssl/ssl.h>
#include <openssl/crypto.h>

/* OpenSSL doc: struct CRYPTO_dynlock_value has to be defined by the application. */
struct CRYPTO_dynlock_value
{
 	pthread_mutex_t mutex;
};

class DepOpenSSL
{
public:
	static void ClassInit();
	static void ClassDestroy();

private:
	static void SetThreadId(CRYPTO_THREADID* id);
	static void LockingFunction(int mode, int n, const char *file, int line);
	static CRYPTO_dynlock_value* DynCreateFunction(const char* file, int line);
	static void DynLockFunction(int mode, CRYPTO_dynlock_value* v, const char* file, int line);
	static void DynDestroyFunction(CRYPTO_dynlock_value* v, const char* file, int line);

private:
	static pthread_mutex_t* mutexes;
	static uint32_t numMutexes;
};

#endif
