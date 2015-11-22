#define MS_CLASS "ControlProtocol::Request"

#include "ControlProtocol/Request.h"
#include "Logger.h"

namespace ControlProtocol
{
	/* Instance methods. */

	Request::Request(Type type) :
		Message(Message::Kind::Request),
		type(type)
	{
		MS_TRACE();
	}

	Request::~Request()
	{
		MS_TRACE();
	}
}  // namespace ControlProtocol
