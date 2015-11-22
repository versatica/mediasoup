#define MS_CLASS "RTC::TCPConnection"

#include "RTC/TCPConnection.h"
#include "RTC/STUNMessage.h"
#include "RTC/DTLSHandler.h"
#include "RTC/RTPPacket.h"
#include "RTC/RTCPPacket.h"
#include "Utils.h"
#include "Logger.h"
#include <cstring>  // std::memmove()

namespace RTC
{
	/* Instance methods. */

	TCPConnection::TCPConnection(Reader* reader, size_t bufferSize) :
		::TCPConnection(bufferSize),
		reader(reader)
	{
		MS_TRACE();
	}

	TCPConnection::~TCPConnection()
	{
		MS_TRACE();
	}

	void TCPConnection::userOnTCPConnectionRead(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		MS_DEBUG("%zu bytes received [local: %s : %u | remote: %s : %u]", len,
			GetLocalIP().c_str(), (unsigned int)GetLocalPort(), GetPeerIP().c_str(), (unsigned int)GetPeerPort());

		/*
		 * Framing RFC 4571
		 *
		 *     0                   1                   2                   3
		 *     0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 *     ---------------------------------------------------------------
		 *     |             LENGTH            |  STUN / DTLS / RTP / RTCP   |
		 *     ---------------------------------------------------------------
		 *
		 * A 16-bit unsigned integer LENGTH field, coded in network byte order
		 * (big-endian), begins the frame.  If LENGTH is non-zero, an RTP or
		 * RTCP packet follows the LENGTH field.  The value coded in the LENGTH
		 * field MUST equal the number of octets in the RTP or RTCP packet.
		 * Zero is a valid value for LENGTH, and it codes the null packet.
		 */

		// Be ready to parse more than a single frame in a single TCP chunk.
		while (true)
		{
			size_t data_len = this->bufferDataLen - this->frameStart;
			size_t packet_len;

			if (data_len >= 2)
				packet_len = (size_t)Utils::Byte::Get2Bytes(this->buffer + this->frameStart, 0);

			// We have packet_len bytes.
			if (data_len >= 2 && data_len >= 2 + packet_len)
			{
				const MS_BYTE* packet = this->buffer + this->frameStart + 2;

				// Ignore if zero-length.
				if (packet_len == 0)
				{
					MS_DEBUG("ignoring 0 length received frame");
				}
				// Check if it's STUN.
				else if (STUNMessage::IsSTUN(packet, packet_len))
				{
					this->reader->onSTUNDataRecv(this, packet, packet_len);
				}
				// Check if it's RTCP.
				else if (RTCPPacket::IsRTCP(packet, packet_len))
				{
					this->reader->onRTCPDataRecv(this, packet, packet_len);
				}
				// Check if it's RTP.
				else if (RTPPacket::IsRTP(packet, packet_len))
				{
					this->reader->onRTPDataRecv(this, packet, packet_len);
				}
				// Check if it's DTLS.
				else if (DTLSHandler::IsDTLS(packet, packet_len))
				{
					this->reader->onDTLSDataRecv(this, packet, packet_len);
				}
				else
				{
					MS_DEBUG("packet of unknown type received, closing the connection");

					// Close the connection.
					Close();

					// Exit the parsing loop.
					break;
				}

				// If there is no more space available in the buffer and that is because
				// the latest parsed frame filled it, then empty the full buffer.
				if ((this->frameStart + 2 + packet_len) == this->bufferSize)
				{
					MS_DEBUG("no more space in the buffer, emptying the buffer data");

					this->frameStart = 0;
					this->bufferDataLen = 0;
				}
				// If there is still space in the buffer, set the beginning of the next
				// frame to the next position after the parsed frame.
				else
				{
					this->frameStart += 2 + packet_len;
				}

				// If there is more data in the buffer after the parsed frame then
				// parse again. Otherwise break here and wait for more data.
				if (this->bufferDataLen > this->frameStart)
				{
					MS_DEBUG("there is more data after the parsed frame, continue parsing");
					continue;
				}
				else
				{
					break;
				}
			}

			// Incomplete packet.
			else
			{
				// Check if the buffer is full.
				if (this->bufferDataLen == this->bufferSize)
				{
					// First case: the uncomplete frame does not begin at position 0 of
					// the buffer, so move the frame to the position 0.
					if (this->frameStart != 0)
					{
						MS_DEBUG("no more space in the buffer, moving parsed bytes to the beginning of the buffer and wait for more data");

						std::memmove(this->buffer, this->buffer + this->frameStart, this->bufferSize - this->frameStart);
						this->bufferDataLen = this->bufferSize - this->frameStart;
						this->frameStart = 0;
					}
					// Second case: the uncomplete frame begins at position 0 of the buffer.
					// The frame is too big, so close the connection.
					else
					{
						MS_ERROR("no more space in the buffer for the unfinished frame being parsed, closing the connection");

						// Close the socket.
						Close();
					}
				}
				// The buffer is not full.
				else
				{
					MS_DEBUG("frame not finished yet, waiting for more data");
				}

				// Exit the parsing loop.
				break;
			}
		}  // while()
	}

	void TCPConnection::Send(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		// Write according to Framing RFC 4571.

		MS_BYTE frame_len[2];

		Utils::Byte::Set2Bytes(frame_len, 0, len);

		Write(frame_len, 2, data, len);
	}
}  // namespace RTC
