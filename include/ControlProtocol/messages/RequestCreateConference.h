#ifndef MS_CONTROL_PROTOCOL_REQUEST_CREATE_CONFERENCE_H
#define MS_CONTROL_PROTOCOL_REQUEST_CREATE_CONFERENCE_H


#include "ControlProtocol/Request.h"


namespace ControlProtocol {


class RequestCreateConference : public ControlProtocol::Request {
public:
	RequestCreateConference();
	virtual ~RequestCreateConference();

	virtual void Dump() override;
};


}  // namespace ControlProtocol


#endif
