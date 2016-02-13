#ifndef MS_RTC_STUN_MESSAGE_H
#define MS_RTC_STUN_MESSAGE_H

#include "common.h"
#include <string>

namespace RTC
{
	class StunMessage
	{
	public:
		// STUN message class.
		enum class Class : uint16_t
		{
			Request         = 0,
			Indication      = 1,
			SuccessResponse = 2,
			ErrorResponse   = 3
		};

		// STUN message method.
		enum class Method : uint16_t
		{
			Binding = 1
		};

		// Attribute type.
		enum class Attribute : uint16_t
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
		static bool IsStun(const uint8_t* data, size_t len);
		static StunMessage* Parse(const uint8_t* data, size_t len);

	private:
		static const uint8_t magicCookie[];

	public:
		StunMessage(Class klass, Method method, const uint8_t* transactionId, const uint8_t* raw, size_t length);
		~StunMessage();

		void Dump();
		Class GetClass();
		Method GetMethod();
		const uint8_t* GetRaw();
		void SetLength(size_t length);
		size_t GetLength();
		void SetUsername(const char* username, size_t len);
		void SetPriority(const uint32_t priority);
		void SetIceControlling(const uint64_t iceControlling);
		void SetIceControlled(const uint64_t iceControlled);
		void SetUseCandidate();
		void SetXorMappedAddress(const struct sockaddr* xorMappedAddress);
		void SetErrorCode(uint16_t errorCode);
		void SetMessageIntegrity(const uint8_t* messageIntegrity);
		void SetFingerprint();
		const std::string& GetUsername();
		const uint32_t GetPriority();
		const uint64_t GetIceControlling();
		const uint64_t GetIceControlled();
		bool HasUseCandidate();
		uint16_t GetErrorCode();
		bool HasMessageIntegrity();
		bool HasFingerprint();
		Authentication CheckAuthentication(const std::string &local_username, const std::string &local_password);
		StunMessage* CreateSuccessResponse();
		StunMessage* CreateErrorResponse(uint16_t errorCode);
		void Authenticate(const std::string &password);
		void Serialize();

	private:
		// Passed by argument.
		Class klass;  // 2 bytes.
		Method method;  // 2 bytes.
		const uint8_t* transactionId = nullptr;  // 12 bytes.
		uint8_t* raw = nullptr;  // Allocated when Serialize().
		size_t length = 0;  // The full message size (including header).
		// Others.
		bool isSerialized = false;
		// STUN attributes.
		std::string username;  // Less than 513 bytes.
		uint32_t priority = 0;  // 4 bytes unsigned integer.
		uint64_t iceControlling = 0;  // 8 bytes unsigned integer.
		uint64_t iceControlled = 0;  // 8 bytes unsigned integer.
		bool hasUseCandidate = false;  // 0 bytes.
		const uint8_t* messageIntegrity = nullptr;  // 20 bytes.
		bool hasFingerprint = false;  // 4 bytes.
		const struct sockaddr* xorMappedAddress = nullptr;  // 8 or 20 bytes.
		uint16_t errorCode = 0;  // 4 bytes (no reason phrase).
		std::string password;
	};

	/* Inline methods. */

	inline
	bool StunMessage::IsStun(const uint8_t* data, size_t len)
	{
		return (
			// STUN headers are 20 bytes.
			(len >= 20) &&
			// DOC: https://tools.ietf.org/html/draft-petithuguenin-avtcore-rfc5764-mux-fixes-00
			(data[0] < 20) &&
			// Magic cookie must match.
			(data[4] == StunMessage::magicCookie[0]) &&
			(data[5] == StunMessage::magicCookie[1]) &&
			(data[6] == StunMessage::magicCookie[2]) &&
			(data[7] == StunMessage::magicCookie[3])
		);
	}

	inline
	StunMessage::Class StunMessage::GetClass()
	{
		return this->klass;
	}

	inline
	StunMessage::Method StunMessage::GetMethod()
	{
		return this->method;
	}

	inline
	const uint8_t* StunMessage::GetRaw()
	{
		return this->raw;
	}

	inline
	void StunMessage::SetLength(size_t length)
	{
		this->length = length;
	}

	inline
	size_t StunMessage::GetLength()
	{
		return this->length;
	}

	inline
	void StunMessage::SetUsername(const char* username, size_t len)
	{
		if (this->username.empty())
			this->username = std::string(username, len);
	}

	inline
	void StunMessage::SetPriority(const uint32_t priority)
	{
		if (!this->priority)
			this->priority = priority;
	}

	inline
	void StunMessage::SetIceControlling(const uint64_t iceControlling)
	{
		if (!this->iceControlling)
			this->iceControlling = iceControlling;
	}

	inline
	void StunMessage::SetIceControlled(const uint64_t iceControlled)
	{
		if (!this->iceControlled)
			this->iceControlled = iceControlled;
	}

	inline
	void StunMessage::SetUseCandidate()
	{
		this->hasUseCandidate = true;
	}

	inline
	void StunMessage::SetXorMappedAddress(const struct sockaddr* xorMappedAddress)
	{
		if (!this->xorMappedAddress)
			this->xorMappedAddress = xorMappedAddress;
	}

	inline
	void StunMessage::SetErrorCode(uint16_t errorCode)
	{
		this->errorCode = errorCode;
	}

	inline
	void StunMessage::SetMessageIntegrity(const uint8_t* messageIntegrity)
	{
		if (!this->messageIntegrity)
			this->messageIntegrity = messageIntegrity;
	}

	inline
	void StunMessage::SetFingerprint()
	{
		this->hasFingerprint = true;
	}

	inline
	const std::string& StunMessage::GetUsername()
	{
		return this->username;
	}

	inline
	const uint32_t StunMessage::GetPriority()
	{
		return this->priority;
	}

	inline
	const uint64_t StunMessage::GetIceControlling()
	{
		return this->iceControlling;
	}

	inline
	const uint64_t StunMessage::GetIceControlled()
	{
		return this->iceControlled;
	}

	inline
	bool StunMessage::HasUseCandidate()
	{
		return this->hasUseCandidate;
	}

	inline
	uint16_t StunMessage::GetErrorCode()
	{
		return this->errorCode;
	}

	inline
	bool StunMessage::HasMessageIntegrity()
	{
		return (this->messageIntegrity ? true : false);
	}

	inline
	bool StunMessage::HasFingerprint()
	{
		return this->hasFingerprint;
	}
}

#endif
