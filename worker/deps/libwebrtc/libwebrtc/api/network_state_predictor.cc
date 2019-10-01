
#include "api/network_state_predictor.h"

namespace webrtc {

// MS_NOTE: added function.
std::string BandwidthUsage2String(BandwidthUsage bandwidthUsage) {
	switch (bandwidthUsage) {
		case BandwidthUsage::kBwNormal:
			return "normal";
		case BandwidthUsage::kBwUnderusing:
			return "underusing";
		case BandwidthUsage::kBwOverusing:
			return "overusing";
		default:
			return "unknown";
	}
}

}  // namespace webrtc
