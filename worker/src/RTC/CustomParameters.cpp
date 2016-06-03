#define MS_CLASS "RTC::CustomParameters"

#include "RTC/CustomParameters.h"
#include "Logger.h"

namespace RTC
{
	/* Global methods. */

	void FillCustomParameters(CustomParameters& parameters, Json::Value& data)
	{
		MS_TRACE();

		MS_ASSERT(data.isObject(), "data is not a JSON object");

		for (Json::Value::iterator it = data.begin(); it != data.end(); ++it)
		{
			std::string key = it.key().asString();
			Json::Value value = (*it);

			switch (value.type())
			{
				case Json::booleanValue:
				{
					bool booleanValue = value.asBool();

					parameters[key] = CustomParameterValue(booleanValue);

					break;
				}

				case Json::intValue:
				{
					int32_t signedIntegerValue = (int32_t)value.asInt();

					parameters[key] = CustomParameterValue(signedIntegerValue);

					break;
				}

				case Json::realValue:
				{
					double doubleValue = value.asDouble();

					parameters[key] = CustomParameterValue(doubleValue);

					break;
				}

				case Json::stringValue:
				{
					std::string stringValue = value.asString();

					parameters[key] = CustomParameterValue(stringValue);

					break;
				}

				default:
					;  // Just ignore other value types.
			}
		}
	}
}
