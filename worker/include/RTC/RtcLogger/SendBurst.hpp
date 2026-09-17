#ifndef MS_RTC_RTC_LOGGER_SEND_BURST_HPP
#define MS_RTC_RTC_LOGGER_SEND_BURST_HPP

#include "common.hpp"
#include <map>
#include <string>

namespace RTC
{
	namespace RtcLogger
	{
		/**
		 * How many RTP packets a transport sends within a single iteration of the event
		 * loop.
		 *
		 * Nothing spaces those packets out, so a whole iteration's worth of them reaches
		 * the wire back to back and lands in the first queue of the path as a single
		 * lump. This gathers how big those lumps are and how much of each one is
		 * retransmissions, which is what says whether there is anything worth pacing.
		 */
		class SendBurst
		{
		private:
			/**
			 * What all the bursts of a given size added up to.
			 */
			struct Counters
			{
				/**
				 * How many of them there were.
				 */
				uint64_t count{ 0 };
				/**
				 * Retransmissions they carried between all of them, so that the tail of
				 * the distribution can be attributed.
				 */
				uint64_t retransmissions{ 0 };
				/**
				 * Probation packets they carried between all of them.
				 */
				uint64_t probations{ 0 };
			};

		public:
			SendBurst()  = default;
			~SendBurst() = default;

			/**
			 * Account for a packet that is about to be sent.
			 *
			 * @param loopTimeMs - Time at which the current iteration of the event loop
			 * began, which is what tells one burst from the next.
			 */
			void Sent(uint64_t loopTimeMs, size_t length, bool isRetransmission, bool isProbation);

			/**
			 * Print what has been gathered so far and start over.
			 */
			void Log();

		private:
			/**
			 * Add the burst in progress, if there is one, to the distribution.
			 */
			void CloseBurst();

			void Clear();

		public:
			std::string transportId;

		private:
			// Time of the iteration of the event loop that the burst in progress belongs
			// to.
			uint64_t loopTimeMs{ 0 };
			size_t packets{ 0 };
			size_t retransmissions{ 0 };
			size_t probations{ 0 };
			size_t bytes{ 0 };
			// Bursts seen, keyed by how many packets each of them carried.
			std::map<size_t /*packets*/, Counters> bursts;
			size_t maxPackets{ 0 };
			size_t maxBytes{ 0 };
		};
	} // namespace RtcLogger
} // namespace RTC

#endif
