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
      appData.append("callId= ").append("\"").append(this->callId).append("\"").append(" peerId= ").append("\"").append(this->peerId)
      .append("\"").append(" mirrorId= ").append( "\"").append(this->mirrorId).append( "\"").append(" streamName= ")
      .append("\"").append(this->streamName).append("\"").append(" objectId= ").append("\"").append(this->id).append("\"") ;
    }
    return appData;
  }
}