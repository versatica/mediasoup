#ifndef MS_RTC_RTCP_SDES_HPP
#define MS_RTC_RTCP_SDES_HPP

#include "common.hpp"
#include "RTC/RTCP/Packet.hpp"
#include <vector>
#include <map>
#include <string>

namespace RTC { namespace RTCP
{
	/* SDES Item. */
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
			char value[];
		};

	public:
		static SdesItem* Parse(const uint8_t* data, size_t len);
		static const std::string& Type2String(Type type);

	public:
		explicit SdesItem(Header* header);
		explicit SdesItem(SdesItem* item);
		SdesItem(Type type, size_t len, const char* value);
		~SdesItem();

		void Dump();
		size_t Serialize(uint8_t* buffer);
		size_t GetSize() const;

		SdesItem::Type GetType() const;
		uint8_t GetLength() const;
		char* GetValue() const;

	private:
		// Passed by argument.
		Header* header = nullptr;
		std::unique_ptr<uint8_t> raw;

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
		explicit SdesChunk(const uint32_t ssrc);
		explicit SdesChunk(SdesChunk* chunk);
		~SdesChunk();

		void Dump();
		void Serialize();
		size_t Serialize(uint8_t* buffer);
		size_t GetSize() const;
		uint32_t GetSsrc() const;
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
		virtual ~SdesPacket();

		void AddChunk(SdesChunk* chunk);
		Iterator Begin();
		Iterator End();

	/* Pure virtual methods inherited from Packet. */
	public:
		virtual void Dump() override;
		virtual size_t Serialize(uint8_t* buffer) override;
		virtual size_t GetCount() const override;
		virtual size_t GetSize() const override;

	private:
		std::vector<SdesChunk*> chunks;
	};

	/* SDES Item inline instance methods */

	inline
	SdesItem::SdesItem(Header* header) :
		header(header)
	{}

	inline
	SdesItem::SdesItem(SdesItem* item) :
		header(item->header)
	{}

	inline
	SdesItem::~SdesItem()
	{
	}

	inline
	size_t SdesItem::GetSize() const
	{
		return 2 + size_t(this->header->length);
	}

	inline
	SdesItem::Type SdesItem::GetType() const
	{
		return (SdesItem::Type)this->header->type;
	}

	inline
	uint8_t SdesItem::GetLength() const
	{
		return this->header->length;
	}

	inline
	char* SdesItem::GetValue() const
	{
		return this->header->value;
	}

	/* Inline instance methods. */

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
		for (auto item : this->items)
		{
			delete item;
		}
	}

	inline
	size_t SdesChunk::GetSize() const
	{
		size_t size = sizeof(this->ssrc);

		for (auto item : this->items)
		{
			size += item->GetSize();
		}

		// http://stackoverflow.com/questions/11642210/computing-padding-required-for-n-byte-alignment
		// Consider pading to 32 bits (4 bytes) boundary.
		return (size + 3) & ~3;
	}

	inline
	uint32_t SdesChunk::GetSsrc() const
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

	/* Inline instance methods. */

	inline
	SdesPacket::SdesPacket()
		: Packet(Type::SDES)
	{}

	inline
	SdesPacket::~SdesPacket()
	{
		for (auto chunk : this->chunks)
		{
			delete chunk;
		}
	}

	inline
	size_t SdesPacket::GetCount() const
	{
		return this->chunks.size();
	}

	inline
	size_t SdesPacket::GetSize() const
	{
		size_t size = sizeof(Packet::CommonHeader);

		for (auto chunk : this->chunks)
		{
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
}}

#endif
