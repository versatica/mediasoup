#include "RTC/SCTP/association/FuzzerStateCookie.hpp"
#include "Utils.hpp"
#include "RTC/SCTP/association/StateCookie.hpp"
#include <cstdlib> // std::malloc(), std::free()
#include <cstring> // std::memcpy()

thread_local static uint8_t StateCookieSerializeBuffer[65536];
thread_local static uint8_t StateCookieCloneBuffer[65536];

void Fuzzer::RTC::SCTP::StateCookie::Fuzz(const uint8_t* data, size_t len)
{
	auto* clonedData = static_cast<uint8_t*>(std::malloc(len));

	std::memcpy(clonedData, data, len);

	// We need to force `data` to be a StateCookie since it's too hard that random
	// data matches it.
	if (len > ::RTC::SCTP::StateCookie::StateCookieLength)
	{
		len = ::Utils::Crypto::GetRandomUInt(
		  ::RTC::SCTP::StateCookie::StateCookieLength, ::RTC::SCTP::StateCookie::StateCookieLength + 10);

		if (len < ::RTC::SCTP::StateCookie::StateCookieLength + 5)
		{
			::Utils::Byte::Set4Bytes(clonedData, 0, ::RTC::SCTP::StateCookie::MagicValue1);
			::Utils::Byte::Set2Bytes(clonedData, 34, ::RTC::SCTP::StateCookie::MagicValue2);
		}
	}

	::RTC::SCTP::StateCookie* stateCookie = ::RTC::SCTP::StateCookie::Parse(clonedData, len);

	if (!stateCookie)
	{
		std::free(clonedData);

		return;
	}

	stateCookie->GetMyVerificationTag();
	stateCookie->GetPeerVerificationTag();
	stateCookie->GetMyInitialTsn();
	stateCookie->GetPeerInitialTsn();
	stateCookie->GetMyAdvertisedReceiverWindowCredit();
	stateCookie->GetTieTag();
	stateCookie->GetNegotiatedCapabilities();

	stateCookie->Serialize(StateCookieSerializeBuffer, len);

	stateCookie->GetMyVerificationTag();
	stateCookie->GetPeerVerificationTag();
	stateCookie->GetMyInitialTsn();
	stateCookie->GetPeerInitialTsn();
	stateCookie->GetMyAdvertisedReceiverWindowCredit();
	stateCookie->GetTieTag();
	stateCookie->GetNegotiatedCapabilities();

	auto* clonedStateCookie = stateCookie->Clone(StateCookieCloneBuffer, len);

	delete stateCookie;

	clonedStateCookie->GetMyVerificationTag();
	clonedStateCookie->GetPeerVerificationTag();
	clonedStateCookie->GetMyInitialTsn();
	clonedStateCookie->GetPeerInitialTsn();
	clonedStateCookie->GetMyAdvertisedReceiverWindowCredit();
	clonedStateCookie->GetTieTag();
	clonedStateCookie->GetNegotiatedCapabilities();

	clonedStateCookie->Serialize(StateCookieSerializeBuffer, len);

	std::free(clonedData);
	delete clonedStateCookie;
}
