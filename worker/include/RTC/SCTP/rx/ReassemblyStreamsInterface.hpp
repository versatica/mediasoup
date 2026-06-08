#ifndef MS_RTC_SCTP_REASSEMBLY_STREAMS_INTERFACE_HPP
#define MS_RTC_SCTP_REASSEMBLY_STREAMS_INTERFACE_HPP

#include "common.hpp"
#include "RTC/SCTP/packet/UserData.hpp"
#include "RTC/SCTP/packet/chunks/AnyForwardTsnChunk.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SCTP/public/SctpTypes.hpp"
#include <span>

namespace RTC
{
	namespace SCTP
	{
		/**
		 * Implementations of this interface will be called when data is received,
		 * when data should be skipped/forgotten or when sequence number should be
		 * reset.
		 *
		 * As a result of these operations - mainly when data is received - the
		 * implementations of this interface should notify when a message has been
		 * assembled, by calling the provided callback of type `OnAssembledMessage()`.
		 * How it assembles messages will depend on e.g. if a message was sent on an
		 * ordered or unordered stream.
		 *
		 * Implementations will - for each operation - indicate how much additional
		 * memory that has been used as a result of performing the operation. This
		 * is used to limit the maximum amount of memory used, to prevent
		 * out-of-memory situations.
		 */
		class ReassemblyStreamsInterface
		{
		public:
			/**
			 * This callback will be provided as an argument to the constructor of the
			 * concrete class implementing this interface and should be called when a
			 * message has been assembled as well as indicating from which TSNs this
			 * message was assembled from.
			 */
			using OnAssembledMessage =
			  std::function<void(std::span<const Types::UnwrappedTsn> tsns, Message message)>;

		public:
			virtual ~ReassemblyStreamsInterface() = default;

		public:
			/**
			 * Adds a data chunk to a stream as identified in `data`. If it was the
			 * last remaining chunk in a message, reassemble one (or several, in case
			 * of ordered chunks) messages.
			 *
			 * Returns the additional number of bytes added to the queue as a result
			 * of performing this operation. If this addition resulted in messages
			 * being assembled and delivered, this may be negative.
			 */
			virtual int32_t AddData(Types::UnwrappedTsn tsn, UserData data) = 0;

			/**
			 * Called for incoming FORWARD-TSN/I-FORWARD-TSN chunks - when the sender
			 * wishes the received to skip/forget about data up until the provided
			 * TSN. This is used to implement partial reliability, such as limiting
			 * the number of retransmissions or the an expiration duration. As a
			 * result of skipping data, this may result in the implementation being
			 * able to assemble messages in ordered streams.
			 *
			 * Returns the number of bytes removed from the queue as a result of this
			 * operation.
			 */
			virtual size_t HandleForwardTsn(
			  Types::UnwrappedTsn newCumulativeTsn,
			  std::span<const AnyForwardTsnChunk::SkippedStream> skippedStreams) = 0;

			/**
			 * Called for incoming (possibly deferred) RE-CONFIG chunks asking for
			 * either a few streams, or all streams (when the list is empty) to be
			 * reset - to have their next SSN or message ID to be zero.
			 */
			virtual void ResetStreams(std::span<const uint16_t> streamIds) = 0;
		};
	} // namespace SCTP
} // namespace RTC

#endif
