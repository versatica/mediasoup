#ifndef MS_RTC_NEW_RTCP_COMPOUND_PACKET_HPP
#define MS_RTC_NEW_RTCP_COMPOUND_PACKET_HPP

#include "common.hpp"
#include "RTC/NEW_RTCP/Packet.hpp"
#include "RTC/Serializable.hpp"
#include <vector>

namespace RTC
{
	namespace NEW_RTCP
	{
		/**
		 * RTCP Compound Packet.
		 *
		 * @see RFC 3550.
		 */
		class CompoundPacket : public Serializable
		{
		public:
			using PacketsIterator = typename std::vector<Packet*>::const_iterator;

			/**
			 * Parse an RTCP compound packet.
			 *
			 * @remarks
			 * - `bufferLength` must be the exact length of the compound packet.
			 */
			static CompoundPacket* Parse(const uint8_t* buffer, size_t bufferLength);

			/**
			 * Create an RTCP compound packet.
			 */
			static CompoundPacket* Factory(uint8_t* buffer, size_t bufferLength);

		protected:
			/**
			 * Constructor is protected because we only want to create packets
			 * instances via Parse() and Factory() in subclasses.
			 */
			CompoundPacket(uint8_t* buffer, size_t bufferLength);

		public:
			~CompoundPacket() override;

			void Dump(int indentation = 0) const override;

			/**
			 * Must be overridden by each subclass.
			 */
			CompoundPacket* Clone(uint8_t* buffer, size_t bufferLength) const override;

			bool HasPackets() const
			{
				return this->packets.size() > 0;
			}

			size_t GetPacketsCount() const
			{
				return this->packets.size();
			}

			PacketsIterator PacketsBegin() const
			{
				return this->packets.begin();
			}

			PacketsIterator PacketsEnd() const
			{
				return this->packets.end();
			}

			const Packet* GetPacketAt(size_t idx) const
			{
				if (idx >= this->packets.size())
				{
					return nullptr;
				}

				return this->packets[idx];
			}

			template<typename T>
			const T* GetFirstPacketOfType() const
			{
				for (const auto* packet : this->packets)
				{
					if (typeid(*packet) == typeid(T))
					{
						return static_cast<const T*>(packet);
					}
				}

				return nullptr;
			}

			/**
			 * Clone given packet into compound packet's buffer.
			 *
			 * @remarks
			 * - Once this method is called, the caller may want to free the original
			 *   given packet (otherwise it will leak since the compound packet manages
			 *   a clone of it).
			 *
			 * @throw
			 * - MediaSoupError - If `BuildPacketInPlace()` was called before and the
			 *   caller didn't invoke `Consolidate()` on the returned packet yet.
			 */
			void AddPacket(const Packet* packet);

			/**
			 * Build a packet within the compound packet's buffer and append it to the
			 * list of packets. The caller can perform modifications in that packet
			 * and those will affect the compound packet body where the packet is
			 * serialzed. The desired packet class type is given via template argument.
			 *
			 * @returns Pointer of the created packet specific class.
			 *
			 * @throw
			 * - MediaSoupError - If `BuildPacketInPlace()` was called before and the
			 *   caller didn't invoke `Consolidate()` on the returned packet yet.
			 *
			 * @remarks
			 * - The caller MUST invoke `Consolidate()` once the packet is completed.
			 * - The caller MUST NOT call `BuildPacketInPlace()` while other packet is
			 *   in progress.
			 * - The caller MUST NOT free the obtained packet pointer since it's now
			 *   part of the compound packet.
			 * - The caller MUST free the obtained packet only in case the
			 *   `Consolidate()` method on the packet throws.
			 * - Method implemented in header file due to C++ template usage.
			 *
			 * @example
			 * ```c++
			 * auto* fooPacket = compoundPacket->BuildPacketInPlace<FooPacket>();
			 * ```
			 */
			template<typename T>
			T* BuildPacketInPlace()
			{
				AssertDoesNotNeedConsolidation();

				// The new packet will be added after other packet in the compound
				// packet, this is, at the end of the compound packet.
				auto* ptr = const_cast<uint8_t*>(GetBuffer()) + GetLength();
				// The remaining length in the buffer is the potential buffer length
				// of the packet.
				size_t packetMaxBufferLength = GetBufferLength() - (ptr - GetBuffer());

				auto* packet = T::Factory(ptr, packetMaxBufferLength);

				// NOTE: Do not fix/update the packet buffer length since the caller
				// probably wants to modify the packet.

				HandleInPlacePacket(packet);

				return packet;
			}

			/**
			 * Whether `BuildPacketInPlace()` was called and the caller didn't invoke
			 * `Consolidate()` on the returned packet yet.
			 */
			bool NeedsConsolidation() const
			{
				return this->needsConsolidation;
			}

		private:
			virtual void HandleInPlacePacket(Packet* packet) final;

			virtual void AssertDoesNotNeedConsolidation() const final;

		private:
			// Packets.
			std::vector<Packet*> packets;
			// Whether `BuildPacketInPlace()` was called and the caller didn't invoke
			// `Consolidate()` on the returned packet yet.
			bool needsConsolidation{ false };
		};
	} // namespace NEW_RTCP
} // namespace RTC

#endif
