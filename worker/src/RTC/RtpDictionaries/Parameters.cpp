#define MS_CLASS "RTC::Parameters"
// #define MS_LOG_DEV

#include "RTC/Parameters.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	Json::Value Parameters::toJson()
	{
		MS_TRACE();

		Json::Value json(Json::objectValue);

		for (auto& kv : this->mapKeyValues)
		{
			auto& key = kv.first;
			auto& value = kv.second;

			switch (value.type)
			{
				case Value::Type::BOOLEAN:
				{
					json[key] = value.booleanValue;

					break;
				}

				case Value::Type::INTEGER:
				{
					json[key] = (Json::Int)value.integerValue;

					break;
				}

				case Value::Type::DOUBLE:
				{
					json[key] = value.doubleValue;

					break;
				}

				case Value::Type::STRING:
				{
					json[key] = value.stringValue;

					break;
				}

				case Value::Type::ARRAY_OF_INTEGERS:
				{
					Json::Value array(Json::arrayValue);

					for (auto& entry : value.arrayOfIntegers)
						array.append((Json::Int)entry);

					json[key] = array;

					break;
				}
			}
		}

		return json;
	}

	void Parameters::Set(Json::Value& data)
	{
		MS_TRACE();

		MS_ASSERT(data.isObject(), "data is not a JSON object");

		for (Json::Value::iterator it = data.begin(); it != data.end(); it++)
		{
			std::string key = it.key().asString();
			Json::Value value = (*it);

			switch (value.type())
			{
				case Json::booleanValue:
				{
					bool booleanValue = value.asBool();

					this->mapKeyValues[key] = Value(booleanValue);

					break;
				}

				case Json::intValue:
				{
					int32_t integerValue = (int32_t)value.asInt();

					this->mapKeyValues[key] = Value(integerValue);

					break;
				}

				case Json::realValue:
				{
					double doubleValue = value.asDouble();

					this->mapKeyValues[key] = Value(doubleValue);

					break;
				}

				case Json::stringValue:
				{
					std::string stringValue = value.asString();

					this->mapKeyValues[key] = Value(stringValue);

					break;
				}

				case Json::arrayValue:
				{
					std::vector<int32_t> arrayOfIntegers;
					bool isValid = true;

					for (Json::UInt i = 0; i < value.size() && isValid; i++)
					{
						auto& entry = value[i];

						if (entry.isInt())
						{
							arrayOfIntegers.push_back((int32_t)entry.asInt());
						}
						else
						{
							isValid = false;
							break;
						}
					}

					if (arrayOfIntegers.size() > 0 && isValid)
						this->mapKeyValues[key] = Value(arrayOfIntegers);

					break;
				}

				default:
					; // Just ignore other value types.
			}
		}
	}

	bool Parameters::GetBoolean(std::string& key)
	{
		MS_TRACE();

		MS_ASSERT(this->mapKeyValues.find(key) != this->mapKeyValues.end(),
			"key does not exist [key:%s]", key.c_str());

		return this->mapKeyValues[key].booleanValue;
	}

	int32_t Parameters::GetInteger(std::string& key)
	{
		MS_TRACE();

		MS_ASSERT(this->mapKeyValues.find(key) != this->mapKeyValues.end(),
			"key does not exist [key:%s]", key.c_str());

		return this->mapKeyValues[key].integerValue;
	}

	double Parameters::GetDouble(std::string& key)
	{
		MS_TRACE();

		MS_ASSERT(this->mapKeyValues.find(key) != this->mapKeyValues.end(),
			"key does not exist [key:%s]", key.c_str());

		return this->mapKeyValues[key].doubleValue;
	}

	std::string& Parameters::GetString(std::string& key)
	{
		MS_TRACE();

		MS_ASSERT(this->mapKeyValues.find(key) != this->mapKeyValues.end(),
			"key does not exist [key:%s]", key.c_str());

		return this->mapKeyValues[key].stringValue;
	}

	std::vector<int32_t>& Parameters::GetArrayOfIntegers(std::string& key)
	{
		MS_TRACE();

		MS_ASSERT(this->mapKeyValues.find(key) != this->mapKeyValues.end(),
			"key does not exist [key:%s]", key.c_str());

		return this->mapKeyValues[key].arrayOfIntegers;
	}
}
