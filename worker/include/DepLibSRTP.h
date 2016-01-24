#ifndef MS_DEP_LIBSRTP_H
#define	MS_DEP_LIBSRTP_H

#include <vector>
#include <srtp2/srtp.h>

class DepLibSRTP
{
public:
	static void ClassInit();
	static void ClassDestroy();
	static bool IsError(srtp_err_status_t code);
	static const char* GetErrorString(srtp_err_status_t code);

private:
	static std::vector<const char*> errors;
};

/* Inline static methods. */

inline
bool DepLibSRTP::IsError(srtp_err_status_t code)
{
	return (code != srtp_err_status_ok);
}

inline
const char* DepLibSRTP::GetErrorString(srtp_err_status_t code)
{
	// This throws out_of_range if the given index is not in the vector.
	return DepLibSRTP::errors.at(code);
}

#endif
