#define MS_CLASS "RTC::CodecManager"

#include "RTC/CodecManager.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	CodecManager::CodecManager()
	{
		MS_TRACE();
	}

	CodecManager::~CodecManager()
	{
		MS_TRACE();
	}

	Json::Value CodecManager::toJson()
	{
		MS_TRACE();

		// TODO

		Json::Value json(Json::objectValue);

		return json;
	}
}
