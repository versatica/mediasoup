#define CATCH_CONFIG_RUNNER

#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "LogLevel.hpp"
#include "Logger.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "include/catch.hpp"
#include <string>

static void init();
static void destroy();

int main(int argc, char* argv[])
{
	Settings::configuration.logLevel = LogLevel::LOG_DEBUG;

	std::string loggerId = "tests";

	// Initialize the Logger.
	Logger::Init(loggerId);

	init();

	int ret = Catch::Session().run(argc, argv);

	destroy();

	return ret;
}

void init()
{
	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();
}

void destroy()
{
	// Free static stuff.
	Utils::Crypto::ClassDestroy();
	DepLibUV::ClassDestroy();
}
