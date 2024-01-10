#define MS_CLASS "RTC::IceServer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/IceServer.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t StunSerializeBufferSize{ 65536 };
	thread_local static uint8_t StunSerializeBuffer[StunSerializeBufferSize];
	static constexpr size_t MaxTuples{ 8 };

	/* Class methods. */
	IceServer::IceState IceStateFromFbs(FBS::WebRtcTransport::IceState state)
	{
		switch (state)
		{
			case FBS::WebRtcTransport::IceState::NEW:
			{
				return IceServer::IceState::NEW;
			}

			case FBS::WebRtcTransport::IceState::CONNECTED:
			{
				return IceServer::IceState::CONNECTED;
			}

			case FBS::WebRtcTransport::IceState::COMPLETED:
			{
				return IceServer::IceState::COMPLETED;
			}

			case FBS::WebRtcTransport::IceState::DISCONNECTED:
			{
				return IceServer::IceState::DISCONNECTED;
			}
		}
	}

	FBS::WebRtcTransport::IceState IceServer::IceStateToFbs(IceServer::IceState state)
	{
		switch (state)
		{
			case IceServer::IceState::NEW:
				return FBS::WebRtcTransport::IceState::NEW;

			case IceServer::IceState::CONNECTED:
				return FBS::WebRtcTransport::IceState::CONNECTED;

			case IceServer::IceState::COMPLETED:
				return FBS::WebRtcTransport::IceState::COMPLETED;

			case IceServer::IceState::DISCONNECTED:
				return FBS::WebRtcTransport::IceState::DISCONNECTED;
		}
	}

	/* Instance methods. */

	IceServer::IceServer(Listener* listener, const std::string& usernameFragment, const std::string& password)
	  : listener(listener), usernameFragment(usernameFragment), password(password)
	{
		MS_TRACE();

		// Notify the listener.
		this->listener->OnIceServerLocalUsernameFragmentAdded(this, usernameFragment);
	}

	IceServer::~IceServer()
	{
		MS_TRACE();

		// Here we must notify the listener about the removal of current
		// usernameFragments (and also the old one if any) and all tuples.

		this->listener->OnIceServerLocalUsernameFragmentRemoved(this, usernameFragment);

		if (!this->oldUsernameFragment.empty())
		{
			this->listener->OnIceServerLocalUsernameFragmentRemoved(this, this->oldUsernameFragment);
		}

		for (const auto& it : this->tuples)
		{
			auto* storedTuple = const_cast<RTC::TransportTuple*>(std::addressof(it));

			// Notify the listener.
			this->listener->OnIceServerTupleRemoved(this, storedTuple);
		}

		this->tuples.clear();
	}

	void IceServer::ProcessStunPacket(RTC::StunPacket* packet, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		// Must be a Binding method.
		if (packet->GetMethod() != RTC::StunPacket::Method::BINDING)
		{
			if (packet->GetClass() == RTC::StunPacket::Class::REQUEST)
			{
				MS_WARN_TAG(
				  ice,
				  "unknown method %#.3x in STUN Request => 400",
				  static_cast<unsigned int>(packet->GetMethod()));

				// Reply 400.
				RTC::StunPacket* response = packet->CreateErrorResponse(400);

				response->Serialize(StunSerializeBuffer);
				this->listener->OnIceServerSendStunPacket(this, response, tuple);

				delete response;
			}
			else
			{
				MS_WARN_TAG(
				  ice,
				  "ignoring STUN Indication or Response with unknown method %#.3x",
				  static_cast<unsigned int>(packet->GetMethod()));
			}

			return;
		}

		// Must use FINGERPRINT (optional for ICE STUN indications).
		if (!packet->HasFingerprint() && packet->GetClass() != RTC::StunPacket::Class::INDICATION)
		{
			if (packet->GetClass() == RTC::StunPacket::Class::REQUEST)
			{
				MS_WARN_TAG(ice, "STUN Binding Request without FINGERPRINT => 400");

				// Reply 400.
				RTC::StunPacket* response = packet->CreateErrorResponse(400);

				response->Serialize(StunSerializeBuffer);
				this->listener->OnIceServerSendStunPacket(this, response, tuple);

				delete response;
			}
			else
			{
				MS_WARN_TAG(ice, "ignoring STUN Binding Response without FINGERPRINT");
			}

			return;
		}

		switch (packet->GetClass())
		{
			case RTC::StunPacket::Class::REQUEST:
			{
				// USERNAME, MESSAGE-INTEGRITY and PRIORITY are required.
				if (!packet->HasMessageIntegrity() || (packet->GetPriority() == 0u) || packet->GetUsername().empty())
				{
					MS_WARN_TAG(ice, "mising required attributes in STUN Binding Request => 400");

					// Reply 400.
					RTC::StunPacket* response = packet->CreateErrorResponse(400);

					response->Serialize(StunSerializeBuffer);
					this->listener->OnIceServerSendStunPacket(this, response, tuple);

					delete response;

					return;
				}

				// Check authentication.
				switch (packet->CheckAuthentication(this->usernameFragment, this->password))
				{
					case RTC::StunPacket::Authentication::OK:
					{
						if (!this->oldUsernameFragment.empty() && !this->oldPassword.empty())
						{
							MS_DEBUG_TAG(ice, "new ICE credentials applied");

							// Notify the listener.
							this->listener->OnIceServerLocalUsernameFragmentRemoved(this, this->oldUsernameFragment);

							this->oldUsernameFragment.clear();
							this->oldPassword.clear();
						}

						break;
					}

					case RTC::StunPacket::Authentication::UNAUTHORIZED:
					{
						// We may have changed our usernameFragment and password, so check
						// the old ones.
						// clang-format off
						if (
							!this->oldUsernameFragment.empty() &&
							!this->oldPassword.empty() &&
							packet->CheckAuthentication(this->oldUsernameFragment, this->oldPassword) == RTC::StunPacket::Authentication::OK
						)
						// clang-format on
						{
							MS_DEBUG_TAG(ice, "using old ICE credentials");

							break;
						}

						MS_WARN_TAG(ice, "wrong authentication in STUN Binding Request => 401");

						// Reply 401.
						RTC::StunPacket* response = packet->CreateErrorResponse(401);

						response->Serialize(StunSerializeBuffer);
						this->listener->OnIceServerSendStunPacket(this, response, tuple);

						delete response;

						return;
					}

					case RTC::StunPacket::Authentication::BAD_REQUEST:
					{
						MS_WARN_TAG(ice, "cannot check authentication in STUN Binding Request => 400");

						// Reply 400.
						RTC::StunPacket* response = packet->CreateErrorResponse(400);

						response->Serialize(StunSerializeBuffer);
						this->listener->OnIceServerSendStunPacket(this, response, tuple);

						delete response;

						return;
					}
				}

				// The remote peer must be ICE controlling.
				if (packet->GetIceControlled())
				{
					MS_WARN_TAG(ice, "peer indicates ICE-CONTROLLED in STUN Binding Request => 487");

					// Reply 487 (Role Conflict).
					RTC::StunPacket* response = packet->CreateErrorResponse(487);

					response->Serialize(StunSerializeBuffer);
					this->listener->OnIceServerSendStunPacket(this, response, tuple);

					delete response;

					return;
				}

				MS_DEBUG_DEV(
				  "processing STUN Binding Request [Priority:%" PRIu32 ", UseCandidate:%s]",
				  static_cast<uint32_t>(packet->GetPriority()),
				  packet->HasUseCandidate() ? "true" : "false");

				// Create a success response.
				RTC::StunPacket* response = packet->CreateSuccessResponse();

				// Add XOR-MAPPED-ADDRESS.
				response->SetXorMappedAddress(tuple->GetRemoteAddress());

				// Authenticate the response.
				if (this->oldPassword.empty())
				{
					response->Authenticate(this->password);
				}
				else
				{
					response->Authenticate(this->oldPassword);
				}

				// Send back.
				response->Serialize(StunSerializeBuffer);
				this->listener->OnIceServerSendStunPacket(this, response, tuple);

				delete response;

				uint32_t nomination{ 0u };

				if (packet->HasNomination())
				{
					nomination = packet->GetNomination();
				}

				// Handle the tuple.
				HandleTuple(tuple, packet->HasUseCandidate(), packet->HasNomination(), nomination);

				break;
			}

			case RTC::StunPacket::Class::INDICATION:
			{
				MS_DEBUG_TAG(ice, "STUN Binding Indication processed");

				break;
			}

			case RTC::StunPacket::Class::SUCCESS_RESPONSE:
			{
				MS_DEBUG_TAG(ice, "STUN Binding Success Response processed");

				break;
			}

			case RTC::StunPacket::Class::ERROR_RESPONSE:
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
		MS_TRACE();

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
		if (!removedTuple)
		{
			return;
		}

		// Notify the listener.
		this->listener->OnIceServerTupleRemoved(this, removedTuple);

		// Remove it from the list of tuples.
		// NOTE: Do it after notifying the listener since the listener may need to
		// use/read the tuple being removed so we cannot free it yet.
		this->tuples.erase(it);

		// If this is the selected tuple, do things.
		if (removedTuple == this->selectedTuple)
		{
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

				// Reset remote nomination.
				this->remoteNomination = 0u;

				// Notify the listener.
				this->listener->OnIceServerDisconnected(this);
			}
		}
	}

	void IceServer::MayForceSelectedTuple(const RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		if (this->state != IceState::CONNECTED && this->state != IceState::COMPLETED)
		{
			MS_WARN_TAG(ice, "cannot force selected tuple if not in state 'connected' or 'completed'");

			return;
		}

		auto* storedTuple = HasTuple(tuple);

		if (!storedTuple)
		{
			MS_WARN_TAG(ice, "cannot force selected tuple if the given tuple was not already a valid one");

			return;
		}

		// Mark it as selected tuple.
		SetSelectedTuple(storedTuple);
	}

	void IceServer::HandleTuple(
	  RTC::TransportTuple* tuple, bool hasUseCandidate, bool hasNomination, uint32_t nomination)
	{
		MS_TRACE();

		switch (this->state)
		{
			case IceState::NEW:
			{
				// There shouldn't be a selected tuple.
				MS_ASSERT(!this->selectedTuple, "state is 'new' but there is selected tuple");

				if (!hasUseCandidate && !hasNomination)
				{
					MS_DEBUG_TAG(
					  ice,
					  "transition from state 'new' to 'connected' [hasUseCandidate:%s, hasNomination:%s, nomination:%" PRIu32
					  "]",
					  hasUseCandidate ? "true" : "false",
					  hasNomination ? "true" : "false",
					  nomination);

					// Store the tuple.
					auto* storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);

					// Update state.
					this->state = IceState::CONNECTED;

					// Notify the listener.
					this->listener->OnIceServerConnected(this);
				}
				else
				{
					// Store the tuple.
					auto* storedTuple = AddTuple(tuple);

					if ((hasNomination && nomination > this->remoteNomination) || !hasNomination)
					{
						MS_DEBUG_TAG(
						  ice,
						  "transition from state 'new' to 'completed' [hasUseCandidate:%s, hasNomination:%s, nomination:%" PRIu32
						  "]",
						  hasUseCandidate ? "true" : "false",
						  hasNomination ? "true" : "false",
						  nomination);

						// Mark it as selected tuple.
						SetSelectedTuple(storedTuple);

						// Update state.
						this->state = IceState::COMPLETED;

						// Update nomination.
						if (hasNomination && nomination > this->remoteNomination)
						{
							this->remoteNomination = nomination;
						}

						// Notify the listener.
						this->listener->OnIceServerCompleted(this);
					}
				}

				break;
			}

			case IceState::DISCONNECTED:
			{
				// There shouldn't be a selected tuple.
				MS_ASSERT(!this->selectedTuple, "state is 'disconnected' but there is selected tuple");

				if (!hasUseCandidate && !hasNomination)
				{
					MS_DEBUG_TAG(
					  ice,
					  "transition from state 'disconnected' to 'connected' [hasUseCandidate:%s, hasNomination:%s, nomination:%" PRIu32
					  "]",
					  hasUseCandidate ? "true" : "false",
					  hasNomination ? "true" : "false",
					  nomination);

					// Store the tuple.
					auto* storedTuple = AddTuple(tuple);

					// Mark it as selected tuple.
					SetSelectedTuple(storedTuple);

					// Update state.
					this->state = IceState::CONNECTED;

					// Notify the listener.
					this->listener->OnIceServerConnected(this);
				}
				else
				{
					// Store the tuple.
					auto* storedTuple = AddTuple(tuple);

					if ((hasNomination && nomination > this->remoteNomination) || !hasNomination)
					{
						MS_DEBUG_TAG(
						  ice,
						  "transition from state 'disconnected' to 'completed' [hasUseCandidate:%s, hasNomination:%s, nomination:%" PRIu32
						  "]",
						  hasUseCandidate ? "true" : "false",
						  hasNomination ? "true" : "false",
						  nomination);

						// Mark it as selected tuple.
						SetSelectedTuple(storedTuple);

						// Update state.
						this->state = IceState::COMPLETED;

						// Update nomination.
						if (hasNomination && nomination > this->remoteNomination)
						{
							this->remoteNomination = nomination;
						}

						// Notify the listener.
						this->listener->OnIceServerCompleted(this);
					}
				}

				break;
			}

			case IceState::CONNECTED:
			{
				// There should be some tuples.
				MS_ASSERT(!this->tuples.empty(), "state is 'connected' but there are no tuples");

				// There should be a selected tuple.
				MS_ASSERT(this->selectedTuple, "state is 'connected' but there is not selected tuple");

				if (!hasUseCandidate && !hasNomination)
				{
					// Store the tuple.
					AddTuple(tuple);
				}
				else
				{
					MS_DEBUG_TAG(
					  ice,
					  "transition from state 'connected' to 'completed' [hasUseCandidate:%s, hasNomination:%s, nomination:%" PRIu32
					  "]",
					  hasUseCandidate ? "true" : "false",
					  hasNomination ? "true" : "false",
					  nomination);

					// Store the tuple.
					auto* storedTuple = AddTuple(tuple);

					if ((hasNomination && nomination > this->remoteNomination) || !hasNomination)
					{
						// Mark it as selected tuple.
						SetSelectedTuple(storedTuple);

						// Update state.
						this->state = IceState::COMPLETED;

						// Update nomination.
						if (hasNomination && nomination > this->remoteNomination)
						{
							this->remoteNomination = nomination;
						}

						// Notify the listener.
						this->listener->OnIceServerCompleted(this);
					}
				}

				break;
			}

			case IceState::COMPLETED:
			{
				// There should be some tuples.
				MS_ASSERT(!this->tuples.empty(), "state is 'completed' but there are no tuples");

				// There should be a selected tuple.
				MS_ASSERT(this->selectedTuple, "state is 'completed' but there is not selected tuple");

				if (!hasUseCandidate && !hasNomination)
				{
					// Store the tuple.
					AddTuple(tuple);
				}
				else
				{
					// Store the tuple.
					auto* storedTuple = AddTuple(tuple);

					if ((hasNomination && nomination > this->remoteNomination) || !hasNomination)
					{
						// Mark it as selected tuple.
						SetSelectedTuple(storedTuple);

						// Update nomination.
						if (hasNomination && nomination > this->remoteNomination)
						{
							this->remoteNomination = nomination;
						}
					}
				}

				break;
			}
		}
	}

	inline RTC::TransportTuple* IceServer::AddTuple(RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		auto* storedTuple = HasTuple(tuple);

		if (storedTuple)
		{
			MS_DEBUG_DEV('tuple already exists');

			return storedTuple;
		}

		// Add the new tuple at the beginning of the list.
		this->tuples.push_front(*tuple);

		storedTuple = std::addressof(*this->tuples.begin());

		// If it is UDP then we must store the remote address (until now it is
		// just a pointer that will be freed soon).
		if (storedTuple->GetProtocol() == TransportTuple::Protocol::UDP)
		{
			storedTuple->StoreUdpRemoteAddress();
		}

		// Notify the listener.
		this->listener->OnIceServerTupleAdded(this, storedTuple);

		// Don't allow more than MaxTuples.
		if (this->tuples.size() > MaxTuples)
		{
			MS_WARN_TAG(ice, "too too many tuples, removing the oldest non selected one");

			// Find the oldest tuple which is neither the added one nor the selected
			// one (if any), and remove it.
			RTC::TransportTuple* removedTuple{ nullptr };
			auto it = this->tuples.rbegin();

			for (; it != this->tuples.rend(); ++it)
			{
				RTC::TransportTuple* otherStoredTuple = std::addressof(*it);

				if (otherStoredTuple != storedTuple && otherStoredTuple != this->selectedTuple)
				{
					removedTuple = otherStoredTuple;

					break;
				}
			}

			// This should not happen by design.
			MS_ASSERT(removedTuple, "couldn't find any tuple to be removed");

			// Notify the listener.
			this->listener->OnIceServerTupleRemoved(this, removedTuple);

			// Remove it from the list of tuples.
			// NOTE: Do it after notifying the listener since the listener may need to
			// use/read the tuple being removed so we cannot free it yet.
			// NOTE: This trick is needed since it is a reverse_iterator and
			// erase() requires a iterator, const_iterator or bidirectional_iterator.
			this->tuples.erase(std::next(it).base());
		}

		// Return the address of the inserted tuple.
		return storedTuple;
	}

	inline RTC::TransportTuple* IceServer::HasTuple(const RTC::TransportTuple* tuple) const
	{
		MS_TRACE();

		// Check the current selected tuple (if any).
		if (this->selectedTuple && this->selectedTuple->Compare(tuple))
		{
			return this->selectedTuple;
		}

		// Otherwise check other stored tuples.
		for (const auto& it : this->tuples)
		{
			auto* storedTuple = const_cast<RTC::TransportTuple*>(std::addressof(it));

			if (storedTuple->Compare(tuple))
			{
				return storedTuple;
			}
		}

		return nullptr;
	}

	inline void IceServer::SetSelectedTuple(RTC::TransportTuple* storedTuple)
	{
		MS_TRACE();

		// If already the selected tuple do nothing.
		if (storedTuple == this->selectedTuple)
		{
			return;
		}

		this->selectedTuple = storedTuple;

		// Notify the listener.
		this->listener->OnIceServerSelectedTuple(this, this->selectedTuple);
	}
} // namespace RTC
