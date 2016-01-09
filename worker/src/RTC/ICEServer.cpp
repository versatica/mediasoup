#define MS_CLASS "RTC::ICEServer"

#include "RTC/ICEServer.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	ICEServer::ICEServer(Listener* listener, ICEServer::IceComponent iceComponent, const std::string& usernameFragment, const std::string password) :
		listener(listener),
		iceComponent(iceComponent),
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

	void ICEServer::ProcessSTUNMessage(RTC::STUNMessage* msg, RTC::TransportTuple* tuple)
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
				this->listener->onOutgoingSTUNMessage(this, response, tuple);
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
				this->listener->onOutgoingSTUNMessage(this, response, tuple);
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
					this->listener->onOutgoingSTUNMessage(this, response, tuple);
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
						this->listener->onOutgoingSTUNMessage(this, response, tuple);
						delete response;

						return;
					}

					case RTC::STUNMessage::Authentication::BadRequest:
					{
						MS_DEBUG("cannot check authentication in STUN Binding Request => 400");

						// Reply 400.
						RTC::STUNMessage* response = msg->CreateErrorResponse(400);
						response->Serialize();
						this->listener->onOutgoingSTUNMessage(this, response, tuple);
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
					this->listener->onOutgoingSTUNMessage(this, response, tuple);
					delete response;

					return;
				}

				MS_DEBUG("processing STUN Binding Request [Priority:%u, UseCandidate:%s]", (unsigned int)msg->GetPriority(), msg->HasUseCandidate() ? "true" : "false");

				// Create a success response.
				RTC::STUNMessage* response = msg->CreateSuccessResponse();

				// Add XOR-MAPPED-ADDRESS.
				response->SetXorMappedAddress(tuple->GetRemoteAddress());

				// Authenticate the response.
				response->Authenticate(this->password);

				// Send back.
				response->Serialize();
				this->listener->onOutgoingSTUNMessage(this, response, tuple);
				delete response;

				// Handle the tuple.
				HandleTuple(tuple, msg->HasUseCandidate());

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

	bool ICEServer::IsValidTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		if (HasTuple(tuple))
			return true;
		else
			return false;
	}

	void ICEServer::ForceSelectedTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		MS_ASSERT(this->state != IceState::NEW, "cannot force the selected tuple if not in 'connecting' or 'connected' state");

		// TODO: Remove as this assert is reduntant.
		MS_ASSERT(this->selectedTuple != nullptr, "cannot force the selected tuple if there was not a selected tuple");

		auto stored_tuple = HasTuple(tuple);

		MS_ASSERT(stored_tuple, "cannot force the selected tuple if the given tuple was not already a valid tuple");

		// Mark it as selected tuple.
		SetSelectedTuple(stored_tuple);
	}

	void ICEServer::HandleTuple(RTC::TransportTuple* tuple, bool has_use_candidate)
	{
		MS_TRACE();

		switch (this->state)
		{
			case IceState::NEW:
			{
				// There should be no tuples.
				MS_ASSERT(this->tuples.size() == 0, "state is 'new' but there are %zu tuples", this->tuples.size());

				// There shouldn't be a selected tuple.
				MS_ASSERT(this->selectedTuple == nullptr, "state is 'new' but there is selected tuple");

				if (!has_use_candidate)
				{
					MS_DEBUG("transition from state 'new' to 'connected'");

					// Store the tuple.
					auto stored_tuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(stored_tuple);

					// Update state.
					this->state = IceState::CONNECTED;

					// Notify the listener.
					this->listener->onICEConnected(this);
				}
				else
				{
					MS_DEBUG("transition from state 'new' to 'completed'");

					// Store the tuple.
					auto stored_tuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(stored_tuple);

					// Update state.
					this->state = IceState::COMPLETED;

					// Notify the listener.
					this->listener->onICECompleted(this);
				}

				break;
			}

			case IceState::CONNECTED:
			{
				// There should be some tuples.
				MS_ASSERT(this->tuples.size() > 0, "state is 'connected' but there are no tuples");

				// There should be a selected tuple.
				MS_ASSERT(this->selectedTuple != nullptr, "state is 'connected' but there is not selected tuple");

				if (!has_use_candidate)
				{
					// If a new tuple store it.
					if (!HasTuple(tuple))
						AddTuple(tuple);
				}
				else
				{
					MS_DEBUG("transition from state 'connected' to 'completed'");

					auto stored_tuple = HasTuple(tuple);

					// If a new tuple store it.
					if (!stored_tuple)
						stored_tuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(stored_tuple);

					// Update state.
					this->state = IceState::COMPLETED;

					// Notify the listener.
					this->listener->onICECompleted(this);
				}

				break;
			}

			case IceState::COMPLETED:
			{
				// There should be some tuples.
				MS_ASSERT(this->tuples.size() > 0, "state is 'completed' but there are no tuples");

				// There should be a selected tuple.
				MS_ASSERT(this->selectedTuple != nullptr, "state is 'completed' but there is not selected tuple");

				if (!has_use_candidate)
				{
					// If a new tuple store it.
					if (!HasTuple(tuple))
						AddTuple(tuple);
				}
				else
				{
					auto stored_tuple = HasTuple(tuple);

					// If a new tuple store it.
					if (!stored_tuple)
						stored_tuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(stored_tuple);
				}

				break;
			}
		}
	}

	inline
	RTC::TransportTuple* ICEServer::AddTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Add the new tuple at the beginning of the list.
		this->tuples.push_front(*tuple);

		auto stored_tuple = &(*this->tuples.begin());

		// If it is UDP then we must store the remote address (until now it is
		// just a pointer that will be freed soon).
		if (stored_tuple->GetProtocol() == TransportTuple::Protocol::UDP)
			stored_tuple->StoreUdpRemoteAddress();

		// Return the address of the inserted tuple.
		return stored_tuple;
	}

	inline
	RTC::TransportTuple* ICEServer::HasTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// If there is no selected tuple yet then we know that the tuples list
		// is empty.
		if (!this->selectedTuple)
			return nullptr;

		// Check the current selected tuple.
		if (this->selectedTuple->Compare(tuple))
			return this->selectedTuple;

		// Otherwise check other stored tuples.
		for (auto it = this->tuples.begin(); it != this->tuples.end(); ++it)
		{
			RTC::TransportTuple* stored_tuple = &(*it);

			if (stored_tuple->Compare(tuple))
				return stored_tuple;
		}

		return nullptr;
	}

	inline
	void ICEServer::SetSelectedTuple(RTC::TransportTuple* stored_tuple)
	{
		MS_TRACE();

		// If already the selected tuple do nothing.
		if (stored_tuple == this->selectedTuple)
			return;

		this->selectedTuple = stored_tuple;

		// Notify the listener.
		this->listener->onICESelectedTuple(this, this->selectedTuple);
	}

	// TODO
	// bool Transport::RemoveTuple(RTC::TransportTuple* tuple)
	// {
	// 	for (auto it = this->tuples.begin(); it != this->tuples.end(); ++it)
	// 	{
	// 		RTC::TransportTuple* valid_tuple = &(*it);

	// 		// If the tuple was a valid tuple then remove it.
	// 		if (valid_tuple->Compare(tuple))
	// 		{
	// 			this->tuples.erase(it);

	// 			// If it was the media tuple then unset it and set the media
	// 			// tuple to the first valid tuple (if any).
	// 			if (this->mediaTuple == valid_tuple)
	// 			{
	// 				if (this->tuples.begin() != this->tuples.end())
	// 				{
	// 					this->mediaTuple = &(*this->tuples.begin());
	// 				}
	// 				else
	// 				{
	// 					this->mediaTuple = nullptr;
	// 					// This is just useful is there are just TCP tuples.
	// 					this->icePaired = false;
	// 					this->icePairedWithUseCandidate = false;

	// 					// TODO: This should fire a 'icefailed' event.
	// 				}
	// 			}

	// 			return true;
	// 		}
	// 	}

	// 	return false;
	// }
}
