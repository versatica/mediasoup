#ifndef MS_DEP_LIBSRTP_H
#define	MS_DEP_LIBSRTP_H

#include <vector>
#include <srtp/srtp.h>

class DepLibSRTP
{
public:
	static void ClassInit();
	static void ClassDestroy();
	static bool IsError(err_status_t code);
	static const char* GetErrorString(err_status_t code);

private:
	static std::vector<const char*> errors;
};

/* Inline static methods. */

inline
bool DepLibSRTP::IsError(err_status_t code)
{
	return (code != err_status_ok);
}

inline
const char* DepLibSRTP::GetErrorString(err_status_t code)
{
	// This throws out_of_range if the given index is not in the vector.
	return DepLibSRTP::errors.at(code);
}

#endif
