#define MS_CLASS "RTC::StunMessage"
// #define MS_LOG_DEV

#include "RTC/StunMessage.hpp"
#include "Logger.hpp"
#include "Utils.hpp"
#include <cstdio>  // std::snprintf()
#include <cstring> // std::memcmp(), std::memcpy()

namespace RTC
{
	/* Class variables. */

	const uint8_t StunMessage::magicCookie[] = {0x21, 0x12, 0xA4, 0x42};

	/* Class methods. */

	StunMessage* StunMessage::Parse(const uint8_t* data, size_t len)
	{
		MS_TRACE();

		if (!StunMessage::IsStun(data, len))
			return nullptr;

		/*
		  The message type field is decomposed further into the following
		    structure:

		    0                 1
		    2  3  4 5 6 7 8 9 0 1 2 3 4 5
		       +--+--+-+-+-+-+-+-+-+-+-+-+-+-+
		       |M |M |M|M|M|C|M|M|M|C|M|M|M|M|
		       |11|10|9|8|7|1|6|5|4|0|3|2|1|0|
		       +--+--+-+-+-+-+-+-+-+-+-+-+-+-+

		    Figure 3: Format of STUN Message Type Field

		   Here the bits in the message type field are shown as most significant
		   (M11) through least significant (M0).  M11 through M0 represent a 12-
		   bit encoding of the method.  C1 and C0 represent a 2-bit encoding of
		   the class.
		 */

		// Get type field.
		uint16_t msgType = Utils::Byte::Get2Bytes(data, 0);

		// Get length field.
		uint16_t msgLength = Utils::Byte::Get2Bytes(data, 2);

		// length field must be total size minus header's 20 bytes, and must be multiple of 4 Bytes.
		if (((size_t)msgLength != len - 20) || (msgLength & 0x03))
		{
			MS_WARN_TAG(
			    ice,
			    "length field + 20 does not match total size (or it is not multiple of 4 bytes), "
			    "message discarded");

			return nullptr;
		}

		// Get STUN method.
		uint16_t msgMethod = (msgType & 0x000f) | ((msgType & 0x00e0) >> 1) | ((msgType & 0x3E00) >> 2);

		// Get STUN class.
		uint16_t msgClass = ((data[0] & 0x01) << 1) | ((data[1] & 0x10) >> 4);

		// Create a new StunMessage (data + 8 points to the received TransactionID field).
		StunMessage* msg = new StunMessage((Class)msgClass, (Method)msgMethod, data + 8, data, len);

		/*
		    STUN Attributes

		    After the STUN header are zero or more attributes.  Each attribute
		    MUST be TLV encoded, with a 16-bit type, 16-bit length, and value.
		    Each STUN attribute MUST end on a 32-bit boundary.  As mentioned
		    above, all fields in an attribute are transmitted most significant
		    bit first.

		        0                   1                   2                   3
		        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		       |         Type                  |            Length             |
		       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		       |                         Value (variable)                ....
		       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */

		// Start looking for attributes after STUN header (Byte #20).
		size_t pos = 20;
		// Flags (positions) for special MESSAGE-INTEGRITY and FINGERPRINT attributes.
		bool hasMessageIntegrity = false;
		bool hasFingerprint      = false;
		size_t fingerprintAttrPos; // Will point to the beginning of the attribute.
		uint32_t fingerprint;      // Holds the value of the FINGERPRINT attribute.

		// Ensure there are at least 4 remaining bytes (attribute with 0 length).
		while (pos + 4 <= len)
		{
			// Get the attribute type.
			Attribute attrType = (Attribute)Utils::Byte::Get2Bytes(data, pos);

			// Get the attribute length.
			uint16_t attrLength = Utils::Byte::Get2Bytes(data, pos + 2);

			// Ensure the attribute length is not greater than the remaining size.
			if ((pos + 4 + attrLength) > len)
			{
				MS_WARN_TAG(ice, "the attribute length exceeds the remaining size, message discarded");

				delete msg;
				return nullptr;
			}

			// FINGERPRINT must be the last attribute.
			if (hasFingerprint)
			{
				MS_WARN_TAG(ice, "attribute after FINGERPRINT is not allowed, message discarded");

				delete msg;
				return nullptr;
			}

			// After a MESSAGE-INTEGRITY attribute just FINGERPRINT is allowed.
			if (hasMessageIntegrity && attrType != Attribute::Fingerprint)
			{
				MS_WARN_TAG(
				    ice,
				    "attribute after MESSAGE_INTEGRITY other than FINGERPRINT is not allowed, "
				    "message discarded");

				delete msg;
				return nullptr;
			}

			const uint8_t* attrValuePos = data + pos + 4;

			switch (attrType)
			{
				case Attribute::Username:
				{
					msg->SetUsername((const char*)attrValuePos, (size_t)attrLength);

					break;
				}
				case Attribute::Priority:
				{
					msg->SetPriority(Utils::Byte::Get4Bytes(attrValuePos, 0));

					break;
				}
				case Attribute::IceControlling:
				{
					msg->SetIceControlling(Utils::Byte::Get8Bytes(attrValuePos, 0));

					break;
				}
				case Attribute::IceControlled:
				{
					msg->SetIceControlled(Utils::Byte::Get8Bytes(attrValuePos, 0));

					break;
				}
				case Attribute::UseCandidate:
				{
					msg->SetUseCandidate();

					break;
				}
				case Attribute::MessageIntegrity:
				{
					hasMessageIntegrity = true;
					msg->SetMessageIntegrity(attrValuePos);

					break;
				}
				case Attribute::Fingerprint:
				{
					hasFingerprint     = true;
					fingerprintAttrPos = pos;
					fingerprint        = Utils::Byte::Get4Bytes(attrValuePos, 0);
					msg->SetFingerprint();

					break;
				}
				case Attribute::ErrorCode:
				{
					uint8_t errorClass  = Utils::Byte::Get1Byte(attrValuePos, 2);
					uint8_t errorNumber = Utils::Byte::Get1Byte(attrValuePos, 3);
					uint16_t errorCode  = (uint16_t)(errorClass * 100 + errorNumber);

					msg->SetErrorCode(errorCode);

					break;
				}
				default:;
			}

			// Set next attribute position.
			pos = (size_t)Utils::Byte::PadTo4Bytes((uint16_t)(pos + 4 + attrLength));
		}

		// Ensure current position matches the total length.
		if (pos != len)
		{
			MS_WARN_TAG(ice, "computed message size does not match total size, message discarded");

			delete msg;
			return nullptr;
		}

		// If it has FINGERPRINT attribute then verify it.
		if (hasFingerprint)
		{
			// Compute the CRC32 of the received message up to (but excluding) the
			// FINGERPRINT attribute and XOR it with 0x5354554e.
			uint32_t computedFingerprint = Utils::Crypto::GetCRC32(data, fingerprintAttrPos) ^ 0x5354554e;

			// Compare with the FINGERPRINT value in the message.
			if (fingerprint != computedFingerprint)
			{
				MS_WARN_TAG(
				    ice,
				    "computed FINGERPRINT value does not match the value in the message, "
				    "message discarded");

				delete msg;
				return nullptr;
			}
		}

		return msg;
	}

