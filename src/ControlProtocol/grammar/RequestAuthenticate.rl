%%{
	machine RequestAuthenticate;

	action on_request_type_Authenticate
	{
		this->msg = new ControlProtocol::RequestAuthenticate();
	}

	action on_auth_user
	{
		static_cast<RequestAuthenticate*>(this->msg)->SetUser(PTR_TO_MARK, LEN_FROM_MARK);
	}

	action on_auth_passwd
	{
		static_cast<RequestAuthenticate*>(this->msg)->SetPasswd(PTR_TO_MARK, LEN_FROM_MARK);
	}

	auth_user               = token >mark %on_auth_user;
	auth_passwd             = token >mark %on_auth_passwd;

	RequestAuthenticate     = "Request" SP "Authenticate" CRLF  %on_request_type_Authenticate
	                          transaction CRLF
	                          "user:" SP auth_user CRLF
	                          "passwd:" SP auth_passwd CRLF
	                          CRLF;
}%%
