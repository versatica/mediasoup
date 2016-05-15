#define MS_CLASS "RTC::CustomParameterValue"

#include "RTC/RtpParameters.h"
#include "MediaSoupError.h"
#include "Logger.h"

namespace RTC
{
	CustomParameterValue::CustomParameterValue(bool booleanValue) :
		type(Type::BOOLEAN),
		booleanValue(booleanValue)
	{
		MS_TRACE();
	}

	CustomParameterValue::CustomParameterValue(uint32_t integerValue) :
		type(Type::INTEGER),
		integerValue(integerValue)
	{
		MS_TRACE();
	}

	CustomParameterValue::CustomParameterValue(double doubleValue) :
		type(Type::DOUBLE),
		doubleValue(doubleValue)
	{
		MS_TRACE();
	}

	CustomParameterValue::CustomParameterValue(std::string& stringValue) :
		type(Type::STRING),
		stringValue(stringValue)
	{
		MS_TRACE();
	}

	CustomParameterValue::~CustomParameterValue()
	{
		MS_TRACE();
	}
}
