#ifndef MS_RTC_REMOTE_CAPTURE_TIME_ESTIMATOR_HPP
#define MS_RTC_REMOTE_CAPTURE_TIME_ESTIMATOR_HPP

#include "common.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RemoteClockOffsetEstimator.hpp"

namespace RTC
{
	/**
	 * Tells at which instant of our own monotonic clock the media carried by a given
	 * RTP timestamp was captured, for every RTP stream of a same remote sender.
	 *
	 * The capture instant is read from one of two sources, and the same one is used
	 * for every stream of the sender. Mixing them would reintroduce the very inter
	 * stream skew this is meant to remove, since they do not carry the same residual
	 * error: `abs-capture-time` is exact while a Sender Report is biased by its own
	 * one way delay.
	 *
	 * The source is chosen from what the Producers of the sender negotiated. The
	 * first Producer chooses it, and a single Producer that did not negotiate
	 * `abs-capture-time` moves it to Sender Report for the rest of the life of this
	 * object, never back.
	 *
	 * A single instance is meant to be shared by all the RTP streams of a given
	 * CNAME, which is what identifies media coming from a same machine and hence
	 * from a same wall clock.
	 */
	class RemoteCaptureTimeEstimator
	{
	public:
		enum class Source : uint8_t
		{
			ABS_CAPTURE_TIME = 1,
			SENDER_REPORT
		};

	public:
		/**
		 * Take a Producer of this sender into account to choose the source.
		 *
		 * @param absCaptureTimeNegotiated - Whether the Producer negotiated the
		 * `abs-capture-time` RTP header extension.
		 */
		void UpdateSource(bool absCaptureTimeNegotiated);

		/**
		 * Source the capture instant is being read from.
		 *
		 * @returns No value until the first Producer has been taken into account.
		 */
		std::optional<Source> GetSource() const
		{
			return this->source;
		}

		/**
		 * Notify that a Sender Report has been received on one of the RTP streams of
		 * this sender.
		 */
		void SenderReportReceived(const RTC::RTP::RtpStreamRecv* rtpStream);

		/**
		 * Capture instant of the given RTP timestamp of the given RTP stream,
		 * expressed in our own monotonic clock.
		 *
		 * @param rtpStream - RTP stream the RTP timestamp belongs to.
		 * @param ts - RTP timestamp whose capture instant is wanted.
		 *
		 * @returns No value while the capture instant cannot be told yet.
		 */
		std::optional<uint64_t> GetLocalCaptureMs(const RTC::RTP::RtpStreamRecv* rtpStream, uint32_t ts) const;

	private:
		// Offset between the wall clock of the sender and our monotonic one.
		RTC::RemoteClockOffsetEstimator clockOffsetEstimator;
		// Source the capture instant is read from, for every stream of this sender.
		std::optional<Source> source;
	};
} // namespace RTC

#endif
