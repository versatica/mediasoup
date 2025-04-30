#define MS_CLASS "RTC::SCTP::Packet"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/StateCookie.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		StateCookie* StateCookie::Parse(const uint8_t* buffer, size_t bufferLength)
		{
			MS_TRACE();

			if (bufferLength != StateCookie::StateCookieLength)
			{
				MS_WARN_TAG(sctp, "StateCookie buffer length must be %zu", StateCookie::StateCookieLength);

				return nullptr;
			}

			if (Utils::Byte::Get4Bytes(buffer, 0) != StateCookie::MagicValue1)
			{
				MS_WARN_TAG(sctp, "incorrect Magic Value 1");

				return nullptr;
			}

			auto* negotiatedCapabilitiesField =
			  reinterpret_cast<NegotiatedCapabilitiesField*>(const_cast<uint8_t*>(buffer) + 32);

			if (uint16_t{ ntohs(negotiatedCapabilitiesField->magicValue2) } != StateCookie::MagicValue2)
			{
				MS_WARN_TAG(sctp, "incorrect Magic Value 2");

				return nullptr;
			}

			auto* stateCookie = new StateCookie(const_cast<uint8_t*>(buffer), bufferLength);

			// Mark the state cookie as frozen since we are parsing.
			stateCookie->Freeze();

			return stateCookie;
		}

		StateCookie* StateCookie::Factory(
		  uint8_t* buffer,
		  size_t bufferLength,
		  uint32_t myVerificationTag,
		  uint32_t peerVerificationTag,
		  uint32_t myInitialTsn,
		  uint32_t peerInitialTsn,
		  uint32_t myAdvertisedReceiverWindowCredit,
		  uint64_t tieTag,
		  const NegotiatedCapabilities& negotiatedCapabilities)
		{
			MS_TRACE();

			if (bufferLength < StateCookie::StateCookieLength)
			{
				MS_THROW_TYPE_ERROR("buffer too small");
			}

			Utils::Byte::Set4Bytes(buffer, 0, StateCookie::MagicValue1);
			Utils::Byte::Set4Bytes(buffer, 4, myVerificationTag);
			Utils::Byte::Set4Bytes(buffer, 8, peerVerificationTag);
			Utils::Byte::Set4Bytes(buffer, 12, myInitialTsn);
			Utils::Byte::Set4Bytes(buffer, 16, peerInitialTsn);
			Utils::Byte::Set4Bytes(buffer, 20, myAdvertisedReceiverWindowCredit);
			Utils::Byte::Set8Bytes(buffer, 24, tieTag);

			auto* stateCookie = new StateCookie(buffer, StateCookie::StateCookieLength);

			auto* negotiatedCapabilitiesField = stateCookie->GetNegotiatedCapabilitiesField();

			negotiatedCapabilitiesField->reserved    = 0;
			negotiatedCapabilitiesField->bitA        = negotiatedCapabilities.partialReliability;
			negotiatedCapabilitiesField->bitB        = negotiatedCapabilities.messageInterleaving;
			negotiatedCapabilitiesField->bitC        = negotiatedCapabilities.reconfig;
			negotiatedCapabilitiesField->bitD        = negotiatedCapabilities.zeroChecksum;
			negotiatedCapabilitiesField->magicValue2 = uint16_t{ htons(StateCookie::MagicValue2) };
			negotiatedCapabilitiesField->maxOutboundStreams =
			  uint16_t{ htons(negotiatedCapabilities.maxOutboundStreams) };
			negotiatedCapabilitiesField->maxInboundStreams =
			  uint16_t{ htons(negotiatedCapabilities.maxInboundStreams) };

			return stateCookie;
		}

		/* Instance methods. */

		StateCookie::StateCookie(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();

			SetLength(StateCookie::StateCookieLength);
		}

		StateCookie::~StateCookie()
		{
			MS_TRACE();
		}

		void StateCookie::Dump(int indentation) const
		{
			MS_TRACE();

			auto negotiatedCapabilities = GetNegotiatedCapabilities();

			MS_DUMP_CLEAN(indentation, "<SCTP::StateCookie>");
			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(indentation, "  frozen: %s", IsFrozen() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  my verification tag: %" PRIu32, GetMyVerificationTag());
			MS_DUMP_CLEAN(indentation, "  peer verification tag: %" PRIu32, GetPeerVerificationTag());
			MS_DUMP_CLEAN(indentation, "  my initial tsn: %" PRIu32, GetMyInitialTsn());
			MS_DUMP_CLEAN(indentation, "  peer initial tsn: %" PRIu32, GetPeerInitialTsn());
			MS_DUMP_CLEAN(
			  indentation,
			  "  my advertised receiver window credit: %" PRIu32,
			  GetMyAdvertisedReceiverWindowCredit());
			MS_DUMP_CLEAN(indentation, "  tie-tag: %" PRIu64, GetTieTag());
			negotiatedCapabilities.Dump(indentation + 1);
			MS_DUMP_CLEAN(indentation, "</SCTP::StateCookie>");
		}

		StateCookie* StateCookie::Clone(uint8_t* buffer, size_t bufferLength) const
		{
			MS_TRACE();

			auto* clonedStateCookie = new StateCookie(buffer, bufferLength);

			Serializable::CloneInto(clonedStateCookie);

			return clonedStateCookie;
		}

		NegotiatedCapabilities StateCookie::GetNegotiatedCapabilities() const
		{
			MS_TRACE();

			auto* negotiatedCapabilitiesField = GetNegotiatedCapabilitiesField();

			NegotiatedCapabilities negotiatedCapabilities;

			negotiatedCapabilities.maxOutboundStreams =
			  uint16_t{ ntohs(negotiatedCapabilitiesField->maxOutboundStreams) };
			negotiatedCapabilities.maxInboundStreams =
			  uint16_t{ ntohs(negotiatedCapabilitiesField->maxInboundStreams) };
			negotiatedCapabilities.partialReliability  = negotiatedCapabilitiesField->bitA;
			negotiatedCapabilities.messageInterleaving = negotiatedCapabilitiesField->bitB;
			negotiatedCapabilities.reconfig            = negotiatedCapabilitiesField->bitC;
			negotiatedCapabilities.zeroChecksum        = negotiatedCapabilitiesField->bitD;

			// NOTE: No need to std::move(). Copy elision (RVO) is used for free in GCC
			// and clang in C++17 or higher.
			return negotiatedCapabilities;
		}
	} // namespace SCTP
} // namespace RTC
