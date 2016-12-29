#define MS_CLASS "RTC::IceServer"

#include "RTC/IceServer.h"
#include "Logger.h"

namespace RTC
{
	/* Instance methods. */

	IceServer::IceServer(Listener* listener, const std::string& usernameFragment, const std::string& password) :
		listener(listener),
		usernameFragment(usernameFragment),
		password(password)
	{
		MS_TRACE();

		MS_DEBUG("[usernameFragment:%s, password:%s]", this->usernameFragment.c_str(), this->password.c_str());
	}

	void IceServer::Close()
	{
		MS_TRACE();

		delete this;
	}

	void IceServer::ProcessStunMessage(RTC::StunMessage* msg, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Must be a Binding method.
		if (msg->GetMethod() != RTC::StunMessage::Method::Binding)
		{
			if (msg->GetClass() == RTC::StunMessage::Class::Request)
			{
				MS_DEBUG("unknown method %#.3x in STUN Request => 400", (unsigned int)msg->GetMethod());

				// Reply 400.
				RTC::StunMessage* response = msg->CreateErrorResponse(400);
				response->Serialize();
				this->listener->onOutgoingStunMessage(this, response, tuple);
				delete response;
			}
			else
			{
				MS_DEBUG("ignoring STUN Indication or Response with unknown method %#.3x", (unsigned int)msg->GetMethod());
			}

			return;
		}

		// Must use FINGERPRINT (optional for ICE STUN indications).
		if (!msg->HasFingerprint() && msg->GetClass() != RTC::StunMessage::Class::Indication)
		{
			if (msg->GetClass() == RTC::StunMessage::Class::Request)
			{
				MS_DEBUG("STUN Binding Request without FINGERPRINT => 400");

				// Reply 400.
				RTC::StunMessage* response = msg->CreateErrorResponse(400);
				response->Serialize();
				this->listener->onOutgoingStunMessage(this, response, tuple);
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
			case RTC::StunMessage::Class::Request:
			{
				// USERNAME, MESSAGE-INTEGRITY and PRIORITY are required.
				if (!msg->HasMessageIntegrity() || !msg->GetPriority() || msg->GetUsername().empty())
				{
					MS_DEBUG("mising required attributes in STUN Binding Request => 400");

					// Reply 400.
					RTC::StunMessage* response = msg->CreateErrorResponse(400);
					response->Serialize();
					this->listener->onOutgoingStunMessage(this, response, tuple);
					delete response;

					return;
				}

				// Check authentication.
				switch (msg->CheckAuthentication(this->usernameFragment, this->password))
				{
					case RTC::StunMessage::Authentication::OK:
						break;

					case RTC::StunMessage::Authentication::Unauthorized:
					{
						MS_DEBUG("wrong authentication in STUN Binding Request => 401");

						// Reply 401.
						RTC::StunMessage* response = msg->CreateErrorResponse(401);
						response->Serialize();
						this->listener->onOutgoingStunMessage(this, response, tuple);
						delete response;

						return;
					}

					case RTC::StunMessage::Authentication::BadRequest:
					{
						MS_DEBUG("cannot check authentication in STUN Binding Request => 400");

						// Reply 400.
						RTC::StunMessage* response = msg->CreateErrorResponse(400);
						response->Serialize();
						this->listener->onOutgoingStunMessage(this, response, tuple);
						delete response;

						return;
					}
				}

				// The remote peer must be ICE controlling.
				if (msg->GetIceControlled())
				{
					MS_DEBUG("peer indicates ICE-CONTROLLED in STUN Binding Request => 487");

					// Reply 487 (Role Conflict).
					RTC::StunMessage* response = msg->CreateErrorResponse(487);
					response->Serialize();
					this->listener->onOutgoingStunMessage(this, response, tuple);
					delete response;

					return;
				}

				// TODO: yes or no
				// MS_DEBUG("processing STUN Binding Request [Priority:%" PRIu32 ", UseCandidate:%s]", (uint32_t)msg->GetPriority(), msg->HasUseCandidate() ? "true" : "false");

				// Create a success response.
				RTC::StunMessage* response = msg->CreateSuccessResponse();

				// Add XOR-MAPPED-ADDRESS.
				response->SetXorMappedAddress(tuple->GetRemoteAddress());

				// Authenticate the response.
				response->Authenticate(this->password);

				// Send back.
				response->Serialize();
				this->listener->onOutgoingStunMessage(this, response, tuple);
				delete response;

				// Handle the tuple.
				HandleTuple(tuple, msg->HasUseCandidate());

				break;
			}

			case RTC::StunMessage::Class::Indication:
			{
				MS_DEBUG("STUN Binding Indication processed");

				break;
			}

			case RTC::StunMessage::Class::SuccessResponse:
			{
				MS_DEBUG("STUN Binding Success Response processed");

				break;
			}

			case RTC::StunMessage::Class::ErrorResponse:
			{
				MS_DEBUG("STUN Binding Error Response processed");

				break;
			}
		}
	}

	bool IceServer::IsValidTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		return HasTuple(tuple) ? true : false;
	}

