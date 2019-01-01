#define CATCH_CONFIG_RUNNER

#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "LogLevel.hpp"
#include "Logger.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "catch.hpp"
#include <string>

int main(int argc, char* argv[])
{
	Settings::configuration.logLevel = LogLevel::LOG_NONE;
	std::string loggerId             = "tests";

	// Initialize static stuff.
	Logger::ClassInit(loggerId);
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();

	int status = Catch::Session().run(argc, argv);

	// Free static stuff.
	DepLibUV::ClassDestroy();
	Utils::Crypto::ClassDestroy();

	return status;
}
