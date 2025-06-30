#ifndef MS_RTC_SHARED_RTP_PACKET_HPP
#define MS_RTC_SHARED_RTP_PACKET_HPP

#include "common.hpp"
#include "RTC/RtpPacket.hpp"

namespace RTC
{
	class SharedRtpPacket
	{
	public:
		/**
		 * Empty constructor.
		 */
		SharedRtpPacket();

		/**
		 * Constructor with RtpPacket pointer. If a packet is given it's internally
		 * cloned.
		 */
		explicit SharedRtpPacket(const RTC::RtpPacket* packet);

		/**
		 * Copy constructor.
		 *
		 * @remarks
		 * No need to declare it but let's be explicit.
		 */
		SharedRtpPacket(const SharedRtpPacket&) = default;

		/**
		 * Copy assignment operator.
		 *
		 * @remarks
		 * No need to declare it but let's be explicit.
		 */
		SharedRtpPacket& operator=(const SharedRtpPacket&) = default;

		/**
		 * Destructor.
		 */
		~SharedRtpPacket();

	public:
		void Dump(int indentation = 0) const;

		bool HasPacket() const
		{
			return this->sharedPtr->get() != nullptr;
		}

		RTC::RtpPacket* GetPacket() const
		{
			return this->sharedPtr->get();
		}

		/**
		 * Assign given packet (could be nullptr). If packet is given it's internally
		 * cloned.
		 */
		void Assign(const RTC::RtpPacket* packet);

		void Reset();

	private:
		// NOTE: This needs to be a shared pointer that holds an unique pointer.
		// Otherwise, when copying/storing the shared pointer in other locations
		// (buffers, etc), reseting its internal value wouldn't affect other copies
		// of the shared pointer.
		std::shared_ptr<std::unique_ptr<RTC::RtpPacket>> sharedPtr;
	};
} // namespace RTC

#endif
