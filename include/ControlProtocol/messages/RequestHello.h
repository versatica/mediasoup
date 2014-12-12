#ifndef MS_CONTROL_PROTOCOL_REQUEST_HELLO_H
#define MS_CONTROL_PROTOCOL_REQUEST_HELLO_H


#include "ControlProtocol/Request.h"


namespace ControlProtocol {


class RequestHello : public ControlProtocol::Request {
public:
	RequestHello();
	virtual ~RequestHello();

	virtual void Dump() override;
};


}  // namespace ControlProtocol


#endif
