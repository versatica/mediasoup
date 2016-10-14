#ifndef MS_RTC_RTCP_SDES_H
#define MS_RTC_RTCP_SDES_H

#include "common.h"
#include "RTC/RTCP/Packet.h"

#include <vector>
#include <map>
#include <string>

namespace RTC { namespace RTCP
{

	/* SDES Item */
	class SdesItem
	{

	public:
		typedef enum Type : uint8_t
		{
			END = 0,
			CNAME,
			NAME,
			EMAIL,
			PHONE,
			LOC,
			TOOL,
			NOTE,
			PRIV
		} Type;

	private:
		struct Header
		{
			uint8_t type;
			uint8_t length;
			char	value[];
		};

	public:
		static SdesItem* Parse(const uint8_t* data, size_t len);
		static const std::string& Type2String(Type type);

	public:
		SdesItem(Header* header);
		SdesItem(SdesItem* item);
		SdesItem(Type type, size_t len, const char* value);
		~SdesItem();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();

		SdesItem::Type GetType();
		uint8_t GetLength();
		char* GetValue();

	private:
		// Passed by argument.
		Header* header = nullptr;
		uint8_t* raw = nullptr;

	private:
		static std::map<SdesItem::Type, std::string> type2String;
	};

	class SdesChunk
	{
	public:
		typedef std::vector<SdesItem*>::iterator Iterator;

	public:
		static SdesChunk* Parse(const uint8_t* data, size_t len);

	public:
		SdesChunk(const uint32_t ssrc);
		SdesChunk(SdesChunk* chunk);
		~SdesChunk();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* data);
		size_t GetSize();

		uint32_t GetSsrc();
		void SetSsrc(uint32_t ssrc);
		void AddItem(SdesItem* item);
		Iterator Begin();
		Iterator End();

	private:
		uint32_t ssrc;
		std::vector<SdesItem*> items;
	};

	class SdesPacket
		: public Packet
	{
	public:
		typedef std::vector<SdesChunk*>::iterator Iterator;

	public:
		static SdesPacket* Parse(const uint8_t* data, size_t len);

	public:
		SdesPacket();
		~SdesPacket();

		// Virtual methods inherited from Packet
		void Dump() override;
		size_t Serialize(uint8_t* data) override;
		size_t GetCount() override;
		size_t GetSize() override;

	public:
		void AddChunk(SdesChunk* chunk);
		Iterator Begin();
		Iterator End();

	private:
		std::vector<SdesChunk*> chunks;
	};

	/* SDES Item inline instance methods */

	inline
	SdesItem::SdesItem(Header* header) :
		header(header)
	{
	}

	inline
	SdesItem::SdesItem(SdesItem* item) :
		header(item->header)
	{
	}

	inline
	SdesItem::~SdesItem()
	{
		if (this->raw) {
			delete this->raw;
		}
	}

	inline
	size_t SdesItem::GetSize()
	{
		return 2 + size_t(this->header->length);
	}

	inline
	SdesItem::Type SdesItem::GetType()
	{
		return (SdesItem::Type)this->header->type;
	}

	inline
	uint8_t SdesItem::GetLength()
	{
		return this->header->length;
	}

	inline
	char* SdesItem::GetValue()
	{
		return this->header->value;
	}

	/* SDES Chunk inline instance methods */

	inline
	SdesChunk::SdesChunk(uint32_t ssrc)
	{
		this->ssrc = htonl(ssrc);
	}

	inline
	SdesChunk::SdesChunk(SdesChunk* chunk)
	{
		this->ssrc = chunk->ssrc;

		for (Iterator it = chunk->Begin(); it != chunk->End(); ++it)
		{
			this->AddItem(new SdesItem(*it));
		}
	}

	inline
	SdesChunk::~SdesChunk()
	{
		for (auto item : this->items) {
			delete item;
		}
	}

	inline
	size_t SdesChunk::GetSize()
	{
		size_t size = sizeof(this->ssrc);

		for (auto item : this->items) {
			size += item->GetSize();
		}

		// http://stackoverflow.com/questions/11642210/computing-padding-required-for-n-byte-alignment
		// Consider pading to 32 bits (4 bytes) boundary
		return (size + 3) & ~3;
	}

	inline
	uint32_t SdesChunk::GetSsrc()
	{
		return (uint32_t)ntohl(this->ssrc);
	}

	inline
	void SdesChunk::SetSsrc(uint32_t ssrc)
	{
		this->ssrc = (uint32_t)htonl(ssrc);
	}

	inline
	void SdesChunk::AddItem(SdesItem* item)
	{
		this->items.push_back(item);
	}

	inline
	SdesChunk::Iterator SdesChunk::Begin()
	{
		return this->items.begin();
	}

	inline
	SdesChunk::Iterator SdesChunk::End()
	{
		return this->items.end();
	}

	/* SDES Packet inline instance methods */

	inline
	SdesPacket::SdesPacket()
		: Packet(Type::SDES)
	{
	}

	inline
	SdesPacket::~SdesPacket()
	{
		for (auto chunk : this->chunks) {
			delete chunk;
		}
	}

	inline
	size_t SdesPacket::GetCount()
	{
		return this->chunks.size();
	}

	inline
	size_t SdesPacket::GetSize()
	{
		size_t size = sizeof(Packet::CommonHeader);

		for (auto chunk : this->chunks) {
			size += chunk->GetSize();
		}

		return size;
	}

	inline
	void SdesPacket::AddChunk(SdesChunk* chunk)
	{
		this->chunks.push_back(chunk);
	}

	inline
	SdesPacket::Iterator SdesPacket::Begin()
	{
		return this->chunks.begin();
	}

	inline
	SdesPacket::Iterator SdesPacket::End()
	{
		return this->chunks.end();
	}
}
}

#endif
