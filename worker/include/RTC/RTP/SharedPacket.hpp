#ifndef MS_RTC_RTP_SHARED_PACKET_HPP
#define MS_RTC_RTP_SHARED_PACKET_HPP

#include "common.hpp"
#include "RTC/RTP/Packet.hpp"

namespace RTC
{
	namespace RTP
	{
		class SharedPacket
		{
		public:
#ifdef MS_DUMP_RTP_SHARED_PACKET_MEMORY_USAGE
			thread_local static uint64_t allocatedMemory;
#endif

		public:
			/**
			 * Empty constructor.
			 */
			SharedPacket();

			/**
			 * Constructor with RTP Packet pointer. If a packet is given it's internally
			 * cloned.
			 */
			explicit SharedPacket(RTC::RTP::Packet* packet);

			/**
			 * Copy constructor.
			 *
			 * @remarks
			 * No need to declare it but let's be explicit.
			 */
			SharedPacket(const SharedPacket&) = default;

			/**
			 * Copy assignment operator.
			 *
			 * @remarks
			 * - No need to declare it but let's be explicit.
			 *
			 * @throws MediasoupError if the Packet is too big.
			 */
			SharedPacket& operator=(const SharedPacket&) = default;

			/**
			 * Destructor.
			 */
			~SharedPacket() = default;

		public:
			void Dump(int indentation = 0) const;

			bool HasPacket() const
			{
				return this->sharedPtr->get() != nullptr;
			}

			RTC::RTP::Packet* GetPacket() const
			{
				return this->sharedPtr->get();
			}

			/**
			 * Assign given packet (could be nullptr). If packet is given it's internally
			 * cloned.
			 *
			 * @throws MediasoupError if the Packet is too big.
			 */
			void Assign(RTC::RTP::Packet* packet);

			/**
			 * Resets the internal packet to nullptr.
			 *
			 * @remarks
			 * This affects to ALL copies of this SharedPacket object.
			 */
			void Reset();

			/**
			 * Assert that RTP Packet contained in this SharedPacket is a clone of the
			 * given other packet (or there is no packet inside and no other packet has
			 * been given).
			 */
			void AssertSamePacket(const RTC::RTP::Packet* otherPacket) const;

		private:
			void StorePacket(RTC::RTP::Packet* packet);

		private:
			// NOTE: This needs to be a shared pointer that holds an unique pointer.
			// Otherwise, when copying/storing the shared pointer in other locations
			// (buffers, etc), reseting its internal value wouldn't affect other copies
			// of the shared pointer.
			std::shared_ptr<std::unique_ptr<RTC::RTP::Packet>> sharedPtr;
		};
	} // namespace RTP
} // namespace RTC

#endif
