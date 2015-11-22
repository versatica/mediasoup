#define MS_CLASS "RTC::STUNMessage"

#include "RTC/STUNMessage.h"
#include "Utils.h"
#include "Logger.h"
#include <cstdio>  // std::snprintf()
#include <cstring>  // std::memcmp(), std::memcpy()

namespace RTC
{
	/* Static variables. */

	const MS_BYTE STUNMessage::magicCookie[] = {0x21, 0x12, 0xA4, 0x42};

	/* Static methods. */

	STUNMessage* STUNMessage::Parse(const MS_BYTE* data, size_t len)
	{
		MS_TRACE();

		if (!STUNMessage::IsSTUN(data, len))
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
		MS_2BYTES msg_type = Utils::Byte::Get2Bytes(data, 0);

		// Get length field.
		MS_2BYTES msg_length = Utils::Byte::Get2Bytes(data, 2);

		// length field must be total size minus header's 20 bytes, and must be multiple of 4 Bytes.
		if (((size_t)msg_length != len - 20) || (msg_length & 0x03))
		{
			MS_DEBUG("length field + 20 does not match total size (or it is not multiple of 4 bytes) | message discarded");
			return nullptr;
		}

		// Get STUN method.
		MS_2BYTES msg_method = (msg_type & 0x000f) | ((msg_type & 0x00e0)>>1) | ((msg_type & 0x3E00)>>2);

		// Get STUN class.
		MS_2BYTES msg_class = ((data[0] & 0x01) << 1) | ((data[1] & 0x10) >> 4);

		// Create a new STUNMessage (data + 8 points to the received TransactionID field).
		STUNMessage* msg = new STUNMessage((Class)msg_class, (Method)msg_method, data + 8, data, len);

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
		size_t fingerprint_attr_pos;  // Will point to the beginning of the attribute.
		MS_4BYTES fingerprint;  // Holds the value of the FINGERPRINT attribute.

		// Ensure there are at least 4 remaining bytes (attribute with 0 length).
		while (pos + 4 <= len)
		{
			// Get the attribute type.
			Attribute attr_type = (Attribute)Utils::Byte::Get2Bytes(data, pos);

			// Get the attribute length.
			MS_2BYTES attr_length = Utils::Byte::Get2Bytes(data, pos + 2);

			// Ensure the attribute length is not greater than the remaining size.
			if ((pos + 4 + attr_length) > len)
			{
				MS_DEBUG("the attribute length exceeds the remaining size | message discarded");
				delete msg;
				return nullptr;
			}

			// FINGERPRINT must be the last attribute.
			if (has_fingerprint)
			{
				MS_DEBUG("attribute after FINGERPRINT is not allowed | message discarded");
				delete msg;
				return nullptr;
			}

			// After a MESSAGE-INTEGRITY attribute just FINGERPRINT is allowed.
			if (has_message_integrity && attr_type != Attribute::Fingerprint)
			{
				MS_DEBUG("attribute after MESSAGE_INTEGRITY other than FINGERPRINT is not allowed | message discarded");
				delete msg;
				return nullptr;
			}

			const MS_BYTE* attr_value_pos = data + pos + 4;

			switch (attr_type)
			{
				case Attribute::Username:
					msg->SetUsername((const char*)attr_value_pos, (size_t)attr_length);
					break;
				case Attribute::Priority:
					msg->SetPriority(Utils::Byte::Get4Bytes(attr_value_pos, 0));
					break;
				case Attribute::IceControlling:
					msg->SetIceControlling(Utils::Byte::Get8Bytes(attr_value_pos, 0));
					break;
				case Attribute::IceControlled:
					msg->SetIceControlled(Utils::Byte::Get8Bytes(attr_value_pos, 0));
					break;
				case Attribute::UseCandidate:
					msg->SetUseCandidate();
					break;
				case Attribute::MessageIntegrity:
					has_message_integrity = true;
					msg->SetMessageIntegrity(attr_value_pos);
					break;
				case Attribute::Fingerprint:
					has_fingerprint = true;
					fingerprint_attr_pos = pos;
					fingerprint = Utils::Byte::Get4Bytes(attr_value_pos, 0);
					msg->SetFingerprint();
					break;
				case Attribute::ErrorCode: {
					MS_BYTE error_class = Utils::Byte::Get1Byte(attr_value_pos, 2);
					MS_BYTE error_number = Utils::Byte::Get1Byte(attr_value_pos, 3);
					MS_2BYTES error_code = (MS_2BYTES)(error_class * 100 + error_number);
					msg->SetErrorCode(error_code);
					break;
				}
				default:
					break;
			}

			// Set next attribute position.
			pos = (size_t)Utils::Byte::PadTo4Bytes((MS_2BYTES)(pos + 4 + attr_length));
		}

		// Ensure current position matches the total length.
		if (pos != len)
		{
			MS_DEBUG("computed message size does not match total size | message discarded");
			delete msg;
			return nullptr;
		}

		// If it has FINGERPRINT attribute then verify it.
		if (has_fingerprint)
		{
			// Compute the CRC32 of the received message up to (but excluding) the
			// FINGERPRINT attribute and XOR it with 0x5354554e.
			MS_4BYTES computed_fingerprint = Utils::Crypto::CRC32(data, fingerprint_attr_pos) ^ 0x5354554e;

			// Compare with the FINGERPRINT value in the message.
			if (fingerprint != computed_fingerprint)
			{
				MS_DEBUG("computed FINGERPRINT value does not match the value in the message | message discarded");
				delete msg;
				return nullptr;
			}
		}

		return msg;
	}

