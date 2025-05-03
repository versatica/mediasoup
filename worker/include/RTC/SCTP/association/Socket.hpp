#ifndef MS_RTC_SCTP_SOCKET_HPP
#define MS_RTC_SCTP_SOCKET_HPP

#include "common.hpp"
#include "RTC/SCTP/association/TransmissionControlBlock.hpp"

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
			// TODO
			Socket();

			~Socket();

			void Dump(int indentation = 0) const;

		private:
			// Once the SCTP association is established a Transmission Control Block
			// is created (or also when we are the initiators of the association).
			std::unique_ptr<TransmissionControlBlock> tcb;
		};
	} // namespace SCTP
} // namespace RTC

#endif
