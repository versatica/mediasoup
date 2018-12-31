#define MS_CLASS "main"
// #define MS_LOG_DEV

#include "common.hpp"
#include "DepLibSRTP.hpp"
#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "Logger.hpp"
#include "MediaSoupError.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
// #include "Worker.hpp"
#include "Channel/Notifier.hpp"
#include "Channel/UnixStreamSocket.hpp"
// #include "RTC/DtlsTransport.hpp"
#include "RTC/SrtpSession.hpp"
#include <cerrno>
#include <csignal>  // sigaction()
#include <cstdlib>  // std::_Exit()
#include <iostream> // std::cerr, std::endl
#include <map>
#include <string>
#include <unistd.h> // getpid(), usleep()

static constexpr int ChannelFd{ 3 };

void ignoreSignals();

int main(int argc, char* argv[])
{
	// Ensure we are called by our Node library.
	if (argc == 1)
	{
		std::cerr << "ERROR: U AIN'T MY REAL FATHER" << std::endl;

		std::_Exit(1);
	}

	std::string workerId = std::string(argv[1]);

	// Initialize libuv stuff (we need it for the Channel).
	DepLibUV::ClassInit();

	// Create the Channel socket (it will be handled and deleted by the Worker).
	auto* channel = new Channel::UnixStreamSocket(ChannelFd);

	// Initialize the Logger.
	Logger::ClassInit(workerId, channel);

	MS_DEBUG_TAG(info, "starting mediasoup-worker [pid:%ld]", (long)getpid());

#if defined(MS_LITTLE_ENDIAN)
	MS_DEBUG_TAG(info, "Little-Endian CPU detected");
#elif defined(MS_BIG_ENDIAN)
	MS_DEBUG_TAG(info, "Big-Endian CPU detected");
#endif

#if defined(INTPTR_MAX) && defined(INT32_MAX) && (INTPTR_MAX == INT32_MAX)
	MS_DEBUG_TAG(info, "32 bits architecture detected");
#elif defined(INTPTR_MAX) && defined(INT64_MAX) && (INTPTR_MAX == INT64_MAX)
	MS_DEBUG_TAG(info, "64 bits architecture detected");
#else
	MS_WARN_TAG(info, "can not determine whether the architecture is 32 or 64 bits");
#endif

	try
	{
		Settings::SetConfiguration(argc, argv);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("settings error: %s", error.what());

		std::_Exit(42);
	}

	Settings::PrintConfiguration();
	DepLibUV::PrintVersion();

	try
	{
		DepLibUV::PrintVersion();

		// Initialize static stuff.
		DepOpenSSL::ClassInit();
		DepLibSRTP::ClassInit();
		Utils::Crypto::ClassInit();
		// RTC::DtlsTransport::ClassInit();
		RTC::SrtpSession::ClassInit();
		Channel::Notifier::ClassInit(channel);

		// Ignore some signals.
		ignoreSignals();

		// Run the Worker.
		// Worker worker(channel);

		// TODO: REMOVE:
		Channel::Notifier::Emit(workerId, "running");

		// Free static stuff.
		DepLibUV::ClassDestroy();
		DepLibSRTP::ClassDestroy();
		Utils::Crypto::ClassDestroy();
		// RTC::DtlsTransport::ClassDestroy();

		// Wait a bit so peding messages to stdout/Channel arrive to the Node
		// process.
		usleep(200000);

		std::_Exit(EXIT_SUCCESS);
	}
	catch (const MediaSoupError& error)
	{
		MS_ERROR_STD("failure exit: %s", error.what());

		std::_Exit(1);
	}
}

void ignoreSignals()
{
	MS_TRACE();

	int err;
	struct sigaction act;
	// clang-format off
	std::map<std::string, int> ignoredSignals =
	{
		{ "PIPE", SIGPIPE },
		{ "HUP",  SIGHUP  },
		{ "ALRM", SIGALRM },
		{ "USR1", SIGUSR2 },
		{ "USR2", SIGUSR1}
	};
	// clang-format on

	// Ignore clang-tidy cppcoreguidelines-pro-type-cstyle-cast.
	act.sa_handler = SIG_IGN; // NOLINT
	act.sa_flags   = 0;
	err            = sigfillset(&act.sa_mask);

	if (err != 0)
		MS_THROW_ERROR("sigfillset() failed: %s", std::strerror(errno));

	for (auto& ignoredSignal : ignoredSignals)
	{
		auto& sigName = ignoredSignal.first;
		int sigId     = ignoredSignal.second;

		err = sigaction(sigId, &act, nullptr);

		if (err != 0)
		{
			MS_THROW_ERROR("sigaction() failed for signal %s: %s", sigName.c_str(), std::strerror(errno));
		}
	}
}
