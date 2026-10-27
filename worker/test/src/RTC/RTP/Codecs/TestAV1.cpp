#include "common.hpp"
#include "RTC/RTP/Codecs/AV1.hpp"
#include "RTC/RTP/Codecs/DependencyDescriptor.hpp"
#include "RTC/RTP/Packet.hpp"
#include "test/include/RTC/RTP/rtpCommon.hpp"
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

/**
 * The Dependency Descriptors below are REAL bytes captured from Chrome
 * producing AV1 with `scalabilityMode: 'L3T3'`.
 *
 * An equivalent capture can be taken and dumped with:
 *
 *   tshark -i <interface> -f "udp portrange <rtcMinPort>-<rtcMaxPort>" -s 0 \
 *     -w capture.pcapng
 *
 *   tshark -r capture.pcapng -o rtp.heuristic_rtp:TRUE \
 *     -Y "rtp.p_type==<av1 payload type>" -T fields \
 *     -e rtp.seq -e rtp.marker \
 *     -e rtp.ext.rfc5285.id -e rtp.ext.rfc5285.len -e rtp.ext.rfc5285.data
 *
 * The Dependency Descriptor is the extension whose value is ~90 bytes long in
 * the first packet of a key frame and 3 bytes long everywhere else.
 */

namespace
{
	/**
	 * Key frame, carrying the template dependency structure of an L3T3 stream:
	 * 9 decode targets and 15 templates. These are the ones used below, with
	 * their spatial and temporal layers and their decode target indications:
	 *
	 *   id  sId  tId  decodeTargetIndications
	 *    1    0    0  SSS------
	 *    2    0    1  -DS------
	 *    3    0    2  --D------
	 *    4    0    2  --D------
	 *    6    1    0  ---SSS---
	 *    7    1    1  ----DS---
	 *    8    1    2  -----D---
	 *    9    1    2  -----D---
	 *   11    2    0  ------SSS
	 *   12    2    1  -------DS
	 *   13    2    2  --------D
	 *   14    2    2  --------D
	 */
	// clang-format off
	const uint8_t KeyFrameData[] =
	{
		0xc0, 0x00, 0x4f, 0x80, 0x08, 0x14, 0x85, 0x21, 0x4e, 0xaa, 0xaa, 0xa8,
		0x00, 0x06, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x02, 0xaa, 0x80, 0xa8,
		0x00, 0x06, 0x00, 0x00, 0x40, 0x00, 0x10, 0x00, 0x02, 0xa0, 0x00, 0xa8,
		0x00, 0x06, 0x00, 0x00, 0x40, 0x00, 0x16, 0xd5, 0x49, 0x24, 0x1b, 0x55,
		0x24, 0x90, 0x6d, 0x54, 0x92, 0x31, 0x57, 0xe0, 0x01, 0x97, 0x4c, 0xa8,
		0x64, 0x33, 0x0e, 0x22, 0x23, 0x96, 0xec, 0xa8, 0x65, 0x53, 0x04, 0x22,
		0x43, 0x90, 0xec, 0xa8, 0x77, 0x53, 0x01, 0x3f, 0x00, 0xb3, 0x02, 0x7f,
		0x01, 0x67, 0x04, 0xff, 0x02, 0xcf
	};
	// clang-format on

