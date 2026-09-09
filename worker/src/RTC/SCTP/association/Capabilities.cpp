#define MS_CLASS "RTC::SCTP::Capabilities"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/Capabilities.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		Capabilities Capabilities::Factory(const AnyInitChunk* remoteChunk)
		{
			MS_TRACE();

			Capabilities capabilities{};

			const auto* remoteSupportedExtensionsParameter =
			  remoteChunk->template GetFirstParameterOfType<SupportedExtensionsParameter>();
			const auto* remoteForwardTsnSupportedParameter =
			  remoteChunk->template GetFirstParameterOfType<ForwardTsnSupportedParameter>();
			const auto* remoteZeroChecksumAcceptableParameter =
			  remoteChunk->template GetFirstParameterOfType<ZeroChecksumAcceptableParameter>();

			// Streams announced by the remote endpoint in its INIT or INIT-ACK chunk.
			capabilities.maxOutboundStreams = remoteChunk->GetNumberOfOutboundStreams();
			capabilities.maxInboundStreams  = remoteChunk->GetNumberOfInboundStreams();

			// Remote announces Partial Reliability Extension support via
			// Forward-TSN-Supported parameter or via Supported Extensions parameter.
			capabilities.partialReliability =
			  remoteForwardTsnSupportedParameter ||
			  (remoteSupportedExtensionsParameter &&
				 remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::FORWARD_TSN));

			// Remote announces Message Interleaving support via Supported Extensions
			// parameter.
			capabilities.messageInterleaving =
			  remoteSupportedExtensionsParameter &&
			  remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::I_DATA) &&
			  remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::I_FORWARD_TSN);

			// Remote announces Stream Re-Configuration support via Supported
			// Extensions parameter.
			capabilities.reConfig =
			  remoteSupportedExtensionsParameter &&
			  remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::RE_CONFIG);

			// Alternate Error Detection Method for Zero Checksum announced by the
			// remote endpoint (NONE if not announced).
			capabilities.zeroChecksumAlternateErrorDetectionMethod =
			  remoteZeroChecksumAcceptableParameter
			    ? remoteZeroChecksumAcceptableParameter->GetAlternateErrorDetectionMethod()
			    : ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE;

			// NOTE: No need to std::move(). Copy elision (RVO) is used for free in GCC
			// and clang in C++17 or higher.
			return capabilities;
		}

		/* Instance methods. */

		void Capabilities::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::Capabilities>");
			MS_DUMP_CLEAN(indentation, "  max outbound streams: %" PRIu16, this->maxOutboundStreams);
			MS_DUMP_CLEAN(indentation, "  max inbound streams: %" PRIu16, this->maxInboundStreams);
			MS_DUMP_CLEAN(indentation, "  partial reliability: %s", this->partialReliability ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation, "  message interleaving: %s", this->messageInterleaving ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  re-config: %s", this->reConfig ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation,
			  "  zero checksum alternate error detection method: %s",
			  ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethodToString(
			    this->zeroChecksumAlternateErrorDetectionMethod)
			    .c_str());
			MS_DUMP_CLEAN(indentation, "</SCTP::Capabilities>");
		}
	} // namespace SCTP
} // namespace RTC
