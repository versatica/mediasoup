#define MS_CLASS "DepUsrSCTP"
#define MS_LOG_DEV_LEVEL 3

#include "DepUsrSCTP.hpp"
#ifdef MS_LIBURING_SUPPORTED
#include "DepLibUring.hpp"
#endif
#include "DepLibUV.hpp"
#include "Logger.hpp"
#include <usrsctp.h>
#include <cstdio>  // std::vsnprintf()
#include <cstring> // std::memcpy()
#include <mutex>

/* Static. */

static constexpr size_t CheckerInterval{ 10u }; // In ms.
static std::mutex GlobalSyncMutex;
static size_t GlobalInstances{ 0u };

/* Static methods for UV callbacks. */

inline static void onAsync(uv_async_t* handle)
{
	MS_TRACE();
	MS_DUMP_STD("---------- onAsync!!");

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	// Get the sending data from the map.
	auto* store = DepUsrSCTP::GetSendSctpDataStore(handle);

	if (!store)
	{
		MS_WARN_DEV("store not found");

		return;
	}

	auto* sctpAssociation = store->sctpAssociation;
	auto* data            = store->data;
	auto len              = store->len;

	MS_DUMP_STD("---------- onAsync, sending SCTP data!!");

	sctpAssociation->OnUsrSctpSendSctpData(data, len);

	// Must delete the mem copied data once sent.
	delete[] store->data;
}

/* Static methods for usrsctp global callbacks. */

inline static int onSendSctpData(void* addr, void* data, size_t len, uint8_t /*tos*/, uint8_t /*setDf*/)
{
	MS_TRACE();

	auto* sctpAssociation = DepUsrSCTP::RetrieveSctpAssociation(reinterpret_cast<uintptr_t>(addr));

	if (!sctpAssociation)
	{
		MS_WARN_TAG(sctp, "no SctpAssociation found");

		return -1;
	}

	DepUsrSCTP::SendSctpData(sctpAssociation, static_cast<uint8_t*>(data), len);

	// NOTE: Must not free data, usrsctp lib does it.

	return 0;
}

// Static method for printing usrsctp debug.
inline static void sctpDebug(const char* format, ...)
{
	char buffer[10000];
	va_list ap;

	va_start(ap, format);
	vsnprintf(buffer, sizeof(buffer), format, ap);

	// Remove the artificial carriage return set by usrsctp.
	buffer[std::strlen(buffer) - 1] = '\0';

	MS_DEBUG_TAG(sctp, "%s", buffer);

	va_end(ap);
}

/* Static variables. */

thread_local DepUsrSCTP::Checker* DepUsrSCTP::checker{ nullptr };
uint64_t DepUsrSCTP::numSctpAssociations{ 0u };
uintptr_t DepUsrSCTP::nextSctpAssociationId{ 0u };
absl::flat_hash_map<uintptr_t, RTC::SctpAssociation*> DepUsrSCTP::mapIdSctpAssociation;
absl::flat_hash_map<const uv_async_t*, DepUsrSCTP::SendSctpDataStore> DepUsrSCTP::mapAsyncHandlerSendSctpData;

/* Static methods. */

void DepUsrSCTP::ClassInit()
{
	MS_TRACE();

	MS_DUMP_STD("---------- DepUsrSCTP::ClassInit()");
	MS_DEBUG_TAG(info, "usrsctp");

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	if (GlobalInstances == 0)
	{
		usrsctp_init_nothreads(0, onSendSctpData, sctpDebug);

		// Disable explicit congestion notifications (ecn).
		usrsctp_sysctl_set_sctp_ecn_enable(0);

#ifdef SCTP_DEBUG
		usrsctp_sysctl_set_sctp_debug_on(SCTP_DEBUG_ALL);
#endif
	}

	++GlobalInstances;
}

