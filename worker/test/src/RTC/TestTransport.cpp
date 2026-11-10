#include "common.hpp"
#include "RTC/Consts.hpp"
#include "RTC/RTCP/packet/AnyReportPacket.hpp"
#include "RTC/RTCP/packet/CompoundPacket.hpp"
#include "RTC/RTCP/packet/Packet.hpp"
#include "RTC/RTCP/packet/ReceiverReportPacket.hpp"
#include "RTC/RTCP/packet/SdesPacket.hpp"
#include "RTC/RTCP/packet/SenderReportPacket.hpp"
#include "RTC/RTCP/packet/XrPacket.hpp"
#include "RTC/RTCP/packet/extendedReports/DlrrExtendedReport.hpp"
#include "RTC/RTCP/packet/extendedReports/RrtExtendedReport.hpp"
#include "RTC/RTP/Packet.hpp"
#include "RTC/RTP/RtpStream.hpp"
#include "RTC/RTP/RtpStreamRecv.hpp"
#include "RTC/RTP/RtpStreamSend.hpp"
#include "RTC/RtpDictionaries.hpp"
#include "Utils.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include "mocks/include/MockShared.hpp"
#include <ankerl/unordered_dense.h>
#include <catch2/catch_test_macros.hpp>
#include <string>
#include <vector>

