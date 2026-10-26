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
	 * error: 'abs-capture-time' is exact while a Sender Report is biased by its own
	 * one way delay.
	 *
	 * The source is chosen from what the Producers of the sender negotiated. The
	 * first Producer chooses it, and a single Producer that did not negotiate
	 * 'abs-capture-time' moves it to Sender Report for the rest of the life of this
	 * object, never back.
	 *
	 * A sender may negotiate that extension and then never send it, which would
	 * leave every stream of it without a capture instant forever. So the source also
	 * falls back to Sender Report when no stream of this sender has carried the
	 * extension for long enough since its first packet.
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
		 * Offset between the wall clock of this sender and our own monotonic one (us).
		 *
		 * @returns No value while the offset cannot be told yet.
		 */
		std::optional<int64_t> GetClockOffsetUs() const
		{
			return this->clockOffsetEstimator.GetOffsetUs();
		}

		/**
		 * Capture instant of the given RTP timestamp of the given RTP stream,
		 * expressed in our own monotonic clock.
		 *
		 * @param rtpStream - RTP stream the RTP timestamp belongs to.
		 * @param ts - RTP timestamp whose capture instant is wanted.
		 * @param nowUs - Current instant, which is also what the fall back to Sender
		 *   Report is timed against.
		 *
		 * @returns No value while the capture instant cannot be told yet.
		 */
		std::optional<int64_t> GetLocalCaptureAtUs(
		  const RTC::RTP::RtpStreamRecv* rtpStream, uint32_t ts, int64_t nowUs);

	private:
		/**
		 * Move the source to Sender Report when 'abs-capture-time' was negotiated but
		 * no stream of this sender has ever carried it, which is told apart from it
		 * being merely early by how long this sender has been sending.
		 *
		 * @param rtpStream - RTP stream whose packet is being looked at.
		 * @param nowUs - Current instant.
		 */
		void MayFallBackToSenderReport(const RTC::RTP::RtpStreamRecv* rtpStream, int64_t nowUs);

	private:
		// Offset between the wall clock of the sender and our monotonic one.
		RTC::RemoteClockOffsetEstimator clockOffsetEstimator;
		// Source the capture instant is read from, for every stream of this sender.
		std::optional<Source> source;
		// Instant the first packet of this sender was asked about, or no value until
		// then. The fall back to Sender Report is measured against it, so it must be
		// something that exists even when the extension never arrives.
		//
		// NOTE: Not every packet is asked about, only the one holding the highest RTP
		// timestamp of its stream, so this is the first one that got here rather than
		// the first one that arrived.
		std::optional<int64_t> firstPacketAtUs;
		// Whether any stream of this sender has ever carried 'abs-capture-time', which
		// is what tells a sender that does not send it from a stream whose first
		// packets carrying it are still on their way.
		bool absCaptureTimeReceived{ false };
	};
} // namespace RTC

#endif