void DepUsrSCTP::ClassDestroy()
{
	MS_TRACE();

	MS_DUMP_STD("---------- DepUsrSCTP::ClassDestroy()");

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	--GlobalInstances;

	if (GlobalInstances == 0)
	{
		usrsctp_finish();

		numSctpAssociations   = 0u;
		nextSctpAssociationId = 0u;

		DepUsrSCTP::mapIdSctpAssociation.clear();
		DepUsrSCTP::mapAsyncHandlerSendSctpData.clear();
	}
}

void DepUsrSCTP::CreateChecker()
{
	MS_TRACE();

	MS_ASSERT(DepUsrSCTP::checker == nullptr, "Checker already created");

	DepUsrSCTP::checker = new DepUsrSCTP::Checker();
}

void DepUsrSCTP::CloseChecker()
{
	MS_TRACE();

	MS_ASSERT(DepUsrSCTP::checker != nullptr, "Checker not created");

	delete DepUsrSCTP::checker;
}

uintptr_t DepUsrSCTP::GetNextSctpAssociationId()
{
	MS_TRACE();

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	// NOTE: usrsctp_connect() fails with a value of 0.
	if (DepUsrSCTP::nextSctpAssociationId == 0u)
	{
		++DepUsrSCTP::nextSctpAssociationId;
	}

	// In case we've wrapped around and need to find an empty spot from a removed
	// SctpAssociation. Assumes we'll never be full.
	while (DepUsrSCTP::mapIdSctpAssociation.find(DepUsrSCTP::nextSctpAssociationId) !=
	       DepUsrSCTP::mapIdSctpAssociation.end())
	{
		++DepUsrSCTP::nextSctpAssociationId;

		if (DepUsrSCTP::nextSctpAssociationId == 0u)
		{
			++DepUsrSCTP::nextSctpAssociationId;
		}
	}

	return DepUsrSCTP::nextSctpAssociationId++;
}

void DepUsrSCTP::RegisterSctpAssociation(RTC::SctpAssociation* sctpAssociation)
{
	MS_TRACE();

	MS_DUMP_STD("------ DepUsrSCTP::RegisterSctpAssociation()");

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	MS_ASSERT(DepUsrSCTP::checker != nullptr, "Checker not created");

	auto it  = DepUsrSCTP::mapIdSctpAssociation.find(sctpAssociation->id);
	auto it2 = DepUsrSCTP::mapAsyncHandlerSendSctpData.find(sctpAssociation->GetAsyncHandle());

	MS_ASSERT(
	  it == DepUsrSCTP::mapIdSctpAssociation.end(),
	  "the id of the SctpAssociation is already in the mapIdSctpAssociation map");
	MS_ASSERT(
	  it2 == DepUsrSCTP::mapAsyncHandlerSendSctpData.end(),
	  "the id of the SctpAssociation is already in the mapAsyncHandlerSendSctpData map");

	DepUsrSCTP::mapIdSctpAssociation[sctpAssociation->id] = sctpAssociation;
	DepUsrSCTP::mapAsyncHandlerSendSctpData[sctpAssociation->GetAsyncHandle()] = { sctpAssociation };

	sctpAssociation->InitializeSyncHandle(onAsync);

	if (++DepUsrSCTP::numSctpAssociations == 1u)
	{
		DepUsrSCTP::checker->Start();
	}
}

void DepUsrSCTP::DeregisterSctpAssociation(RTC::SctpAssociation* sctpAssociation)
{
	MS_TRACE();

	MS_DUMP_STD("------ DepUsrSCTP::DeregisterSctpAssociation() !!!!!!!!");

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	MS_ASSERT(DepUsrSCTP::checker != nullptr, "Checker not created");

	auto found1 = DepUsrSCTP::mapIdSctpAssociation.erase(sctpAssociation->id);
	auto found2 = DepUsrSCTP::mapAsyncHandlerSendSctpData.erase(sctpAssociation->GetAsyncHandle());

	MS_ASSERT(found1 > 0, "SctpAssociation not found in mapIdSctpAssociation map");
	MS_ASSERT(found2 > 0, "SctpAssociation not found in mapAsyncHandlerSendSctpData map");
	MS_ASSERT(DepUsrSCTP::numSctpAssociations > 0u, "numSctpAssociations was not higher than 0");

	if (--DepUsrSCTP::numSctpAssociations == 0u)
	{
		DepUsrSCTP::checker->Stop();
	}
}

