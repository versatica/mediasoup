#include "fct.h"
#include "Settings.h"

FCT_BGN()
{
	Settings::configuration.logLevel = LogLevel::LOG_WARN;

	FCTMF_SUITE_CALL(test_rtcp);
}
FCT_END();
