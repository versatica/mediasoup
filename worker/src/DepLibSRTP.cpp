#define MS_CLASS "DepLibSRTP"

#include "DepLibSRTP.h"
#include "MediaSoupError.h"
#include "Logger.h"

/* Static variables. */

std::vector<const char*> DepLibSRTP::errors =
{
	// From 0 (srtp_err_status_ok) to 24 (srtp_err_status_pfkey_err).
	"nothing to report (err_status_ok)",
	"unspecified failure (err_status_fail)",
	"unsupported parameter (err_status_bad_param)",
	"couldn't allocate memory (err_status_alloc_fail)",
	"couldn't deallocate memory (err_status_dealloc_fail)",
	"couldn't initialize (err_status_init_fail)",
	"can’t process as much data as requested (err_status_terminus)",
	"authentication failure (err_status_auth_fail)",
	"cipher failure (err_status_cipher_fail)",
	"replay check failed (bad index) (err_status_replay_fail)",
	"replay check failed (index too old) (err_status_replay_old)",
	"algorithm failed test routine (err_status_algo_fail)",
	"unsupported operation (err_status_no_such_op)",
	"no appropriate context found (err_status_no_ctx)",
	"unable to perform desired validation (err_status_cant_check)",
	"can’t use key any more (err_status_key_expired)",
	"error in use of socket (err_status_socket_err)",
	"error in use POSIX signals (err_status_signal_err)",
	"nonce check failed (err_status_nonce_bad)",
	"couldn’t read data (err_status_read_fail)",
	"couldn’t write data (err_status_write_fail)",
	"error parsing data (err_status_parse_err)",
	"error encoding data (err_status_encode_err)",
	"error while using semaphores (err_status_semaphore_err)",
	"error while using pfkey (err_status_pfkey_err)"
};

/* Static methods. */

void DepLibSRTP::ClassInit()
{
	MS_TRACE();

	srtp_err_status_t err;

	err = srtp_init();
	if (DepLibSRTP::IsError(err))
		MS_THROW_ERROR("srtp_init() failed: %s", DepLibSRTP::GetErrorString(err));
}

void DepLibSRTP::ClassDestroy()
{
	MS_TRACE();

	srtp_err_status_t err;

	err = srtp_shutdown();
	if (DepLibSRTP::IsError(err))
		MS_ERROR("srtp_shutdown() failed: %s", DepLibSRTP::GetErrorString(err));
}
