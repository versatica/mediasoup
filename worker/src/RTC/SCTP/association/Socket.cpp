#define MS_CLASS "RTC::SCTP::Socket"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/SCTP/association/Socket.hpp"
#include "Logger.hpp"

namespace RTC
{
	namespace SCTP
	{
		/* Static. */

		// static constexpr size_t FactoryBufferLength{ 65536 };
		// thread_local static uint8_t FactoryBuffer[FactoryBufferLength];

		/* Instance methods. */

		Socket::Socket(Socket::SocketOptions options) : options(options)
		{
			MS_TRACE();
		}

		Socket::~Socket()
		{
			MS_TRACE();
		}

		void Socket::Dump(int indentation) const
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