	/**
	 * One full repetition of the layers pattern that follows the key frame:
	 * 4 pictures of 3 spatial layers each, with temporal layers T2, T1, T2, T0.
	 *
	 * NOTE: All of them are end of frame packets, which is what `AV1::Process()`
	 * requires to switch layers down and to set the marker bit. Frames that the
	 * capture sent in several packets are hence represented by their last one.
	 */
	// clang-format off
	const std::vector<std::vector<uint8_t>> DeltaFrames =
	{
		{ 0xc3, 0x00, 0x5e }, // frame  94, template  3, S0T2
		{ 0xc8, 0x00, 0x5f }, // frame  95, template  8, S1T2
		{ 0x4d, 0x00, 0x60 }, // frame  96, template 13, S2T2
		{ 0xc2, 0x00, 0x61 }, // frame  97, template  2, S0T1
		{ 0xc7, 0x00, 0x62 }, // frame  98, template  7, S1T1
		{ 0xcc, 0x00, 0x63 }, // frame  99, template 12, S2T1
		{ 0xc4, 0x00, 0x64 }, // frame 100, template  4, S0T2
		{ 0xc9, 0x00, 0x65 }, // frame 101, template  9, S1T2
		{ 0xce, 0x00, 0x66 }, // frame 102, template 14, S2T2
		{ 0xc1, 0x00, 0x67 }, // frame 103, template  1, S0T0
		{ 0xc6, 0x00, 0x68 }, // frame 104, template  6, S1T0
		{ 0x4b, 0x00, 0x69 }, // frame 105, template 11, S2T0
	};
	// clang-format on

	// Indexes into `DeltaFrames` of the frames some tests act upon.
	constexpr size_t FrameS0T1Index{ 3 };
	constexpr size_t FrameS0T2Index{ 6 };
	constexpr size_t FrameS0T0Index{ 9 };
	constexpr size_t FrameS1T0Index{ 10 };

	class Listener : public RTC::RTP::Codecs::DependencyDescriptor::Listener
	{
	public:
		bool OnDependencyDescriptorUpdated(const uint8_t* /*data*/, size_t /*len*/) override
		{
			return true;
		}
	};

	struct ProcessResult
	{
		bool forwarded{ false };
		bool marker{ false };
		uint8_t spatialLayer{ 0 };
		uint8_t temporalLayer{ 0 };
	};
} // namespace

