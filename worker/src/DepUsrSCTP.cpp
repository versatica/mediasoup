#define MS_CLASS "DepUsrSCTP"
// #define MS_LOG_DEV

#include "DepUsrSCTP.h"
#include "Logger.h"
// #include <usrsctp.h>

/* Static methods. */

void DepUsrSCTP::ClassInit()
{
	MS_TRACE();

	// Initialize usrsctp.
	// TODO: second argment must be a function to get outgoing SCTP data
	// (to be sent on top of DTLS)
	// usrsctp_init(0, nullptr, nullptr);
}

void DepUsrSCTP::ClassDestroy()
{
	MS_TRACE();

	// int err;

	// Free usrsctp.
	// err = usrsctp_finish();
	// if (err)
		// MS_ERROR("usrsctp_finish() failed");
}
