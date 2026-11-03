#define MS_CLASS "RTC::NEW_RTCP::ExtendedReport"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/NEW_RTCP/packet/ExtendedReport.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include <limits> // std::numeric_limits

namespace RTC
{
	namespace NEW_RTCP
	{
		/* Class variables. */

		// clang-format off
		const ankerl::unordered_dense::map<ExtendedReport::BlockType, std::string> ExtendedReport::BlockType2String =
		{
			{ ExtendedReport::BlockType::LRLE, "LRLE" },
			{ ExtendedReport::BlockType::DRLE, "DRLE" },
			{ ExtendedReport::BlockType::PRT,  "PRT"  },
			{ ExtendedReport::BlockType::RRT,  "RRT"  },
			{ ExtendedReport::BlockType::DLRR, "DLRR" },
			{ ExtendedReport::BlockType::SS,   "SS"   },
			{ ExtendedReport::BlockType::VM,   "VM"   },
		};
		// clang-format on

		/* Class methods. */

		const std::string& ExtendedReport::BlockTypeToString(BlockType blockType)
		{
			MS_TRACE();

			static const std::string Unknown("UNKNOWN");

			const auto it = ExtendedReport::BlockType2String.find(blockType);

			if (it == ExtendedReport::BlockType2String.end())
			{
				return Unknown;
			}

			const std::string& blockTypeString = it->second;

			return blockTypeString;
		}

		bool ExtendedReport::IsExtendedReport(
		  const uint8_t* buffer, size_t bufferLength, BlockType& blockType, size_t& extendedReportLength)
		{
			MS_TRACE();

			if (bufferLength < ExtendedReport::ExtendedReportHeaderLength)
			{
				MS_WARN_TAG(rtcp, "no space for Extended Report block header");

				return false;
			}

			blockType            = static_cast<ExtendedReport::BlockType>(buffer[0]);
			extendedReportLength = (static_cast<size_t>(Utils::Byte::Get2Bytes(buffer, 2)) + 1) * 4;

			if (bufferLength < extendedReportLength)
			{
				MS_WARN_TAG(
				  rtcp,
				  "no space for announced Extended Report block length [blockType:%s, extendedReportLength:%zu, bufferLength:%zu]",
				  ExtendedReport::BlockTypeToString(blockType).c_str(),
				  extendedReportLength,
				  bufferLength);

				return false;
			}

			return true;
		}

		/* Instance methods. */

		ExtendedReport::ExtendedReport(uint8_t* buffer, size_t bufferLength)
		  : Serializable(buffer, bufferLength)
		{
			MS_TRACE();
		}

		ExtendedReport::~ExtendedReport()
		{
			MS_TRACE();
		}

		void ExtendedReport::DumpCommon(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(
			  indentation,
			  "  block type: %" PRIu8 " (%s) (unknown: %s)",
			  static_cast<uint8_t>(GetType()),
			  ExtendedReport::BlockTypeToString(GetType()).c_str(),
			  HasUnknownType() ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  length: %zu (buffer length: %zu)", GetLength(), GetBufferLength());
			MS_DUMP_CLEAN(indentation, "  type-specific field: %" PRIu8, GetTypeSpecific());
		}

		void ExtendedReport::SoftSerialize(const uint8_t* buffer)
		{
			MS_TRACE();

			SetBuffer(const_cast<uint8_t*>(buffer));
		}

		void ExtendedReport::SoftCloneInto(ExtendedReport* extendedReport) const
		{
			MS_TRACE();

			// Need to manually set Serializable length.
			extendedReport->SetLength(GetLength());
		}

		void ExtendedReport::InitializeHeader(BlockType blockType, uint8_t typeSpecific, size_t length)
		{
			MS_TRACE();

			auto* const header = GetHeaderPointer();

			header->blockType    = blockType;
			header->typeSpecific = typeSpecific;

			SetLengthField(length);
		}

		void ExtendedReport::SetTypeSpecific(uint8_t typeSpecific)
		{
			MS_TRACE();

			GetHeaderPointer()->typeSpecific = typeSpecific;
		}

		void ExtendedReport::SetLengthField(size_t length)
		{
			MS_TRACE();

			if (length < ExtendedReport::ExtendedReportHeaderLength)
			{
				MS_THROW_TYPE_ERROR(
				  "length (%zu bytes) cannot be lower than %zu",
				  length,
				  ExtendedReport::ExtendedReportHeaderLength);
			}

			// The block length field is the length of the Extended Report block in
			// 32-bit words minus one, so the length must be multiple of 4 bytes.
			if (!Utils::Byte::IsPaddedTo4Bytes(length))
			{
				MS_THROW_TYPE_ERROR("length (%zu bytes) must be multiple of 4", length);
			}

			// The maximum representable length is (65535 + 1) * 4 bytes.
			if (length > (std::numeric_limits<uint16_t>::max() + 1) * 4)
			{
				MS_THROW_TYPE_ERROR("length (%zu bytes) cannot be greater than 262144", length);
			}

			Utils::Byte::Set2Bytes(const_cast<uint8_t*>(GetBuffer()), 2, (length / 4) - 1);
		}

		void ExtendedReport::SetVariableLengthValueLength(size_t valueLength)
		{
			MS_TRACE();

			const size_t previousLength      = GetLength();
			const size_t previousLengthField = GetLengthFieldComputed();
			const size_t newLength           = ExtendedReport::ExtendedReportHeaderLength + valueLength;

			try
			{
				// Let's call SetLength() on parent with the new computed length.
				// NOTE: If there is no space in the buffer for it, it will throw.
				SetLength(newLength);

				// Update length field.
				// NOTE: This will throw if computed value is too big.
				SetLengthField(newLength);
			}
			catch (const MediaSoupError&)
			{
				// Rollback.
				SetLength(previousLength);
				SetLengthField(previousLengthField);

				throw;
			}
		}
	} // namespace NEW_RTCP
} // namespace RTC
