#ifndef MS_RTC_RTP_PACKET_HPP
#define MS_RTC_RTP_PACKET_HPP

#include "common.hpp"
#include "Utils.hpp"
#include "RTC/Codecs/PayloadDescriptorHandler.hpp"
#include <map>
#include <string>

namespace RTC
{
	// Max RTP length.
	constexpr size_t RtpBufferSize{ 65536 };
	// Max MTU size.
	constexpr size_t MtuSize{ 1500 };

	class RtpPacket
	{
	public:
		/* Struct for RTP header. */
		struct Header
		{
#if defined(MS_LITTLE_ENDIAN)
			uint8_t csrcCount : 4;
			uint8_t extension : 1;
			uint8_t padding : 1;
			uint8_t version : 2;
			uint8_t payloadType : 7;
			uint8_t marker : 1;
#elif defined(MS_BIG_ENDIAN)
			uint8_t version : 2;
			uint8_t padding : 1;
			uint8_t extension : 1;
			uint8_t csrcCount : 4;
			uint8_t marker : 1;
			uint8_t payloadType : 7;
#endif
			uint16_t sequenceNumber;
			uint32_t timestamp;
			uint32_t ssrc;
		};

	private:
		/* Struct for RTP extension header. */
		struct ExtensionHeader
		{
			uint16_t id;
			uint16_t length; // Size of value in multiples of 4 bytes.
			uint8_t value[1];
		};

	private:
		/* Struct for One-Byte extension. */
		struct OneByteExtension
		{
#if defined(MS_LITTLE_ENDIAN)
			uint8_t len : 4;
			uint8_t id : 4;
#elif defined(MS_BIG_ENDIAN)
			uint8_t id : 4;
			uint8_t len : 4;
#endif
			uint8_t value[1];
		};

	private:
		/* Struct for Two-Bytes extension. */
		struct TwoBytesExtension
		{
			uint8_t id : 8;
			uint8_t len : 8;
			uint8_t value[1];
		};

	public:
		static bool IsRtp(const uint8_t* data, size_t len);
		static RtpPacket* Parse(const uint8_t* data, size_t len);

	public:
		RtpPacket(
		  Header* header,
		  ExtensionHeader* extensionHeader,
		  const uint8_t* payload,
		  size_t payloadLength,
		  uint8_t payloadPadding,
		  size_t size);
		~RtpPacket();

		void Dump() const;
		const uint8_t* GetData() const;
		size_t GetSize() const;
		uint8_t GetPayloadType() const;
		void SetPayloadType(uint8_t payloadType);
		bool HasMarker() const;
		void SetMarker(bool marker);
		void SetPayloadPaddingFlag(bool flag);
		uint16_t GetSequenceNumber() const;
		void SetSequenceNumber(uint16_t seq);
		uint32_t GetTimestamp() const;
		void SetTimestamp(uint32_t timestamp);
		uint32_t GetSsrc() const;
		void SetSsrc(uint32_t ssrc);
		bool HasExtensionHeader() const;
		uint16_t GetExtensionHeaderId() const;
		size_t GetExtensionHeaderLength() const;
		uint8_t* GetExtensionHeaderValue() const;
		// After calling this method, all the extIDs are reset to 0.
		void MangleExtensionHeaderIds(const std::map<uint8_t, uint8_t>& idMapping);
		bool HasOneByteExtensions() const;
		bool HasTwoBytesExtensions() const;
		void SetAudioLevelExtensionId(uint8_t id);
		void SetVideoOrientationExtensionId(uint8_t id);
		void SetAbsSendTimeExtensionId(uint8_t id);
		void SetMidExtensionId(uint8_t id);
		void SetRidExtensionId(uint8_t id);
		void SetRepairedRidExtensionId(uint8_t id);
		bool ReadAudioLevel(uint8_t& volume, bool& voice) const;
		bool ReadVideoOrientation(bool& camera, bool& flip, uint16_t& rotation) const;
		bool ReadAbsSendTime(uint32_t& time) const;
		bool ReadMid(std::string& mid) const;
		bool ReadRid(std::string& rid) const;
		uint8_t* GetExtension(uint8_t id, uint8_t& len) const;
		uint8_t* GetPayload() const;
		size_t GetPayloadLength() const;
		uint8_t GetPayloadPadding() const;
		bool IsKeyFrame() const;
		RtpPacket* Clone(const uint8_t* buffer) const;
		void RtxEncode(uint8_t payloadType, uint32_t ssrc, uint16_t seq);
		bool RtxDecode(uint8_t payloadType, uint32_t ssrc);
		void SetPayloadDescriptorHandler(RTC::Codecs::PayloadDescriptorHandler* payloadDescriptorHandler);
		bool EncodePayload(RTC::Codecs::EncodingContext* context);
		void RestorePayload();
		void ShiftPayload(size_t payloadOffset, size_t shift, bool expand = true);

	private:
		void ParseExtensions();

