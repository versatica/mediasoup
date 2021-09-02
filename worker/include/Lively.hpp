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
    std::string streamKey;

  public:
    AppData() = default;
    AppData(const AppData& d) : callId(d.callId), peerId(d.peerId), mirrorId(d.mirrorId), streamKey(d.streamKey) {}
    
    AppData& operator=(const AppData& d) { callId = d.callId; peerId = d.peerId; mirrorId = d.mirrorId; streamKey = d.streamKey; return *this; }

    std::string ToStr() const;
  };

/*
  std::string AppData::ToStr() const
  {
    std::string appstr;
    
    if (!this->callId.empty())
    {
      appstr.append("callId=\"").append(this->callId).append("\"");
    }
    if (!this->peerId.empty())
    {
      if (!appstr.empty())
        appstr.append(" ");
      appstr.append("peerId=\"").append(this->peerId).append("\"");
    }
    if (!this->mirrorId.empty())
    {
      if (!appstr.empty())
        appstr.append(" ");
      appstr.append("mirrorId=\"").append(this->mirrorId).append("\"");
    }
    if (!this->streamKey.empty())
    {
      if (!appstr.empty())
        appstr.append(" ");
      appstr.append("streamKey=\"").append(this->streamKey).append("\"");
    }

    return appstr;
  }*/
}; // namespace Lively
#endif