#define MS_CLASS "mediasoup-worker"
// #define MS_LOG_DEV_LEVEL 3

#include "common.hpp"
#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepLibWebRTC.hpp"
#include "DepOpenSSL.hpp"
#include "DepUsrSCTP.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "Worker.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include "PayloadChannel/Notifier.hpp"
#include "PayloadChannel/UnixStreamSocket.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/SrtpSession.hpp"
#include <uv.h>
#include <cerrno>
#include <csignal>  // sigaction()
#include <cstdlib>  // std::_Exit(), std::genenv()
#include <iostream> // std::cerr, std::endl
#include <map>
#include <string>
#include <mutex>

/* Static. */

static std::once_flag globalInitOnce;

extern "C" int run_worker(
    int argc,
    char* argv[],
    char* version,
    int consumerChannelFd,
    int producerChannelFd,
    int payloadConsumeChannelFd,
    int payloadProduceChannelFd
)
{
	// Initialize libuv stuff (we need it for the Channel).
	DepLibUV::ClassInit();

	// Channel socket (it will be handled and deleted by the Worker).
	Channel::UnixStreamSocket* channel{ nullptr };

	// PayloadChannel socket (it will be handled and deleted by the Worker).
	PayloadChannel::UnixStreamSocket* payloadChannel{ nullptr };

	try
	{
		channel = new Channel::UnixStreamSocket(consumerChannelFd, producerChannelFd);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("error creating the Channel: %s", error.what());

		return 1;
	}

	try
	{
		payloadChannel =
		  new PayloadChannel::UnixStreamSocket(payloadConsumeChannelFd, payloadProduceChannelFd);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("error creating the RTC Channel: %s", error.what());

		return 1;
	}

	// Initialize the Logger.
	Logger::ClassInit(channel);

	try
	{
		Settings::SetConfiguration(argc, argv);
	}
	catch (const MediaSoupTypeError& error)
	{
		MS_ERROR_STD("settings error: %s", error.what());

		// 42 is a custom exit code to notify "settings error" to the Node library.
		return 42;
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("unexpected settings error: %s", error.what());

		return 1;
	}

	MS_DEBUG_TAG(info, "starting mediasoup-worker process [version:%s]", version);

#if defined(MS_LITTLE_ENDIAN)
	MS_DEBUG_TAG(info, "little-endian CPU detected");
#elif defined(MS_BIG_ENDIAN)
	MS_DEBUG_TAG(info, "big-endian CPU detected");
#else
	MS_WARN_TAG(info, "cannot determine whether little-endian or big-endian");
#endif

#if defined(INTPTR_MAX) && defined(INT32_MAX) && (INTPTR_MAX == INT32_MAX)
	MS_DEBUG_TAG(info, "32 bits architecture detected");
#elif defined(INTPTR_MAX) && defined(INT64_MAX) && (INTPTR_MAX == INT64_MAX)
	MS_DEBUG_TAG(info, "64 bits architecture detected");
#else
	MS_WARN_TAG(info, "cannot determine 32 or 64 bits architecture");
#endif

	Settings::PrintConfiguration();
	DepLibUV::PrintVersion();

	try
	{
		std::call_once(globalInitOnce, []{
			// Initialize global static stuff once.
			DepOpenSSL::ClassInit();
			DepLibSRTP::ClassInit();
			DepLibWebRTC::ClassInit();
		});

		// Initialize static stuff.
		DepUsrSCTP::ClassInit();
		Utils::Crypto::ClassInit();
		RTC::DtlsTransport::ClassInit();
		RTC::SrtpSession::ClassInit();
		Channel::Notifier::ClassInit(channel);
		PayloadChannel::Notifier::ClassInit(payloadChannel);

		// Run the Worker.
		Worker worker(channel, payloadChannel, false);

		// Free static stuff.
		DepLibUV::ClassDestroy();
		Utils::Crypto::ClassDestroy();
		RTC::DtlsTransport::ClassDestroy();
		DepUsrSCTP::ClassDestroy();

		// Wait a bit so peding messages to stdout/Channel arrive to the Node
		// process.
		uv_sleep(200);

		return 0;
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("failure exit: %s", error.what());

		return 1;
	}
}
