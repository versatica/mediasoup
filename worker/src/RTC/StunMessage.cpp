#define MS_CLASS "RTC::StunMessage"
// #define MS_LOG_DEV

#include "RTC/StunMessage.h"
#include "Utils.h"
#include "Logger.h"
#include <cstdio> // std::snprintf()
#include <cstring> // std::memcmp(), std::memcpy()

namespace RTC
{
	/* Class variables. */

	const uint8_t StunMessage::magicCookie[] = { 0x21, 0x12, 0xA4, 0x42 };

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
		uint16_t msg_type = Utils::Byte::Get2Bytes(data, 0);

		// Get length field.
		uint16_t msg_length = Utils::Byte::Get2Bytes(data, 2);

		// length field must be total size minus header's 20 bytes, and must be multiple of 4 Bytes.
		if (((size_t)msg_length != len - 20) || (msg_length & 0x03))
		{
			MS_WARN_TAG(ice, "length field + 20 does not match total size (or it is not multiple of 4 bytes), message discarded");

			return nullptr;
		}

		// Get STUN method.
		uint16_t msg_method = (msg_type & 0x000f) | ((msg_type & 0x00e0)>>1) | ((msg_type & 0x3E00)>>2);

		// Get STUN class.
		uint16_t msg_class = ((data[0] & 0x01) << 1) | ((data[1] & 0x10) >> 4);

		// Create a new StunMessage (data + 8 points to the received TransactionID field).
		StunMessage* msg = new StunMessage((Class)msg_class, (Method)msg_method, data + 8, data, len);

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
		bool has_message_integrity = false;
		bool has_fingerprint = false;
		size_t fingerprint_attr_pos; // Will point to the beginning of the attribute.
		uint32_t fingerprint; // Holds the value of the FINGERPRINT attribute.

		// Ensure there are at least 4 remaining bytes (attribute with 0 length).
		while (pos + 4 <= len)
		{
			// Get the attribute type.
			Attribute attr_type = (Attribute)Utils::Byte::Get2Bytes(data, pos);

			// Get the attribute length.
			uint16_t attr_length = Utils::Byte::Get2Bytes(data, pos + 2);

			// Ensure the attribute length is not greater than the remaining size.
			if ((pos + 4 + attr_length) > len)
			{
				MS_WARN_TAG(ice, "the attribute length exceeds the remaining size, message discarded");

				delete msg;
				return nullptr;
			}

			// FINGERPRINT must be the last attribute.
			if (has_fingerprint)
			{
				MS_WARN_TAG(ice, "attribute after FINGERPRINT is not allowed, message discarded");

				delete msg;
				return nullptr;
			}

			// After a MESSAGE-INTEGRITY attribute just FINGERPRINT is allowed.
			if (has_message_integrity && attr_type != Attribute::Fingerprint)
			{
				MS_WARN_TAG(ice, "attribute after MESSAGE_INTEGRITY other than FINGERPRINT is not allowed, message discarded");

				delete msg;
				return nullptr;
			}

			const uint8_t* attr_value_pos = data + pos + 4;

			switch (attr_type)
			{
				case Attribute::Username:
				{
					msg->SetUsername((const char*)attr_value_pos, (size_t)attr_length);

					break;
				}
				case Attribute::Priority:
				{
					msg->SetPriority(Utils::Byte::Get4Bytes(attr_value_pos, 0));

					break;
				}
				case Attribute::IceControlling:
				{
					msg->SetIceControlling(Utils::Byte::Get8Bytes(attr_value_pos, 0));

					break;
				}
				case Attribute::IceControlled:
				{
					msg->SetIceControlled(Utils::Byte::Get8Bytes(attr_value_pos, 0));

					break;
				}
				case Attribute::UseCandidate:
				{
					msg->SetUseCandidate();

					break;
				}
				case Attribute::MessageIntegrity:
				{
					has_message_integrity = true;
					msg->SetMessageIntegrity(attr_value_pos);

					break;
				}
				case Attribute::Fingerprint:
				{
					has_fingerprint = true;
					fingerprint_attr_pos = pos;
					fingerprint = Utils::Byte::Get4Bytes(attr_value_pos, 0);
					msg->SetFingerprint();

					break;
				}
				case Attribute::ErrorCode:
				{
					uint8_t error_class = Utils::Byte::Get1Byte(attr_value_pos, 2);
					uint8_t error_number = Utils::Byte::Get1Byte(attr_value_pos, 3);
					uint16_t error_code = (uint16_t)(error_class * 100 + error_number);
					msg->SetErrorCode(error_code);

					break;
				}
				default:
					;
			}

			// Set next attribute position.
			pos = (size_t)Utils::Byte::PadTo4Bytes((uint16_t)(pos + 4 + attr_length));
		}

