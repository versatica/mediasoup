#include "LivelyAppDataToJson.hpp"
#include <json.hpp>

using json = nlohmann::json;

namespace Lively {
  void to_json(json& j, const AppData& d) {
    j = json{{"callId", d.callId}, {"peerId", d.peerId}, {"mirrorId", d.mirrorId}, {"streamKey", d.streamKey}};
  }

  void from_json(const json& j, AppData& d) {
    j.at("callId").get_to(d.callId);
    j.at("peerId").get_to(d.peerId);
    j.at("mirrorId").get_to(d.mirrorId);
    j.at("streamKey").get_to(d.streamKey);
  }
}