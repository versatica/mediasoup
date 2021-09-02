#ifndef MS_LIVELY_APPDATA_TO_JSON_HPP
#define MS_LIVELY_APPDATA_TO_JSON_HPP

#include <json.hpp>
#include <string>
#include "Lively.hpp"

using json = nlohmann::json;

namespace Lively
{
  class AppData;

  //void to_json(json& j, const AppData& d);
  //void from_json(const json& j, AppData& d);
  inline void to_json(json& j, const AppData& d) {
    j = json{{"callId", d.callId}, {"peerId", d.peerId}, {"mirrorId", d.mirrorId}, {"streamKey", d.streamKey}};
  }

  inline void from_json(const json& j, AppData& d) {
    j.at("callId").get_to(d.callId);
    j.at("peerId").get_to(d.peerId);
    j.at("mirrorId").get_to(d.mirrorId);
    j.at("streamKey").get_to(d.streamKey);
  }
}; // namespace Lively
#endif