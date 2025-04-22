#define MS_CLASS "RTC::SCTP::ChunkParameter"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/ChunkParameter.hpp"
#include "Logger.hpp"
#include "Utils.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class variables. */

		// clang-format off
		std::unordered_map<ChunkParameter::ChunkParameterType, std::string> ChunkParameter::chunkParameterType2String =
		{
			{ ChunkParameter::ChunkParameterType::HEARTBEAT_INFO,          "HEARTBEAT_INFO"          },
			{ ChunkParameter::ChunkParameterType::IPV4_ADDRESS,            "IPV4_ADDRESS"            },
			{ ChunkParameter::ChunkParameterType::IPV6_ADDRESS,            "IPV6_ADDRESS"            },
			{ ChunkParameter::ChunkParameterType::COOKIE_PRESERVATIVE,     "COOKIE_PRESERVATIVE"     },
			{ ChunkParameter::ChunkParameterType::SUPPORTED_ADDRESS_TYPES, "SUPPORTED_ADDRESS_TYPES" },
			// TODO: Add more.
		};
		// clang-format on

		/* Class methods. */

		bool ChunkParameter::IsChunkParameter(
		  const uint8_t* buffer,
		  size_t bufferLength,
		  ChunkParameter::ChunkParameterType& parameterType,
		  uint16_t& parameterLength,
		  uint8_t& padding)
		{
			MS_TRACE();

			if (!PacketItemBase::IsPacketItemBase(buffer, bufferLength, parameterLength, padding))
			{
				return false;
			}

			parameterType =
			  static_cast<ChunkParameter::ChunkParameterType>(Utils::Byte::Get2Bytes(buffer, 0));

			return true;
		}

		const std::string& ChunkParameter::ChunkParameterType2String(ChunkParameterType parameterType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			auto it = ChunkParameter::chunkParameterType2String.find(parameterType);

			if (it == ChunkParameter::chunkParameterType2String.end())
			{
				return Unknown;
			}

			return it->second;
		}

		/* Instance methods. */

		ChunkParameter::ChunkParameter(uint8_t* buffer, size_t bufferLength)
		  : PacketItemBase(buffer, bufferLength)
		{
			MS_TRACE();
		}

		ChunkParameter::~ChunkParameter()
		{
			MS_TRACE();
		}

		void ChunkParameter::SoftCloneInto(ChunkParameter* parameter) const
		{
			MS_TRACE();

			// Need to manually set Serializable length.
			parameter->SetLength(GetLength());
		}

		void ChunkParameter::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation,
			  "  type: %" PRIu16 " (%s) (unknown: %s)",
			  static_cast<uint16_t>(GetType()),
			  ChunkParameter::ChunkParameterType2String(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			PacketItemBase::DumpCommon(indentation);
		}

		void ChunkParameter::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			SetBuffer(const_cast<uint8_t*>(buffer));
		}

		void ChunkParameter::InitializeHeader(ChunkParameterType parameterType, uint16_t lengthFieldValue)
		{
			MS_TRACE();

			SetType(parameterType);
			InitializePacketBaseItemHeader(lengthFieldValue);
		}
	} // namespace SCTP
} // namespace RTC
