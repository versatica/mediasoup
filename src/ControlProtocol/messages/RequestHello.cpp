#define MS_CLASS "ControlProtocol::RequestHello"

#include "ControlProtocol/messages/RequestHello.h"
#include "Logger.h"


namespace ControlProtocol {


/* Instance methods. */

RequestHello::RequestHello() : Request(Type::Hello) {
	MS_DEBUG("kind: %d, type: %d", (int)this->kind, (int)this->type);
}


RequestHello::~RequestHello() {
	MS_TRACE();
}


void RequestHello::Dump() {
	MS_DEBUG("[Request Hello]");
	MS_DEBUG("- transaction: %zu", (size_t)this->transaction);
	MS_DEBUG("[/Request Hello]");
}


}  // namespace ControlProtocol
