%%{
	machine RequestHello;

	action on_request_type_Hello
	{
		this->msg = new ControlProtocol::RequestHello();
	}

	RequestHello            = "Request" SP "Hello" CRLF  %on_request_type_Hello
	                          transaction CRLF
	                          CRLF;
}%%
