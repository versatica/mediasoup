#define MS_CLASS "mediasoup-worker"
// #define MS_LOG_DEV_LEVEL 3

#include "Logger.hpp"
#include "lib.hpp"
#include <cstdlib> // std::_Exit()
#include <string>

static constexpr int ConsumerChannelFd{ 3 };
static constexpr int ProducerChannelFd{ 4 };

int main(int argc, char* argv[])
{
	// Ensure we are called by our Node library.
	if (!std::getenv("MEDIASOUP_VERSION"))
	{
		MS_ERROR_STD("you don't seem to be my real father!");

		// 41 is a custom exit code to notify about "missing MEDIASOUP_VERSION" env.
		std::_Exit(41);
	}

	const std::string version = std::getenv("MEDIASOUP_VERSION");

	const auto statusCode = mediasoup_worker_run(
	  /*argc*/ argc,
	  /*argv*/ argv,
	  /*version*/ version.c_str(),
	  /*consumerChannelFd*/ ConsumerChannelFd,
	  /*producerChannelFd*/ ProducerChannelFd,
	  /*channelReadFn*/ nullptr,
	  /*channelReadCtx*/ nullptr,
	  /*channelWriteFn*/ nullptr,
	  /*channelWriteCtx*/ nullptr);

	std::_Exit(statusCode);
}
