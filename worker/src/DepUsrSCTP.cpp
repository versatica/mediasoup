#define MS_CLASS "DepUsrSCTP"
// #define MS_LOG_DEV

#include "DepUsrSCTP.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include "RTC/SctpAssociation.hpp"
#include <usrsctp.h>

/* Static. */

static constexpr size_t CheckerInterval{ 10 }; // In ms.

/* Static methods for usrsctp global callbacks. */

inline static int onSendSctpData(void* addr, void* data, size_t len, uint8_t /*tos*/, uint8_t /*setDf*/)
{
	auto* sctpAssociation = static_cast<RTC::SctpAssociation*>(addr);

	if (sctpAssociation == nullptr)
		return -1;

	sctpAssociation->OnUsrSctpSendSctpData(data, len);

	// NOTE: Must not free data, usrsctp lib does it.

	return 0;
}

// Static method for printing usrsctp debug.
inline static void sctpDebug(const char* format, ...)
{
	char buffer[10000];
	va_list ap;

	va_start(ap, format);
	vsprintf(buffer, format, ap);

	// Remove the artificial carriage return set by usrsctp.
	buffer[std::strlen(buffer) - 1] = '\0';

	MS_DEBUG_TAG(sctp, "%s", buffer);

	va_end(ap);
}

/* Static variables. */

DepUsrSCTP::Checker* DepUsrSCTP::checker{ nullptr };
uint64_t DepUsrSCTP::numSctpAssociations{ 0 };

/* Static methods. */

void DepUsrSCTP::ClassInit()
{
	MS_TRACE();

	MS_DEBUG_TAG(info, "usrsctp");

	usrsctp_init_nothreads(0, onSendSctpData, sctpDebug);

	// Disable explicit congestion notifications (ecn).
	usrsctp_sysctl_set_sctp_ecn_enable(0);

#ifdef SCTP_DEBUG
	usrsctp_sysctl_set_sctp_debug_on(SCTP_DEBUG_ALL);
#endif

	DepUsrSCTP::checker = new DepUsrSCTP::Checker();
}

void DepUsrSCTP::ClassDestroy()
{
	MS_TRACE();

	usrsctp_finish();

	delete DepUsrSCTP::checker;
}

void DepUsrSCTP::IncreaseSctpAssociations()
{
	MS_TRACE();

	if (++DepUsrSCTP::numSctpAssociations == 1)
		DepUsrSCTP::checker->Start();
}

void DepUsrSCTP::DecreaseSctpAssociations()
{
	MS_TRACE();

	MS_ASSERT(DepUsrSCTP::numSctpAssociations > 0, "numSctpAssociations was not higher than 0");

	if (--DepUsrSCTP::numSctpAssociations == 0)
		DepUsrSCTP::checker->Stop();
}

/* DepUsrSCTP::Checker instance methods. */

DepUsrSCTP::Checker::Checker()
{
	MS_TRACE();

	this->timer = new Timer(this);
}

DepUsrSCTP::Checker::~Checker()
{
	MS_TRACE();

	delete this->timer;
}

void DepUsrSCTP::Checker::Start()
{
	MS_TRACE();

	MS_DEBUG_TAG(sctp, "usrsctp periodic check started");

	this->lastCalledAt = 0;

	this->timer->Start(CheckerInterval, CheckerInterval);
}

void DepUsrSCTP::Checker::Stop()
{
	MS_TRACE();

	MS_DEBUG_TAG(sctp, "usrsctp periodic check stopped");

	this->lastCalledAt = 0;

	this->timer->Stop();
}

void DepUsrSCTP::Checker::OnTimer(Timer* /*timer*/)
{
	MS_TRACE();

	auto now  = DepLibUV::GetTime();
	int delta = this->lastCalledAt ? static_cast<int>(now - this->lastCalledAt) : 0;

	usrsctp_fire_timer(delta);

	this->lastCalledAt = now;
}