SCENARIO("Transport", "[rtcp][transport]")
{
	rtpCommon::ResetBuffers();

	constexpr uint32_t RtcpSsrc{ 11223344 };
	constexpr int64_t NowUs{ 10 * 1000 * 1000 };
	// As long as the CNAME of a real Transport.
	const std::string rtcpCname{ "abcdefghijklmnopqrs" };

	class TestRtpStreamSendListener : public RTC::RTP::RtpStreamSend::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamRetransmitRtpPacket(
		  RTC::RTP::RtpStreamSend* /*rtpStream*/, RTC::RTP::Packet* /*packet*/) override
		{
		}
	};

	class TestRtpStreamRecvListener : public RTC::RTP::RtpStreamRecv::Listener
	{
	public:
		void OnRtpStreamScore(
		  RTC::RTP::RtpStream* /*rtpStream*/, uint8_t /*score*/, uint8_t /*previousScore*/) override
		{
		}

		void OnRtpStreamSendRtcpPacket(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, const RTC::RTCP::Packet* /*packet*/) override
		{
		}

		uint8_t OnRtpStreamNeedWorstRemoteFractionLost(RTC::RTP::RtpStreamRecv* /*rtpStream*/) override
		{
			return 0;
		}

		void OnRtpStreamSpatialLayerActivityChanged(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*spatialLayer*/, bool /*isActive*/) override
		{
		}

		void OnRtpStreamRestarted(
		  RTC::RTP::RtpStreamRecv* /*rtpStream*/, const RTC::RTP::Packet* /*packet*/) override
		{
		}

		void OnRtpStreamKeyFrameRequired(RTC::RTP::RtpStreamRecv* /*rtpStream*/) override
		{
		}
	};

	struct ReceivedReceiverReferenceTime
	{
		uint32_t compactNtp;
		int64_t receivedAtUs;
	};

	mocks::MockShared shared(/*getTimeUs*/
	                         []() -> int64_t
	                         {
		                         return NowUs;
	                         });

	TestRtpStreamSendListener rtpStreamSendListener;
	TestRtpStreamRecvListener rtpStreamRecvListener;
	// RTCP Receiver Reference Times received and not yet answered, keyed by the
	// SSRC of the XR packet that carried each one.
	ankerl::unordered_dense::map<uint32_t, ReceivedReceiverReferenceTime> receivedReceiverReferenceTimes;
	// Bytes of every compound packet sent.
	std::vector<std::vector<uint8_t>> sentCompoundPackets;
	alignas(4) uint8_t rtcpCompoundPacketBuffer[RTC::Consts::OutgoingRtcpPacketMaxSize];

	// Create a send stream that has sent a packet right now, so it has a Sender
	// Report to tell.
	const auto createRtpStreamSend =
	  [&shared, &rtpStreamSendListener](
	    uint32_t ssrc, const std::string& cname) -> std::unique_ptr<RTC::RTP::RtpStreamSend>
	{
		RTC::RTP::RtpStream::Params params;

		params.ssrc          = ssrc;
		params.clockRate     = 90000;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;
		params.cname         = cname;

		std::string mid;

		auto rtpStream = std::make_unique<RTC::RTP::RtpStreamSend>(
		  std::addressof(rtpStreamSendListener), std::addressof(shared), params, mid);

		const std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		packet->SetPayloadType(100);
		packet->SetSsrc(ssrc);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(1000);

		const auto result = rtpStream->ReceivePacket(packet.get());

		REQUIRE(result.accepted);

		return rtpStream;
	};

	// Create a receive stream that has received a packet right now, so it has a
	// report block to tell.
	const auto createRtpStreamRecv =
	  [&shared, &rtpStreamRecvListener](uint32_t ssrc) -> std::unique_ptr<RTC::RTP::RtpStreamRecv>
	{
		RTC::RTP::RtpStream::Params params;

		params.ssrc          = ssrc;
		params.clockRate     = 90000;
		params.mimeType.type = RTC::RtpCodecMimeType::Type::VIDEO;

		auto rtpStream = std::make_unique<RTC::RTP::RtpStreamRecv>(
		  std::addressof(rtpStreamRecvListener),
		  std::addressof(shared),
		  params,
		  /*sendNackDelayMs*/ 0,
		  /*useRtpInactivityCheck*/ false);

		const std::unique_ptr<RTC::RTP::Packet> packet(
		  RTC::RTP::Packet::Factory(rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)));

		packet->SetPayloadType(100);
		packet->SetSsrc(ssrc);
		packet->SetSequenceNumber(1);
		packet->SetTimestamp(1000);

		REQUIRE(rtpStream->ReceivePacket(packet.get(), NowUs));

		return rtpStream;
	};

	// Parse the compound packet sent at the given position.
	const auto parseSentCompoundPacket =
	  [&sentCompoundPackets](size_t idx) -> std::unique_ptr<RTC::RTCP::CompoundPacket>
	{
		REQUIRE(idx < sentCompoundPackets.size());

		const std::vector<uint8_t>& bytes = sentCompoundPackets.at(idx);

		REQUIRE(bytes.size() <= RTC::Consts::OutgoingRtcpPacketMaxSize);

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket(
		  RTC::RTCP::CompoundPacket::Parse(bytes.data(), bytes.size()));

		REQUIRE(compoundPacket);

		return compoundPacket;
	};

	// NOTE: Copy of `RTC::Transport::SendRtcp()`, which must be kept in sync with
	// it. The Consumers and Producers due to report are given as the RTP streams
	// of each of them, and every compound packet sent is stored in
	// `sentCompoundPackets`.
	const auto sendRtcp =
	  [&shared,
		 &rtcpCname,
		 &receivedReceiverReferenceTimes,
		 &sentCompoundPackets,
		 &rtcpCompoundPacketBuffer](
	    int64_t nowUs,
	    const std::vector<std::vector<RTC::RTP::RtpStreamSend*>>& dueConsumersRtpStreams,
	    const std::vector<std::vector<RTC::RTP::RtpStreamRecv*>>& dueProducersRtpStreams) -> void
	{
		const auto ntp = Utils::Time::TimeUsToNtp(nowUs + shared.GetNtpOffsetUs());

		std::unique_ptr<RTC::RTCP::CompoundPacket> compoundPacket;
		// Send streams whose Sender Report is in the compound packet. Their SDES
		// chunks go in the SDES packet that follows the reports.
		std::vector<RTC::RTP::RtpStreamSend*> senderReportRtpStreams;
		// Length of the SDES chunks of those streams.
		size_t sdesChunksLength{ 0 };
		// Number of DLRR sub-blocks in the XR packet that follows the reports, one
		// per received Receiver Reference Time it answers.
		size_t numDlrrSubBlocks{ 0 };
		// Receiver Report packet being built, if any.
		RTC::RTCP::ReceiverReportPacket* receiverReportPacket{ nullptr };
		// Whether some report block is in a Receiver Report of the compound
		// packet, which makes the XR packet carry a Receiver Reference Time.
		bool hasReceiverReports{ false };
		// Whether there is a Receiver Report packet in the compound packet, even
		// one with no report block.
		bool hasReceiverReportPacket{ false };
		// Whether some Consumer or Producer is due to report, which is what makes
		// an empty Receiver Report go out when there is nothing to report.
		bool rtcpDue{ false };
		// Length of the SDES chunk of this Transport, which goes in every compound
		// packet carrying a Receiver Report or an XR packet, since both are sent
		// with its RTCP SSRC.
		const size_t ownSdesChunkLength =
		  RTC::RTCP::SdesPacket::ComputeChunkLengthFor({ rtcpCname.size() });

		// Length of the SDES and XR packets that follow the reports in the
		// compound packet.
		const auto computeTrailingLength = [ownSdesChunkLength](
		                                     size_t chunksLength,
		                                     size_t numSenderReports,
		                                     size_t numSubBlocks,
		                                     bool withReceiverReports) -> size_t
		{
			const bool withXrPacket = withReceiverReports || numSubBlocks > 0;
			size_t trailingLength{ 0 };
			size_t extendedReportsLength{ 0 };

			if (numSenderReports > 0 || withXrPacket)
			{
				trailingLength += RTC::RTCP::SdesPacket::ComputeLengthFor(
				  chunksLength + (withXrPacket ? ownSdesChunkLength : 0));
			}

			if (numSubBlocks > 0)
			{
				extendedReportsLength += RTC::RTCP::DlrrExtendedReport::ComputeLengthFor(numSubBlocks);
			}

			if (withReceiverReports)
			{
				extendedReportsLength += RTC::RTCP::RrtExtendedReport::RrtExtendedReportLength;
			}

			if (extendedReportsLength > 0)
			{
				trailingLength += RTC::RTCP::XrPacket::ComputeLengthFor(extendedReportsLength);
			}

			return trailingLength;
		};

		// Start a new compound packet.
		const auto startCompoundPacket = [&receivedReceiverReferenceTimes,
		                                  &rtcpCompoundPacketBuffer,
		                                  &compoundPacket,
		                                  &senderReportRtpStreams,
		                                  &sdesChunksLength,
		                                  &numDlrrSubBlocks,
		                                  &hasReceiverReports,
		                                  &hasReceiverReportPacket]() -> void
		{
			compoundPacket.reset();
			compoundPacket.reset(
			  RTC::RTCP::CompoundPacket::Factory(
			    rtcpCompoundPacketBuffer, sizeof(rtcpCompoundPacketBuffer)));

			senderReportRtpStreams.clear();
			sdesChunksLength = 0;
			numDlrrSubBlocks = std::min(
			  receivedReceiverReferenceTimes.size(),
			  RTC::Consts::MaxRtcpDelaySinceLastReceiverReportSubBlocks);
			hasReceiverReports      = false;
			hasReceiverReportPacket = false;
		};

		// Consolidate the Receiver Report packet being built, if any.
		const auto consolidateReceiverReportPacket = [&receiverReportPacket]() -> void
		{
			if (!receiverReportPacket)
			{
				return;
			}

			receiverReportPacket->Consolidate();

			receiverReportPacket = nullptr;
		};

		// Add the SDES and XR packets that follow the reports and send the
		// compound packet, if it has any report.
		const auto sendCompoundPacket = [&rtcpCname,
		                                 &receivedReceiverReferenceTimes,
		                                 &sentCompoundPackets,
		                                 &ntp,
		                                 nowUs,
		                                 ownSdesChunkLength,
		                                 &compoundPacket,
		                                 &senderReportRtpStreams,
		                                 &numDlrrSubBlocks,
		                                 &hasReceiverReports,
		                                 &hasReceiverReportPacket,
		                                 &consolidateReceiverReportPacket]() -> void
		{
			consolidateReceiverReportPacket();

			if (senderReportRtpStreams.empty() && !hasReceiverReportPacket)
			{
				return;
			}

			// The XR packet goes whenever there is a report block, whose Receiver
			// Reference Time it carries, or some DLRR sub-block to send.
			const bool withXrPacket = hasReceiverReports || numDlrrSubBlocks > 0;
			// The Receiver Report and XR packets are sent with the RTCP SSRC of this
			// Transport, whose CNAME must then be in the SDES packet.
			const bool withOwnSdesChunk = hasReceiverReportPacket || withXrPacket;

			if (!senderReportRtpStreams.empty() || withOwnSdesChunk)
			{
				auto* const sdesPacket = compoundPacket->BuildPacketInPlace<RTC::RTCP::SdesPacket>();

				if (withOwnSdesChunk)
				{
					REQUIRE(sdesPacket->GetAvailableLength() >= ownSdesChunkLength);

					sdesPacket->AddChunk(RtcpSsrc);
					sdesPacket->AddItem(RTC::RTCP::SdesPacket::ItemType::CNAME, rtcpCname);
				}

				for (auto* const rtpStream : senderReportRtpStreams)
				{
					const bool added = rtpStream->AddRtcpSdesChunk(sdesPacket);

					REQUIRE(added);
				}

				sdesPacket->Consolidate();
			}

			if (withXrPacket)
			{
				auto* const xrPacket = compoundPacket->BuildPacketInPlace<RTC::RTCP::XrPacket>();

				xrPacket->SetSsrc(RtcpSsrc);

				if (hasReceiverReports)
				{
					auto* const rrtExtendedReport =
					  xrPacket->BuildExtendedReportInPlace<RTC::RTCP::RrtExtendedReport>();

					rrtExtendedReport->SetNtp(ntp);
					rrtExtendedReport->Consolidate();
				}

				if (numDlrrSubBlocks > 0)
				{
					auto* const dlrrExtendedReport =
					  xrPacket->BuildExtendedReportInPlace<RTC::RTCP::DlrrExtendedReport>();

					// SSRCs whose received Receiver Reference Time is answered here, so it is
					// not answered again.
					std::vector<uint32_t> answeredSsrcs;

					answeredSsrcs.reserve(numDlrrSubBlocks);

					for (const auto& kv : receivedReceiverReferenceTimes)
					{
						if (answeredSsrcs.size() == numDlrrSubBlocks)
						{
							break;
						}

						const uint32_t ssrc                                                = kv.first;
						const ReceivedReceiverReferenceTime& receivedReceiverReferenceTime = kv.second;
						// Get delay in microseconds.
						const int64_t delayUs = nowUs - receivedReceiverReferenceTime.receivedAtUs;
						// Express delay in units of 1/65536 seconds.
						auto dlrr = static_cast<uint32_t>((delayUs / 1000000) << 16);

						dlrr |= static_cast<uint32_t>(((delayUs % 1000000) * 65536) / 1000000);

						REQUIRE(
						  dlrrExtendedReport->GetAvailableLength() >=
						  RTC::RTCP::DlrrExtendedReport::SubBlockLength);

						dlrrExtendedReport->AddSubBlock(
						  RTC::RTCP::DlrrExtendedReport::SubBlock{ .ssrc = ssrc,
							                                         .lastReceiverReport =
							                                           receivedReceiverReferenceTime.compactNtp,
							                                         .delaySinceLastReceiverReport = dlrr });

						answeredSsrcs.push_back(ssrc);
					}

					dlrrExtendedReport->Consolidate();

					for (const uint32_t ssrc : answeredSsrcs)
					{
						receivedReceiverReferenceTimes.erase(ssrc);
					}
				}

				xrPacket->Consolidate();
			}

			sentCompoundPackets.emplace_back(
			  compoundPacket->GetBuffer(), compoundPacket->GetBuffer() + compoundPacket->GetLength());
		};

		// Add a report block about the given stream, or about its RTX stream, to
		// the Receiver Report packet being built, starting a new one if needed.
		const auto addReportBlock = [nowUs,
		                             &compoundPacket,
		                             &senderReportRtpStreams,
		                             &sdesChunksLength,
		                             &numDlrrSubBlocks,
		                             &receiverReportPacket,
		                             &hasReceiverReports,
		                             &hasReceiverReportPacket,
		                             &computeTrailingLength,
		                             &startCompoundPacket,
		                             &consolidateReceiverReportPacket,
		                             &sendCompoundPacket](
		                              RTC::RTP::RtpStreamRecv* rtpStream, bool rtx) -> void
		{
			if (receiverReportPacket && receiverReportPacket->GetNumberOfReportBlocks() == RTC::RTCP::AnyReportPacket::MaxReportBlocks)
			{
				consolidateReceiverReportPacket();
			}

			// The report block, the header of a new Receiver Report packet if none is
			// being built, and the packets that follow the reports.
			const size_t requiredLength =
			  RTC::RTCP::AnyReportPacket::ReportBlockLength +
			  (receiverReportPacket ? 0 : RTC::RTCP::ReceiverReportPacket::ComputeLengthFor(0)) +
			  computeTrailingLength(
			    sdesChunksLength,
			    senderReportRtpStreams.size(),
			    numDlrrSubBlocks,
			    /*withReceiverReports*/ true);
			const size_t availableLength = receiverReportPacket
			                                 ? receiverReportPacket->GetAvailableLength()
			                                 : compoundPacket->GetAvailableLength();

			// NOTE: The SDES packet may already be full with the chunks of the
			// Sender Reports, and the first Receiver Report brings in the chunk of
			// this Transport.
			if (availableLength < requiredLength || senderReportRtpStreams.size() == RTC::RTCP::SdesPacket::MaxChunks)
			{
				sendCompoundPacket();
				startCompoundPacket();
			}

			if (!receiverReportPacket)
			{
				receiverReportPacket = compoundPacket->BuildPacketInPlace<RTC::RTCP::ReceiverReportPacket>();

				receiverReportPacket->SetSsrc(RtcpSsrc);

				hasReceiverReportPacket = true;
			}

			const bool added = rtx ? rtpStream->AddRtxRtcpReportBlock(receiverReportPacket, nowUs)
			                       : rtpStream->AddRtcpReportBlock(receiverReportPacket, nowUs);

			REQUIRE(added);

			hasReceiverReports = true;
		};

		startCompoundPacket();

		for (const std::vector<RTC::RTP::RtpStreamSend*>& rtpStreams : dueConsumersRtpStreams)
		{
			rtcpDue = true;

			for (auto* const rtpStream : rtpStreams)
			{
				if (!rtpStream->HasRtcpSenderReport(nowUs))
				{
					continue;
				}

				const size_t sdesChunkLength = rtpStream->GetRtcpSdesChunkLength();
				// The Sender Report, and the packets that follow the reports with the
				// SDES chunk of this stream.
				const size_t requiredLength =
				  RTC::RTCP::SenderReportPacket::ComputeLengthFor(0) + computeTrailingLength(
				                                                         sdesChunksLength + sdesChunkLength,
				                                                         senderReportRtpStreams.size() + 1,
				                                                         numDlrrSubBlocks,
				                                                         hasReceiverReports);

				// The chunk of this Transport counts if the compound packet carries an XR
				// packet.
				const size_t numSdesChunks =
				  senderReportRtpStreams.size() + (hasReceiverReports || numDlrrSubBlocks > 0 ? 1 : 0);

				if (numSdesChunks == RTC::RTCP::SdesPacket::MaxChunks || compoundPacket->GetAvailableLength() < requiredLength)
				{
					sendCompoundPacket();
					startCompoundPacket();
				}

				const bool added = rtpStream->AddRtcpSenderReportPacket(compoundPacket.get(), nowUs);

				REQUIRE(added);

				senderReportRtpStreams.push_back(rtpStream);
				sdesChunksLength += sdesChunkLength;
			}
		}

		for (const std::vector<RTC::RTP::RtpStreamRecv*>& rtpStreams : dueProducersRtpStreams)
		{
			rtcpDue = true;

			for (auto* const rtpStream : rtpStreams)
			{
				if (rtpStream->HasRtcpReportBlock(nowUs))
				{
					addReportBlock(rtpStream, /*rtx*/ false);
				}

				if (rtpStream->HasRtxRtcpReportBlock(nowUs))
				{
					addReportBlock(rtpStream, /*rtx*/ true);
				}
			}
		}

		// It is time to report but there is nothing to report on, so an empty
		// Receiver Report goes instead (RFC 3550 section 6.4.2).
		//
		// NOTE: A compound packet with nothing in it means that none was sent
		// before either, since a new one is only started to add something to it.
		if (rtcpDue && senderReportRtpStreams.empty() && !hasReceiverReportPacket)
		{
			receiverReportPacket = compoundPacket->BuildPacketInPlace<RTC::RTCP::ReceiverReportPacket>();

			receiverReportPacket->SetSsrc(RtcpSsrc);

			hasReceiverReportPacket = true;
		}

		sendCompoundPacket();
	};

	SECTION("nothing is sent when no Consumer or Producer is due to report")
	{
		sendRtcp(NowUs, {}, {});

		REQUIRE(sentCompoundPackets.empty());
	}

	SECTION(
	  "an empty Receiver Report goes with the SDES chunk of the Transport when there is nothing to report")
	{
		// A Producer is due but its stream has received nothing for too long.
		const auto rtpStream = createRtpStreamRecv(1111);

		sendRtcp(NowUs + (RTC::Consts::RtcpReportBlockTimeoutMs * 1000), {}, { { rtpStream.get() } });

		REQUIRE(sentCompoundPackets.size() == 1);

		const auto compoundPacket = parseSentCompoundPacket(0);

		REQUIRE(compoundPacket->GetPacketsCount() == 2);

		const auto* const receiverReportPacket =
		  compoundPacket->GetFirstPacketOfType<RTC::RTCP::ReceiverReportPacket>();

		REQUIRE(receiverReportPacket);
		REQUIRE(compoundPacket->GetPacketAt(0) == receiverReportPacket);
		REQUIRE(receiverReportPacket->GetSsrc() == RtcpSsrc);
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 0);

		const auto* const sdesPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(sdesPacket);
		REQUIRE(sdesPacket->GetNumberOfChunks() == 1);

		const std::vector<RTC::RTCP::SdesPacket::Item> items = sdesPacket->GetItems();

		REQUIRE(items.size() == 1);
		REQUIRE(items.at(0).ssrc == RtcpSsrc);
		REQUIRE(items.at(0).type == RTC::RTCP::SdesPacket::ItemType::CNAME);
		REQUIRE(items.at(0).value == rtcpCname);

		REQUIRE(!compoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>());
	}

	SECTION("Sender Reports go with the SDES chunks of their streams and nothing of the Transport")
	{
		const auto rtpStream = createRtpStreamSend(2222, "stream-cname");

		sendRtcp(NowUs, { { rtpStream.get() } }, {});

		REQUIRE(sentCompoundPackets.size() == 1);

		const auto compoundPacket = parseSentCompoundPacket(0);

		REQUIRE(compoundPacket->GetPacketsCount() == 2);

		const auto* const senderReportPacket =
		  compoundPacket->GetFirstPacketOfType<RTC::RTCP::SenderReportPacket>();

		REQUIRE(senderReportPacket);
		REQUIRE(compoundPacket->GetPacketAt(0) == senderReportPacket);
		REQUIRE(senderReportPacket->GetSsrc() == 2222);

		const auto* const sdesPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(sdesPacket);

		const std::vector<RTC::RTCP::SdesPacket::Item> items = sdesPacket->GetItems();

		REQUIRE(items.size() == 1);
		REQUIRE(items.at(0).ssrc == 2222);
		REQUIRE(items.at(0).value == rtpStream->GetCname());

		REQUIRE(!compoundPacket->GetFirstPacketOfType<RTC::RTCP::ReceiverReportPacket>());
		REQUIRE(!compoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>());
	}

	SECTION(
	  "report blocks go in a Receiver Report of the Transport with its SDES chunk and a Receiver Reference Time")
	{
		const auto rtpStream = createRtpStreamRecv(3333);

		sendRtcp(NowUs, {}, { { rtpStream.get() } });

		REQUIRE(sentCompoundPackets.size() == 1);

		const auto compoundPacket = parseSentCompoundPacket(0);

		REQUIRE(compoundPacket->GetPacketsCount() == 3);

		const auto* const receiverReportPacket =
		  compoundPacket->GetFirstPacketOfType<RTC::RTCP::ReceiverReportPacket>();

		REQUIRE(receiverReportPacket);
		REQUIRE(compoundPacket->GetPacketAt(0) == receiverReportPacket);
		REQUIRE(receiverReportPacket->GetSsrc() == RtcpSsrc);
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 1);
		REQUIRE(receiverReportPacket->GetReportBlocks().at(0).ssrc == 3333);

		const auto* const sdesPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(sdesPacket);

		const std::vector<RTC::RTCP::SdesPacket::Item> items = sdesPacket->GetItems();

		REQUIRE(items.size() == 1);
		REQUIRE(items.at(0).ssrc == RtcpSsrc);
		REQUIRE(items.at(0).value == rtcpCname);

		const auto* const xrPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>();

		REQUIRE(xrPacket);
		REQUIRE(xrPacket->GetSsrc() == RtcpSsrc);
		REQUIRE(xrPacket->GetExtendedReportsCount() == 1);

		const auto* const rrtExtendedReport =
		  xrPacket->GetFirstExtendedReportOfType<RTC::RTCP::RrtExtendedReport>();

		REQUIRE(rrtExtendedReport);

		const Utils::Time::Ntp expectedNtp = Utils::Time::TimeUsToNtp(NowUs + shared.GetNtpOffsetUs());
		const Utils::Time::Ntp ntp         = rrtExtendedReport->GetNtp();

		REQUIRE(ntp.seconds == expectedNtp.seconds);
		REQUIRE(ntp.fractions == expectedNtp.fractions);
	}

	SECTION(
	  "a received Receiver Reference Time is answered once in a DLRR sub-block addressed to its sender")
	{
		receivedReceiverReferenceTimes[4444] =
		  ReceivedReceiverReferenceTime{ .compactNtp = 0x12345678, .receivedAtUs = NowUs - 1500000 };

		const auto rtpStream = createRtpStreamSend(2222, "stream-cname");

		sendRtcp(NowUs, { { rtpStream.get() } }, {});

		REQUIRE(sentCompoundPackets.size() == 1);

		const auto compoundPacket = parseSentCompoundPacket(0);

		// The XR packet goes with the RTCP SSRC of the Transport, so its CNAME goes
		// along with the one of the stream.
		const auto* const sdesPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(sdesPacket);
		REQUIRE(sdesPacket->GetNumberOfChunks() == 2);
		REQUIRE(sdesPacket->GetItems().at(0).ssrc == RtcpSsrc);
		REQUIRE(sdesPacket->GetItems().at(1).ssrc == 2222);

		const auto* const xrPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>();

		REQUIRE(xrPacket);
		REQUIRE(xrPacket->GetSsrc() == RtcpSsrc);
		// No report block, so no Receiver Reference Time.
		REQUIRE(!xrPacket->GetFirstExtendedReportOfType<RTC::RTCP::RrtExtendedReport>());

		const auto* const dlrrExtendedReport =
		  xrPacket->GetFirstExtendedReportOfType<RTC::RTCP::DlrrExtendedReport>();

		REQUIRE(dlrrExtendedReport);

		const std::vector<RTC::RTCP::DlrrExtendedReport::SubBlock> subBlocks =
		  dlrrExtendedReport->GetSubBlocks();

		REQUIRE(subBlocks.size() == 1);
		REQUIRE(subBlocks.at(0).ssrc == 4444);
		REQUIRE(subBlocks.at(0).lastReceiverReport == 0x12345678);
		// One second and a half in units of 1/65536 seconds.
		REQUIRE(subBlocks.at(0).delaySinceLastReceiverReport == 65536 + 32768);
		REQUIRE(receivedReceiverReferenceTimes.empty());

		// The next time it is not answered again.
		sendRtcp(NowUs, { { rtpStream.get() } }, {});

		REQUIRE(sentCompoundPackets.size() == 2);

		const auto nextCompoundPacket = parseSentCompoundPacket(1);

		REQUIRE(!nextCompoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>());
	}

	SECTION(
	  "report blocks that do not fit in a Receiver Report go in another one of the same compound packet")
	{
		std::vector<std::unique_ptr<RTC::RTP::RtpStreamRecv>> rtpStreams;
		std::vector<RTC::RTP::RtpStreamRecv*> producerRtpStreams;

		for (uint32_t ssrc{ 1000 }; ssrc < 1000 + RTC::RTCP::AnyReportPacket::MaxReportBlocks + 1; ++ssrc)
		{
			rtpStreams.push_back(createRtpStreamRecv(ssrc));
			producerRtpStreams.push_back(rtpStreams.back().get());
		}

		sendRtcp(NowUs, {}, { producerRtpStreams });

		REQUIRE(sentCompoundPackets.size() == 1);

		const auto compoundPacket = parseSentCompoundPacket(0);

		REQUIRE(compoundPacket->GetPacketsCount() == 4);

		const auto* const firstReceiverReportPacket =
		  dynamic_cast<const RTC::RTCP::ReceiverReportPacket*>(compoundPacket->GetPacketAt(0));
		const auto* const secondReceiverReportPacket =
		  dynamic_cast<const RTC::RTCP::ReceiverReportPacket*>(compoundPacket->GetPacketAt(1));

		REQUIRE(firstReceiverReportPacket);
		REQUIRE(firstReceiverReportPacket->GetSsrc() == RtcpSsrc);
		REQUIRE(
		  firstReceiverReportPacket->GetNumberOfReportBlocks() ==
		  RTC::RTCP::AnyReportPacket::MaxReportBlocks);
		REQUIRE(secondReceiverReportPacket);
		REQUIRE(secondReceiverReportPacket->GetSsrc() == RtcpSsrc);
		REQUIRE(secondReceiverReportPacket->GetNumberOfReportBlocks() == 1);

		const auto* const sdesPacket = compoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(sdesPacket);
		REQUIRE(sdesPacket->GetNumberOfChunks() == 1);
		REQUIRE(compoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>());
	}

	SECTION("Sender Reports that do not fit in the length of a compound packet go in another one")
	{
		const std::string cname{ "a-long-stream-cname" };
		// What each stream takes: its Sender Report and its SDES chunk.
		const size_t streamLength = RTC::RTCP::SenderReportPacket::ComputeLengthFor(0) +
		                            RTC::RTCP::SdesPacket::ComputeChunkLengthFor({ cname.size() });
		// Sender Reports that fit along with the SDES packet that carries their
		// chunks.
		const size_t maxSenderReports =
		  (RTC::Consts::OutgoingRtcpPacketMaxSize - RTC::RTCP::SdesPacket::ComputeLengthFor(0)) /
		  streamLength;

		// Otherwise it would be the limit of SDES chunks, and not the length, that
		// splits them.
		REQUIRE(maxSenderReports < RTC::RTCP::SdesPacket::MaxChunks);

		std::vector<std::unique_ptr<RTC::RTP::RtpStreamSend>> rtpStreamsSend;
		std::vector<RTC::RTP::RtpStreamSend*> consumerRtpStreams;

		for (uint32_t ssrc{ 1000 }; ssrc < 1000 + maxSenderReports + 1; ++ssrc)
		{
			rtpStreamsSend.push_back(createRtpStreamSend(ssrc, cname));
			consumerRtpStreams.push_back(rtpStreamsSend.back().get());
		}

		sendRtcp(NowUs, { consumerRtpStreams }, {});

		REQUIRE(sentCompoundPackets.size() == 2);

		const auto firstCompoundPacket = parseSentCompoundPacket(0);

		REQUIRE(firstCompoundPacket->GetPacketsCount() == maxSenderReports + 1);

		const auto* const firstSdesPacket =
		  firstCompoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(firstSdesPacket);
		REQUIRE(firstSdesPacket->GetNumberOfChunks() == maxSenderReports);

		const auto secondCompoundPacket = parseSentCompoundPacket(1);

		REQUIRE(secondCompoundPacket->GetPacketsCount() == 2);

		const auto* const secondSenderReportPacket =
		  secondCompoundPacket->GetFirstPacketOfType<RTC::RTCP::SenderReportPacket>();

		REQUIRE(secondSenderReportPacket);
		REQUIRE(secondSenderReportPacket->GetSsrc() == 1000 + maxSenderReports);
	}

	SECTION(
	  "a report block after as many Sender Reports as SDES chunks fit goes in another compound packet")
	{
		std::vector<std::unique_ptr<RTC::RTP::RtpStreamSend>> rtpStreamsSend;
		std::vector<RTC::RTP::RtpStreamSend*> consumerRtpStreams;

		for (uint32_t ssrc{ 1000 }; ssrc < 1000 + RTC::RTCP::SdesPacket::MaxChunks; ++ssrc)
		{
			// NOTE: The shortest CNAME, so that as many Sender Reports as SDES chunks
			// fit in a compound packet.
			rtpStreamsSend.push_back(createRtpStreamSend(ssrc, "c"));
			consumerRtpStreams.push_back(rtpStreamsSend.back().get());
		}

		const auto rtpStreamRecv = createRtpStreamRecv(3333);

		sendRtcp(NowUs, { consumerRtpStreams }, { { rtpStreamRecv.get() } });

		REQUIRE(sentCompoundPackets.size() == 2);

		const auto firstCompoundPacket = parseSentCompoundPacket(0);

		REQUIRE(firstCompoundPacket->GetPacketsCount() == RTC::RTCP::SdesPacket::MaxChunks + 1);
		REQUIRE(!firstCompoundPacket->GetFirstPacketOfType<RTC::RTCP::ReceiverReportPacket>());
		REQUIRE(!firstCompoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>());

		const auto* const firstSdesPacket =
		  firstCompoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(firstSdesPacket);
		REQUIRE(firstSdesPacket->GetNumberOfChunks() == RTC::RTCP::SdesPacket::MaxChunks);

		const auto secondCompoundPacket = parseSentCompoundPacket(1);

		REQUIRE(secondCompoundPacket->GetPacketsCount() == 3);

		const auto* const receiverReportPacket =
		  secondCompoundPacket->GetFirstPacketOfType<RTC::RTCP::ReceiverReportPacket>();

		REQUIRE(receiverReportPacket);
		REQUIRE(secondCompoundPacket->GetPacketAt(0) == receiverReportPacket);
		REQUIRE(receiverReportPacket->GetNumberOfReportBlocks() == 1);

		const auto* const secondSdesPacket =
		  secondCompoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(secondSdesPacket);
		REQUIRE(secondSdesPacket->GetNumberOfChunks() == 1);
		REQUIRE(secondSdesPacket->GetItems().at(0).ssrc == RtcpSsrc);
	}

	SECTION("with a DLRR sub-block to send, one Sender Report less fits in the compound packet")
	{
		receivedReceiverReferenceTimes[4444] =
		  ReceivedReceiverReferenceTime{ .compactNtp = 0x12345678, .receivedAtUs = NowUs };

		std::vector<std::unique_ptr<RTC::RTP::RtpStreamSend>> rtpStreamsSend;
		std::vector<RTC::RTP::RtpStreamSend*> consumerRtpStreams;

		for (uint32_t ssrc{ 1000 }; ssrc < 1000 + RTC::RTCP::SdesPacket::MaxChunks; ++ssrc)
		{
			// NOTE: The shortest CNAME, so that as many Sender Reports as SDES chunks
			// fit in a compound packet.
			rtpStreamsSend.push_back(createRtpStreamSend(ssrc, "c"));
			consumerRtpStreams.push_back(rtpStreamsSend.back().get());
		}

		sendRtcp(NowUs, { consumerRtpStreams }, {});

		REQUIRE(sentCompoundPackets.size() == 2);

		const auto firstCompoundPacket = parseSentCompoundPacket(0);
		const auto* const firstSdesPacket =
		  firstCompoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(firstSdesPacket);
		// The chunk of the Transport and the ones of the streams.
		REQUIRE(firstSdesPacket->GetNumberOfChunks() == RTC::RTCP::SdesPacket::MaxChunks);
		REQUIRE(firstCompoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>());

		const auto secondCompoundPacket = parseSentCompoundPacket(1);
		const auto* const secondSdesPacket =
		  secondCompoundPacket->GetFirstPacketOfType<RTC::RTCP::SdesPacket>();

		REQUIRE(secondSdesPacket);
		REQUIRE(secondSdesPacket->GetNumberOfChunks() == 1);
		REQUIRE(!secondCompoundPacket->GetFirstPacketOfType<RTC::RTCP::XrPacket>());
	}
}
