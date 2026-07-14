#include "RTC/SCTP/FuzzerStateCookie.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()

namespace
{
	alignas(4) thread_local uint8_t DataBuffer[65536];
	alignas(4) thread_local uint8_t StateCookieSerializeBuffer[65536];
	alignas(4) thread_local uint8_t StateCookieCloneBuffer[65536];
} // namespace

void FuzzerRtcSctpStateCookie::Fuzz(const uint8_t* data, size_t len)
{
	// NOTE: We need to copy given data into another buffer because we are gonna
	// write into it.
	std::memcpy(DataBuffer, data, len);

	// We need to force `data` to be a StateCookie since it's too hard that
	// random data matches it.
	if (len > RTC::SCTP::StateCookie::StateCookieLength)
	{
		len = Utils::Crypto::GetRandomUInt<size_t>(
		  RTC::SCTP::StateCookie::StateCookieLength, RTC::SCTP::StateCookie::StateCookieLength + 10);

		if (len < RTC::SCTP::StateCookie::StateCookieLength + 5)
		{
			Utils::Byte::Set8Bytes(DataBuffer, 0, RTC::SCTP::StateCookie::Magic1);
			Utils::Byte::Set2Bytes(
			  DataBuffer, RTC::SCTP::StateCookie::RemoteCapabilitiesOffset, RTC::SCTP::StateCookie::Magic2);
		}
	}

	RTC::SCTP::StateCookie* stateCookie = RTC::SCTP::StateCookie::Parse(DataBuffer, len);

	if (!stateCookie)
	{
		return;
	}

	stateCookie->GetLocalVerificationTag();
	stateCookie->GetRemoteVerificationTag();
	stateCookie->GetLocalInitialTsn();
	stateCookie->GetRemoteInitialTsn();
	stateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	stateCookie->GetTieTag();
	stateCookie->GetRemoteCapabilities();

	stateCookie->Serialize(StateCookieSerializeBuffer, len);

	stateCookie->GetLocalVerificationTag();
	stateCookie->GetRemoteVerificationTag();
	stateCookie->GetLocalInitialTsn();
	stateCookie->GetRemoteInitialTsn();
	stateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	stateCookie->GetTieTag();
	stateCookie->GetRemoteCapabilities();

	auto* clonedStateCookie = stateCookie->Clone(StateCookieCloneBuffer, len);

	delete stateCookie;

	clonedStateCookie->GetLocalVerificationTag();
	clonedStateCookie->GetRemoteVerificationTag();
	clonedStateCookie->GetLocalInitialTsn();
	clonedStateCookie->GetRemoteInitialTsn();
	clonedStateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	clonedStateCookie->GetTieTag();
	clonedStateCookie->GetRemoteCapabilities();

	clonedStateCookie->Serialize(StateCookieSerializeBuffer, len);

	delete clonedStateCookie;
}
