#define MS_CLASS "RTC::IceServer"
// #define MS_LOG_DEV

#include <utility>

#include "Logger.hpp"
#include "RTC/IceServer.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t StunSerializeBufferSize{ 65536 };
	static uint8_t StunSerializeBuffer[StunSerializeBufferSize];

	/* Instance methods. */

	IceServer::IceServer(Listener* listener, const std::string& usernameFragment, const std::string& password)
	  : listener(listener), usernameFragment(usernameFragment), password(password)
	{
		MS_TRACE();

		MS_DEBUG_TAG(
		  ice,
		  "[usernameFragment:%s, password:%s]",
		  this->usernameFragment.c_str(),
		  this->password.c_str());
	}

	void IceServer::ProcessStunMessage(RTC::StunMessage* msg, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Must be a Binding method.
		if (msg->GetMethod() != RTC::StunMessage::Method::BINDING)
		{
			if (msg->GetClass() == RTC::StunMessage::Class::REQUEST)
			{
				MS_WARN_TAG(
				  ice, "unknown method %#.3x in STUN Request => 400", (unsigned int)msg->GetMethod());

				// Reply 400.
				RTC::StunMessage* response = msg->CreateErrorResponse(400);

				response->Serialize(StunSerializeBuffer);
				this->listener->OnOutgoingStunMessage(this, response, tuple);
				delete response;
			}
			else
			{
				MS_WARN_TAG(
				  ice,
				  "ignoring STUN Indication or Response with unknown method %#.3x",
				  (unsigned int)msg->GetMethod());
			}

			return;
		}

		// Must use FINGERPRINT (optional for ICE STUN indications).
		if (!msg->HasFingerprint() && msg->GetClass() != RTC::StunMessage::Class::INDICATION)
		{
			if (msg->GetClass() == RTC::StunMessage::Class::REQUEST)
			{
				MS_WARN_TAG(ice, "STUN Binding Request without FINGERPRINT => 400");

				// Reply 400.
				RTC::StunMessage* response = msg->CreateErrorResponse(400);

				response->Serialize(StunSerializeBuffer);
				this->listener->OnOutgoingStunMessage(this, response, tuple);
				delete response;
			}
			else
			{
				MS_WARN_TAG(ice, "ignoring STUN Binding Response without FINGERPRINT");
			}

			return;
		}

		switch (msg->GetClass())
		{
			case RTC::StunMessage::Class::REQUEST:
			{
				// USERNAME, MESSAGE-INTEGRITY and PRIORITY are required.
				if (!msg->HasMessageIntegrity() || (msg->GetPriority() == 0u) || msg->GetUsername().empty())
				{
					MS_WARN_TAG(ice, "mising required attributes in STUN Binding Request => 400");

					// Reply 400.
					RTC::StunMessage* response = msg->CreateErrorResponse(400);

					response->Serialize(StunSerializeBuffer);
					this->listener->OnOutgoingStunMessage(this, response, tuple);
					delete response;

					return;
				}

				// Check authentication.
				switch (msg->CheckAuthentication(this->usernameFragment, this->password))
				{
					case RTC::StunMessage::Authentication::OK:
						break;

					case RTC::StunMessage::Authentication::UNAUTHORIZED:
					{
						MS_WARN_TAG(ice, "wrong authentication in STUN Binding Request => 401");

						// Reply 401.
						RTC::StunMessage* response = msg->CreateErrorResponse(401);

						response->Serialize(StunSerializeBuffer);
						this->listener->OnOutgoingStunMessage(this, response, tuple);
						delete response;

						return;
					}

					case RTC::StunMessage::Authentication::BAD_REQUEST:
					{
						MS_WARN_TAG(ice, "cannot check authentication in STUN Binding Request => 400");

						// Reply 400.
						RTC::StunMessage* response = msg->CreateErrorResponse(400);

						response->Serialize(StunSerializeBuffer);
						this->listener->OnOutgoingStunMessage(this, response, tuple);
						delete response;

						return;
					}
				}

				// NOTE: Should be rejected with 487, but this makes Chrome happy:
				//   https://bugs.chromium.org/p/webrtc/issues/detail?id=7478
				// The remote peer must be ICE controlling.
				// if (msg->GetIceControlled())
				// {
				// 	MS_WARN_TAG(ice, "peer indicates ICE-CONTROLLED in STUN Binding Request => 487");

				// 	// Reply 487 (Role Conflict).
				// 	RTC::StunMessage* response = msg->CreateErrorResponse(487);

				// 	response->Serialize(StunSerializeBuffer);
				// 	this->listener->OnOutgoingStunMessage(this, response, tuple);
				// 	delete response;

				// 	return;
				// }

				MS_DEBUG_DEV(
				  "processing STUN Binding Request [Priority:%" PRIu32 ", UseCandidate:%s]",
				  static_cast<uint32_t>(msg->GetPriority()),
				  msg->HasUseCandidate() ? "true" : "false");

				// Create a success response.
				RTC::StunMessage* response = msg->CreateSuccessResponse();

				// Add XOR-MAPPED-ADDRESS.
				response->SetXorMappedAddress(tuple->GetRemoteAddress());

				// Authenticate the response.
				response->Authenticate(this->password);

				// Send back.
				response->Serialize(StunSerializeBuffer);
				this->listener->OnOutgoingStunMessage(this, response, tuple);
				delete response;

				// Handle the tuple.
				HandleTuple(tuple, msg->HasUseCandidate());

				break;
			}

			case RTC::StunMessage::Class::INDICATION:
			{
				MS_DEBUG_TAG(ice, "STUN Binding Indication processed");

				break;
			}

			case RTC::StunMessage::Class::SUCCESS_RESPONSE:
			{
				MS_DEBUG_TAG(ice, "STUN Binding Success Response processed");

				break;
			}

			case RTC::StunMessage::Class::ERROR_RESPONSE:
			{
				MS_DEBUG_TAG(ice, "STUN Binding Error Response processed");

				break;
			}
		}
	}

	bool IceServer::IsValidTuple(const RTC::TransportTuple* tuple) const
	{
		MS_TRACE();

		return HasTuple(tuple) != nullptr;
	}

	void IceServer::RemoveTuple(RTC::TransportTuple* tuple)
	{
		RTC::TransportTuple* removedTuple{ nullptr };

		// Find the removed tuple.
		auto it = this->tuples.begin();
		for (; it != this->tuples.end(); ++it)
		{
			RTC::TransportTuple* storedTuple = std::addressof(*it);

			if (storedTuple->Compare(tuple))
			{
				removedTuple = storedTuple;
				break;
			}
		}

		// If not found, ignore.
		if (removedTuple == nullptr)
			return;

		// If this is not the selected tuple just remove it.
		if (removedTuple != this->selectedTuple)
		{
			this->tuples.erase(it);

			return;
		}
		// Otherwise this was the selected tuple.

		this->tuples.erase(it);
		this->selectedTuple = nullptr;

		// Mark the first tuple as selected tuple (if any).
		if (this->tuples.begin() != this->tuples.end())
		{
			SetSelectedTuple(std::addressof(*this->tuples.begin()));
		}
		// Or just emit 'disconnected'.
		else
		{
			// Update state.
			this->state = IceState::DISCONNECTED;
			// Notify the listener.
			this->listener->OnIceDisconnected(this);
		}
	}

	void IceServer::ForceSelectedTuple(const RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		MS_ASSERT(
		  this->selectedTuple != nullptr,
		  "cannot force the selected tuple if there was not a selected tuple");

		auto storedTuple = HasTuple(tuple);

		MS_ASSERT(
		  storedTuple,
		  "cannot force the selected tuple if the given tuple was not already a valid tuple");

		// Mark it as selected tuple.
		SetSelectedTuple(storedTuple);
	}

	void IceServer::HandleTuple(RTC::TransportTuple* tuple, bool hasUseCandidate)
	{
		MS_TRACE();

		switch (this->state)
		{
			case IceState::NEW:
			{
				// There should be no tuples.
				MS_ASSERT(
				  this->tuples.empty(), "state is 'new' but there are %zu tuples", this->tuples.size());

				// There shouldn't be a selected tuple.
				MS_ASSERT(this->selectedTuple == nullptr, "state is 'new' but there is selected tuple");

				if (!hasUseCandidate)
				{
					MS_DEBUG_TAG(ice, "transition from state 'new' to 'connected'");

					// Store the tuple.
					auto storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);
					// Update state.
					this->state = IceState::CONNECTED;
					// Notify the listener.
					this->listener->OnIceConnected(this);
				}
				else
				{
					MS_DEBUG_TAG(ice, "transition from state 'new' to 'completed'");

					// Store the tuple.
					auto storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);
					// Update state.
					this->state = IceState::COMPLETED;
					// Notify the listener.
					this->listener->OnIceCompleted(this);
				}

				break;
			}

			case IceState::DISCONNECTED:
			{
				// There should be no tuples.
				MS_ASSERT(
				  this->tuples.empty(),
				  "state is 'disconnected' but there are %zu tuples",
				  this->tuples.size());

				// There shouldn't be a selected tuple.
				MS_ASSERT(
				  this->selectedTuple == nullptr, "state is 'disconnected' but there is selected tuple");

				if (!hasUseCandidate)
				{
					MS_DEBUG_TAG(ice, "transition from state 'disconnected' to 'connected'");

					// Store the tuple.
					auto storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);
					// Update state.
					this->state = IceState::CONNECTED;
					// Notify the listener.
					this->listener->OnIceConnected(this);
				}
				else
				{
					MS_DEBUG_TAG(ice, "transition from state 'disconnected' to 'completed'");

					// Store the tuple.
					auto storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);
					// Update state.
					this->state = IceState::COMPLETED;
					// Notify the listener.
					this->listener->OnIceCompleted(this);
				}

				break;
			}

			case IceState::CONNECTED:
			{
				// There should be some tuples.
				MS_ASSERT(!this->tuples.empty(), "state is 'connected' but there are no tuples");

				// There should be a selected tuple.
				MS_ASSERT(
				  this->selectedTuple != nullptr, "state is 'connected' but there is not selected tuple");

				if (!hasUseCandidate)
				{
					// If a new tuple store it.
					if (HasTuple(tuple) == nullptr)
						AddTuple(tuple);
				}
				else
				{
					MS_DEBUG_TAG(ice, "transition from state 'connected' to 'completed'");

					auto storedTuple = HasTuple(tuple);

					// If a new tuple store it.
					if (storedTuple == nullptr)
						storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);
					// Update state.
					this->state = IceState::COMPLETED;
					// Notify the listener.
					this->listener->OnIceCompleted(this);
				}

				break;
			}

			case IceState::COMPLETED:
			{
				// There should be some tuples.
				MS_ASSERT(!this->tuples.empty(), "state is 'completed' but there are no tuples");

				// There should be a selected tuple.
				MS_ASSERT(
				  this->selectedTuple != nullptr, "state is 'completed' but there is not selected tuple");

				if (!hasUseCandidate)
				{
					// If a new tuple store it.
					if (HasTuple(tuple) == nullptr)
						AddTuple(tuple);
				}
				else
				{
					auto storedTuple = HasTuple(tuple);

					// If a new tuple store it.
					if (storedTuple == nullptr)
						storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);
				}

				break;
			}
		}
	}

	inline RTC::TransportTuple* IceServer::AddTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Add the new tuple at the beginning of the list.
		this->tuples.push_front(*tuple);

		auto storedTuple = std::addressof(*this->tuples.begin());

		// If it is UDP then we must store the remote address (until now it is
		// just a pointer that will be freed soon).
		if (storedTuple->GetProtocol() == TransportTuple::Protocol::UDP)
			storedTuple->StoreUdpRemoteAddress();

		// Return the address of the inserted tuple.
		return storedTuple;
	}

	inline RTC::TransportTuple* IceServer::HasTuple(const RTC::TransportTuple* tuple) const
	{
		MS_TRACE();

		// If there is no selected tuple yet then we know that the tuples list
		// is empty.
		if (this->selectedTuple == nullptr)
			return nullptr;

		// Check the current selected tuple.
		if (this->selectedTuple->Compare(tuple))
			return this->selectedTuple;

		// Otherwise check other stored tuples.
		for (const auto& it : this->tuples)
		{
			auto* storedTuple = const_cast<RTC::TransportTuple*>(std::addressof(it));

			if (storedTuple->Compare(tuple))
				return storedTuple;
		}

		return nullptr;
	}

	inline void IceServer::SetSelectedTuple(RTC::TransportTuple* storedTuple)
	{
		MS_TRACE();

		// If already the selected tuple do nothing.
		if (storedTuple == this->selectedTuple)
			return;

		this->selectedTuple = storedTuple;

		// Notify the listener.
		this->listener->OnIceSelectedTuple(this, this->selectedTuple);
	}
} // namespace RTC
