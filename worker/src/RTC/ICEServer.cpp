#define MS_CLASS "RTC::ICEServer"

#include "RTC/ICEServer.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	ICEServer::ICEServer(Listener* listener, std::string& usernameFragment, std::string password) :
		listener(listener),
		usernameFragment(usernameFragment),
		password(password)
	{
		MS_TRACE();

		MS_DEBUG("[usernameFragment:%s, password:%s]", this->usernameFragment.c_str(), this->password.c_str());
	}

	void ICEServer::Close()
	{
		MS_TRACE();

		delete this;
	}

	void ICEServer::ProcessSTUNMessage(RTC::STUNMessage* msg, RTC::TransportSource* source)
	{
		MS_TRACE();

		// Must be a Binding method.
		if (msg->GetMethod() != RTC::STUNMessage::Method::Binding)
		{
			if (msg->GetClass() == RTC::STUNMessage::Class::Request)
			{
				MS_DEBUG("unknown method %#.3x in STUN Request => 400", (unsigned int)msg->GetMethod());

				// Reply 400.
				RTC::STUNMessage* response = msg->CreateErrorResponse(400);
				response->Serialize();
				this->listener->onOutgoingSTUNMessage(this, response, source);
				delete response;
			}
			else
			{
				MS_DEBUG("ignoring STUN Indication or Response with unknown method %#.3x", (unsigned int)msg->GetMethod());
			}

			return;
		}

		// Must use FINGERPRINT (optional for ICE STUN indications).
		if (!msg->HasFingerprint() && msg->GetClass() != RTC::STUNMessage::Class::Indication)
		{
			if (msg->GetClass() == RTC::STUNMessage::Class::Request)
			{
				MS_DEBUG("STUN Binding Request without FINGERPRINT => 400");

				// Reply 400.
				RTC::STUNMessage* response = msg->CreateErrorResponse(400);
				response->Serialize();
				this->listener->onOutgoingSTUNMessage(this, response, source);
				delete response;
			}
			else
			{
				MS_DEBUG("ignoring STUN Binding Response without FINGERPRINT");
			}

			return;
		}

		switch (msg->GetClass())
		{
			case RTC::STUNMessage::Class::Request:
			{
				// USERNAME, MESSAGE-INTEGRITY and PRIORITY are required.
				if (!msg->HasMessageIntegrity() || !msg->GetPriority() || msg->GetUsername().empty())
				{
					MS_DEBUG("mising required attributes in STUN Binding Request => 400");

					// Reply 400.
					RTC::STUNMessage* response = msg->CreateErrorResponse(400);
					response->Serialize();
					this->listener->onOutgoingSTUNMessage(this, response, source);
					delete response;

					return;
				}

				// Check authentication.
				switch (msg->CheckAuthentication(this->usernameFragment, this->password))
				{
					case RTC::STUNMessage::Authentication::OK:
						break;

					case RTC::STUNMessage::Authentication::Unauthorized:
					{
						MS_DEBUG("wrong authentication in STUN Binding Request => 401");

						// Reply 401.
						RTC::STUNMessage* response = msg->CreateErrorResponse(401);
						response->Serialize();
						this->listener->onOutgoingSTUNMessage(this, response, source);
						delete response;

						return;
					}

					case RTC::STUNMessage::Authentication::BadRequest:
					{
						MS_DEBUG("cannot check authentication in STUN Binding Request => 400");

						// Reply 400.
						RTC::STUNMessage* response = msg->CreateErrorResponse(400);
						response->Serialize();
						this->listener->onOutgoingSTUNMessage(this, response, source);
						delete response;

						return;
					}
				}

				// The remote peer must be ICE controlling.
				if (msg->GetIceControlled())
				{
					MS_DEBUG("peer indicates ICE-CONTROLLED in STUN Binding Request => 487");

					// Reply 487 (Role Conflict).
					RTC::STUNMessage* response = msg->CreateErrorResponse(487);
					response->Serialize();
					this->listener->onOutgoingSTUNMessage(this, response, source);
					delete response;

					return;
				}

				MS_DEBUG("processing STUN Binding Request with Priority %u%s", (unsigned int)msg->GetPriority(), msg->HasUseCandidate() ? " and UseCandidate" : "");

				// Create a success response.
				RTC::STUNMessage* response = msg->CreateSuccessResponse();

				// Add XOR-MAPPED-ADDRESS.
				response->SetXorMappedAddress(source->GetRemoteAddress());

				// Authenticate the response.
				response->Authenticate(this->password);

				// Send back.
				response->Serialize();
				this->listener->onOutgoingSTUNMessage(this, response, source);
				delete response;

				// Notify the listener with the valid pair.
				this->listener->onICEValidPair(this, source, msg->HasUseCandidate());

				break;
			}

			case RTC::STUNMessage::Class::Indication:
			{
				MS_DEBUG("STUN Binding Indication processed");
				break;
			}

			case RTC::STUNMessage::Class::SuccessResponse:
			{
				MS_DEBUG("STUN Binding Success Response processed");
				break;
			}

			case RTC::STUNMessage::Class::ErrorResponse:
			{
				MS_DEBUG("STUN Binding Error Response processed");
				break;
			}
		}
	}
}
