#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"
#include "RTC/SCTP/packet/Packet.hpp"

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
				 * Maximum size of a SCTP packet. It doesn't include any overhead of
				 * DTLS, TURN, UDP or IP headers.
				 */
				size_t mtu{ Socket::MaxSafeMTUSize };
				/**
				 * Maximum received window buffer size. It must be larger than the
				 * largest sized message we want to be able to receive.
				 *
				 * @remarks
				 * Default value copied from dcSCTP library.
				 */
				uint32_t myAdvertisedReceiverWindowCredit{ 5 * 1024 * 1024 };
			};

		public:
			/**
			 * Largest safe SCTP packet. Starting from the minimum guaranteed MTU value
			 * of 1280 for IPv6 (which may not support fragmentation), take off 85
			 * bytes for DTLS/TURN/TCP/IP and ciphertext overhead.
			 *
			 * Additionally, it's possible that TURN adds an additional 4 bytes of
			 * overhead after a channel has been established, so an additional 4 bytes
			 * is subtracted.
			 *
			 * 1280 IPV6 MTU
			 *  -40 IPV6 header
			 *   -8 UDP
			 *  -24 GCM Cipher
			 *  -13 DTLS record header
			 *   -4 TURN ChannelData
			 * = 1191 bytes.
			 *
			 * @remarks
			 * Value copied from dcSCTP library.
			 */
			static constexpr size_t MaxSafeMTUSize{ 1191 };

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
