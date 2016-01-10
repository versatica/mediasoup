#ifndef MS_RTC_STUN_MESSAGE_H
#define MS_RTC_STUN_MESSAGE_H

#include "common.h"
#include <string>

namespace RTC
{
	class STUNMessage
	{
	public:
		// STUN message class.
		enum class Class : MS_2BYTES
		{
			Request         = 0,
			Indication      = 1,
			SuccessResponse = 2,
			ErrorResponse   = 3
		};

		// STUN message method.
		enum class Method : MS_2BYTES
		{
			Binding = 1
		};

		// Attribute type.
		enum class Attribute : MS_2BYTES
		{
			MappedAddress     = 0x0001,
			Username          = 0x0006,
			MessageIntegrity  = 0x0008,
			ErrorCode         = 0x0009,
			UnknownAttributes = 0x000A,
			Realm             = 0x0014,
			Nonce             = 0x0015,
			XorMappedAddress  = 0x0020,
			Priority          = 0x0024,
			UseCandidate      = 0x0025,
			Software          = 0x8022,
			AlternateServer   = 0x8023,
			Fingerprint       = 0x8028,
			IceControlled     = 0x8029,
			IceControlling    = 0x802A
		};

		// Authentication result.
		enum class Authentication
		{
			OK           = 0,
			Unauthorized = 1,
			BadRequest   = 2
		};

	public:
		static bool IsSTUN(const MS_BYTE* data, size_t len);
		static STUNMessage* Parse(const MS_BYTE* data, size_t len);

	private:
		static const MS_BYTE magicCookie[];

	public:
		STUNMessage(Class klass, Method method, const MS_BYTE* transactionId, const MS_BYTE* raw, size_t length);
		~STUNMessage();

		void Dump();
		Class GetClass();
		Method GetMethod();
		const MS_BYTE* GetRaw();
		void SetLength(size_t length);
		size_t GetLength();
		void SetUsername(const char* username, size_t len);
		void SetPriority(const MS_4BYTES priority);
		void SetIceControlling(const MS_8BYTES iceControlling);
		void SetIceControlled(const MS_8BYTES iceControlled);
		void SetUseCandidate();
		void SetXorMappedAddress(const struct sockaddr* xorMappedAddress);
		void SetErrorCode(MS_2BYTES errorCode);
		void SetMessageIntegrity(const MS_BYTE* messageIntegrity);
		void SetFingerprint();
		const std::string& GetUsername();
		const MS_4BYTES GetPriority();
		const MS_8BYTES GetIceControlling();
		const MS_8BYTES GetIceControlled();
		bool HasUseCandidate();
		MS_2BYTES GetErrorCode();
		bool HasMessageIntegrity();
		bool HasFingerprint();
		Authentication CheckAuthentication(const std::string &local_username, const std::string &local_password);
		STUNMessage* CreateSuccessResponse();
		STUNMessage* CreateErrorResponse(MS_2BYTES errorCode);
		void Authenticate(const std::string &password);
		void Serialize();

	private:
		// Passed by argument.
		Class klass;  // 2 bytes.
		Method method;  // 2 bytes.
		const MS_BYTE* transactionId = nullptr;  // 12 bytes.
		MS_BYTE* raw = nullptr;  // Allocated when Serialize().
		size_t length = 0;  // The full message size (including header).
		// Others.
		bool isSerialized = false;
		// STUN attributes.
		std::string username;  // Less than 513 bytes.
		MS_4BYTES priority = 0;  // 4 bytes unsigned integer.
		MS_8BYTES iceControlling = 0;  // 8 bytes unsigned integer.
		MS_8BYTES iceControlled = 0;  // 8 bytes unsigned integer.
		bool hasUseCandidate = false;  // 0 bytes.
		const MS_BYTE* messageIntegrity = nullptr;  // 20 bytes.
		bool hasFingerprint = false;  // 4 bytes.
		const struct sockaddr* xorMappedAddress = nullptr;  // 8 or 20 bytes.
		MS_2BYTES errorCode = 0;  // 4 bytes (no reason phrase).
		std::string password;
	};

	/* Inline methods. */

	inline
	bool STUNMessage::IsSTUN(const MS_BYTE* data, size_t len)
	{
		return (
			// STUN headers are 20 bytes.
			(len >= 20) &&
			// DOC: https://tools.ietf.org/html/draft-petithuguenin-avtcore-rfc5764-mux-fixes-00
			(data[0] < 20) &&
			// Magic cookie must match.
			(data[4] == STUNMessage::magicCookie[0]) &&
			(data[5] == STUNMessage::magicCookie[1]) &&
			(data[6] == STUNMessage::magicCookie[2]) &&
			(data[7] == STUNMessage::magicCookie[3])
		);
	}

	inline
	STUNMessage::Class STUNMessage::GetClass()
	{
		return this->klass;
	}

	inline
	STUNMessage::Method STUNMessage::GetMethod()
	{
		return this->method;
	}

	inline
	const MS_BYTE* STUNMessage::GetRaw()
	{
		return this->raw;
	}

	inline
	void STUNMessage::SetLength(size_t length)
	{
		this->length = length;
	}

	inline
	size_t STUNMessage::GetLength()
	{
		return this->length;
	}

	inline
	void STUNMessage::SetUsername(const char* username, size_t len)
	{
		if (this->username.empty())
			this->username = std::string(username, len);
	}

	inline
	void STUNMessage::SetPriority(const MS_4BYTES priority)
	{
		if (!this->priority)
			this->priority = priority;
	}

	inline
	void STUNMessage::SetIceControlling(const MS_8BYTES iceControlling)
	{
		if (!this->iceControlling)
			this->iceControlling = iceControlling;
	}

	inline
	void STUNMessage::SetIceControlled(const MS_8BYTES iceControlled)
	{
		if (!this->iceControlled)
			this->iceControlled = iceControlled;
	}

	inline
	void STUNMessage::SetUseCandidate()
	{
		this->hasUseCandidate = true;
	}

	inline
	void STUNMessage::SetXorMappedAddress(const struct sockaddr* xorMappedAddress)
	{
		if (!this->xorMappedAddress)
			this->xorMappedAddress = xorMappedAddress;
	}

	inline
	void STUNMessage::SetErrorCode(MS_2BYTES errorCode)
	{
		this->errorCode = errorCode;
	}

	inline
	void STUNMessage::SetMessageIntegrity(const MS_BYTE* messageIntegrity)
	{
		if (!this->messageIntegrity)
			this->messageIntegrity = messageIntegrity;
	}

	inline
	void STUNMessage::SetFingerprint()
	{
		this->hasFingerprint = true;
	}

	inline
	const std::string& STUNMessage::GetUsername()
	{
		return this->username;
	}

	inline
	const MS_4BYTES STUNMessage::GetPriority()
	{
		return this->priority;
	}

	inline
	const MS_8BYTES STUNMessage::GetIceControlling()
	{
		return this->iceControlling;
	}

	inline
	const MS_8BYTES STUNMessage::GetIceControlled()
	{
		return this->iceControlled;
	}

	inline
	bool STUNMessage::HasUseCandidate()
	{
		return this->hasUseCandidate;
	}

	inline
	MS_2BYTES STUNMessage::GetErrorCode()
	{
		return this->errorCode;
	}

	inline
	bool STUNMessage::HasMessageIntegrity()
	{
		return (this->messageIntegrity ? true : false);
	}

	inline
	bool STUNMessage::HasFingerprint()
	{
		return this->hasFingerprint;
	}
}

#endif
