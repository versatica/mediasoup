#include "Lively.hpp"
#include <json.hpp>
#include "LivelyAppDataToJson.hpp"

using json = nlohmann::json;

namespace Lively {
  std::string AppData::ToStr() const 
  {
    json j; 
    to_json(j, *this); 
    return j.dump();
  }
}