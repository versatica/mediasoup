#define MS_CLASS "ControlProtocol::Message"

#include "ControlProtocol/Message.h"
#include "Logger.h"


namespace ControlProtocol {


/* Instance methods. */

Message::Message(Kind kind) :
	kind(kind)
{
	MS_TRACE();
}


Message::~Message() {
	MS_TRACE();
}


// TMP: must be pure virtual.
bool Message::IsValid() {
	MS_TRACE();

	return true;
}


}  // namespace ControlProtocol
