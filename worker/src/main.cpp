#define MS_CLASS "main"

#include "common.h"
#include "Settings.h"
#include "DepLibUV.h"
#include "DepOpenSSL.h"
#include "DepLibSRTP.h"
#include "DepUsrSCTP.h"
#include "Utils.h"
#include "Channel/UnixStreamSocket.h"
#include "RTC/UDPSocket.h"
#include "RTC/TCPServer.h"
#include "RTC/DTLSAgent.h"
#include "RTC/SRTPSession.h"
#include "Loop.h"
#include "MediaSoupError.h"
#include "Logger.h"
#include <map>
#include <string>
#include <cstdlib>  // std::_Exit(), std::genenv()
#include <csignal>  // sigaction()
#include <cerrno>
#include <unistd.h>  // getpid(), usleep()
#include <uv.h>

static void init();
static void ignoreSignals();
static void destroy();

int main(int argc, char* argv[], char** envp)
{
	// Initialize libuv stuff (we need it for the Channel).
	DepLibUV::ClassInit();

	int channelFd = std::stoi(std::getenv("MEDIASOUP_CHANNEL_FD"));

	// Set the Channel socket (this will be handled and deleted by the Loop).
	Channel::UnixStreamSocket* channel = new Channel::UnixStreamSocket(channelFd);

	std::string id = std::string(argv[1]);

	Logger::Init(id, channel);

	// Print libuv version.
	MS_DEBUG("loaded libuv version: %s", uv_version_string());

	try
	{
		Settings::SetConfiguration(argc, argv);
	}
	catch (const MediaSoupError &error)
	{
		MS_ERROR("failed to start: %s", error.what());
		usleep(100000);
		std::_Exit(EXIT_FAILURE);
	}

	// Print configuration.
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
		std::_Exit(EXIT_SUCCESS);
	}
	catch (const MediaSoupError &error)
	{
		destroy();
		MS_ERROR_STD("failure exit: %s", error.what());
		usleep(100000);
		std::_Exit(EXIT_FAILURE);
	}
}

void init()
{
	MS_TRACE();

	ignoreSignals();

	// Initialize static stuff.
	DepOpenSSL::ClassInit();
	DepLibSRTP::ClassInit();
	DepUsrSCTP::ClassInit();
	RTC::UDPSocket::ClassInit();
	RTC::TCPServer::ClassInit();
	RTC::DTLSAgent::ClassInit();
	RTC::SRTPSession::ClassInit();
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
	RTC::DTLSAgent::ClassDestroy();
	Utils::Crypto::ClassDestroy();
}
