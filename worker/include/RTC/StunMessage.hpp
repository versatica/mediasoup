#ifndef MS_RTC_STUN_MESSAGE_HPP
#define MS_RTC_STUN_MESSAGE_HPP

#include "common.hpp"
#include <string>

namespace RTC
{
	class StunMessage
	{
	public:
		// STUN message class.
		enum class Class : uint16_t
		{
			REQUEST          = 0,
			INDICATION       = 1,
			SUCCESS_RESPONSE = 2,
			ERROR_RESPONSE   = 3
		};

		// STUN message method.
		enum class Method : uint16_t
		{
			BINDING = 1
		};

		// Attribute type.
		enum class Attribute : uint16_t
		{
			MAPPED_ADDRESS     = 0x0001,
			USERNAME           = 0x0006,
			MESSAGE_INTEGRITY  = 0x0008,
			ERROR_CODE         = 0x0009,
			UNKNOWN_ATTRIBUTES = 0x000A,
			REALM              = 0x0014,
			NONCE              = 0x0015,
			XOR_MAPPED_ADDRESS = 0x0020,
			PRIORITY           = 0x0024,
			USE_CANDIDATE      = 0x0025,
			SOFTWARE           = 0x8022,
			ALTERNATE_SERVER   = 0x8023,
			FINGERPRINT        = 0x8028,
			ICE_CONTROLLED     = 0x8029,
			ICE_CONTROLLING    = 0x802A
		};

		// Authentication result.
		enum class Authentication
		{
			OK           = 0,
			UNAUTHORIZED = 1,
			BAD_REQUEST  = 2
		};

	public:
		static bool IsStun(const uint8_t* data, size_t len);
		static StunMessage* Parse(const uint8_t* data, size_t len);

	private:
		static const uint8_t magicCookie[];

	public:
		StunMessage(
		  Class klass, Method method, const uint8_t* transactionId, const uint8_t* data, size_t size);
		~StunMessage();

		void Dump() const;
		Class GetClass() const;
		Method GetMethod() const;
		const uint8_t* GetData() const;
		size_t GetSize() const;
		void SetUsername(const char* username, size_t len);
		void SetPriority(uint32_t priority);
		void SetIceControlling(uint64_t iceControlling);
		void SetIceControlled(uint64_t iceControlled);
		void SetUseCandidate();
		void SetXorMappedAddress(const struct sockaddr* xorMappedAddress);
		void SetErrorCode(uint16_t errorCode);
		void SetMessageIntegrity(const uint8_t* messageIntegrity);
		void SetFingerprint();
		const std::string& GetUsername() const;
		uint32_t GetPriority() const;
		uint64_t GetIceControlling() const;
		uint64_t GetIceControlled() const;
		bool HasUseCandidate() const;
		uint16_t GetErrorCode() const;
		bool HasMessageIntegrity() const;
		bool HasFingerprint() const;
		Authentication CheckAuthentication(
		  const std::string& localUsername, const std::string& localPassword);
		StunMessage* CreateSuccessResponse();
		StunMessage* CreateErrorResponse(uint16_t errorCode);
		void Authenticate(const std::string& password);
		void Serialize(uint8_t* buffer);

	private:
		// Passed by argument.
		Class klass;                             // 2 bytes.
		Method method;                           // 2 bytes.
		const uint8_t* transactionId{ nullptr }; // 12 bytes.
		uint8_t* data{ nullptr };                // Pointer to binary data.
		size_t size{ 0 };                        // The full message size (including header).
		// STUN attributes.
		std::string username;                               // Less than 513 bytes.
		uint32_t priority{ 0 };                             // 4 bytes unsigned integer.
		uint64_t iceControlling{ 0 };                       // 8 bytes unsigned integer.
		uint64_t iceControlled{ 0 };                        // 8 bytes unsigned integer.
		bool hasUseCandidate{ false };                      // 0 bytes.
		const uint8_t* messageIntegrity{ nullptr };         // 20 bytes.
		bool hasFingerprint{ false };                       // 4 bytes.
		const struct sockaddr* xorMappedAddress{ nullptr }; // 8 or 20 bytes.
		uint16_t errorCode{ 0 };                            // 4 bytes (no reason phrase).
		std::string password;
	};

	/* Inline class methods. */

	inline bool StunMessage::IsStun(const uint8_t* data, size_t len)
	{
		return (
		  // STUN headers are 20 bytes.
		  (len >= 20) &&
		  // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
		  (data[0] < 3) &&
		  // Magic cookie must match.
		  (data[4] == StunMessage::magicCookie[0]) && (data[5] == StunMessage::magicCookie[1]) &&
		  (data[6] == StunMessage::magicCookie[2]) && (data[7] == StunMessage::magicCookie[3]));
	}

	/* Inline instance methods. */

	inline StunMessage::Class StunMessage::GetClass() const
	{
		return this->klass;
	}

	inline StunMessage::Method StunMessage::GetMethod() const
	{
		return this->method;
	}

	inline const uint8_t* StunMessage::GetData() const
	{
		return this->data;
	}

	inline size_t StunMessage::GetSize() const
	{
		return this->size;
	}

	inline void StunMessage::SetUsername(const char* username, size_t len)
	{
		if (this->username.empty())
			this->username = std::string(username, len);
	}

	inline void StunMessage::SetPriority(const uint32_t priority)
	{
		if (!this->priority)
			this->priority = priority;
	}

	inline void StunMessage::SetIceControlling(const uint64_t iceControlling)
	{
		if (!this->iceControlling)
			this->iceControlling = iceControlling;
	}

	inline void StunMessage::SetIceControlled(const uint64_t iceControlled)
	{
		if (!this->iceControlled)
			this->iceControlled = iceControlled;
	}

	inline void StunMessage::SetUseCandidate()
	{
		this->hasUseCandidate = true;
	}

	inline void StunMessage::SetXorMappedAddress(const struct sockaddr* xorMappedAddress)
	{
		if (!this->xorMappedAddress)
			this->xorMappedAddress = xorMappedAddress;
	}

	inline void StunMessage::SetErrorCode(uint16_t errorCode)
	{
		this->errorCode = errorCode;
	}

	inline void StunMessage::SetMessageIntegrity(const uint8_t* messageIntegrity)
	{
		if (!this->messageIntegrity)
			this->messageIntegrity = messageIntegrity;
	}

	inline void StunMessage::SetFingerprint()
	{
		this->hasFingerprint = true;
	}

	inline const std::string& StunMessage::GetUsername() const
	{
		return this->username;
	}

	inline uint32_t StunMessage::GetPriority() const
	{
		return this->priority;
	}

	inline uint64_t StunMessage::GetIceControlling() const
	{
		return this->iceControlling;
	}

	inline uint64_t StunMessage::GetIceControlled() const
	{
		return this->iceControlled;
	}

	inline bool StunMessage::HasUseCandidate() const
	{
		return this->hasUseCandidate;
	}

	inline uint16_t StunMessage::GetErrorCode() const
	{
		return this->errorCode;
	}

	inline bool StunMessage::HasMessageIntegrity() const
	{
		return (this->messageIntegrity ? true : false);
	}

	inline bool StunMessage::HasFingerprint() const
	{
		return this->hasFingerprint;
	}
} // namespace RTC

#endif
