#ifndef MS_RTC_SCTP_CHUNK_HPP
#define MS_RTC_SCTP_CHUNK_HPP

#include "common.hpp"
#include <absl/container/flat_hash_map.h>
#include <string>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Chunk.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |  Chunk Type   |  Chunk Flags  |         Chunk Length          |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * \                                                               \
		 * /                          Chunk Value                          /
		 * \                                                               \
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 */
		class Chunk
		{
		public:
			/**
			 * Chunk types.
			 */
			enum class ChunkType : uint8_t
			{
				DATA              = 0x00,
				INIT              = 0x01,
				INIT_ACK          = 0x02,
				SACK              = 0x03,
				HEARTBEAT         = 0x04,
				HEARTBEAT_ACK     = 0x05,
				ABORT             = 0x06,
				SHUTDOWN          = 0x07,
				SHUTDOWN_ACK      = 0x08,
				ERROR             = 0x09,
				COOKIE_ECHO       = 0x0A,
				COOKIE_ACK        = 0x0B,
				ECNE              = 0x0C,
				CWR               = 0x0D,
				SHUTDOWN_COMPLETE = 0x0E
			};

			/**
			 * Struct of a SCTP Chunk Header.
			 */
			struct Header
			{
				ChunkType type;
				uint8_t flags;
				/**
				 * The value of the Chunk Length field, which represents the size of the
				 * chunk in bytes, including the Chunk Type, Chunk Flags, Chunk Length
				 * and Chunk Value fields. So if the Chunk Value field is zero-length,
				 * the Length field must be 4. The Chunk Length field does not count any
				 * chunk padding.
				 */
				uint16_t length;
			};

		public:
			static const size_t HeaderLength{ 4 };

			/**
			 * Parses given `data` with length `len` and returns an allocated instance
			 * of Chunk (or nullptr if it's not a valid SCTP chunk).
			 *
			 * If `exactLen` is set to true, then given `len` must be the total length
			 * of this chunk. Otherwise we assume that other chunks could follow this
			 * one.
			 *
			 * So if `exactLen` is set to true, `len` must be multiple of 4 bytes (it
			 * must include padding if needed).
			 */
			static Chunk* Parse(const uint8_t* data, size_t len, bool exactLen);

			static const std::string& ChunkType2String(ChunkType chunkType);

		private:
			static absl::flat_hash_map<ChunkType, std::string> chunkType2String;

		public:
			Chunk(const uint8_t* data, size_t size);

			virtual ~Chunk();

			void Dump() const;

			const uint8_t* GetData() const
			{
				return reinterpret_cast<const uint8_t*>(this->data);
			}

			size_t GetSize() const
			{
				return this->size;
			}

			ChunkType GetType() const
			{
				return static_cast<ChunkType>(this->header->type);
			}

			void SetType(ChunkType type)
			{
				this->header->type = static_cast<ChunkType>(type);
			}

			uint8_t GetFlags() const
			{
				return this->header->flags;
			}

			void SetFlags(uint8_t flags)
			{
				this->header->flags = flags;
			}

			/**
			 * The length of the Chunk Value field. It does not count any padding.
			 *
			 * @remarks
			 * This is not the value of the Chunk Length field in the Chunk Header
			 * but the real length of the Chunk Value.
			 */
			uint16_t GetValueLength() const
			{
				return uint16_t{ ntohs(this->header->length) } - Chunk::HeaderLength;
			}

		private:
			void SetSize(size_t size)
			{
				this->size = size;
			}

		private:
			// Pointer to the data buffer containing the chunk.
			uint8_t* data{ nullptr };
			// Full size of the chunk in bytes.
			size_t size{ 0u };
			// Pointer to the Chunk Header (it points to `data` too).
			Header* header{ nullptr };
		};
	} // namespace SCTP
} // namespace RTC

#endif
