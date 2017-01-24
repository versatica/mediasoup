#ifndef MS_RTC_RTCP_PACKET_H
#define MS_RTC_RTCP_PACKET_H

#include "common.h"
#include <map>
#include <string>

#define MS_RTCP_BUFFER_SIZE 65536

namespace RTC { namespace RTCP
{
	enum class Type : uint8_t
	{
		FIR   = 192,
		NACK  = 193,
		SR    = 200,
		RR    = 201,
		SDES  = 202,
		BYE   = 203,
		APP   = 204,
		RTPFB = 205,
		PSFB  = 206
	};

	class Packet
	{
	public:
		/* Struct for RTCP common header. */
		struct CommonHeader
		{
			#if defined(MS_LITTLE_ENDIAN)
				uint8_t count:5;
				uint8_t padding:1;
				uint8_t version:2;
			#elif defined(MS_BIG_ENDIAN)
				uint8_t version:2;
				uint8_t padding:1;
				uint8_t count:5;
			#endif
			uint8_t packet_type:8;
			uint16_t length:16;
		};

	public:
		static bool IsRtcp(const uint8_t* data, size_t len);
		static Packet* Parse(const uint8_t* data, size_t len);

	private:
		static const std::string& Type2String(Type type);

	private:
		static std::map<Type, std::string> type2String;

	public:
		explicit Packet(Type type);
		virtual ~Packet();

		void SetNext(Packet* packet);
		Packet* GetNext();
		Type GetType();
		const uint8_t* GetData();

	public:
		virtual void Dump() = 0;
		virtual size_t Serialize(uint8_t* buffer) = 0;
		virtual size_t GetCount() = 0;
		virtual size_t GetSize() = 0;

	private:
		Type type;
		Packet* next = nullptr;
		CommonHeader* header = nullptr;
	};

	/* Inline static methods. */

	inline
	bool Packet::IsRtcp(const uint8_t* data, size_t len)
	{
		CommonHeader* header = const_cast<CommonHeader*>(reinterpret_cast<const CommonHeader*>(data));

		return (
			(len >= sizeof(CommonHeader)) &&
			// DOC: https://tools.ietf.org/html/draft-ietf-avtcore-rfc5764-mux-fixes
			(data[0] > 127 && data[0] < 192) &&
			// RTP Version must be 2.
			(header->version == 2) &&
			// RTCP packet types defined by IANA:
			// http://www.iana.org/assignments/rtp-parameters/rtp-parameters.xhtml#rtp-parameters-4
			// RFC 5761 (RTCP-mux) states this range for secure RTCP/RTP detection.
			(header->packet_type >= 192 && header->packet_type <= 223)
		);
	}

	/* Inline instance methods. */

	inline
	Packet::Packet(Type type)
		:type(type)
	{}

	inline
	Packet::~Packet()
	{}

	inline
	Packet* Packet::GetNext()
	{
		return this->next;
	}

	inline
	void Packet::SetNext(Packet* packet)
	{
		this->next = packet;
	}

	inline
	Type Packet::GetType()
	{
		return this->type;
	}

	inline
	size_t Packet::GetCount()
	{
		return 0;
	}

	inline
	const uint8_t* Packet::GetData()
	{
		return (uint8_t*)this->header;
	}
}}

#endif
