#ifndef MS_LIVELY_HPP
#define MS_LIVELY_HPP

#include <string>

namespace Lively
{
  class AppData
  {
  public:
    std::string callId;
    std::string peerId;
    std::string mirrorId;
    std::string streamName;

  public:
    AppData() = default;
    AppData(const AppData& d) : callId(d.callId), peerId(d.peerId), mirrorId(d.mirrorId), streamName(d.streamName) {}
    
    AppData& operator=(const AppData& d) { callId = d.callId; peerId = d.peerId; mirrorId = d.mirrorId; streamName = d.streamName; return *this; }

    std::string ToStr() const;
  };
}; // namespace Lively
#endif