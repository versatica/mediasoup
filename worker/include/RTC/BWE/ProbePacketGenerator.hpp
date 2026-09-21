#ifndef MS_RTC_BWE_PROBE_PACKET_GENERATOR_HPP
#define MS_RTC_BWE_PROBE_PACKET_GENERATOR_HPP

#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTP/Packet.hpp"
#include <array>

namespace RTC
{
	namespace BWE
	{
		/**
		 * Makes the RTP packets a probing burst is made of.
		 *
		 * A burst asks for a number of bytes to be put on the wire over a few
		 * milliseconds, and those bytes have to travel as RTP so that the receiver
		 * reports when each of them arrived. This turns the bytes into packets and
		 * hands them over one by one.
		 *
		 * What it produces is a stream of its own that carries nothing: a fixed SSRC
		 * the receiver has negotiated, a payload of zeroes that nothing decodes, and
		 * the header extensions that make the packets reportable. The only thing ever
		 * read back from them is when each one arrived.
		 */
		class ProbePacketGenerator
		{
		public:
			class Listener
			{
			public:
				virtual ~Listener() = default;

			public:
				/**
				 * A packet of the burst is ready.
				 *
				 * @returns Whether the rest of the burst is still wanted, so that a
				 * packet that couldn't be sent stops the ones behind it.
				 *
				 * @remarks
				 * - The packet must be sent before returning, since the next one is
				 *   built over the very same instance.
				 */
				virtual bool OnProbePacketGeneratorSendRtpPacket(
				  ProbePacketGenerator* probePacketGenerator, RTC::RTP::Packet* packet) = 0;
			};

#ifdef MS_TEST
		public:
#else
		private:
#endif
			/**
			 * Largest a packet of a burst may be (bytes), which leaves room below the
			 * MTU for the overhead that encrypting it adds on the way out.
			 */
			static constexpr size_t MaxPacketLength{ 1400 };

			/**
			 * Most bytes a single burst may be asked for, which is the highest bitrate
			 * the bandwidth estimation deals in held for 20 ms.
			 *
			 * @remarks
			 * - A burst larger than this is taken as one that cannot be real, and
			 *   asking for it would have this handing packets over for a very long
			 *   while, all within a single iteration of the event loop.
			 * - Those 20 ms are the longest time between two shots of a burst that
			 *   whoever asks for them is configured with, which this cannot enforce.
			 *   Raising that configuration beyond them starts cutting bursts that are
			 *   perfectly real.
			 */
			static constexpr size_t MaxGeneratePacketsSize{ (RTC::Consts::BweMaxBitrate * 20 * 1000) /
			                                                (8 * 1000000) };

		public:
			explicit ProbePacketGenerator(Listener* listener);

			~ProbePacketGenerator() = default;

			// NOTE: The packet is written over a buffer of this very instance, so one
			// that was copied or moved would keep writing over the buffer of the one
			// it came from.
			ProbePacketGenerator(const ProbePacketGenerator&)            = delete;
			ProbePacketGenerator& operator=(const ProbePacketGenerator&) = delete;

			/**
			 * Hand over the packets that carry the given count of bytes.
			 *
			 * @remarks
			 * - What goes out is at least what was asked for, since a burst falling
			 *   short of the bytes it was meant to carry measures something else, and
			 *   the last packet can't be smaller than a header.
			 */
			void GeneratePackets(size_t size);

		private:
			// Passed by argument.
			Listener* listener{ nullptr };
			// Others.
			// Bytes the packet is written over. It belongs to this instance rather
			// than being shared, since the packet's header lives in it and the
			// sequence number of the next packet is read back from there. The empty
			// braces zero it, so the payload never carries whatever was in that
			// memory before.
			std::array<uint8_t, MaxPacketLength> packetBuffer{};
			// Allocated by this.
			std::unique_ptr<RTC::RTP::Packet> packet;
			// Length of the packet with no payload at all, so its header and its
			// extensions.
			size_t minPacketLength{ 0 };
		};
	} // namespace BWE
} // namespace RTC

#endif