	/* Instance methods. */

	StunMessage::StunMessage(
	    Class klass, Method method, const uint8_t* transactionId, const uint8_t* data, size_t size)
	    : klass(klass), method(method), transactionId(transactionId), data((uint8_t*)data), size(size)
	{
		MS_TRACE();
	}

	StunMessage::~StunMessage()
	{
		MS_TRACE();
	}

	void StunMessage::Dump() const
	{
		MS_TRACE();

		MS_DUMP("<StunMessage>");

		std::string klass;
		switch (this->klass)
		{
			case Class::Request:
				klass = "Request";
				break;
			case Class::Indication:
				klass = "Indication";
				break;
			case Class::SuccessResponse:
				klass = "SuccessResponse";
				break;
			case Class::ErrorResponse:
				klass = "ErrorResponse";
				break;
		}
		if (this->method == Method::Binding)
		{
			MS_DUMP("  Binding %s", klass.c_str());
		}
		else
		{
			// This prints the unknown method number. Example: TURN Allocate => 0x003.
			MS_DUMP("  %s with unknown method %#.3x", klass.c_str(), (uint16_t)this->method);
		}
		MS_DUMP("  size: %zu bytes", this->size);

		static char TransactionId[25];

		for (int i = 0; i < 12; ++i)
		{
			// NOTE: n must be 3 because snprintf adds a \0 after printed chars.
			std::snprintf(TransactionId + (i * 2), 3, "%.2x", this->transactionId[i]);
		}
		MS_DUMP("  transactionId: %s", TransactionId);
		if (this->errorCode)
			MS_DUMP("  errorCode: %" PRIu16, this->errorCode);
		if (!this->username.empty())
			MS_DUMP("  username: %s", this->username.c_str());
		if (this->priority)
			MS_DUMP("  priority: %" PRIu32, this->priority);
		if (this->iceControlling)
			MS_DUMP("  iceControlling: %" PRIu64, this->iceControlling);
		if (this->iceControlled)
			MS_DUMP("  iceControlled: %" PRIu64, this->iceControlled);
		if (this->hasUseCandidate)
			MS_DUMP("  useCandidate");
		if (this->xorMappedAddress)
		{
			int family;
			uint16_t port;
			std::string ip;

			Utils::IP::GetAddressInfo(this->xorMappedAddress, &family, ip, &port);

			MS_DUMP("  xorMappedAddress: %s : %" PRIu16, ip.c_str(), port);
		}
		if (this->messageIntegrity)
		{
			static char MessageIntegrity[41];

			for (int i = 0; i < 20; ++i)
			{
				std::snprintf(MessageIntegrity + (i * 2), 3, "%.2x", this->messageIntegrity[i]);
			}

			MS_DUMP("  messageIntegrity: %s", MessageIntegrity);
		}
		if (this->hasFingerprint)
			MS_DUMP("  fingerprint");

		MS_DUMP("</StunMessage>");
	}

