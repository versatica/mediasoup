#ifndef MS_LIVELY_APPDATA_TO_JSON_HPP
#define MS_LIVELY_APPDATA_TO_JSON_HPP

#include <nlohmann/json.hpp>
#include <string>
#include "Lively.hpp"

using json = nlohmann::json;

namespace Lively
{
  class AppData;

  inline void to_json(json& j, const AppData& d)
  {
    j = json{{"callId", d.callId}, {"peerId", d.peerId}, {"mirrorId", d.mirrorId}, {"streamName", d.streamName}};
  }

  inline void from_json(const json& j, AppData& d)
  {
    try {
      j.at("callId").get_to(d.callId);
    }	catch (const std::exception& e) {
      d.callId = "";
		}
    try {
      j.at("peerId").get_to(d.peerId);
    }	catch (const std::exception& e) {
      d.peerId = "";
		}
    try {
      j.at("mirrorId").get_to(d.mirrorId);
    }	catch (const std::exception& e) {
      d.mirrorId = "";
		}
    try {
      j.at("streamName").get_to(d.streamName);
    }	catch (const std::exception& e) {
      d.streamName = "";
		}
  }
}; // namespace Lively
#endif