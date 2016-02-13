#define MS_CLASS "main"

#include "common.h"
#include "Settings.h"
#include "DepLibUV.h"
#include "DepOpenSSL.h"
#include "DepLibSRTP.h"
#include "DepUsrSCTP.h"
#include "Utils.h"
#include "Channel/UnixStreamSocket.h"
#include "RTC/UdpSocket.h"
#include "RTC/TcpServer.h"
#include "RTC/DtlsTransport.h"
#include "RTC/SrtpSession.h"
#include "Loop.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <map>
#include <string>
#include <iostream>  // std::cerr, std::endl
#include <cstdlib>  // std::_Exit(), std::genenv()
#include <csignal>  // sigaction()
#include <cerrno>
#include <unistd.h>  // getpid(), usleep()
#include <uv.h>

static void init();
static void ignoreSignals();
static void destroy();
static void exitSuccess();
static void exitWithError();

int main(int argc, char* argv[])
{
	// Ensure we are called by our Node library.
	if (argc == 1 || !std::getenv("MEDIASOUP_CHANNEL_FD"))
	{
		std::cerr << "ERROR: you don't seem to be my real father" << std::endl;
		std::_Exit(EXIT_FAILURE);
	}

	std::string id = std::string(argv[1]);
	int channelFd = std::stoi(std::getenv("MEDIASOUP_CHANNEL_FD"));

	// Initialize libuv stuff (we need it for the Channel).
	DepLibUV::ClassInit();

	// Set the Channel socket (this will be handled and deleted by the Loop).
	Channel::UnixStreamSocket* channel = new Channel::UnixStreamSocket(channelFd);

	// Initialize the Logger.
	Logger::Init(id, channel);

	// Setup the configuration.
	try
	{
		Settings::SetConfiguration(argc, argv);
	}
	catch (const MediaSoupError &error)
	{
		MS_ERROR("configuration error: %s", error.what());

		exitWithError();
	}

	// Print the effective configuration.
	Settings::PrintConfiguration();

	MS_DEBUG("starting " MS_PROCESS_NAME " [pid:%ld]", (long)getpid());

	#if defined(MS_LITTLE_ENDIAN)
		MS_DEBUG("detected Little-Endian CPU");
	#elif defined(MS_BIG_ENDIAN)
		MS_DEBUG("detected Big-Endian CPU");
	#endif

	#if defined(INTPTR_MAX) && defined(INT32_MAX) && (INTPTR_MAX == INT32_MAX)
		MS_DEBUG("detected 32 bits architecture");
	#elif defined(INTPTR_MAX) && defined(INT64_MAX) && (INTPTR_MAX == INT64_MAX)
		MS_DEBUG("detected 64 bits architecture");
	#else
		MS_WARN("cannot determine whether the architecture is 32 or 64 bits");
	#endif

	try
	{
		init();

		// Run the Loop.
		Loop loop(channel);

		destroy();

		MS_DEBUG_STD("success exit");
		exitSuccess();
	}
	catch (const MediaSoupError &error)
	{
		destroy();

		MS_ERROR_STD("failure exit: %s", error.what());
		exitWithError();
	}
}

void init()
{
	MS_TRACE();

	ignoreSignals();

	DepLibUV::PrintVersion();

	// Initialize static stuff.
	DepOpenSSL::ClassInit();
	DepLibSRTP::ClassInit();
	DepUsrSCTP::ClassInit();
	RTC::UdpSocket::ClassInit();
	RTC::TcpServer::ClassInit();
	RTC::DtlsTransport::ClassInit();
	RTC::SrtpSession::ClassInit();
	Utils::Crypto::ClassInit();
}

void ignoreSignals()
{
	MS_TRACE();

	int err;
	struct sigaction act;
	std::map<std::string, int> ignored_signals =
	{
		{ "PIPE", SIGPIPE },
		{ "HUP",  SIGHUP  },
		{ "ALRM", SIGALRM },
		{ "USR1", SIGUSR2 },
		{ "USR2", SIGUSR1 }
	};

	act.sa_handler = SIG_IGN;
	act.sa_flags = 0;
	err = sigfillset(&act.sa_mask);
	if (err)
		MS_THROW_ERROR("sigfillset() failed: %s", std::strerror(errno));

	for (auto it = ignored_signals.begin(); it != ignored_signals.end(); ++it)
	{
		std::string sig_name = it->first;
		int sig_id = it->second;

		err = sigaction(sig_id, &act, nullptr);
		if (err)
			MS_THROW_ERROR("sigaction() failed for signal %s: %s", sig_name.c_str(), std::strerror(errno));
	}
}

void destroy()
{
	MS_TRACE();

	// Free static stuff.
	DepLibUV::ClassDestroy();
	DepOpenSSL::ClassDestroy();
	DepLibSRTP::ClassDestroy();
	DepUsrSCTP::ClassDestroy();
	RTC::DtlsTransport::ClassDestroy();
	Utils::Crypto::ClassDestroy();
}

void exitSuccess()
{
	// Wait a bit so peding messages to stdout/Channel arrive to the main process.
	usleep(100000);

	// And exit with success status.
	std::_Exit(EXIT_SUCCESS);
}

void exitWithError()
{
	// Wait a bit so peding messages to stderr arrive to the main process.
	usleep(100000);

	// And exit with error status.
	std::_Exit(EXIT_FAILURE);
}
