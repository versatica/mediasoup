#define CATCH_CONFIG_RUNNER

#include "DepLibUV.hpp"
#include "DepOpenSSL.hpp"
#include "LogLevel.hpp"
#include "Settings.hpp"
#include "Utils.hpp"
#include "catch.hpp"
#include <cstdlib> // std::getenv()

int main(int argc, char* argv[])
{
	LogLevel logLevel{ LogLevel::LOG_NONE };

	// Get logLevel from ENV variable.
	if (std::getenv("MS_TEST_LOG_LEVEL"))
	{
		if (std::string(std::getenv("MS_TEST_LOG_LEVEL")) == "debug")
			logLevel = LogLevel::LOG_DEBUG;
		else if (std::string(std::getenv("MS_TEST_LOG_LEVEL")) == "warn")
			logLevel = LogLevel::LOG_WARN;
		else if (std::string(std::getenv("MS_TEST_LOG_LEVEL")) == "error")
			logLevel = LogLevel::LOG_ERROR;
	}

	Settings::configuration.logLevel = logLevel;

	// Initialize static stuff.
	DepLibUV::ClassInit();
	DepOpenSSL::ClassInit();
	Utils::Crypto::ClassInit();

	int status = Catch::Session().run(argc, argv);

	// Free static stuff.
	DepLibUV::ClassDestroy();
	Utils::Crypto::ClassDestroy();

	return status;
}
