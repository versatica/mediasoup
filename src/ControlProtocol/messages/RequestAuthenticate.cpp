#define MS_CLASS "ControlProtocol::RequestAuthenticate"

#include "ControlProtocol/messages/RequestAuthenticate.h"
#include "Logger.h"

namespace ControlProtocol
{
	/* Instance methods. */

	RequestAuthenticate::RequestAuthenticate() :
		Request(Type::Authenticate)
	{
		MS_DEBUG("kind: %d, type: %d", (int)this->kind, (int)this->type);
	}

	RequestAuthenticate::~RequestAuthenticate()
	{
		MS_TRACE();
	}

	void RequestAuthenticate::SetUser(const char* str, size_t len)
	{
		MS_TRACE();

		this->user = std::string(str, len);
	}

	void RequestAuthenticate::SetPasswd(const char* str, size_t len)
	{
		MS_TRACE();

		this->passwd = std::string(str, len);
	}

	void RequestAuthenticate::Dump()
	{
		MS_DEBUG("[Request Authenticate]");
		MS_DEBUG("- transaction: %zu", (size_t)this->transaction);
		MS_DEBUG("- user: %s", this->user.c_str());
		MS_DEBUG("- passwd: %s", this->passwd.c_str());
		MS_DEBUG("[/Request Authenticate]");
	}
}  // namespace ControlProtocol