	/* Instance methods. */

	STUNMessage::STUNMessage(Class klass, Method method, const MS_BYTE* transactionId, const MS_BYTE* raw, size_t length) :
		klass(klass),
		method(method),
		transactionId(transactionId),
		raw((MS_BYTE*)raw),
		length(length)
	{
		MS_TRACE();
	}

	STUNMessage::~STUNMessage()
	{
		MS_TRACE();

		if (this->isSerialized)
			delete this->raw;
	}

	STUNMessage::Authentication STUNMessage::CheckAuthentication(const std::string &local_username, const std::string &local_password)
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
			// Set the length: full length - header length (20) - FINGERPRINT length (8).
			Utils::Byte::Set2Bytes(this->raw, 2, (MS_2BYTES)(this->length - 20 - 8));

		// Calculate the HMAC-SHA1 of the message according to MESSAGE-INTEGRITY rules.
		const MS_BYTE* computed_message_integrity = Utils::Crypto::HMAC_SHA1(local_password, this->raw, (this->messageIntegrity - 4) - this->raw);

		Authentication result;

		// Compare the computed HMAC-SHA1 with the MESSAGE-INTEGRITY in the message.
		if (std::memcmp(this->messageIntegrity, computed_message_integrity, 20) == 0)
			result = Authentication::OK;
		else
			result = Authentication::Unauthorized;

		// Restore the header length field.
		if (this->hasFingerprint)
			Utils::Byte::Set2Bytes(this->raw, 2, (MS_2BYTES)(this->length - 20));