	void IceServer::RemoveTuple(RTC::TransportTuple* tuple)
	{
		auto it = this->tuples.begin();
		RTC::TransportTuple* removed_tuple = nullptr;

		// Find the removed tuple.
		for (; it != this->tuples.end(); ++it)
		{
			RTC::TransportTuple* stored_tuple = std::addressof(*it);

			if (stored_tuple->Compare(tuple))
			{
				removed_tuple = stored_tuple;
				break;
			}
		}

		// If not found, ignore.
		if (!removed_tuple)
			return;

		// If this is not the selected tuple just remove it.
		if (removed_tuple != this->selectedTuple)
		{
			this->tuples.erase(it);

			return;
		}
		// Otherwise this was the selected tuple.
		else
		{
			this->tuples.erase(it);
			this->selectedTuple = nullptr;

			// Mark the first tuple as selected tuple (if any).
			if (this->tuples.begin() != this->tuples.end())
			{
				SetSelectedTuple(std::addressof(*this->tuples.begin()));
			}
			// Or just emit 'disconnected' and the null selected tuple..
			else
			{
				// Update state.
				this->state = IceState::DISCONNECTED;

				// Notify the listener.
				this->listener->onIceDisconnected(this);
			}
		}
	}

	void IceServer::ForceSelectedTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		MS_ASSERT(this->selectedTuple != nullptr, "cannot force the selected tuple if there was not a selected tuple");

		auto stored_tuple = HasTuple(tuple);

		MS_ASSERT(stored_tuple, "cannot force the selected tuple if the given tuple was not already a valid tuple");

		// Mark it as selected tuple.
		SetSelectedTuple(stored_tuple);
	}

	void IceServer::HandleTuple(RTC::TransportTuple* tuple, bool has_use_candidate)
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
					this->listener->onIceConnected(this);
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
					this->listener->onIceCompleted(this);
				}

				break;
			}

			case IceState::DISCONNECTED:
			{
				// There should be no tuples.
				MS_ASSERT(this->tuples.size() == 0, "state is 'disconnected' but there are %zu tuples", this->tuples.size());

				// There shouldn't be a selected tuple.
				MS_ASSERT(this->selectedTuple == nullptr, "state is 'disconnected' but there is selected tuple");

				if (!has_use_candidate)
				{
					MS_DEBUG("transition from state 'disconnected' to 'connected'");

					// Store the tuple.
					auto stored_tuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(stored_tuple);

					// Update state.
					this->state = IceState::CONNECTED;

					// Notify the listener.
					this->listener->onIceConnected(this);
				}
				else
				{
					MS_DEBUG("transition from state 'disconnected' to 'completed'");

					// Store the tuple.
					auto stored_tuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(stored_tuple);

					// Update state.
					this->state = IceState::COMPLETED;

					// Notify the listener.
					this->listener->onIceCompleted(this);
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
					this->listener->onIceCompleted(this);
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
	RTC::TransportTuple* IceServer::AddTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Add the new tuple at the beginning of the list.
		this->tuples.push_front(*tuple);

		auto stored_tuple = std::addressof(*this->tuples.begin());

		// If it is UDP then we must store the remote address (until now it is
		// just a pointer that will be freed soon).
		if (stored_tuple->GetProtocol() == TransportTuple::Protocol::UDP)
			stored_tuple->StoreUdpRemoteAddress();

		// Return the address of the inserted tuple.
		return stored_tuple;
	}

	inline
	RTC::TransportTuple* IceServer::HasTuple(RTC::TransportTuple* tuple)
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
			RTC::TransportTuple* stored_tuple = std::addressof(*it);

			if (stored_tuple->Compare(tuple))
				return stored_tuple;
		}

		return nullptr;
	}

	inline
	void IceServer::SetSelectedTuple(RTC::TransportTuple* stored_tuple)
	{
		MS_TRACE();

		// If already the selected tuple do nothing.
		if (stored_tuple == this->selectedTuple)
			return;

		this->selectedTuple = stored_tuple;

		// Notify the listener.
		this->listener->onIceSelectedTuple(this, this->selectedTuple);
	}
}
