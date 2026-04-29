#ifndef MS_DEP_USRSCTP_HPP
#define MS_DEP_USRSCTP_HPP

#include "common.hpp"
#include "SharedInterface.hpp"
#include "RTC/SctpAssociation.hpp"
#include "handles/TimerHandleInterface.hpp"
#include <absl/container/flat_hash_map.h>

class DepUsrSCTP
{
private:
	class Checker : public TimerHandleInterface::Listener
	{
	public:
		Checker(SharedInterface* shared);
		~Checker() override;

	public:
		void Start();
		void Stop();

		/* Pure virtual methods inherited from TimerHandleInterface::Listener. */
	public:
		void OnTimer(TimerHandleInterface* timer) override;

	private:
		TimerHandleInterface* timer{ nullptr };
		uint64_t lastCalledAtMs{ 0u };
	};

public:
	static void ClassInit();
	static void ClassDestroy();
	static void CreateChecker(SharedInterface* shared);
	static void CloseChecker();
	static uintptr_t GetNextSctpAssociationId();
	static void RegisterSctpAssociation(RTC::SctpAssociation* sctpAssociation);
	static void DeregisterSctpAssociation(RTC::SctpAssociation* sctpAssociation);
	static RTC::SctpAssociation* RetrieveSctpAssociation(uintptr_t id);

private:
	thread_local static Checker* checker;
	static uint64_t numSctpAssociations;
	static uintptr_t nextSctpAssociationId;
	static absl::flat_hash_map<uintptr_t, RTC::SctpAssociation*> mapIdSctpAssociation;
};

#endif