		return result;
	}

	STUNMessage* STUNMessage::CreateSuccessResponse()
	{
		MS_TRACE();

		MS_ASSERT(this->klass == Class::Request, "attempt to create a success response for a non Request STUN message");

		return new STUNMessage(Class::SuccessResponse, this->method, this->transactionId, nullptr, 0);
	}

	STUNMessage* STUNMessage::CreateErrorResponse(MS_2BYTES errorCode)
	{
		MS_TRACE();

		MS_ASSERT(this->klass == Class::Request, "attempt to create an error response for a non Request STUN message");

		STUNMessage* response = new STUNMessage(Class::ErrorResponse, this->method, this->transactionId, nullptr, 0);

		response->SetErrorCode(errorCode);

		return response;
	}

	void STUNMessage::Authenticate(const std::string &password)
	{
		// Just for Request, Indication and SuccessResponse messages.
		if (this->klass == Class::ErrorResponse)
		{
			MS_ERROR("cannot set password for ErrorResponse messages");
			return;
		}

		this->password = password;
	}

	void STUNMessage::Serialize()
	{
		MS_TRACE();

		if (this->isSerialized)
			delete this->raw;

		// Set this->isSerialized so the destructor will free the buffer.
		this->isSerialized = true;

		// Some useful variables.
		MS_2BYTES username_padded_len = 0;
		MS_2BYTES xor_mapped_address_padded_len = 0;
		bool add_xor_mapped_address = (this->xorMappedAddress && this->method == STUNMessage::Method::Binding && this->klass == Class::SuccessResponse);
		bool add_error_code = (this->errorCode && this->klass == Class::ErrorResponse);
		bool add_message_integrity = (this->klass != Class::ErrorResponse && !this->password.empty());
		bool add_fingerprint = true;  // Do always.

		// First calculate the total required size for the entire message.
		this->length = 20;  // Header.

		if (!this->username.empty())
		{
			username_padded_len = Utils::Byte::PadTo4Bytes((MS_2BYTES)this->username.length());
			this->length += 4 + username_padded_len;
		}

		if (this->priority)
			this->length += 4 + 4;

		if (this->iceControlling)
			this->length += 4 + 8;

		if (this->iceControlled)
			this->length += 4 + 8;

		if (this->hasUseCandidate)
			this->length += 4;

		if (add_xor_mapped_address) {
			switch (this->xorMappedAddress->sa_family) {
				case AF_INET:
					xor_mapped_address_padded_len = 8;
					this->length += 4 + 8;
					break;
				case AF_INET6:
					xor_mapped_address_padded_len = 20;
					this->length += 4 + 20;
					break;
				default:
					MS_ERROR("invalid inet family in XOR-MAPPED-ADDRESS attribute");
					add_xor_mapped_address = false;
					break;
			}
		}

		if (add_error_code)
			this->length += 4 + 4;

		if (add_message_integrity)
			this->length += 4 + 20;

		if (add_fingerprint)
			this->length += 4 + 4;

		// Allocate it.
		this->raw = new MS_BYTE[this->length];

		// Merge class and method fields into type.
		MS_2BYTES type_field = ((MS_2BYTES)this->method & 0x0f80) << 2;
		type_field |= ((MS_2BYTES)this->method & 0x0070) << 1;
		type_field |= ((MS_2BYTES)this->method & 0x000f);
		type_field |= ((MS_2BYTES)this->klass & 0x02) << 7;
		type_field |= ((MS_2BYTES)this->klass & 0x01) << 4;

		// Set type field.
		Utils::Byte::Set2Bytes(this->raw, 0, type_field);

		// Set length field.
		Utils::Byte::Set2Bytes(this->raw, 2, (MS_2BYTES)this->length - 20);

		// Set magic cookie.
		std::memcpy(this->raw + 4, STUNMessage::magicCookie, 4);

		// Set TransactionId field.
		std::memcpy(this->raw + 8, this->transactionId, 12);

		// Update the transaction ID pointer.
		this->transactionId = this->raw + 8;

		// Add atributes.
		size_t pos = 20;

		// Add USERNAME.
		if (username_padded_len)
		{
			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::Username);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, (MS_2BYTES)this->username.length());
			std::memcpy(this->raw + pos + 4, this->username.c_str(), this->username.length());
			pos += 4 + username_padded_len;
		}

		// Add PRIORITY.
		if (this->priority)
		{
			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::Priority);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, 4);
			Utils::Byte::Set4Bytes(this->raw, pos + 4, this->priority);
			pos += 4 + 4;
		}

		// Add ICE-CONTROLLING.
		if (this->iceControlling)
		{
			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::IceControlling);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, 8);
			Utils::Byte::Set8Bytes(this->raw, pos + 4, this->iceControlling);
			pos += 4 + 8;
		}

		// Add ICE-CONTROLLED.
		if (this->iceControlled)
		{
			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::IceControlled);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, 8);
			Utils::Byte::Set8Bytes(this->raw, pos + 4, this->iceControlled);
			pos += 4 + 8;
		}

		// Add USE-CANDIDATE.
		if (this->hasUseCandidate)
		{
			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::UseCandidate);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, 0);
			pos += 4;
		}

		// Add XOR-MAPPED-ADDRESS
		if (add_xor_mapped_address)
		{
			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::XorMappedAddress);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, xor_mapped_address_padded_len);
			MS_BYTE* attr_value = this->raw + pos + 4;
			switch (this->xorMappedAddress->sa_family)
			{
				case AF_INET: {
					// Set first byte to 0.
					attr_value[0] = 0;
					// Set inet family.
					attr_value[1] = 0x01;
					// Set port and XOR it.
					std::memcpy(attr_value + 2, &((const sockaddr_in*)(this->xorMappedAddress))->sin_port, 2);
					attr_value[2] ^= STUNMessage::magicCookie[0];
					attr_value[3] ^= STUNMessage::magicCookie[1];
					// Set address and XOR it.
					std::memcpy(attr_value + 4, &((const sockaddr_in*)this->xorMappedAddress)->sin_addr.s_addr, 4);
					attr_value[4] ^= STUNMessage::magicCookie[0];
					attr_value[5] ^= STUNMessage::magicCookie[1];
					attr_value[6] ^= STUNMessage::magicCookie[2];
					attr_value[7] ^= STUNMessage::magicCookie[3];

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
					attr_value[2] ^= STUNMessage::magicCookie[0];
					attr_value[3] ^= STUNMessage::magicCookie[1];
					// Set address and XOR it.
					std::memcpy(attr_value + 4, &((const sockaddr_in6*)this->xorMappedAddress)->sin6_addr.s6_addr, 16);
					attr_value[4] ^= STUNMessage::magicCookie[0];
					attr_value[5] ^= STUNMessage::magicCookie[1];
					attr_value[6] ^= STUNMessage::magicCookie[2];
					attr_value[7] ^= STUNMessage::magicCookie[3];
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
			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::ErrorCode);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, 4);
			MS_BYTE code_class = (MS_BYTE)(this->errorCode / 100);
			MS_BYTE code_number = (MS_BYTE)this->errorCode - (code_class * 100);
			Utils::Byte::Set2Bytes(this->raw, pos + 4, 0);
			Utils::Byte::Set1Byte(this->raw, pos + 6, code_class);
			Utils::Byte::Set1Byte(this->raw, pos + 7, code_number);
			pos += 4 + 4;
		}

		// Add MESSAGE-INTEGRITY.
		if (add_message_integrity)
		{
			// Ignore FINGERPRINT.
			if (add_fingerprint)
				Utils::Byte::Set2Bytes(this->raw, 2, (MS_2BYTES)(this->length - 20 - 8));

			// Calculate the HMAC-SHA1 of the message according to MESSAGE-INTEGRITY rules.
			const MS_BYTE* computed_message_integrity = Utils::Crypto::HMAC_SHA1(this->password, this->raw, pos);

			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::MessageIntegrity);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, 20);
			std::memcpy(this->raw + pos + 4, computed_message_integrity, 20);

			// Update the pointer.
			this->messageIntegrity = this->raw + pos + 4;
			pos += 4 + 20;

			// Restore length field.
			if (add_fingerprint)
				Utils::Byte::Set2Bytes(this->raw, 2, (MS_2BYTES)(this->length - 20));
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
			MS_4BYTES computed_fingerprint = Utils::Crypto::CRC32(this->raw, pos) ^ 0x5354554e;

			Utils::Byte::Set2Bytes(this->raw, pos, (MS_2BYTES)Attribute::Fingerprint);
			Utils::Byte::Set2Bytes(this->raw, pos + 2, 4);
			Utils::Byte::Set4Bytes(this->raw, pos + 4, computed_fingerprint);
			pos += 4 + 4;
			// Set flag.
			this->hasFingerprint = true;
		}
		else
		{
			this->hasFingerprint = false;
		}

		MS_ASSERT(pos == this->length, "pos != this->length");
	}

	void STUNMessage::Dump()
	{
		MS_TRACE();

		if (!Logger::HasDebugLevel())
			return;

		MS_DEBUG("[STUNMessage]");
		std::string klass;
		switch (this->klass)
		{
			case Class::Request:         klass = "Request";         break;
			case Class::Indication:      klass = "Indication";      break;
			case Class::SuccessResponse: klass = "SuccessResponse"; break;
			case Class::ErrorResponse:   klass = "ErrorResponse";   break;
		}
		if (this->method == Method::Binding)
			MS_DEBUG("- Binding %s", klass.c_str());
		else
			// This prints the unknown method number. Example: TURN Allocate => 0x003.
			MS_DEBUG("- %s with unknown method %#.3x", klass.c_str(), (unsigned int)this->method);
		MS_DEBUG("- Length (with header): %zu bytes", this->length);
		char transaction_id[25];
		for (int i=0; i<12; i++)
		{
			// NOTE: n must be 3 because snprintf adds a \0 after printed chars.
			std::snprintf(transaction_id+(i*2), 3, "%.2x", this->transactionId[i]);
		}
		MS_DEBUG("- TransactionId: %s", transaction_id);
		if (this->errorCode)
			MS_DEBUG("- ErrorCode: %u", (unsigned int)this->errorCode);
		if (!this->username.empty())
			MS_DEBUG("- Username: %s", this->username.c_str());
		if (this->priority)
			MS_DEBUG("- Priority: %llu", (unsigned long long)this->priority);
		if (this->iceControlling)
			MS_DEBUG("- IceControlling: %llu", (unsigned long long)this->iceControlling);
		if (this->iceControlled)
			MS_DEBUG("- IceControlled: %llu", (unsigned long long)this->iceControlled);
		if (this->hasUseCandidate)
			MS_DEBUG("- has UseCandidate");
		if (this->xorMappedAddress)
		{
			int family;
			MS_PORT port;
			std::string ip;
			Utils::IP::GetAddressInfo(this->xorMappedAddress, &family, ip, &port);
			MS_DEBUG("- XorMappedAddress: %s : %u", ip.c_str(), (unsigned int)port);
		}
		if (this->messageIntegrity)
		{
			char message_integrity[41];
			for (int i=0; i<20; i++)
			{
				std::snprintf(message_integrity+(i*2), 3, "%.2x", this->messageIntegrity[i]);
			}
			MS_DEBUG("- MessageIntegrity: %s", message_integrity);
		}
		if (this->hasFingerprint)
			MS_DEBUG("- has Fingerprint");
		MS_DEBUG("[/STUNMessage]");
	}
}  // namespace RTC
