#ifndef MS_CONTROL_PROTOCOL_REQUEST_AUTHENTICATE_H
#define MS_CONTROL_PROTOCOL_REQUEST_AUTHENTICATE_H

#include "ControlProtocol/Request.h"
#include <string>

namespace ControlProtocol
{
	class RequestAuthenticate : public ControlProtocol::Request
	{
	public:
		RequestAuthenticate();
		virtual ~RequestAuthenticate();

		void SetUser(const char* str, size_t len);
		void SetPasswd(const char* str, size_t len);

		virtual void Dump() override;

	private:
		std::string user;
		std::string passwd;
	};
}  // namespace ControlProtocol

#endif