RTC::SctpAssociation* DepUsrSCTP::RetrieveSctpAssociation(uintptr_t id)
{
	MS_TRACE();

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	auto it = DepUsrSCTP::mapIdSctpAssociation.find(id);

	if (it == DepUsrSCTP::mapIdSctpAssociation.end())
	{
		return nullptr;
	}

	return it->second;
}

void DepUsrSCTP::SendSctpData(RTC::SctpAssociation* sctpAssociation, uint8_t* data, size_t len)
{
	MS_TRACE();

	const std::lock_guard<std::mutex> lock(GlobalSyncMutex);

	// Store the sending data into the map.

	auto it = DepUsrSCTP::mapAsyncHandlerSendSctpData.find(sctpAssociation->GetAsyncHandle());

	MS_ASSERT(
	  it != DepUsrSCTP::mapAsyncHandlerSendSctpData.end(),
	  "SctpAssociation not found in mapAsyncHandlerSendSctpData map");

	SendSctpDataStore& store = it->second;

	// NOTE: In Rust, DepUsrSCTP::SendSctpData() is called from onSendSctpData()
	// callback from a different thread and usrsctp immediately frees |data| when
	// the callback execution finishes. So we have to mem copy it.
	// TODO: This must be freed, but I'd prefer if we used a static thread_local
	// buffer, but I don't know max size of this (if any).
	store.data            = new uint8_t[len];
	store.len             = len;

	std::memcpy(store.data, data, len);

	// Invoke UV async send.
	int err = uv_async_send(sctpAssociation->GetAsyncHandle());

	if (err != 0)
	{
		MS_WARN_TAG(sctp, "uv_async_send() failed: %s", uv_strerror(err));
	}
}

DepUsrSCTP::SendSctpDataStore* DepUsrSCTP::GetSendSctpDataStore(uv_async_t* handle)
{
	MS_TRACE();

	auto it = DepUsrSCTP::mapAsyncHandlerSendSctpData.find(handle);

	if (it == DepUsrSCTP::mapAsyncHandlerSendSctpData.end())
	{
		return nullptr;
	}

	SendSctpDataStore& store = it->second;

	return std::addressof(store);
}

/* DepUsrSCTP::Checker instance methods. */

DepUsrSCTP::Checker::Checker() : timer(new TimerHandle(this))
{
	MS_TRACE();
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

	this->lastCalledAtMs = 0u;

	this->timer->Start(CheckerInterval, CheckerInterval);
}

void DepUsrSCTP::Checker::Stop()
{
	MS_TRACE();

	MS_DEBUG_TAG(sctp, "usrsctp periodic check stopped");

	this->lastCalledAtMs = 0u;

	this->timer->Stop();
}

void DepUsrSCTP::Checker::OnTimer(TimerHandle* /*timer*/)
{
	MS_TRACE();

	auto nowMs          = DepLibUV::GetTimeMs();
	const int elapsedMs = this->lastCalledAtMs ? static_cast<int>(nowMs - this->lastCalledAtMs) : 0;

#ifdef MS_LIBURING_SUPPORTED
	// Activate liburing usage.
	// 'usrsctp_handle_timers()' will synchronously call the send/recv
	// callbacks for the pending data. If there are multiple messages to be
	// sent over the network then we will send those messages within a single
	// system call.
	DepLibUring::SetActive();
#endif

	usrsctp_handle_timers(elapsedMs);

#ifdef MS_LIBURING_SUPPORTED
	// Submit all prepared submission entries.
	DepLibUring::Submit();
#endif

	this->lastCalledAtMs = nowMs;
}
