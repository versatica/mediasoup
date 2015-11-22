#define MS_CLASS "ControlProtocol::RequestCreateConference"

#include "ControlProtocol/messages/RequestCreateConference.h"
#include "Logger.h"

namespace ControlProtocol
{
	/* Instance methods. */

	RequestCreateConference::RequestCreateConference() : Request(Type::CreateConference)
	{
		MS_DEBUG("kind: %d, type: %d", (int)this->kind, (int)this->type);
	}

	RequestCreateConference::~RequestCreateConference()
	{
		MS_TRACE();
	}

	void RequestCreateConference::Dump()
	{
		MS_DEBUG("[Request CreateConference]");
		MS_DEBUG("- transaction: %zu", (size_t)this->transaction);
		MS_DEBUG("[/Request CreateConference]");
	}
}  // namespace ControlProtocol
