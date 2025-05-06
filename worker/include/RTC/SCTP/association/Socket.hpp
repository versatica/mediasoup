#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Packet.hpp"
#include "RTC/SCTP/packet/parameters/ZeroChecksumAcceptableParameter.hpp"

namespace RTC
{
	namespace SCTP
	{
		/**
		 * The SCTP Socket class represents the mediasoup side of an SCTP
		 * association with a peer.
		 *
		 * It manages all Packet and Chunk dispatching and the connection flow.
		 */
		class Socket
		{
		public:
			struct SocketOptions
			{
				/**
				 * Signaled local port.
				 */
				uint16_t localPort{ 0 };
				/**
				 * Signaled destination port.
				 */
				uint16_t destinationPort{ 0 };
				/**
				 * Announced maximum number of outbound streams (OS).
				 * NOTE: We use maximum value by default.
				 */
				uint16_t maxOutboundStreams{ 65535 };
				/**
				 * Announced maximum number of inbound streams (MIS).
				 * NOTE: We use maximum value by default.
				 */
				uint16_t maxInboundStreams{ 65535 };
				/**
				 * Maximum received window buffer size. It must be larger than the
				 * largest sized message we want to be able to receive.
				 *
				 * @remarks
				 * Default value copied from dcSCTP library.
				 */
				uint32_t myAdvertisedReceiverWindowCredit{ 5 * 1024 * 1024 };
				/**
				 * Use Partial Reliability Extension.
				 * @see RFC 3758.
				 */
				bool partialReliability{ false };
				/**
				 * Use Stream Schedulers and User Message Interleaving (I-DATA Chunks).
				 * @see RFC 8260.
				 */
				bool messageInterleaving{ false };
				/**
				 * Alternate error detection method for Zero Checksum.
				 *
				 * @remarks
				 * This feature is only enabled if both peers signal their wish to use
				 * the same (non-zero) Zero Checksum Alternate Error Detection Method.
				 *
				 * @see RFC 9653.
				 */
				ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod zeroCheksumAlternateErrorDetectionMethod{
					ZeroChecksumAcceptableParameter::AlternateErrorDetectionMethod::NONE
				};
				/**
				 * Maximum size of a SCTP packet. It doesn't include any overhead of
				 * DTLS, TURN, UDP or IP headers.
				 */
				size_t mtu{ RTC::Consts::MaxSafeMtuSizeForSctp };
			};

		public:
			explicit Socket(SocketOptions options);

			~Socket();

			void Dump(int indentation = 0) const;

		private:
			void SendInitChunk();

		private:
			// Socket options given in th econstructor.
			Socket::SocketOptions options;
			// Once the SCTP association is established a Transmission Control Block
			// is created (and also when we are the initiator of the association).
			std::unique_ptr<TransmissionControlBlock> tcb;
		};
	} // namespace SCTP
} // namespace RTC

#endif