	private:
		// Passed by argument.
		Header* header{ nullptr };
		uint8_t* csrcList{ nullptr };
		ExtensionHeader* extensionHeader{ nullptr };
		std::map<uint8_t, OneByteExtension*> oneByteExtensions;
		std::map<uint8_t, TwoBytesExtension*> twoBytesExtensions;
		uint8_t audioLevelExtensionId{ 0 };
		uint8_t videoOrientationExtensionId{ 0 };
		uint8_t absSendTimeExtensionId{ 0 };
		uint8_t midExtensionId{ 0 };
		uint8_t ridExtensionId{ 0 };
		uint8_t rridExtensionId{ 0 };
		uint8_t* payload{ nullptr };
		size_t payloadLength{ 0 };
		uint8_t payloadPadding{ 0 };
		size_t size{ 0 }; // Full size of the packet in bytes.
		// Codecs
		std::unique_ptr<Codecs::PayloadDescriptorHandler> payloadDescriptorHandler;
	};

	/* Inline static methods. */

	inline bool RtpPacket::IsRtp(const uint8_t* data, size_t len)
	{
		// NOTE: RtcpPacket::IsRtcp() must always be called before this method.

		auto header = const_cast<Header*>(reinterpret_cast<const Header*>(data));

		return (
		  (len >= sizeof(Header)) &&
		  // DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
		  (data[0] > 127 && data[0] < 192) &&
		  // RTP Version must be 2.
		  (header->version == 2));
	}

	/* Inline instance methods. */

	inline const uint8_t* RtpPacket::GetData() const
	{
		return (const uint8_t*)this->header;
	}

	inline size_t RtpPacket::GetSize() const
	{
		return this->size;
	}

	inline uint8_t RtpPacket::GetPayloadType() const
	{
		return this->header->payloadType;
	}

	inline void RtpPacket::SetPayloadType(uint8_t payloadType)
	{
		this->header->payloadType = payloadType;
	}

	inline bool RtpPacket::HasMarker() const
	{
		return this->header->marker;
	}

	inline void RtpPacket::SetMarker(bool marker)
	{
		this->header->marker = marker;
	}

	inline void RtpPacket::SetPayloadPaddingFlag(bool flag)
	{
		this->header->padding = flag;
	}

	inline uint16_t RtpPacket::GetSequenceNumber() const
	{
		return uint16_t{ ntohs(this->header->sequenceNumber) };
	}

	inline void RtpPacket::SetSequenceNumber(uint16_t seq)
	{
		this->header->sequenceNumber = uint16_t{ htons(seq) };
	}

	inline uint32_t RtpPacket::GetTimestamp() const
	{
		return uint32_t{ ntohl(this->header->timestamp) };
	}

	inline void RtpPacket::SetTimestamp(uint32_t timestamp)
	{
		this->header->timestamp = uint32_t{ htonl(timestamp) };
	}

	inline uint32_t RtpPacket::GetSsrc() const
	{
		return uint32_t{ ntohl(this->header->ssrc) };
	}

	inline void RtpPacket::SetSsrc(uint32_t ssrc)
	{
		this->header->ssrc = uint32_t{ htonl(ssrc) };
	}

	inline bool RtpPacket::HasExtensionHeader() const
	{
		return (this->extensionHeader ? true : false);
	}

	inline uint16_t RtpPacket::GetExtensionHeaderId() const
	{
		if (!this->extensionHeader)
			return 0;

		return uint16_t{ ntohs(this->extensionHeader->id) };
	}

	inline size_t RtpPacket::GetExtensionHeaderLength() const
	{
		if (!this->extensionHeader)
			return 0;

		return static_cast<size_t>(ntohs(this->extensionHeader->length) * 4);
	}

	inline uint8_t* RtpPacket::GetExtensionHeaderValue() const
	{
		if (!this->extensionHeader)
			return nullptr;

		return this->extensionHeader->value;
	}

	inline bool RtpPacket::HasOneByteExtensions() const
	{
		return GetExtensionHeaderId() == 0xBEDE;
	}

	inline bool RtpPacket::HasTwoBytesExtensions() const
	{
		return (GetExtensionHeaderId() & 0b1111111111110000) == 0b0001000000000000;
	}

	inline void RtpPacket::SetAudioLevelExtensionId(uint8_t id)
	{
		if (id == 0u)
			return;

		this->audioLevelExtensionId = id;
	}

	inline void RtpPacket::SetVideoOrientationExtensionId(uint8_t id)
	{
		if (id == 0u)
			return;

		this->videoOrientationExtensionId = id;
	}

	inline void RtpPacket::SetAbsSendTimeExtensionId(uint8_t id)
	{
		if (id == 0u)
			return;

		this->absSendTimeExtensionId = id;
	}

	inline void RtpPacket::SetMidExtensionId(uint8_t id)
	{
		if (id == 0u)
			return;

		this->midExtensionId = id;
	}

