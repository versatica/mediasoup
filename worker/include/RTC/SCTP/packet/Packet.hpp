#ifndef MS_RTC_SCTP_PACKET_HPP
#define MS_RTC_SCTP_PACKET_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/Chunk.hpp"
#include "RTC/Serializable.hpp"
#include <vector>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * SCTP Packet.
		 *
		 * @see RFC 9260.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                         Common Header                         |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           Chunk #1                            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                              ...                              |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           Chunk #n                            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * It's mandatory that the packet total length is multiple of 4 bytes.
		 */

		/**
		 * SCTP Common Header.
		 *
		 *  0                   1                   2                   3
		 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |      Source Port Number       |    Destination Port Number    |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                       Verification Tag                        |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 * |                           Checksum                            |
		 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * - Source port (16 bits).
		 * - Destination port (16 bits).
		 * - Verification Tag (32 bits).
		 * - Checksum (32 bits).
		 */

		class Packet : public Serializable
		{
		public:
			using ChunksIterator = typename std::vector<Chunk*>::const_iterator;

		public:
			/**
			 * Struct of an SCTP Packet Common Header.
			 *
			 * @remarks
			 * - This struct is guaranteed to be aligned to 4 bytes.
			 */
			struct CommonHeader
			{
				uint16_t sourcePort;
				uint16_t destinationPort;
				uint32_t verificationTag;
				uint32_t checksum;
			};

		public:
			static constexpr size_t CommonHeaderLength{ 12 };

			/**
			 * Whether given buffer could be a valid SCTP packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the packet.
			 * - This check is very lazy. It should NEVER be done before checking if
			 *   given buffer is an RTP or RTCP packet.
			 */
			static bool IsSctp(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Parse an SCTP packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the packet.
			 */
			static Packet* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create an SCTP packet.
			 *
			 * @remarks
			 * - If `transactionId` is not given then a random Transaction ID is
			 *   generated.
			 */
			static Packet* Factory(uint8_t* buffer, size_t bufferLength);

		private:
			/**
			 * Constructor is private because we only want to create packet instances
			 * via `Parse()` and `Factory()`.
			 */
			Packet(uint8_t* buffer, size_t bufferLength);

		public:
			~Packet() override;

			void Dump(int indentation = 0) const final;

			void Serialize(uint8_t* buffer, size_t bufferLength) final;

			Packet* Clone(uint8_t* buffer, size_t bufferLength) const final;

			uint16_t GetSourcePort() const
			{
				return ntohs(GetHeaderPointer()->sourcePort);
			}

			void SetSourcePort(uint16_t sourcePort);

			uint16_t GetDestinationPort() const
			{
				return ntohs(GetHeaderPointer()->destinationPort);
			}

			void SetDestinationPort(uint16_t destinationPort);

			uint32_t GetVerificationTag() const
			{
				return ntohl(GetHeaderPointer()->verificationTag);
			}

			void SetVerificationTag(uint32_t verificationTag);

			uint32_t GetChecksum() const
			{
				return ntohl(GetHeaderPointer()->checksum);
			}

			void SetChecksum(uint32_t checksum);

			bool HasChunks() const
			{
				return this->chunks.size() > 0;
			}

			size_t GetChunksCount() const
			{
				return this->chunks.size();
			}

			ChunksIterator ChunksBegin() const
			{
				return this->chunks.begin();
			}

			ChunksIterator ChunksEnd() const
			{
				return this->chunks.end();
			}

			const Chunk* GetChunkAt(size_t idx) const
			{
				if (idx >= this->chunks.size())
				{
					return nullptr;
				}

				return this->chunks[idx];
			}

			template<typename T>
			const T* GetFirstChunkOfType() const
			{
				for (const auto* chunk : this->chunks)
				{
					if (typeid(*chunk) == typeid(T))
					{
						return static_cast<const T*>(chunk);
					}
				}

				return nullptr;
			}

			/**
			 * Clone given chunk into packet's buffer.
			 *
			 * @remarks
			 * - Once this method is called, the caller may want to free the original
			 *   given chunk (otherwise it will leak since the packet manages a clone
			 *   of it).
			 *
			 * @throw
			 * - MediaSoupError - If `BuildChunkInPlace()` was called before and the
			 *   caller didn't invoke `Consolidate()` on the returned chunk yet.
			 */
			void AddChunk(const Chunk* chunk);

			/**
			 * Build a chunk within the packet's buffer and append it to the list of
			 * chunks. The caller can perform modifications in that chunk and those
			 * will affect the packet body where the chunk is serialzed. The desired
			 * chunk class type is given via template argument.
			 *
			 * @returns Pointer of the created chunk specific class.
			 *
			 * @throw
			 * - MediaSoupError - If `BuildChunkInPlace()` was called before and the
			 *   caller didn't invoke `Consolidate()` on the returned chunk yet.
			 *
			 * @remarks
			 * - The caller MUST invoke `Consolidate()` once the chunk is completed.
			 * - The caller MUST NOT call `BuildChunkInPlace()` while other chunk is
			 *   in progress.
			 * - The caller MUST NOT free the obtained chunk pointer since it's now
			 *   part of the packet.
			 * - The caller MUST free the obtained chunk only in case the
			 *   `Consolidate()` method on the chunk throws.
			 * - Method implemented in header file due to C++ template usage.
			 *
			 * @example
			 * ```c++
			 * auto* initChunk = packet->BuildChunkInPlace<InitChunk>();
			 * ```
			 */
			template<typename T>
			T* BuildChunkInPlace()
			{
				AssertDoesNotNeedConsolidation();

				// The new chunk will be added after other chunks in the packet, this is,
				// at the end of the packet, whose length we know it's padded to 4 bytes,
				// and each parameter total length is also multiple of 4 bytes.
				auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();
				// The remaining length in the buffer is the potential buffer length
				// of the chunk.
				size_t chunkMaxBufferLength = GetBufferLength() - (ptr - GetBuffer());

				auto* chunk = T::Factory(ptr, chunkMaxBufferLength);

				// NOTE: Do not fix/update the chunk buffer length since the caller
				// probably wants to modify the chunk.

				HandleInPlaceChunk(chunk);

				return chunk;
			}

			/**
			 * Whether `BuildChunkInPlace()` was called and the caller didn't invoke
			 * `Consolidate()` on the returned chunk yet.
			 */
			bool NeedsConsolidation() const
			{
				return this->needsConsolidation;
			}

			/**
			 * Calculate CRC32C value of the whole packet and insert it into the
			 * Checksum field.
			 */
			void WriteCRC32cChecksum();

			/**
			 * Validate CRC32C value in the Checksum field.
			 */
			bool ValidateCRC32cChecksum() const;

		private:
			/**
			 * NOTE: Return CommonHeader* instead of const CommonHeader* since we may
			 * want to modify its fields.
			 */
			CommonHeader* GetHeaderPointer() const
			{
				return reinterpret_cast<CommonHeader*>(const_cast<uint8_t*>(GetBuffer()));
			}

			uint8_t* GetChunksPointer() const
			{
				return const_cast<uint8_t*>(GetBuffer()) + Packet::CommonHeaderLength;
			}

			virtual void HandleInPlaceChunk(Chunk* chunk) final;

			virtual void AssertDoesNotNeedConsolidation() const final;

		private:
			// Chunks.
			std::vector<Chunk*> chunks;
			// Whether `BuildChunkInPlace()` was called and the caller didn't invoke
			// `Consolidate()` on the returned chunk yet.
			bool needsConsolidation{ false };
		};
	} // namespace SCTP
} // namespace RTC

#endif
