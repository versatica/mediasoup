#include "fuzzer/include/RTC/SCTP/packet/FuzzerPacket.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/SCTP/packet/ErrorCause.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/Parameter.hpp"
#include "RTC/SCTP/packet/chunks/AbortAssociationChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/CookieEchoChunk.hpp"
#include "RTC/SCTP/packet/chunks/DataChunk.hpp"
#include "RTC/SCTP/packet/chunks/ForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/HeartbeatRequestChunk.hpp"
#include "RTC/SCTP/packet/chunks/IDataChunk.hpp"
#include "RTC/SCTP/packet/chunks/IForwardTsnChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/InitChunk.hpp"
#include "RTC/SCTP/packet/chunks/OperationErrorChunk.hpp"
#include "RTC/SCTP/packet/chunks/ReConfigChunk.hpp"
#include "RTC/SCTP/packet/chunks/SackChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownAckChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownChunk.hpp"
#include "RTC/SCTP/packet/chunks/ShutdownCompleteChunk.hpp"
#include "RTC/SCTP/packet/chunks/UnknownChunk.hpp"
#include "RTC/SCTP/packet/errorCauses/CookieReceivedWhileShuttingDownErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidMandatoryParameterErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/InvalidStreamIdentifierErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/MissingMandatoryParameterErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/NoUserDataErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/OutOfResourceErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/ProtocolViolationErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/RestartOfAnAssociationWithNewAddressesErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/StaleCookieErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnknownErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedChunkTypeErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnrecognizedParametersErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UnresolvableAddressErrorCause.hpp"
#include "RTC/SCTP/packet/errorCauses/UserInitiatedAbortErrorCause.hpp"
#include "RTC/SCTP/packet/parameters/AddIncomingStreamsRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/AddOutgoingStreamsRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/CookiePreservativeParameter.hpp"
#include "RTC/SCTP/packet/parameters/ForwardTsnSupportedParameter.hpp"
#include "RTC/SCTP/packet/parameters/HeartbeatInfoParameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv4AddressParameter.hpp"
#include "RTC/SCTP/packet/parameters/IPv6AddressParameter.hpp"
#include "RTC/SCTP/packet/parameters/IncomingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/OutgoingSsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/ReconfigurationResponseParameter.hpp"
#include "RTC/SCTP/packet/parameters/SsnTsnResetRequestParameter.hpp"
#include "RTC/SCTP/packet/parameters/StateCookieParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedAddressTypesParameter.hpp"
#include "RTC/SCTP/packet/parameters/SupportedExtensionsParameter.hpp"
#include "RTC/SCTP/packet/parameters/UnknownParameter.hpp"
#include "RTC/SCTP/packet/parameters/UnrecognizedParameterParameter.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerAbortAssociationChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerCookieAckChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerCookieEchoChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerDataChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerForwardTsnChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerHeartbeatAckChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerHeartbeatRequestChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerIDataChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerIForwardTsnChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerInitAckChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerInitChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerOperationErrorChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerReConfigChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerSackChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerShutdownAckChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerShutdownChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerShutdownCompleteChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/chunks/FuzzerUnknownChunk.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerCookieReceivedWhileShuttingDownErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerInvalidMandatoryParameterErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerInvalidStreamIdentifierErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerMissingMandatoryParameterErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerNoUserDataErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerOutOfResourceErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerProtocolViolationErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerRestartOfAnAssociationWithNewAddressesErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerStaleCookieErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnknownErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnrecognizedChunkTypeErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnrecognizedParametersErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUnresolvableAddressErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/errorCauses/FuzzerUserInitiatedAbortErrorCause.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerAddIncomingStreamsRequestParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerAddOutgoingStreamsRequestParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerCookiePreservativeParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerForwardTsnSupportedParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerHeartbeatInfoParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerIPv4AddressParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerIPv6AddressParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerIncomingSsnResetRequestParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerOutgoingSsnResetRequestParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerReconfigurationResponseParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerSsnTsnResetRequestParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerStateCookieParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerSupportedAddressTypesParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerSupportedExtensionsParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerUnknownParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerUnrecognizedParameterParameter.hpp"
#include "fuzzer/include/RTC/SCTP/packet/parameters/FuzzerZeroChecksumAcceptableParameter.hpp"

