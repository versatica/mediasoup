#define MS_CLASS "RTC::SCTP::NegotiatedCapabilities"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/NegotiatedCapabilities.hpp"
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
		  SocketOptions socketOptions, InitOrInitAckChunkVariant remoteChunk)
		{
			MS_TRACE();

			// Here it's guaranteed that `remoteChunk` it's a InitChunk or
			// InitAckChunk, and both classes provide same methods (the ones used in
			// the lambda).
			return std::visit(
			  [&](const auto* remoteChunk)
			  {
				  NegotiatedCapabilities negotiatedCapabilities{};

				  auto* remoteSupportedExtensionsParameter =
				    remoteChunk->template GetFirstParameterOfType<SupportedExtensionsParameter>();
				  auto* remoteForwardTsnSupportedParameter =
				    remoteChunk->template GetFirstParameterOfType<ForwardTsnSupportedParameter>();
				  auto* remoteZeroChecksumAcceptableParameter =
				    remoteChunk->template GetFirstParameterOfType<ZeroChecksumAcceptableParameter>();

				  negotiatedCapabilities.maxOutboundStreams =
				    std::min(socketOptions.maxOutboundStreams, remoteChunk->GetNumberOfInboundStreams());

				  negotiatedCapabilities.maxInboundStreams =
				    std::min(socketOptions.maxInboundStreams, remoteChunk->GetNumberOfOutboundStreams());

				  // Partial Reliability Extension is negotiated if we desire it and
				  // peer announces support via Forward-TSN-Supported Parameter or via
				  // Supported Extensions Parameter.
				  negotiatedCapabilities.partialReliability =
				    socketOptions.enablePartialReliability &&
				    (remoteForwardTsnSupportedParameter ||
				     (remoteSupportedExtensionsParameter &&
				      remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::FORWARD_TSN)));

				  // Message Interleaving is negotiated if we desire it and peer
				  // announces support via Supported Extensions Parameter.
				  negotiatedCapabilities.messageInterleaving =
				    socketOptions.enableMessageInterleaving && remoteSupportedExtensionsParameter &&
				    remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::I_DATA) &&
				    remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::I_FORWARD_TSN);

				  // Stream Reconfiguration is negotiated if peer announces support via
				  // Supported Extensions Parameter.
				  negotiatedCapabilities.reconfig =
				    remoteSupportedExtensionsParameter &&
				    remoteSupportedExtensionsParameter->IncludesChunkType(Chunk::ChunkType::RE_CONFIG);

				  // Alternate Error Detection Method for Zero Checksum is negotiated
				  // if we desire it and peer announces the same non-none alternate
				  // error detection method.
				  negotiatedCapabilities.zeroChecksum =
				    socketOptions.zeroChecksumAlternateErrorDetectionMethod !=
				      ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE &&
				    remoteZeroChecksumAcceptableParameter &&
				    remoteZeroChecksumAcceptableParameter->GetAlternateErrorDetectionMethod() ==
				      socketOptions.zeroChecksumAlternateErrorDetectionMethod;

				  return negotiatedCapabilities;
			  },
			  remoteChunk);
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
			MS_DUMP_CLEAN(indentation, "  re-config: %s", this->reconfig ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "  zero checksum: %s", this->zeroChecksum ? "yes" : "no");
			MS_DUMP_CLEAN(indentation, "</SCTP::NegotiatedCapabilities>");
		}
	} // namespace SCTP
} // namespace RTC