	StunMessage::Authentication StunMessage::CheckAuthentication(
	    const std::string& localUsername, const std::string& localPassword)
	{
		MS_TRACE();

		switch (this->klass)
		{
			case Class::Request:
			case Class::Indication:
			{
				// Both USERNAME and MESSAGE-INTEGRITY must be present.
				if (!this->messageIntegrity || this->username.empty())
					return Authentication::BadRequest;

				// Check that USERNAME attribute begins with our local username plus ":".
				size_t localUsernameLen = localUsername.length();

				if (this->username.length() <= localUsernameLen ||
				    this->username.at(localUsernameLen) != ':' ||
				    (this->username.compare(0, localUsernameLen, localUsername) != 0))
					return Authentication::Unauthorized;

				break;
			}
			// This method cannot check authentication in received responses (as we
			// are ICE-Lite and don't generate requests).
			case Class::SuccessResponse:
			case Class::ErrorResponse:
			{
				MS_ERROR("cannot check authentication for a STUN response");

				return Authentication::BadRequest;
			}
		}

		// If there is FINGERPRINT it must be discarded for MESSAGE-INTEGRITY calculation,
		// so the header length field must be modified (and later restored).
		if (this->hasFingerprint)
			// Set the header length field: full size - header length (20) - FINGERPRINT length (8).
			Utils::Byte::Set2Bytes(this->data, 2, (uint16_t)(this->size - 20 - 8));

		// Calculate the HMAC-SHA1 of the message according to MESSAGE-INTEGRITY rules.
		const uint8_t* computedMessageIntegrity = Utils::Crypto::GetHMAC_SHA1(
		    localPassword, this->data, (this->messageIntegrity - 4) - this->data);

		Authentication result;

		// Compare the computed HMAC-SHA1 with the MESSAGE-INTEGRITY in the message.
		if (std::memcmp(this->messageIntegrity, computedMessageIntegrity, 20) == 0)
			result = Authentication::OK;
		else
			result = Authentication::Unauthorized;

		// Restore the header length field.
		if (this->hasFingerprint)
			Utils::Byte::Set2Bytes(this->data, 2, (uint16_t)(this->size - 20));

		return result;
	}

	StunMessage* StunMessage::CreateSuccessResponse()
	{
		MS_TRACE();

		MS_ASSERT(
		    this->klass == Class::Request,
		    "attempt to create a success response for a non Request STUN message");

		return new StunMessage(Class::SuccessResponse, this->method, this->transactionId, nullptr, 0);
	}

	StunMessage* StunMessage::CreateErrorResponse(uint16_t errorCode)
	{
		MS_TRACE();

		MS_ASSERT(
		    this->klass == Class::Request,
		    "attempt to create an error response for a non Request STUN message");

		StunMessage* response =
		    new StunMessage(Class::ErrorResponse, this->method, this->transactionId, nullptr, 0);

		response->SetErrorCode(errorCode);

		return response;
	}

	void StunMessage::Authenticate(const std::string& password)
	{
		// Just for Request, Indication and SuccessResponse messages.
		if (this->klass == Class::ErrorResponse)
		{
			MS_ERROR("cannot set password for ErrorResponse messages");

			return;
		}

		this->password = password;
	}

