#define MS_CLASS "DepLibSRTP"
// #define MS_LOG_DEV_LEVEL 3

#include "DepLibSRTP.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <mutex>

/* Static variables. */

static std::mutex GlobalSyncMutex;
static size_t GlobalInstances = 0;

// NOTE: This map must always be in sync with the srtp_err_status_t in srtp.h
// in libsrtp library:
//   https://github.com/cisco/libsrtp/blob/main/include/srtp.h
//
// clang-format off
std::unordered_map<srtp_err_status_t, std::string> DepLibSRTP::mapErrorCodeString =
{
	{ srtp_err_status_ok,            "nothing to report" },
	{ srtp_err_status_fail,          "unspecified failure" },
	{ srtp_err_status_bad_param,     "unsupported parameter" },
	{ srtp_err_status_alloc_fail,    "couldn't allocate memory" },
	{ srtp_err_status_dealloc_fail,  "couldn't deallocate properly" },
	{ srtp_err_status_init_fail,     "couldn't initialize" },
	{ srtp_err_status_terminus,      "can't process as much data as requested" },
	{ srtp_err_status_auth_fail,     "authentication failure" },
	{ srtp_err_status_cipher_fail,   "cipher failure" },
	{ srtp_err_status_replay_fail,   "replay check failed (bad index)" },
	{ srtp_err_status_replay_old,    "replay check failed (index too old)" },
	{ srtp_err_status_algo_fail,     "algorithm failed test routine" },
	{ srtp_err_status_no_such_op,    "unsupported operation" },
	{ srtp_err_status_no_ctx,        "no appropriate context found" },
	{ srtp_err_status_cant_check,    "unable to perform desired validation" },
	{ srtp_err_status_key_expired,   "can't use key any more" },
	{ srtp_err_status_socket_err,    "error in use of socket" },
	{ srtp_err_status_signal_err,    "error in use POSIX signals" },
	{ srtp_err_status_nonce_bad,     "nonce check failed" },
	{ srtp_err_status_read_fail,     "couldn't read data" },
	{ srtp_err_status_write_fail,    "couldn't write data" },
	{ srtp_err_status_parse_err,     "error parsing data" },
	{ srtp_err_status_encode_err,    "error encoding data" },
	{ srtp_err_status_semaphore_err, "error while using semaphores" },
	{ srtp_err_status_pfkey_err,     "error while using pfkey" },
	{ srtp_err_status_bad_mki,       "error MKI present in packet is invalid" },
	{ srtp_err_status_pkt_idx_old,   "packet index is too old to consider" },
	{ srtp_err_status_pkt_idx_adv,   "packet index advanced, reset needed" },
	{ srtp_err_status_buffer_small,  "out buffer is too small" },
	{ srtp_err_status_cryptex_err,   "unsupported cryptex operation" }
};
// clang-format on

/* Static methods. */

void DepLibSRTP::ClassInit()
{
	MS_TRACE();

	{
		const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

		if (GlobalInstances == 0)
		{
			MS_DEBUG_TAG(info, "libsrtp version: \"%s\"", srtp_get_version_string());

			const srtp_err_status_t err = srtp_init();

			if (DepLibSRTP::IsError(err))
			{
				MS_THROW_ERROR("srtp_init() failed: %s", DepLibSRTP::GetErrorString(err).c_str());
			}
		}

		++GlobalInstances;
	}
}

void DepLibSRTP::ClassDestroy()
{
	MS_TRACE();

	{
		const std::lock_guard<std::mutex> lock(GlobalSyncMutex);
		--GlobalInstances;

		if (GlobalInstances == 0)
		{
			srtp_shutdown();
		}
	}
}

const std::string& DepLibSRTP::GetErrorString(srtp_err_status_t code)
{
	MS_TRACE();

	static const std::string UnknownError("unknown libsrtp error");

	auto it = DepLibSRTP::mapErrorCodeString.find(code);

	if (it == DepLibSRTP::mapErrorCodeString.end())
	{
		return UnknownError;
	}

	return it->second;
}