		// Ensure current position matches the total length.
		if (pos != len)
		{
			MS_WARN_TAG(ice, "computed message size does not match total size, message discarded");

			delete msg;
			return nullptr;
		}

		// If it has FINGERPRINT attribute then verify it.
		if (has_fingerprint)
		{
			// Compute the CRC32 of the received message up to (but excluding) the
			// FINGERPRINT attribute and XOR it with 0x5354554e.
			uint32_t computed_fingerprint = Utils::Crypto::GetCRC32(data, fingerprint_attr_pos) ^ 0x5354554e;

			// Compare with the FINGERPRINT value in the message.
			if (fingerprint != computed_fingerprint)
			{
				MS_WARN_TAG(ice, "computed FINGERPRINT value does not match the value in the message,| message discarded");

				delete msg;
				return nullptr;
			}
		}

		return msg;
	}

	/* Instance methods. */

	StunMessage::StunMessage(Class klass, Method method, const uint8_t* transactionId, const uint8_t* data, size_t size) :
		klass(klass),
		method(method),
		transactionId(transactionId),
		data((uint8_t*)data),
		size(size)
	{
		MS_TRACE();
	}

	StunMessage::~StunMessage()
	{
		MS_TRACE();
	}

	void StunMessage::Dump()
	{
		#ifdef MS_LOG_DEV

		MS_TRACE();

		MS_DEBUG_DEV("<StunMessage>");

		std::string klass;
		switch (this->klass)
		{
			case Class::Request         : klass = "Request";         break;
			case Class::Indication      : klass = "Indication";      break;
			case Class::SuccessResponse : klass = "SuccessResponse"; break;
			case Class::ErrorResponse   : klass = "ErrorResponse";   break;
		}
		if (this->method == Method::Binding)
			MS_DEBUG_DEV("  Binding %s", klass.c_str());
		else
			// This prints the unknown method number. Example: TURN Allocate => 0x003.
			MS_DEBUG_DEV("  %s with unknown method %#.3x", klass.c_str(), (uint16_t)this->method);
		MS_DEBUG_DEV("  size: %zu bytes", this->size);
		char transaction_id[25];
		for (int i=0; i<12; i++)
		{
			// NOTE: n must be 3 because snprintf adds a \0 after printed chars.
			std::snprintf(transaction_id+(i*2), 3, "%.2x", this->transactionId[i]);
		}
		MS_DEBUG_DEV("  transactionId: %s", transaction_id);
		if (this->errorCode)
			MS_DEBUG_DEV("  errorCode: %" PRIu16, this->errorCode);
		if (!this->username.empty())
			MS_DEBUG_DEV("  username: %s", this->username.c_str());
		if (this->priority)
			MS_DEBUG_DEV("  priority: %" PRIu32, this->priority);
		if (this->iceControlling)
			MS_DEBUG_DEV("  iceControlling: %" PRIu64, this->iceControlling);
		if (this->iceControlled)
			MS_DEBUG_DEV("  iceControlled: %" PRIu64, this->iceControlled);
		if (this->hasUseCandidate)
			MS_DEBUG_DEV("  useCandidate");
		if (this->xorMappedAddress)
		{
			int family;
			uint16_t port;
			std::string ip;
			Utils::IP::GetAddressInfo(this->xorMappedAddress, &family, ip, &port);

			MS_DEBUG_DEV("  xorMappedAddress: %s : %" PRIu16, ip.c_str(), port);
		}
		if (this->messageIntegrity)
		{
			char message_integrity[41];
			for (int i=0; i<20; i++)
			{
				std::snprintf(message_integrity+(i*2), 3, "%.2x", this->messageIntegrity[i]);
			}

			MS_DEBUG_DEV("  messageIntegrity: %s", message_integrity);
		}
		if (this->hasFingerprint)
			MS_DEBUG_DEV("  fingerprint");

		MS_DEBUG_DEV("</StunMessage>");

		#endif
	}

	StunMessage::Authentication StunMessage::CheckAuthentication(const std::string &local_username, const std::string &local_password)
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
				size_t local_username_len = local_username.length();
				if (
					this->username.length() <= local_username_len ||
					this->username.at(local_username_len) != ':' ||
					(this->username.compare(0, local_username_len, local_username) != 0)
				)
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
		const uint8_t* computed_message_integrity = Utils::Crypto::GetHMAC_SHA1(local_password, this->data, (this->messageIntegrity - 4) - this->data);

		Authentication result;

		// Compare the computed HMAC-SHA1 with the MESSAGE-INTEGRITY in the message.
		if (std::memcmp(this->messageIntegrity, computed_message_integrity, 20) == 0)
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

		MS_ASSERT(this->klass == Class::Request, "attempt to create a success response for a non Request STUN message");

		return new StunMessage(Class::SuccessResponse, this->method, this->transactionId, nullptr, 0);
	}

	StunMessage* StunMessage::CreateErrorResponse(uint16_t errorCode)
	{
		MS_TRACE();

		MS_ASSERT(this->klass == Class::Request, "attempt to create an error response for a non Request STUN message");

		StunMessage* response = new StunMessage(Class::ErrorResponse, this->method, this->transactionId, nullptr, 0);

		response->SetErrorCode(errorCode);

		return response;
	}

	void StunMessage::Authenticate(const std::string &password)
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
		uint16_t username_padded_len = 0;
		uint16_t xor_mapped_address_padded_len = 0;
		bool add_xor_mapped_address = (this->xorMappedAddress && this->method == StunMessage::Method::Binding && this->klass == Class::SuccessResponse);
		bool add_error_code = (this->errorCode && this->klass == Class::ErrorResponse);
		bool add_message_integrity = (this->klass != Class::ErrorResponse && !this->password.empty());
		bool add_fingerprint = true; // Do always.

		// Update data pointer.
		this->data = buffer;

		// First calculate the total required size for the entire message.
		this->size = 20; // Header.

		if (!this->username.empty())
		{
			username_padded_len = Utils::Byte::PadTo4Bytes((uint16_t)this->username.length());
			this->size += 4 + username_padded_len;
		}

		if (this->priority)
			this->size += 4 + 4;

		if (this->iceControlling)
			this->size += 4 + 8;

		if (this->iceControlled)
			this->size += 4 + 8;

		if (this->hasUseCandidate)
			this->size += 4;

		if (add_xor_mapped_address)
		{
			switch (this->xorMappedAddress->sa_family)
			{
				case AF_INET:
				{
					xor_mapped_address_padded_len = 8;
					this->size += 4 + 8;

					break;
				}
				case AF_INET6:
				{
					xor_mapped_address_padded_len = 20;
					this->size += 4 + 20;

					break;
				}
				default:
				{
					MS_ERROR("invalid inet family in XOR-MAPPED-ADDRESS attribute");

					add_xor_mapped_address = false;
				}
			}
		}

		if (add_error_code)
			this->size += 4 + 4;

		if (add_message_integrity)
			this->size += 4 + 20;

		if (add_fingerprint)
			this->size += 4 + 4;

		// Merge class and method fields into type.
		uint16_t type_field = ((uint16_t)this->method & 0x0f80) << 2;
		type_field |= ((uint16_t)this->method & 0x0070) << 1;
		type_field |= ((uint16_t)this->method & 0x000f);
		type_field |= ((uint16_t)this->klass & 0x02) << 7;
		type_field |= ((uint16_t)this->klass & 0x01) << 4;

		// Set type field.
		Utils::Byte::Set2Bytes(buffer, 0, type_field);

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
		if (username_padded_len)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::Username);
			Utils::Byte::Set2Bytes(buffer, pos + 2, (uint16_t)this->username.length());
			std::memcpy(buffer + pos + 4, this->username.c_str(), this->username.length());
			pos += 4 + username_padded_len;
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
		if (add_xor_mapped_address)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::XorMappedAddress);
			Utils::Byte::Set2Bytes(buffer, pos + 2, xor_mapped_address_padded_len);
			uint8_t* attr_value = buffer + pos + 4;
			switch (this->xorMappedAddress->sa_family)
			{
				case AF_INET:
				{
					// Set first byte to 0.
					attr_value[0] = 0;
					// Set inet family.
					attr_value[1] = 0x01;
					// Set port and XOR it.
					std::memcpy(attr_value + 2, &((const sockaddr_in*)(this->xorMappedAddress))->sin_port, 2);
					attr_value[2] ^= StunMessage::magicCookie[0];
					attr_value[3] ^= StunMessage::magicCookie[1];
					// Set address and XOR it.
					std::memcpy(attr_value + 4, &((const sockaddr_in*)this->xorMappedAddress)->sin_addr.s_addr, 4);
					attr_value[4] ^= StunMessage::magicCookie[0];
					attr_value[5] ^= StunMessage::magicCookie[1];
					attr_value[6] ^= StunMessage::magicCookie[2];
					attr_value[7] ^= StunMessage::magicCookie[3];

					pos += 4 + 8;

					break;
				}
				case AF_INET6:
				{
					// Set first byte to 0.
					attr_value[0] = 0;
					// Set inet family.
					attr_value[1] = 0x02;
					// Set port and XOR it.
					std::memcpy(attr_value + 2, &((const sockaddr_in6*)(this->xorMappedAddress))->sin6_port, 2);
					attr_value[2] ^= StunMessage::magicCookie[0];
					attr_value[3] ^= StunMessage::magicCookie[1];
					// Set address and XOR it.
					std::memcpy(attr_value + 4, &((const sockaddr_in6*)this->xorMappedAddress)->sin6_addr.s6_addr, 16);
					attr_value[4] ^= StunMessage::magicCookie[0];
					attr_value[5] ^= StunMessage::magicCookie[1];
					attr_value[6] ^= StunMessage::magicCookie[2];
					attr_value[7] ^= StunMessage::magicCookie[3];
					attr_value[8] ^= this->transactionId[0];
					attr_value[9] ^= this->transactionId[1];
					attr_value[10] ^= this->transactionId[2];
					attr_value[11] ^= this->transactionId[3];
					attr_value[12] ^= this->transactionId[4];
					attr_value[13] ^= this->transactionId[5];
					attr_value[14] ^= this->transactionId[6];
					attr_value[15] ^= this->transactionId[7];
					attr_value[16] ^= this->transactionId[8];
					attr_value[17] ^= this->transactionId[9];
					attr_value[18] ^= this->transactionId[10];
					attr_value[19] ^= this->transactionId[11];

					pos += 4 + 20;

					break;
				}
			}
		}

		// Add ERROR-CODE.
		if (add_error_code)
		{
			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::ErrorCode);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 4);
			uint8_t code_class = (uint8_t)(this->errorCode / 100);
			uint8_t code_number = (uint8_t)this->errorCode - (code_class * 100);
			Utils::Byte::Set2Bytes(buffer, pos + 4, 0);
			Utils::Byte::Set1Byte(buffer, pos + 6, code_class);
			Utils::Byte::Set1Byte(buffer, pos + 7, code_number);
			pos += 4 + 4;
		}

		// Add MESSAGE-INTEGRITY.
		if (add_message_integrity)
		{
			// Ignore FINGERPRINT.
			if (add_fingerprint)
				Utils::Byte::Set2Bytes(buffer, 2, (uint16_t)(this->size - 20 - 8));

			// Calculate the HMAC-SHA1 of the message according to MESSAGE-INTEGRITY rules.
			const uint8_t* computed_message_integrity = Utils::Crypto::GetHMAC_SHA1(this->password, buffer, pos);

			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::MessageIntegrity);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 20);
			std::memcpy(buffer + pos + 4, computed_message_integrity, 20);

			// Update the pointer.
			this->messageIntegrity = buffer + pos + 4;
			pos += 4 + 20;

			// Restore length field.
			if (add_fingerprint)
				Utils::Byte::Set2Bytes(buffer, 2, (uint16_t)(this->size - 20));
		}
		else
		{
			// Unset the pointer (if it was set).
			this->messageIntegrity = nullptr;
		}

		// Add FINGERPRINT.
		if (add_fingerprint)
		{
			// Compute the CRC32 of the message up to (but excluding) the FINGERPRINT
			// attribute and XOR it with 0x5354554e.
			uint32_t computed_fingerprint = Utils::Crypto::GetCRC32(buffer, pos) ^ 0x5354554e;

			Utils::Byte::Set2Bytes(buffer, pos, (uint16_t)Attribute::Fingerprint);
			Utils::Byte::Set2Bytes(buffer, pos + 2, 4);
			Utils::Byte::Set4Bytes(buffer, pos + 4, computed_fingerprint);
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
