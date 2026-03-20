#define MS_CLASS "RTC::SCTP::NegotiatedCapabilities"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/NegotiatedCapabilities.hpp"
#include "Logger.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Class methods. */

		NegotiatedCapabilities NegotiatedCapabilities::Factory(
		  const SctpOptions& sctpOptions, const AnyInitChunk* remoteChunk)
		{
			MS_TRACE();

			NegotiatedCapabilities negotiatedCapabilities{};

			const auto* remoteSupportedExtensionsParameter =
			  remoteChunk->template GetFirstParameterOfType<SupportedExtensionsParameter>();
			const auto* remoteForwardTsnSupportedParameter =
			  remoteChunk->template GetFirstParameterOfType<ForwardTsnSupportedParameter>();
			const auto* remoteZeroChecksumAcceptableParameter =
			  remoteChunk->template GetFirstParameterOfType<ZeroChecksumAcceptableParameter>();

			negotiatedCapabilities.maxOutboundStreams =
			  std::min(sctpOptions.maxOutboundStreams, remoteChunk->GetNumberOfInboundStreams());

			negotiatedCapabilities.maxInboundStreams =
			  std::min(sctpOptions.maxInboundStreams, remoteChunk->GetNumberOfOutboundStreams());

			// Partial Reliability Extension is negotiated if we desire it and
			// peer announces support via Forward-TSN-Supported Parameter or via
			// Supported Extensions Parameter.
			negotiatedCapabilities.partialReliability =
			  sctpOptions.enablePartialReliability &&
			  (remoteForwardTsnSupportedParameter ||
			   (remoteSupportedExtensionsParameter &&
			    remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::FORWARD_TSN)));

			// Message Interleaving is negotiated if we desire it and peer
			// announces support via Supported Extensions Parameter.
			negotiatedCapabilities.messageInterleaving =
			  sctpOptions.enableMessageInterleaving && remoteSupportedExtensionsParameter &&
			  remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::I_DATA) &&
			  remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::I_FORWARD_TSN);

			// Stream Re-Configuration is negotiated if peer announces support via
			// Supported Extensions Parameter.
			negotiatedCapabilities.reConfig =
			  remoteSupportedExtensionsParameter &&
			  remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::RE_CONFIG);

			// Alternate Error Detection Method for Zero Checksum is negotiated
			// if we desire it and peer announces the same non-none alternate
			// error detection method.
			negotiatedCapabilities.zeroChecksum =
			  sctpOptions.zeroChecksumAlternateErrorDetectionMethod !=
			    ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE &&
			  remoteZeroChecksumAcceptableParameter &&
			  remoteZeroChecksumAcceptableParameter->GetAlternateErrorDetectionMethod() ==
			    sctpOptions.zeroChecksumAlternateErrorDetectionMethod;

			return negotiatedCapabilities;
		}

		/* Instance methods. */

		void NegotiatedCapabilities::Dump(int indentation) const
		{
			MS_TRACE();

			MS_DUMP_CLEAN(indentation, "<SCTP::NegotiatedCapabilities>");
			MS_DUMP_CLEAN(indentation, "  max outbound streams: %" PRIu16, this->maxOutboundStreams);
			MS_DUMP_CLEAN(indentation, "  max inbound streams: %" PRIu16, this->maxInboundStreams);
			MS_DUMP_CLEAN(indentation, "  partial reliability: %s", this->partialReliability ? "yes" : "no");
			MS_DUMP_CLEAN(
			  indentation, "  message interleaving: %s", this->messageInterleaving ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  re-config: %s", this->reConfig ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  zero checksum: %s", this->zeroChecksum ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::NegotiatedCapabilities>");
		}
	} // namespace SCTP
} // namespace RTC
