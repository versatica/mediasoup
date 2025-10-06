#ifndef MS_DEP_LIBSRTP_HPP
#define MS_DEP_LIBSRTP_HPP

#include <srtp.h>
#include <string>
#include <unordered_map>

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
	static std::unordered_map<srtp_err_status_t, std::string> mapErrorCodeString;
};

#endif