	void StunMessage::Serialize(uint8_t* buffer)
	{
		MS_TRACE();

		// Some useful variables.
		uint16_t usernamePaddedLen         = 0;
		uint16_t xorMappedAddressPaddedLen = 0;
		bool addXorMappedAddress =
		    (this->xorMappedAddress && this->method == StunMessage::Method::Binding &&
		     this->klass == Class::SuccessResponse);
		bool addErrorCode        = (this->errorCode && this->klass == Class::ErrorResponse);
		bool addMessageIntegrity = (this->klass != Class::ErrorResponse && !this->password.empty());
		bool addFingerprint      = true; // Do always.

		// Update data pointer.
		this->data = buffer;

		// First calculate the total required size for the entire message.
		this->size = 20; // Header.

		if (!this->username.empty())
		{
			usernamePaddedLen = Utils::Byte::PadTo4Bytes((uint16_t)this->username.length());
			this->size += 4 + usernamePaddedLen;
		}

		if (this->priority)
			this->size += 4 + 4;

		if (this->iceControlling)
			this->size += 4 + 8;

		if (this->iceControlled)
			this->size += 4 + 8;

		if (this->hasUseCandidate)
			this->size += 4;

		if (addXorMappedAddress)
		{
			switch (this->xorMappedAddress->sa_family)
			{
				case AF_INET:
				{
					xorMappedAddressPaddedLen = 8;
					this->size += 4 + 8;

					break;
				}
				case AF_INET6:
				{
					xorMappedAddressPaddedLen = 20;
					this->size += 4 + 20;

					break;
				}
				default:
				{
					MS_ERROR("invalid inet family in XOR-MAPPED-ADDRESS attribute");

					addXorMappedAddress = false;
				}
			}
		}

		if (addErrorCode)
			this->size += 4 + 4;

		if (addMessageIntegrity)
			this->size += 4 + 20;

		if (addFingerprint)
			this->size += 4 + 4;

		// Merge class and method fields into type.
		uint16_t typeField = ((uint16_t)this->method & 0x0f80) << 2;

		typeField |= ((uint16_t)this->method & 0x0070) << 1;
		typeField |= ((uint16_t)this->method & 0x000f);
		typeField |= ((uint16_t)this->klass & 0x02) << 7;
		typeField |= ((uint16_t)this->klass & 0x01) << 4;

		// Set type field.
		Utils::Byte::Set2Bytes(buffer, 0, typeField);

		// Set length field.
		Utils::Byte::Set2Bytes(buffer, 2, (uint16_t)this->size - 20);

		// Set magic cookie.
		std::memcpy(buffer + 4, StunMessage::magicCookie, 4);

		// Set TransactionId field.
		std::memcpy(buffer + 8, this->transactionId, 12);

		// Update the transaction ID pointer.
		this->transactionId = buffer + 8;

		// Add atributes.
		size_t pos = 20;

		// Add USERNAME.
		if (usernamePaddedLen)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::Username);
			Utils::Byte::Set2Bytes(buffer, pos + 2, (uint16_t)this->username.length());
			std::memcpy(buffer + pos + 4, this->username.c_str(), this->username.length());
			pos += 4 + usernamePaddedLen;
		}

		// Add PRIORITY.
		if (this->priority)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::Priority);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 4);
			Utils::Byte::Set4Bytes(buffer, pos + 4, this->priority);
			pos += 4 + 4;
		}

		// Add ICE-CONTROLLING.
		if (this->iceControlling)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::IceControlling);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 8);
			Utils::Byte::Set8Bytes(buffer, pos + 4, this->iceControlling);
			pos += 4 + 8;
		}

		// Add ICE-CONTROLLED.
		if (this->iceControlled)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::IceControlled);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 8);
			Utils::Byte::Set8Bytes(buffer, pos + 4, this->iceControlled);
			pos += 4 + 8;
		}

		// Add USE-CANDIDATE.
		if (this->hasUseCandidate)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::UseCandidate);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 0);
			pos += 4;
		}

		// Add XOR-MAPPED-ADDRESS
		if (addXorMappedAddress)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::XorMappedAddress);
			Utils::Byte::Set2Bytes(buffer, pos + 2, xorMappedAddressPaddedLen);

			uint8_t* attrValue = buffer + pos + 4;

			switch (this->xorMappedAddress->sa_family)
			{
				case AF_INET:
				{
					// Set first byte to 0.
					attrValue[0] = 0;
					// Set inet family.
					attrValue[1] = 0x01;
					// Set port and XOR it.
					std::memcpy(attrValue + 2, &((const sockaddr_in*)(this->xorMappedAddress))->sin_port, 2);
					attrValue[2] ^= StunMessage::magicCookie[0];
					attrValue[3] ^= StunMessage::magicCookie[1];
					// Set address and XOR it.
					std::memcpy(
					    attrValue + 4, &((const sockaddr_in*)this->xorMappedAddress)->sin_addr.s_addr, 4);
					attrValue[4] ^= StunMessage::magicCookie[0];
					attrValue[5] ^= StunMessage::magicCookie[1];
					attrValue[6] ^= StunMessage::magicCookie[2];
					attrValue[7] ^= StunMessage::magicCookie[3];

					pos += 4 + 8;

					break;
				}
				case AF_INET6:
				{
					// Set first byte to 0.
					attrValue[0] = 0;
					// Set inet family.
					attrValue[1] = 0x02;
					// Set port and XOR it.
					std::memcpy(attrValue + 2, &((const sockaddr_in6*)(this->xorMappedAddress))->sin6_port, 2);
					attrValue[2] ^= StunMessage::magicCookie[0];
					attrValue[3] ^= StunMessage::magicCookie[1];
					// Set address and XOR it.
					std::memcpy(
					    attrValue + 4, &((const sockaddr_in6*)this->xorMappedAddress)->sin6_addr.s6_addr, 16);
					attrValue[4] ^= StunMessage::magicCookie[0];
					attrValue[5] ^= StunMessage::magicCookie[1];
					attrValue[6] ^= StunMessage::magicCookie[2];
					attrValue[7] ^= StunMessage::magicCookie[3];
					attrValue[8] ^= this->transactionId[0];
					attrValue[9] ^= this->transactionId[1];
					attrValue[10] ^= this->transactionId[2];
					attrValue[11] ^= this->transactionId[3];
					attrValue[12] ^= this->transactionId[4];
					attrValue[13] ^= this->transactionId[5];
					attrValue[14] ^= this->transactionId[6];
					attrValue[15] ^= this->transactionId[7];
					attrValue[16] ^= this->transactionId[8];
					attrValue[17] ^= this->transactionId[9];
					attrValue[18] ^= this->transactionId[10];
					attrValue[19] ^= this->transactionId[11];

					pos += 4 + 20;

					break;
				}
			}
		}

		// Add ERROR-CODE.
		if (addErrorCode)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::ErrorCode);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 4);

			uint8_t codeClass  = (uint8_t)(this->errorCode / 100);
			uint8_t codeNumber = (uint8_t)this->errorCode - (codeClass * 100);

			Utils::Byte::Set2Bytes(buffer, pos + 4, 0);
			Utils::Byte::Set1Byte(buffer, pos + 6, codeClass);
			Utils::Byte::Set1Byte(buffer, pos + 7, codeNumber);
			pos += 4 + 4;
		}

		// Add MESSAGE-INTEGRITY.
		if (addMessageIntegrity)
		{
			// Ignore FINGERPRINT.
			if (addFingerprint)
				Utils::Byte::Set2Bytes(buffer, 2, (uint16_t)(this->size - 20 - 8));

			// Calculate the HMAC-SHA1 of the message according to MESSAGE-INTEGRITY rules.
			const uint8_t* computedMessageIntegrity =
			    Utils::Crypto::GetHMAC_SHA1(this->password, buffer, pos);

			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::MessageIntegrity);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 20);
			std::memcpy(buffer + pos + 4, computedMessageIntegrity, 20);

			// Update the pointer.
			this->messageIntegrity = buffer + pos + 4;
			pos += 4 + 20;

			// Restore length field.
			if (addFingerprint)
				Utils::Byte::Set2Bytes(buffer, 2, (uint16_t)(this->size - 20));
		}
		else
		{
			// Unset the pointer (if it was set).
			this->messageIntegrity = nullptr;
		}

		// Add FINGERPRINT.
		if (addFingerprint)
		{
			// Compute the CRC32 of the message up to (but excluding) the FINGERPRINT
			// attribute and XOR it with 0x5354554e.
			uint32_t computedFingerprint = Utils::Crypto::GetCRC32(buffer, pos) ^ 0x5354554e;

			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::Fingerprint);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 4);
			Utils::Byte::Set4Bytes(buffer, pos + 4, computedFingerprint);
			pos += 4 + 4;
			// Set flag.
			this->hasFingerprint = true;
		}
		else
		{
			this->hasFingerprint = false;
		}

		MS_ASSERT(pos == this->size, "pos != this->size");
	}
}
