#ifndef MS_DEP_LIBSRTP_HPP
#define MS_DEP_LIBSRTP_HPP

#include <srtp.h>
#include <vector>

class DepLibSRTP
{
public:
	static void ClassInit();
	static void ClassDestroy();
	static bool IsError(srtp_err_status_t code)
	{
		return (code != srtp_err_status_ok);
	}
	static const char* GetErrorString(srtp_err_status_t code)
	{
		// This throws out_of_range if the given index is not in the vector.
		return DepLibSRTP::errors.at(code);
	}

private:
	static std::vector<const char*> errors;
};

#endif
