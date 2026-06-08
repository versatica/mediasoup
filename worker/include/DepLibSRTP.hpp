#ifndef MS_DEP_LIBSRTP_HPP
#define MS_DEP_LIBSRTP_HPP

#include <srtp.h>
#include <ankerl/unordered_dense.h>
#include <string>

class DepLibSRTP
{
public:
	static void ClassInit();

	static void ClassDestroy();

	static bool IsError(srtp_err_status_t code)
	{
		return (code != srtp_err_status_ok);
	}

	static const std::string& GetErrorString(srtp_err_status_t code);

private:
	static const ankerl::unordered_dense::map<srtp_err_status_t, std::string> ErrorCode2String;
};

#endif
