#define MS_CLASS "RTC::ProducerStreamManager"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/ProducerStreamManager.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Instance methods. */

	ProducerStreamManager::ProducerStreamManager(
	  const std::vector<RTC::RtpEncodingParameters>& consumableRtpEncodings,
	  const RTC::ConsumerTypes::VideoLayers& preferredLayers,
	  std::unique_ptr<RTC::RTP::Codecs::EncodingContext> encodingContext,
	  RTC::Media::Kind kind,
	  bool keyFrameSupported,
	  Listener* listener,
	  SharedInterface* shared)
	  : listener(listener),
	    shared(shared),
	    keyFrameSupported(keyFrameSupported),
	    kind(kind),
	    consumableRtpEncodings(consumableRtpEncodings),
	    encodingContext(std::move(encodingContext)),
	    preferredLayers(preferredLayers)
	{
		MS_TRACE();
	}

	void ProducerStreamManager::ProducerSpatialLayerActivityChanged(
	  RTC::RTP::RtpStreamRecv* /*rtpStream*/, uint8_t /*spatialLayer*/, bool /*isActive*/)
	{
		MS_TRACE();
	}

	void ProducerStreamManager::MayChangeLayers()
	{
		MS_TRACE();

		RTC::ConsumerTypes::VideoLayers newTargetLayers;

		if (RecalculateTargetLayers(newTargetLayers))
		{
#ifdef MS_USE_BUILTIN_BWE
			// Which layers are actually run at is the bitrate allocation's to decide, so
			// all this asks for is a round.
			this->listener->OnProducerStreamManagerNeedBitrateChange();
#else
			// If bitrate externally managed, don't bother the transport unless
			// the newTargetSpatialLayer has changed.
			// This is because, if bitrate is externally managed, the target temporal
			// layer is managed by the available given bitrate so the transport
			// will let us change it when it considers.
			if (this->externallyManagedBitrate)
			{
				if (newTargetLayers.spatial != GetTargetLayers().spatial)
				{
					this->listener->OnProducerStreamManagerNeedBitrateChange();
				}
			}
			else
			{
				UpdateTargetLayers(newTargetLayers.spatial, newTargetLayers.temporal);
			}
#endif
		}
	}

#ifdef MS_USE_BUILTIN_BWE
	std::optional<size_t> ProducerStreamManager::GetCurrentBitrateOption(
	  const std::vector<RTC::BWE::BitrateAllocator::Candidate::BitrateOption>& options) const
	{
		MS_TRACE();

		if (!this->bitrateAllocated)
		{
			return std::nullopt;
		}

		const auto targetLayers = GetTargetLayers();

		for (size_t idx{ 0 }; idx < options.size(); ++idx)
		{
			if (options.at(idx).layers == targetLayers)
			{
				return idx;
			}
		}

		return std::nullopt;
	}
#endif
} // namespace RTC
