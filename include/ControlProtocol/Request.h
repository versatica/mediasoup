#ifndef MS_CONTROL_PROTOCOL_REQUEST_H
#define MS_CONTROL_PROTOCOL_REQUEST_H


#include "ControlProtocol/Message.h"


namespace ControlProtocol {


class Request : public ControlProtocol::Message {
public:
	enum class Type {
		Hello = 1,
		Authenticate,
		CreateConference
	};

public:
	Request(Type);
	virtual ~Request();

	virtual void Dump() = 0;

public:
	// Allow fast access to type.
	Type type;
};


}  // namespace ControlProtocol


#endif
