#ifndef MS_DEP_USRSCTP_HPP
#define MS_DEP_USRSCTP_HPP

#include "common.hpp"
#include "RTC/SctpAssociation.hpp"
#include "handles/Timer.hpp"
#include <unordered_map>

class DepUsrSCTP
{
private:
	class Checker : public Timer::Listener
	{
	public:
		Checker();
		~Checker() override;

	public:
		void Start();
		void Stop();

		/* Pure virtual methods inherited from Timer::Listener. */
	public:
		void OnTimer(Timer* timer) override;

	private:
		Timer* timer{ nullptr };
		uint64_t lastCalledAtMs{ 0u };
	};

public:
	static void ClassInit();
	static void ClassDestroy();
	static uintptr_t GetNextSctpAssociationId();
	static void RegisterSctpAssociation(RTC::SctpAssociation* sctpAssociation);
	static void DeregisterSctpAssociation(RTC::SctpAssociation* sctpAssociation);
	static RTC::SctpAssociation* RetrieveSctpAssociation(uintptr_t id);

private:
	static Checker* checker;
	static uint64_t numSctpAssociations;
	static uintptr_t nextSctpAssociationId;
	static std::unordered_map<uintptr_t, RTC::SctpAssociation*> mapIdSctpAssociation;
};

#endif
