#define MS_CLASS "mediasoup-worker"
// #define MS_LOG_DEV

#include "common.hpp"
#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "Logger.hpp"
#include "MediaSoupErrors.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "Worker.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/UnixStreamSocket.hpp"
#include "RTC/DtlsTransport.hpp"
#include "RTC/SrtpSession.hpp"
#include <cerrno>
#include <csignal>  // sigaction()
#include <cstdlib>  // std::_Exit(), std::genenv()
#include <iostream> // std::cerr, std::endl
#include <map>
#include <string>
#include <unistd.h> // usleep()

static constexpr int ChannelFd{ 3 };

void IgnoreSignals();

int main(int argc, char* argv[])
{
	// Ensure we are called by our Node library.
	if (std::getenv("MEDIASOUP_VERSION") == nullptr)
	{
		MS_ERROR_STD("you don't seem to be my real father!");

		std::_Exit(EXIT_FAILURE);
	}

	std::string version = std::getenv("MEDIASOUP_VERSION");

	// Initialize libuv stuff (we need it for the Channel).
	DepLibUV::ClassInit();

	// Channel socket (it will be handled and deleted by the Worker).
	Channel::UnixStreamSocket* channel{ nullptr };

	try
	{
		channel = new Channel::UnixStreamSocket(ChannelFd);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("error creating the Channel: %s", error.what());

		std::_Exit(EXIT_FAILURE);
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
		std::_Exit(42);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("unexpected settings error: %s", error.what());

		std::_Exit(EXIT_FAILURE);
	}

	MS_DEBUG_TAG(info, "starting mediasoup-worker process [version:%s]", version.c_str());

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
		// Initialize static stuff.
		DepOpenSSL::ClassInit();
		DepLibSRTP::ClassInit();
		Utils::Crypto::ClassInit();
		RTC::DtlsTransport::ClassInit();
		RTC::SrtpSession::ClassInit();
		Channel::Notifier::ClassInit(channel);

		// Ignore some signals.
		IgnoreSignals();

		// Run the Worker.
		Worker worker(channel);

		// Free static stuff.
		DepLibUV::ClassDestroy();
		DepLibSRTP::ClassDestroy();
		Utils::Crypto::ClassDestroy();
		RTC::DtlsTransport::ClassDestroy();

		// Wait a bit so peding messages to stdout/Channel arrive to the Node
		// process.
		usleep(200000);
		std::_Exit(EXIT_SUCCESS);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("failure exit: %s", error.what());

		std::_Exit(EXIT_FAILURE);
	}
}

void IgnoreSignals()
{
	MS_TRACE();

	int err;
	struct sigaction act; // NOLINT(cppcoreguidelines-pro-type-member-init)

	// clang-format off
	std::map<std::string, int> ignoredSignals =
	{
		{ "PIPE", SIGPIPE },
		{ "HUP",  SIGHUP  },
		{ "ALRM", SIGALRM },
		{ "USR1", SIGUSR1 },
		{ "USR2", SIGUSR2 }
	};
	// clang-format on

	act.sa_handler = SIG_IGN; // NOLINT(cppcoreguidelines-pro-type-cstyle-cast)
	act.sa_flags   = 0;
	err            = sigfillset(&act.sa_mask);

	if (err != 0)
		MS_THROW_ERROR("sigfillset() failed: %s", std::strerror(errno));

	for (auto& kv : ignoredSignals)
	{
		auto& sigName = kv.first;
		int sigId     = kv.second;

		err = sigaction(sigId, &act, nullptr);

		if (err != 0)
			MS_THROW_ERROR("sigaction() failed for signal %s: %s", sigName.c_str(), std::strerror(errno));
	}
}
