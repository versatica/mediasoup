#include "Lively.hpp"
#include <nlohmann/json.hpp>
#include "LivelyAppDataToJson.hpp"

using json = nlohmann::json;

namespace Lively {
  std::string AppData::ToStr() const 
  {
    std::string appData;
    if(this != nullptr)
    {
      appData = "callId= " + this->callId + " peerId= " + this->peerId + " mirrorId= " + this->mirrorId + " streamName= " + this->streamName + " objectId= " + this->id;
    }
    return appData;
  }
}