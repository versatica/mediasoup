%%{
	machine RequestCreateConference;

	action on_request_type_CreateConference
	{
		this->msg = new ControlProtocol::RequestCreateConference();
	}

	RequestCreateConference = "Request" SP "CreateConference" CRLF  %on_request_type_CreateConference
	                          transaction CRLF
	                          CRLF;
}%%
