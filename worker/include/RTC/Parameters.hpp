#ifndef MS_RTC_PARAMETERS_HPP
#define MS_RTC_PARAMETERS_HPP

#include "common.hpp"
#include <json/json.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace RTC
{
	class Parameters
	{
	public:
		class Value
		{
		public:
			enum class Type
			{
				BOOLEAN = 1,
				INTEGER,
				DOUBLE,
				STRING,
				ARRAY_OF_INTEGERS
			};

		public:
			Value(){};

			explicit Value(bool booleanValue) : type(Type::BOOLEAN), booleanValue(booleanValue)
			{
			}

			explicit Value(int32_t integerValue) : type(Type::INTEGER), integerValue(integerValue)
			{
			}

			explicit Value(double doubleValue) : type(Type::DOUBLE), doubleValue(doubleValue)
			{
			}

			explicit Value(std::string& stringValue) : type(Type::STRING), stringValue(stringValue)
			{
			}

			explicit Value(std::vector<int32_t>& arrayOfIntegers)
			    : type(Type::ARRAY_OF_INTEGERS), arrayOfIntegers(arrayOfIntegers)
			{
			}

		public:
			Type type;
			bool booleanValue{ false };
			int32_t integerValue{ 0 };
			double doubleValue{ 0.0 };
			std::string stringValue;
			std::vector<int32_t> arrayOfIntegers;
		};

	public:
		Parameters(){};

		Json::Value ToJson() const;
		void Set(Json::Value& data);

		bool HasBoolean(const std::string& key);
		bool HasInteger(const std::string& key);
		bool HasDouble(const std::string& key);
		bool HasString(const std::string& key);
		bool HasArrayOfIntegers(const std::string& key);
		bool IncludesInteger(const std::string& key, int32_t integer);

		bool GetBoolean(const std::string& key);
		int32_t GetInteger(const std::string& key);
		double GetDouble(const std::string& key);
		std::string& GetString(const std::string& key);
		std::vector<int32_t>& GetArrayOfIntegers(const std::string& key);

		void SetBoolean(const std::string& key, bool booleanValue);
		void SetInteger(const std::string& key, int32_t integerValue);
		void SetDouble(const std::string& key, double doubleValue);
		void SetString(const std::string& key, std::string& stringValue);
		void SetArrayOfIntegers(const std::string& key, std::vector<int32_t>& arrayOfIntegers);

	private:
		std::unordered_map<std::string, Value> mapKeyValues;
	};

	/* Inline methods. */

	inline bool Parameters::HasBoolean(const std::string& key)
	{
		if (this->mapKeyValues.find(key) == this->mapKeyValues.end())
			return false;

		return this->mapKeyValues[key].type == Value::Type::BOOLEAN;
	}

	inline bool Parameters::HasInteger(const std::string& key)
	{
		if (this->mapKeyValues.find(key) == this->mapKeyValues.end())
			return false;

		return this->mapKeyValues[key].type == Value::Type::INTEGER;
	}

	inline bool Parameters::HasDouble(const std::string& key)
	{
		if (this->mapKeyValues.find(key) == this->mapKeyValues.end())
			return false;

		return this->mapKeyValues[key].type == Value::Type::DOUBLE;
	}

	inline bool Parameters::HasString(const std::string& key)
	{
		if (this->mapKeyValues.find(key) == this->mapKeyValues.end())
			return false;

		return this->mapKeyValues[key].type == Value::Type::STRING;
	}

	inline bool Parameters::HasArrayOfIntegers(const std::string& key)
	{
		if (this->mapKeyValues.find(key) == this->mapKeyValues.end())
			return false;

		return this->mapKeyValues[key].type == Value::Type::ARRAY_OF_INTEGERS;
	}

	inline bool Parameters::IncludesInteger(const std::string& key, int32_t integer)
	{
		if (this->mapKeyValues.find(key) == this->mapKeyValues.end())
			return false;

		auto& array = this->mapKeyValues[key].arrayOfIntegers;

		return std::find(array.begin(), array.end(), integer) != array.end();
	}

	inline void Parameters::SetBoolean(const std::string& key, bool booleanValue)
	{
		this->mapKeyValues[key] = Value(booleanValue);
	}

	inline void Parameters::SetInteger(const std::string& key, int32_t integerValue)
	{
		this->mapKeyValues[key] = Value(integerValue);
	}

	inline void Parameters::SetDouble(const std::string& key, double doubleValue)
	{
		this->mapKeyValues[key] = Value(doubleValue);
	}

	inline void Parameters::SetString(const std::string& key, std::string& stringValue)
	{
		this->mapKeyValues[key] = Value(stringValue);
	}

	inline void Parameters::SetArrayOfIntegers(const std::string& key, std::vector<int32_t>& arrayOfIntegers)
	{
		this->mapKeyValues[key] = Value(arrayOfIntegers);
	}
} // namespace RTC

#endif
