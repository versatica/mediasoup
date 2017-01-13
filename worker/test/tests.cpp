#include "fct.h"
#include "Settings.h"

FCT_BGN()
{
	Settings::configuration.logLevel = LogLevel::LOG_DEBUG;
	Settings::configuration.logTags.rtp = true;

	FCTMF_SUITE_CALL(test_rtcp);
	FCTMF_SUITE_CALL(test_rtp);
}
FCT_END();
