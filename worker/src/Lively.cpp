#include "Lively.hpp"
#include <nlohmann/json.hpp>
#include "LivelyAppDataToJson.hpp"

using json = nlohmann::json;

namespace Lively {
  std::string AppData::ToStr() const 
  {
    // TODO: see which format would be better for logging
    json j; 
    to_json(j, *this); 
    return j.dump();
  }
}