namespace
{
	thread_local uint8_t PacketSerializeBuffer[65536];
	thread_local uint8_t PacketCloneBuffer[65536];
	thread_local uint8_t ChunkSerializeBuffer[65536];
	thread_local uint8_t ChunkCloneBuffer[65536];
	thread_local uint8_t ParameterSerializeBuffer[65536];
	thread_local uint8_t ParameterCloneBuffer[65536];
	thread_local uint8_t ErrorCauseSerializeBuffer[65536];
	thread_local uint8_t ErrorCauseCloneBuffer[65536];
} // namespace

void FuzzerRtcSctpPacket::Fuzz(const uint8_t* data, size_t len)
{
	const auto fuzzChunks = [](const RTC::SCTP::Packet* packet) -> void
	{
		packet->GetSourcePort();
		packet->GetDestinationPort();
		packet->GetVerificationTag();
		packet->GetChecksum();
		packet->ValidateCRC32cChecksum();
		packet->HasChunks();
		packet->GetChunksCount();
		packet->GetChunkAt(0);
		packet->GetChunkAt(1);
		packet->GetChunkAt(2);
		packet->GetChunkAt(666);
		packet->GetFirstChunkOfType<RTC::SCTP::DataChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::InitChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::InitAckChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::SackChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatRequestChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::HeartbeatAckChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::AbortAssociationChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::ShutdownChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::ShutdownAckChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::OperationErrorChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::CookieEchoChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::CookieAckChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::ShutdownCompleteChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::ForwardTsnChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::ReConfigChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::IDataChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::IForwardTsnChunk>();
		packet->GetFirstChunkOfType<RTC::SCTP::UnknownChunk>();

		for (auto chunkIt = packet->ChunksBegin(); chunkIt != packet->ChunksEnd(); ++chunkIt)
		{
			const RTC::SCTP::Chunk* const chunk = *chunkIt;

			chunk->GetType();
			chunk->HasUnknownType();
			chunk->GetActionForUnknownChunkType();
			chunk->GetFlags();
			chunk->CanHaveParameters();
			chunk->HasParameters();
			chunk->GetParametersCount();
			chunk->GetParameterAt(0);
			chunk->GetParameterAt(1);
			chunk->GetParameterAt(666);
			chunk->GetFirstParameterOfType<RTC::SCTP::HeartbeatInfoParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::IPv4AddressParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::IPv6AddressParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::StateCookieParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::UnrecognizedParameterParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::CookiePreservativeParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::SupportedAddressTypesParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::ForwardTsnSupportedParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::SupportedExtensionsParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::OutgoingSsnResetRequestParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::IncomingSsnResetRequestParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::SsnTsnResetRequestParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::ReconfigurationResponseParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::AddOutgoingStreamsRequestParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::AddIncomingStreamsRequestParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::ZeroChecksumAcceptableParameter>();
			chunk->GetFirstParameterOfType<RTC::SCTP::UnknownParameter>();
			chunk->CanHaveErrorCauses();
			chunk->HasErrorCauses();
			chunk->GetErrorCausesCount();
			chunk->GetErrorCauseAt(0);
			chunk->GetErrorCauseAt(1);
			chunk->GetErrorCauseAt(666);
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::InvalidStreamIdentifierErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::MissingMandatoryParameterErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::StaleCookieErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::OutOfResourceErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::UnresolvableAddressErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::UnrecognizedChunkTypeErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::InvalidMandatoryParameterErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::UnrecognizedParametersErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::NoUserDataErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::CookieReceivedWhileShuttingDownErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::UserInitiatedAbortErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::ProtocolViolationErrorCause>();
			chunk->GetFirstErrorCauseOfCode<RTC::SCTP::UnknownErrorCause>();

			const std::unique_ptr<RTC::SCTP::Chunk> clonedChunk{ chunk->Clone(
				ChunkCloneBuffer, chunk->GetLength()) };

			clonedChunk->Serialize(ChunkSerializeBuffer, clonedChunk->GetLength());

			switch (chunk->GetType())
			{
				case RTC::SCTP::Chunk::ChunkType::DATA:
				{
					const auto* const dataChunk = static_cast<const RTC::SCTP::DataChunk*>(chunk);

					FuzzerRtcSctpDataChunk::Fuzz(dataChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::INIT:
				{
					const auto* const initChunk = static_cast<const RTC::SCTP::InitChunk*>(chunk);

					FuzzerRtcSctpInitChunk::Fuzz(initChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::INIT_ACK:
				{
					const auto* const initAckChunk = static_cast<const RTC::SCTP::InitAckChunk*>(chunk);

					FuzzerRtcSctpInitAckChunk::Fuzz(initAckChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::SACK:
				{
					const auto* const sackChunk = static_cast<const RTC::SCTP::SackChunk*>(chunk);

					FuzzerRtcSctpSackChunk::Fuzz(sackChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::HEARTBEAT_REQUEST:
				{
					const auto* const heartbeatRequestChunk =
					  static_cast<const RTC::SCTP::HeartbeatRequestChunk*>(chunk);

					FuzzerRtcSctpHeartbeatRequestChunk::Fuzz(heartbeatRequestChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::HEARTBEAT_ACK:
				{
					const auto* const heartbeatAckChunk =
					  static_cast<const RTC::SCTP::HeartbeatAckChunk*>(chunk);

					FuzzerRtcSctpHeartbeatAckChunk::Fuzz(heartbeatAckChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::ABORT:
				{
					const auto* const abortAssociationChunk =
					  static_cast<const RTC::SCTP::AbortAssociationChunk*>(chunk);

					FuzzerRtcSctpAbortAssociationChunk::Fuzz(abortAssociationChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::SHUTDOWN:
				{
					const auto* const shutdownChunk = static_cast<const RTC::SCTP::ShutdownChunk*>(chunk);

					FuzzerRtcSctpShutdownChunk::Fuzz(shutdownChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::SHUTDOWN_ACK:
				{
					const auto* const shutdownAckChunk = static_cast<const RTC::SCTP::ShutdownAckChunk*>(chunk);

					FuzzerRtcSctpShutdownAckChunk::Fuzz(shutdownAckChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::OPERATION_ERROR:
				{
					const auto* const operationErrorChunk =
					  static_cast<const RTC::SCTP::OperationErrorChunk*>(chunk);

					FuzzerRtcSctpOperationErrorChunk::Fuzz(operationErrorChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::COOKIE_ECHO:
				{
					const auto* const cookieEchoChunk = static_cast<const RTC::SCTP::CookieEchoChunk*>(chunk);

					FuzzerRtcSctpCookieEchoChunk::Fuzz(cookieEchoChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::COOKIE_ACK:
				{
					const auto* const cookieAckChunk = static_cast<const RTC::SCTP::CookieAckChunk*>(chunk);

					FuzzerRtcSctpCookieAckChunk::Fuzz(cookieAckChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::SHUTDOWN_COMPLETE:
				{
					const auto* const shutdownCompleteChunk =
					  static_cast<const RTC::SCTP::ShutdownCompleteChunk*>(chunk);

					FuzzerRtcSctpShutdownCompleteChunk::Fuzz(shutdownCompleteChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::FORWARD_TSN:
				{
					const auto* const forwardTsnChunk = static_cast<const RTC::SCTP::ForwardTsnChunk*>(chunk);

					FuzzerRtcSctpForwardTsnChunk::Fuzz(forwardTsnChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::RE_CONFIG:
				{
					const auto* const reConfigChunk = static_cast<const RTC::SCTP::ReConfigChunk*>(chunk);

					FuzzerRtcSctpReConfigChunk::Fuzz(reConfigChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::I_DATA:
				{
					const auto* const iDataChunk = static_cast<const RTC::SCTP::IDataChunk*>(chunk);

					FuzzerRtcSctpIDataChunk::Fuzz(iDataChunk);

					break;
				}

				case RTC::SCTP::Chunk::ChunkType::I_FORWARD_TSN:
				{
					const auto* const iForwardTsnChunk = static_cast<const RTC::SCTP::IForwardTsnChunk*>(chunk);

					FuzzerRtcSctpIForwardTsnChunk::Fuzz(iForwardTsnChunk);

					break;
				}

				default:
				{
					const auto* const unknownChunk = static_cast<const RTC::SCTP::UnknownChunk*>(chunk);

					FuzzerRtcSctpUnknownChunk::Fuzz(unknownChunk);
				}
			}

			for (auto parameterIt = chunk->ParametersBegin(); parameterIt != chunk->ParametersEnd();
			     ++parameterIt)
			{
				const RTC::SCTP::Parameter* const parameter = *parameterIt;

				parameter->GetType();
				parameter->HasUnknownType();
				parameter->GetActionForUnknownParameterType();

				const std::unique_ptr<RTC::SCTP::Parameter> clonedParameter{ parameter->Clone(
					ParameterCloneBuffer, parameter->GetLength()) };

				clonedParameter->Serialize(ParameterSerializeBuffer, clonedParameter->GetLength());

				switch (parameter->GetType())
				{
					case RTC::SCTP::Parameter::ParameterType::HEARTBEAT_INFO:
					{
						const auto* const heartbeatInfoParameter =
						  static_cast<const RTC::SCTP::HeartbeatInfoParameter*>(parameter);

						FuzzerRtcSctpHeartbeatInfoParameter::Fuzz(heartbeatInfoParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::IPV4_ADDRESS:
					{
						const auto* const ipv4AddressParameter =
						  static_cast<const RTC::SCTP::IPv4AddressParameter*>(parameter);

						FuzzerRtcSctpIPv4AddressParameter::Fuzz(ipv4AddressParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::IPV6_ADDRESS:
					{
						const auto* const ipv6AddressParameter =
						  static_cast<const RTC::SCTP::IPv6AddressParameter*>(parameter);

						FuzzerRtcSctpIPv6AddressParameter::Fuzz(ipv6AddressParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::STATE_COOKIE:
					{
						const auto* const stateCookieParameter =
						  static_cast<const RTC::SCTP::StateCookieParameter*>(parameter);

						FuzzerRtcSctpStateCookieParameter::Fuzz(stateCookieParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::UNRECOGNIZED_PARAMETER:
					{
						const auto* const unrecognizedParameterParameter =
						  static_cast<const RTC::SCTP::UnrecognizedParameterParameter*>(parameter);

						FuzzerRtcSctpUnrecognizedParameterParameter::Fuzz(unrecognizedParameterParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::COOKIE_PRESERVATIVE:
					{
						const auto* const cookiePreservativeParameter =
						  static_cast<const RTC::SCTP::CookiePreservativeParameter*>(parameter);

						FuzzerRtcSctpCookiePreservativeParameter::Fuzz(cookiePreservativeParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::SUPPORTED_ADDRESS_TYPES:
					{
						const auto* const supportedAddressTypesParameter =
						  static_cast<const RTC::SCTP::SupportedAddressTypesParameter*>(parameter);

						FuzzerRtcSctpSupportedAddressTypesParameter::Fuzz(supportedAddressTypesParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::FORWARD_TSN_SUPPORTED:
					{
						const auto* const forwardTsnSupportedParameter =
						  static_cast<const RTC::SCTP::ForwardTsnSupportedParameter*>(parameter);

						FuzzerRtcSctpForwardTsnSupportedParameter::Fuzz(forwardTsnSupportedParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::SUPPORTED_EXTENSIONS:
					{
						const auto* const supportedExtensionsParameter =
						  static_cast<const RTC::SCTP::SupportedExtensionsParameter*>(parameter);

						FuzzerRtcSctpSupportedExtensionsParameter::Fuzz(supportedExtensionsParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::OUTGOING_SSN_RESET_REQUEST:
					{
						const auto* const outgoingSsnResetRequestParameter =
						  static_cast<const RTC::SCTP::OutgoingSsnResetRequestParameter*>(parameter);

						FuzzerRtcSctpOutgoingSsnResetRequestParameter::Fuzz(outgoingSsnResetRequestParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::INCOMING_SSN_RESET_REQUEST:
					{
						const auto* const incomingSsnResetRequestParameter =
						  static_cast<const RTC::SCTP::IncomingSsnResetRequestParameter*>(parameter);

						FuzzerRtcSctpIncomingSsnResetRequestParameter::Fuzz(incomingSsnResetRequestParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::SSN_TSN_RESET_REQUEST:
					{
						const auto* const ssnTsnResetRequestParameter =
						  static_cast<const RTC::SCTP::SsnTsnResetRequestParameter*>(parameter);

						FuzzerRtcSctpSsnTsnResetRequestParameter::Fuzz(ssnTsnResetRequestParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::RECONFIGURATION_RESPONSE:
					{
						const auto* const reconfigurationResponseParameter =
						  static_cast<const RTC::SCTP::ReconfigurationResponseParameter*>(parameter);

						FuzzerRtcSctpReconfigurationResponseParameter::Fuzz(reconfigurationResponseParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::ADD_OUTGOING_STREAMS_REQUEST:
					{
						const auto* const addOutgoingStreamsRequestParameter =
						  static_cast<const RTC::SCTP::AddOutgoingStreamsRequestParameter*>(parameter);

						FuzzerRtcSctpAddOutgoingStreamsRequestParameter::Fuzz(addOutgoingStreamsRequestParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::ADD_INCOMING_STREAMS_REQUEST:
					{
						const auto* const addIncomingStreamsRequestParameter =
						  static_cast<const RTC::SCTP::AddIncomingStreamsRequestParameter*>(parameter);

						FuzzerRtcSctpAddIncomingStreamsRequestParameter::Fuzz(addIncomingStreamsRequestParameter);

						break;
					}

					case RTC::SCTP::Parameter::ParameterType::ZERO_CHECKSUM_ACCEPTABLE:
					{
						const auto* const zeroChecksumAcceptableParameter =
						  static_cast<const RTC::SCTP::ZeroChecksumAcceptableParameter*>(parameter);

						FuzzerRtcSctpZeroChecksumAcceptableParameter::Fuzz(zeroChecksumAcceptableParameter);

						break;
					}

					default:
					{
						const auto* const unknownParameter =
						  static_cast<const RTC::SCTP::UnknownParameter*>(parameter);

						FuzzerRtcSctpUnknownParameter::Fuzz(unknownParameter);
					}
				}
			}

			for (auto errorCauseIt = chunk->ErrorCausesBegin(); errorCauseIt != chunk->ErrorCausesEnd();
			     ++errorCauseIt)
			{
				const RTC::SCTP::ErrorCause* const errorCause = *errorCauseIt;

				errorCause->GetCode();
				errorCause->HasUnknownCode();
				errorCause->ToString();

				const std::unique_ptr<RTC::SCTP::ErrorCause> clonedErrorCause{ errorCause->Clone(
					ErrorCauseCloneBuffer, errorCause->GetLength()) };

				clonedErrorCause->Serialize(ErrorCauseSerializeBuffer, clonedErrorCause->GetLength());

				switch (errorCause->GetCode())
				{
					case RTC::SCTP::ErrorCause::ErrorCauseCode::INVALID_STREAM_IDENTIFIER:
					{
						const auto* const invalidStreamIdentifierErrorCause =
						  static_cast<const RTC::SCTP::InvalidStreamIdentifierErrorCause*>(errorCause);

						FuzzerRtcSctpInvalidStreamIdentifierErrorCause::Fuzz(invalidStreamIdentifierErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::MISSING_MANDATORY_PARAMETER:
					{
						const auto* const missingMandatoryParameterErrorCause =
						  static_cast<const RTC::SCTP::MissingMandatoryParameterErrorCause*>(errorCause);

						FuzzerRtcSctpMissingMandatoryParameterErrorCause::Fuzz(
						  missingMandatoryParameterErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::STALE_COOKIE:
					{
						const auto* const staleCookieErrorCause =
						  static_cast<const RTC::SCTP::StaleCookieErrorCause*>(errorCause);

						FuzzerRtcSctpStaleCookieErrorCause::Fuzz(staleCookieErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::OUT_OF_RESOURCE:
					{
						const auto* const outOfResourceErrorCause =
						  static_cast<const RTC::SCTP::OutOfResourceErrorCause*>(errorCause);

						FuzzerRtcSctpOutOfResourceErrorCause::Fuzz(outOfResourceErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::UNRESOLVABLE_ADDRESS:
					{
						const auto* const unresolvableAddressErrorCause =
						  static_cast<const RTC::SCTP::UnresolvableAddressErrorCause*>(errorCause);

						FuzzerRtcSctpUnresolvableAddressErrorCause::Fuzz(unresolvableAddressErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_CHUNK_TYPE:
					{
						const auto* const unrecognizedChunkTypeErrorCause =
						  static_cast<const RTC::SCTP::UnrecognizedChunkTypeErrorCause*>(errorCause);

						FuzzerRtcSctpUnrecognizedChunkTypeErrorCause::Fuzz(unrecognizedChunkTypeErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::INVALID_MANDATORY_PARAMETER:
					{
						const auto* const invalidMandatoryParameterErrorCause =
						  static_cast<const RTC::SCTP::InvalidMandatoryParameterErrorCause*>(errorCause);

						FuzzerRtcSctpInvalidMandatoryParameterErrorCause::Fuzz(
						  invalidMandatoryParameterErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::UNRECOGNIZED_PARAMETERS:
					{
						const auto* const unrecognizedParametersErrorCause =
						  static_cast<const RTC::SCTP::UnrecognizedParametersErrorCause*>(errorCause);

						FuzzerRtcSctpUnrecognizedParametersErrorCause::Fuzz(unrecognizedParametersErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::NO_USER_DATA:
					{
						const auto* const noUserDataErrorCause =
						  static_cast<const RTC::SCTP::NoUserDataErrorCause*>(errorCause);

						FuzzerRtcSctpNoUserDataErrorCause::Fuzz(noUserDataErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::COOKIE_RECEIVED_WHILE_SHUTTING_DOWN:
					{
						const auto* const cookieReceivedWhileShuttingDownErrorCause =
						  static_cast<const RTC::SCTP::CookieReceivedWhileShuttingDownErrorCause*>(errorCause);

						FuzzerRtcSctpCookieReceivedWhileShuttingDownErrorCause::Fuzz(
						  cookieReceivedWhileShuttingDownErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::RESTART_OF_AN_ASSOCIATION_WITH_NEW_ADDRESSES:
					{
						const auto* const restartOfAnAssociationWithNewAddressesErrorCause =
						  static_cast<const RTC::SCTP::RestartOfAnAssociationWithNewAddressesErrorCause*>(
						    errorCause);

						FuzzerRtcSctpRestartOfAnAssociationWithNewAddressesErrorCause::Fuzz(
						  restartOfAnAssociationWithNewAddressesErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::USER_INITIATED_ABORT:
					{
						const auto* const userInitiatedAbortErrorCause =
						  static_cast<const RTC::SCTP::UserInitiatedAbortErrorCause*>(errorCause);

						FuzzerRtcSctpUserInitiatedAbortErrorCause::Fuzz(userInitiatedAbortErrorCause);

						break;
					}

					case RTC::SCTP::ErrorCause::ErrorCauseCode::PROTOCOL_VIOLATION:
					{
						const auto* const protocolViolationErrorCause =
						  static_cast<const RTC::SCTP::ProtocolViolationErrorCause*>(errorCause);

						FuzzerRtcSctpProtocolViolationErrorCause::Fuzz(protocolViolationErrorCause);

						break;
					}

					default:
					{
						const auto* const unknownErrorCause =
						  static_cast<const RTC::SCTP::UnknownErrorCause*>(errorCause);

						FuzzerRtcSctpUnknownErrorCause::Fuzz(unknownErrorCause);
					}
				}
			}
		}
	};

	std::unique_ptr<RTC::SCTP::Packet> packet{ RTC::SCTP::Packet::Parse(data, len) };

	if (!packet)
	{
		return;
	}

	fuzzChunks(packet.get());

	packet->Serialize(PacketSerializeBuffer, len);

	fuzzChunks(packet.get());

	packet->SetSourcePort(12345);
	packet->SetDestinationPort(54321);
	packet->SetVerificationTag(12345678);
	packet->ValidateCRC32cChecksum();
	packet->SetChecksum(999999);
	packet->WriteCRC32cChecksum();
	packet->ValidateCRC32cChecksum();

	const std::unique_ptr<RTC::SCTP::Packet> clonedPacket{ packet->Clone(PacketCloneBuffer, len) };

	packet.reset();

	fuzzChunks(clonedPacket.get());

	clonedPacket->SetSourcePort(12345);
	clonedPacket->SetDestinationPort(54321);
	clonedPacket->SetVerificationTag(12345678);
	clonedPacket->ValidateCRC32cChecksum();
	clonedPacket->SetChecksum(999999);
	clonedPacket->WriteCRC32cChecksum();
	clonedPacket->ValidateCRC32cChecksum();

	clonedPacket->Serialize(PacketSerializeBuffer, len);

	fuzzChunks(clonedPacket.get());
}
