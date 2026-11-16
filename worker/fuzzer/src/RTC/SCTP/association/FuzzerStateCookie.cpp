#include "fuzzer/include/RTC/SCTP/FuzzerStateCookie.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include "Utils.hpp"
#include <cstring> // std::memcpy()

void FuzzerRtcSctpStateCookie::Fuzz(const uint8_t* data, size_t len)
{
	alignas(4) static thread_local uint8_t dataBuffer[65536];
	alignas(4) static thread_local uint8_t serializeBuffer[65536];
	alignas(4) static thread_local uint8_t cloneBuffer[65536];

	if (len > sizeof(dataBuffer))
	{
		return;
	}

	// NOTE: We need to copy given data into another buffer because we are gonna
	// write into it.
	std::memcpy(dataBuffer, data, len);

	// We need to force `data` to be a StateCookie since it's too hard that
	// random data matches it.
	if (len > RTC::SCTP::StateCookie::StateCookieLength)
	{
		// NOTE: Never beyond the given data, so a crash is reproducible with it.
		len = Utils::Crypto::GetRandomUInt<size_t>(
		  RTC::SCTP::StateCookie::StateCookieLength,
		  std::min(len, RTC::SCTP::StateCookie::StateCookieLength + 10));

		if (len < RTC::SCTP::StateCookie::StateCookieLength + 5)
		{
			Utils::Byte::Set8Bytes(dataBuffer, 0, RTC::SCTP::StateCookie::Magic1);
			Utils::Byte::Set2Bytes(
			  dataBuffer, RTC::SCTP::StateCookie::RemoteCapabilitiesOffset, RTC::SCTP::StateCookie::Magic2);
		}
	}

	std::unique_ptr<RTC::SCTP::StateCookie> stateCookie{ RTC::SCTP::StateCookie::Parse(dataBuffer, len) };

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

	stateCookie->Serialize(serializeBuffer, len);

	stateCookie->GetLocalVerificationTag();
	stateCookie->GetRemoteVerificationTag();
	stateCookie->GetLocalInitialTsn();
	stateCookie->GetRemoteInitialTsn();
	stateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	stateCookie->GetTieTag();
	stateCookie->GetRemoteCapabilities();

	const std::unique_ptr<RTC::SCTP::StateCookie> clonedStateCookie{ stateCookie->Clone(
		cloneBuffer, len) };

	stateCookie.reset();

	clonedStateCookie->GetLocalVerificationTag();
	clonedStateCookie->GetRemoteVerificationTag();
	clonedStateCookie->GetLocalInitialTsn();
	clonedStateCookie->GetRemoteInitialTsn();
	clonedStateCookie->GetRemoteAdvertisedReceiverWindowCredit();
	clonedStateCookie->GetTieTag();
	clonedStateCookie->GetRemoteCapabilities();

	clonedStateCookie->Serialize(serializeBuffer, len);
}