SCENARIO("process AV1 payload descriptor", "[rtp][codecs][av1]")
{
	Listener listener;
	std::shared_ptr<RTC::RTP::Codecs::DependencyDescriptor::TemplateDependencyStructure>
	  templateDependencyStructure;

	RTC::RTP::Codecs::EncodingContext::Params params;

	params.spatialLayers  = 3;
	params.temporalLayers = 3;

	RTC::RTP::Codecs::AV1::EncodingContext context(params);

	/**
	 * Runs a frame through `AV1::PayloadDescriptorHandler::Process()`, which is
	 * what decides whether the packet is forwarded to the Consumer.
	 */
	auto process = [&](const uint8_t* data, size_t len)
	{
		ProcessResult result;

		std::unique_ptr<RTC::RTP::Codecs::DependencyDescriptor> dependencyDescriptor(
		  RTC::RTP::Codecs::DependencyDescriptor::Parse(
		    data, len, std::addressof(listener), templateDependencyStructure));

		REQUIRE(dependencyDescriptor);

		result.spatialLayer  = dependencyDescriptor->spatialLayer;
		result.temporalLayer = dependencyDescriptor->temporalLayer;

		auto* payloadDescriptor = RTC::RTP::Codecs::AV1::Parse(dependencyDescriptor);

		REQUIRE(payloadDescriptor);

		RTC::RTP::Codecs::AV1::PayloadDescriptorHandler payloadDescriptorHandler(payloadDescriptor);

		const std::unique_ptr<RTC::RTP::Packet> packet{ RTC::RTP::Packet::Factory(
			rtpCommon::FactoryBuffer, sizeof(rtpCommon::FactoryBuffer)) };

		result.forwarded = payloadDescriptorHandler.Process(&context, packet.get(), result.marker);

		return result;
	};

	auto processKeyFrame = [&]()
	{
		return process(KeyFrameData, sizeof(KeyFrameData));
	};

	auto processDeltaFrame = [&](size_t index)
	{
		return process(DeltaFrames[index].data(), DeltaFrames[index].size());
	};

	// Asks for the given layers, with nothing forwarded yet.
	auto setTargetLayers = [&](int16_t spatialLayer, int16_t temporalLayer)
	{
		context.SetTargetSpatialLayer(spatialLayer);
		context.SetTargetTemporalLayer(temporalLayer);
		context.SetCurrentSpatialLayer(0);
		context.SetCurrentTemporalLayer(0);
	};

	SECTION("forwards every layer when targeting 2:2")
	{
		setTargetLayers(2, 2);

		REQUIRE(processKeyFrame().forwarded);

		for (size_t i{ 0 }; i < DeltaFrames.size(); ++i)
		{
			INFO("delta frame " << i);

			const auto result = processDeltaFrame(i);

			REQUIRE(result.forwarded);

			// The marker bit is only set in the highest forwarded spatial layer, so
			// that the whole picture reaches the endpoint as a single unit.
			REQUIRE(result.marker == (result.spatialLayer == 2));
		}
	}

	SECTION("drops upper spatial layers when targeting spatial 0")
	{
		setTargetLayers(0, 2);

		REQUIRE(processKeyFrame().forwarded);

		for (size_t i{ 0 }; i < DeltaFrames.size(); ++i)
		{
			INFO("delta frame " << i);

			const auto result = processDeltaFrame(i);

			REQUIRE(result.forwarded == (result.spatialLayer == 0));
		}
	}

	SECTION("drops upper temporal layers when targeting temporal 0")
	{
		setTargetLayers(2, 0);

		REQUIRE(processKeyFrame().forwarded);

		for (size_t i{ 0 }; i < DeltaFrames.size(); ++i)
		{
			INFO("delta frame " << i);

			const auto result = processDeltaFrame(i);

			REQUIRE(result.forwarded == (result.temporalLayer == 0));
		}
	}

	SECTION("upgrades the spatial layer only in a key frame")
	{
		setTargetLayers(0, 0);

		// NOTE: The key frame is what carries the template dependency structure, so
		// it must come first for the delta frames below to be parseable at all.
		REQUIRE(processKeyFrame().forwarded);
		REQUIRE(context.GetCurrentSpatialLayer() == 0);

		// Now ask for every spatial layer.
		context.SetTargetSpatialLayer(2);

		// Delta frames don't upgrade it, so the upper spatial layers are dropped.
		REQUIRE(processDeltaFrame(FrameS0T0Index).forwarded);
		REQUIRE(!processDeltaFrame(FrameS1T0Index).forwarded);
		REQUIRE(context.GetCurrentSpatialLayer() == 0);

		// A key frame does upgrade it.
		REQUIRE(processKeyFrame().forwarded);
		REQUIRE(context.GetCurrentSpatialLayer() == 2);
	}

	/**
	 * Upgrading the temporal layer is only safe in a frame the Dependency
	 * Descriptor marks as a switch point for the decode target being moved into,
	 * this is, with a "S" indication for it.
	 *
	 * Switching up anywhere else would forward frames that depend on frames
	 * already dropped, which the endpoint cannot decode.
	 */
	SECTION("upgrades the temporal layer only in a switch point")
	{
		setTargetLayers(2, 0);

		REQUIRE(processKeyFrame().forwarded);

		// Only temporal layer 0 is wanted so far, so the 0:1 frame is dropped.
		REQUIRE(!processDeltaFrame(FrameS0T1Index).forwarded);

		// Now ask for temporal layer 2.
		context.SetTargetTemporalLayer(2);

		// The 0:2 frame is not a switch point for decode target 0:2 (its
		// indications are "--D------"), and it depends on the 0:1 frame just
		// dropped, so it's not forwarded.
		REQUIRE(!processDeltaFrame(FrameS0T2Index).forwarded);
		REQUIRE(context.GetCurrentTemporalLayer() == 0);

		// The 0:0 frame is a switch point for it ("SSS------"), so the upgrade
		// happens there.
		REQUIRE(processDeltaFrame(FrameS0T0Index).forwarded);
		REQUIRE(context.GetCurrentTemporalLayer() == 2);
	}
}
