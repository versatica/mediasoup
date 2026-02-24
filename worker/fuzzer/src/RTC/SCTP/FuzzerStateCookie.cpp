#include "RTC/SCTP/FuzzerStateCookie.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/StateCookie.hpp"
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memcpy()

thread_local uint8_t StateCookieSerializeBuffer[65536];
thread_local uint8_t StateCookieCloneBuffer[65536];

void FuzzerRtcSctpStateCookie::Fuzz(const uint8_t* data, size_t len)
{
	auto* clonedData = static_cast<uint8_t*>(std::malloc(len));

	std::memcpy(clonedData, data, len);

	// We need to force `data` to be a StateCookie since it's too hard that
	// random data matches it.
	if (len > RTC::SCTP::StateCookie::StateCookieLength)
	{
		len = Utils::Crypto::GetRandomUInt<size_t>(
		  RTC::SCTP::StateCookie::StateCookieLength, RTC::SCTP::StateCookie::StateCookieLength + 10);

		if (len < RTC::SCTP::StateCookie::StateCookieLength + 5)
		{
			Utils::Byte::Set8Bytes(clonedData, 0, RTC::SCTP::StateCookie::Magic1);
			Utils::Byte::Set2Bytes(
			  clonedData,
			  RTC::SCTP::StateCookie::NegotiatedCapabilitiesOffset,
			  RTC::SCTP::StateCookie::Magic2);
		}
	}

	RTC::SCTP::StateCookie* stateCookie = RTC::SCTP::StateCookie::Parse(clonedData, len);

	if (!stateCookie)
	{
		std::free(clonedData);

		return;
	}

	stateCookie->GetLocalVerificationTag();
	stateCookie->GetRemoteVerificationTag();
	stateCookie->GetLocalInitialTsn();
	stateCookie->GetRemoteInitialTsn();
	stateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	stateCookie->GetTieTag();
	stateCookie->GetNegotiatedCapabilities();

	stateCookie->Serialize(StateCookieSerializeBuffer, len);

	stateCookie->GetLocalVerificationTag();
	stateCookie->GetRemoteVerificationTag();
	stateCookie->GetLocalInitialTsn();
	stateCookie->GetRemoteInitialTsn();
	stateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	stateCookie->GetTieTag();
	stateCookie->GetNegotiatedCapabilities();

	auto* clonedStateCookie = stateCookie->Clone(StateCookieCloneBuffer, len);

	delete stateCookie;

	clonedStateCookie->GetLocalVerificationTag();
	clonedStateCookie->GetRemoteVerificationTag();
	clonedStateCookie->GetLocalInitialTsn();
	clonedStateCookie->GetRemoteInitialTsn();
	clonedStateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	clonedStateCookie->GetTieTag();
	clonedStateCookie->GetNegotiatedCapabilities();

	clonedStateCookie->Serialize(StateCookieSerializeBuffer, len);

	std::free(clonedData);
	delete clonedStateCookie;
}
