#ifndef MS_RTC_CODEC_MAANGER_H
#define MS_RTC_CODEC_MAANGER_H

#include "common.h"
#include <string>
#include <json/json.h>

namespace RTC
{
	class CodecManager
	{
	public:
		CodecManager();
		virtual ~CodecManager();

		Json::Value toJson();
	};
}

#endif
