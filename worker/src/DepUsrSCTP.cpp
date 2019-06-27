#define MS_CLASS "DepUsrSCTP"
// #define MS_LOG_DEV

#include "DepUsrSCTP.hpp"
#include "Logger.hpp"
#include "RTC/SctpAssociation.hpp"
#include <usrsctp.h>

/* Static. */

static constexpr size_t CheckerInterval{ 10 }; // In ms.

/* Static methods for usrsctp global callbacks. */

inline static int onSendSctpData(void* addr, void* buffer, size_t len, uint8_t tos, uint8_t setDf)
{
	// TODO: Ensure that this is feasible and we can associate a specific RTC::SctpAssociation
	// into the addr argument somehow.
	auto* sctpAssociation = static_cast<RTC::SctpAssociation*>(addr);

	if (sctpAssociation == nullptr)
		return -1;

	sctpAssociation->OnUsrSctpSendSctpData(buffer, len);

	return 0;
}

/* Static variables. */

DepUsrSCTP::Checker* DepUsrSCTP::checker{ nullptr };
uint64_t DepUsrSCTP::numSctpAssociations{ 0 };

/* Static methods. */

void DepUsrSCTP::ClassInit()
{
	MS_TRACE();

	MS_DEBUG_TAG(info, "usrsctp");

	usrsctp_init_nothreads(0, onSendSctpData, nullptr);

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

	MS_DEBUG_TAG(sctp, "usrsctp periodic checker started");

	this->timer->Start(CheckerInterval, CheckerInterval);
}

void DepUsrSCTP::Checker::Stop()
{
	MS_TRACE();

	MS_DEBUG_TAG(sctp, "usrsctp periodic checker stopped");

	this->timer->Stop();
}

void DepUsrSCTP::Checker::OnTimer(Timer* timer)
{
	MS_TRACE();

	// TODO: Call the usrctp special function to blablabla.
}