	inline void RtpPacket::SetRidExtensionId(uint8_t id)
	{
		if (id == 0u)
			return;

		this->ridExtensionId = id;
	}

	inline void RtpPacket::SetRepairedRidExtensionId(uint8_t id)
	{
		if (id == 0u)
			return;

		this->rridExtensionId = id;
	}

	inline bool RtpPacket::ReadAudioLevel(uint8_t& volume, bool& voice) const
	{
		uint8_t extenLen;
		uint8_t* extenValue = GetExtension(this->audioLevelExtensionId, extenLen);

		if (!extenValue || extenLen != 1)
			return false;

		volume = Utils::Byte::Get1Byte(extenValue, 0);
		voice  = (volume & (1 << 7)) ? true : false;
		volume &= ~(1 << 7);

		return true;
	}

	inline bool RtpPacket::ReadVideoOrientation(bool& camera, bool& flip, uint16_t& rotation) const
	{
		uint8_t extenLen;
		uint8_t* extenValue = GetExtension(this->videoOrientationExtensionId, extenLen);

		if (!extenValue || extenLen != 1)
			return false;

		uint8_t cvoByte       = Utils::Byte::Get1Byte(extenValue, 0);
		uint8_t cameraValue   = ((cvoByte & 0b00001000) >> 3);
		uint8_t flipValue     = ((cvoByte & 0b00000100) >> 2);
		uint8_t rotationValue = (cvoByte & 0b00000011);

		camera = cameraValue ? true : false;
		flip   = flipValue ? true : false;

		// Using counter clockwise values.
		switch (rotationValue)
		{
			case 3:
				rotation = 270;
				break;
			case 2:
				rotation = 180;
				break;
			case 1:
				rotation = 90;
				break;
			default:
				rotation = 0;
		}

		return true;
	}

	inline bool RtpPacket::ReadAbsSendTime(uint32_t& time) const
	{
		uint8_t extenLen;
		uint8_t* extenValue = GetExtension(this->absSendTimeExtensionId, extenLen);

		if (!extenValue || extenLen != 3)
			return false;

		time = Utils::Byte::Get3Bytes(extenValue, 0);

		return true;
	}

	inline bool RtpPacket::ReadMid(std::string& mid) const
	{
		uint8_t extenLen;
		uint8_t* extenValue = GetExtension(this->midExtensionId, extenLen);

		if (!extenValue || extenLen == 0)
			return false;

		mid.assign(reinterpret_cast<const char*>(extenValue), static_cast<size_t>(extenLen));

		return true;
	}

	inline bool RtpPacket::ReadRid(std::string& rid) const
	{
		// First try with the RID id then with the Repaired RID id.
		uint8_t extenLen;
		uint8_t* extenValue = GetExtension(this->ridExtensionId, extenLen);

		if (extenValue && extenLen > 0)
		{
			rid.assign(reinterpret_cast<const char*>(extenValue), static_cast<size_t>(extenLen));

			return true;
		}

		extenValue = GetExtension(this->rridExtensionId, extenLen);

		if (extenValue && extenLen > 0)
		{
			rid.assign(reinterpret_cast<const char*>(extenValue), static_cast<size_t>(extenLen));

			return true;
		}

		return false;
	}

	inline uint8_t* RtpPacket::GetExtension(uint8_t id, uint8_t& len) const
	{
		len = 0;

		if (id == 0u)
		{
			return nullptr;
		}
		else if (HasOneByteExtensions())
		{
			auto it = this->oneByteExtensions.find(id);

			if (it == this->oneByteExtensions.end())
				return nullptr;

			auto* extension = it->second;

			len = extension->len + 1;

			return extension->value;
		}
		else if (HasTwoBytesExtensions())
		{
			auto it = this->twoBytesExtensions.find(id);

			if (it == this->twoBytesExtensions.end())
				return nullptr;

			auto* extension = it->second;

			len = extension->len;

			// In Two-Byte extensions value length may be zero. If so, return nullptr.
			if (extension->len == 0)
				return nullptr;

			return extension->value;
		}
		else
		{
			return nullptr;
		}
	}

	inline uint8_t* RtpPacket::GetPayload() const
	{
		return this->payload;
	}

	inline size_t RtpPacket::GetPayloadLength() const
	{
		return this->payloadLength;
	}

	inline uint8_t RtpPacket::GetPayloadPadding() const
	{
		return this->payloadPadding;
	}

	inline bool RtpPacket::IsKeyFrame() const
	{
		if (!this->payloadDescriptorHandler)
			return false;

		return this->payloadDescriptorHandler->IsKeyFrame();
	}

	inline void RtpPacket::SetPayloadDescriptorHandler(
	  RTC::Codecs::PayloadDescriptorHandler* payloadDescriptorHandler)
	{
		this->payloadDescriptorHandler.reset(payloadDescriptorHandler);
	}
} // namespace RTC

#endif
