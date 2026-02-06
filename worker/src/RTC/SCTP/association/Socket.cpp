#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/Socket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Instance methods. */

		Socket::Socket(Socket::SocketOptions options) : options(options)
		{
			MS_TRACE();
		}

		Socket::~Socket()
		{
			MS_TRACE();
		}

		void Socket::Dump(int /*indentation*/) const
		{
			MS_TRACE();

			// TODO
		}

		void Socket::SendInitChunk()
		{
			MS_TRACE();

			// TODO
		}
	} // namespace SCTP
} // namespace RTC
