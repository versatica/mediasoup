#ifndef MS_DEP_USRSCTP_HPP
#define MS_DEP_USRSCTP_HPP

#include "common.hpp"
#include "handles/Timer.hpp"

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
	static void IncreaseSctpAssociations();
	static void DecreaseSctpAssociations();

private:
	static Checker* checker;
	static uint64_t numSctpAssociations;
};

#endif
