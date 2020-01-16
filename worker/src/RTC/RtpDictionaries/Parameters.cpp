#define MS_CLASS "RTC::Parameters"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/Parameters.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	void Parameters::FillJson(json& jsonObject) const
	{
		MS_TRACE();

		// Force it to be an object even if no key/values are added below.
		jsonObject = json::object();

		for (auto& kv : this->mapKeyValues)
		{
			auto& key   = kv.first;
			auto& value = kv.second;

			switch (value.type)
			{
				case Value::Type::BOOLEAN:
					jsonObject[key] = value.booleanValue;
					break;

				case Value::Type::INTEGER:
					jsonObject[key] = value.integerValue;
					break;

				case Value::Type::DOUBLE:
					jsonObject[key] = value.doubleValue;
					break;

				case Value::Type::STRING:
					jsonObject[key] = value.stringValue;
					break;

				case Value::Type::ARRAY_OF_INTEGERS:
					jsonObject[key] = value.arrayOfIntegers;
					break;
			}
		}
	}

	void Parameters::Set(json& data)
	{
		MS_TRACE();

		MS_ASSERT(data.is_object(), "data is not an object");

		for (json::iterator it = data.begin(); it != data.end(); ++it)
		{
			const std::string& key = it.key();
			auto& value            = it.value();

			switch (value.type())
			{
				case json::value_t::boolean:
				{
					this->mapKeyValues.emplace(key, Value(value.get<bool>()));

					break;
				}

				case json::value_t::number_integer:
				case json::value_t::number_unsigned:
				{
					this->mapKeyValues.emplace(key, Value(value.get<int32_t>()));

					break;
				}

				case json::value_t::number_float:
				{
					this->mapKeyValues.emplace(key, Value(value.get<double>()));

					break;
				}

				case json::value_t::string:
				{
					this->mapKeyValues.emplace(key, Value(value.get<std::string>()));

					break;
				}

				case json::value_t::array:
				{
					std::vector<int32_t> arrayOfIntegers;
					bool isValid = true;

					for (auto& entry : value)
					{
						if (!entry.is_number_integer())
						{
							isValid = false;

							break;
						}

						arrayOfIntegers.emplace_back(entry.get<int32_t>());
					}

					if (!arrayOfIntegers.empty() && isValid)
						this->mapKeyValues.emplace(key, Value(arrayOfIntegers));

					break;
				}

				default:; // Just ignore other value types.
			}
		}
	}

	bool Parameters::HasBoolean(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		if (it == this->mapKeyValues.end())
			return false;

		auto& value = it->second;

		return value.type == Value::Type::BOOLEAN;
	}

	bool Parameters::HasInteger(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		if (it == this->mapKeyValues.end())
			return false;

		auto& value = it->second;

		return value.type == Value::Type::INTEGER;
	}

	bool Parameters::HasPositiveInteger(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		if (it == this->mapKeyValues.end())
			return false;

		auto& value = it->second;

		return value.type == Value::Type::INTEGER && value.integerValue >= 0;
	}

	bool Parameters::HasDouble(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		if (it == this->mapKeyValues.end())
			return false;

		auto& value = it->second;

		return value.type == Value::Type::DOUBLE;
	}

	bool Parameters::HasString(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		if (it == this->mapKeyValues.end())
			return false;

		auto& value = it->second;

		return value.type == Value::Type::STRING;
	}

	bool Parameters::HasArrayOfIntegers(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		if (it == this->mapKeyValues.end())
			return false;

		auto& value = it->second;

		return value.type == Value::Type::ARRAY_OF_INTEGERS;
	}

	bool Parameters::IncludesInteger(const std::string& key, int32_t integer) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		if (it == this->mapKeyValues.end())
			return false;

		auto& value = it->second;
		auto& array = value.arrayOfIntegers;

		return std::find(array.begin(), array.end(), integer) != array.end();
	}

	bool Parameters::GetBoolean(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		MS_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

		auto& value = it->second;

		return value.booleanValue;
	}

	int32_t Parameters::GetInteger(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		MS_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

		auto& value = it->second;

		return value.integerValue;
	}

	double Parameters::GetDouble(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		MS_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

		auto& value = it->second;

		return value.doubleValue;
	}

	const std::string& Parameters::GetString(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		MS_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

		auto& value = it->second;

		return value.stringValue;
	}

	const std::vector<int32_t>& Parameters::GetArrayOfIntegers(const std::string& key) const
	{
		MS_TRACE();

		auto it = this->mapKeyValues.find(key);

		MS_ASSERT(it != this->mapKeyValues.end(), "key does not exist [key:%s]", key.c_str());

		auto& value = it->second;

		return value.arrayOfIntegers;
	}
} // namespace RTC
