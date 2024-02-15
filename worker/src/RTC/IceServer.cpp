#define MS_CLASS "RTC::IceServer"
// #define MS_LOG_DEV_LEVEL 3

#include "RTC/IceServer.hpp"
#include "Logger.hpp"
#include "DepLibUV.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t StunSerializeBufferSize{ 65536 };
	thread_local static uint8_t StunSerializeBuffer[StunSerializeBufferSize];
	static constexpr size_t MaxTuples{ 8 };
	static const std::string SoftwareAttribute{ "mediasoup" };
	static constexpr uint64_t ConsentCheckIntervalMs{ 5000u };
	static constexpr uint64_t ConsentCheckResponseTimeoutMs{ 30000u };

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
			{
				return FBS::WebRtcTransport::IceState::NEW;
			}

			case IceServer::IceState::CONNECTED:
			{
				return FBS::WebRtcTransport::IceState::CONNECTED;
			}

			case IceServer::IceState::COMPLETED:
			{
				return FBS::WebRtcTransport::IceState::COMPLETED;
			}

			case IceServer::IceState::DISCONNECTED:
			{
				return FBS::WebRtcTransport::IceState::DISCONNECTED;
			}
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

		// Clear queue of ongoing sent ICE consent requests.
		this->sentConsents.clear();

		// Delete the ICE consent check timer.
		delete this->consentCheckTimer;
		this->consentCheckTimer = nullptr;
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
				// PRIORITY is required.
				if (packet->GetPriority() == 0u)
				{
					MS_WARN_TAG(ice, "mising required PRIORITY attribute in STUN Binding Request => 400");

					// Reply 400.
					RTC::StunPacket* response = packet->CreateErrorResponse(400);

					response->Serialize(StunSerializeBuffer);
					this->listener->OnIceServerSendStunPacket(this, response, tuple);

					delete response;

					return;
				}

				// Check authentication.
				switch (packet->CheckAuthentication(
				  this->usernameFragment, this->remoteUsernameFragment, this->password))
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
							packet->CheckAuthentication(this->oldUsernameFragment, this->remoteUsernameFragment, this->oldPassword) == RTC::StunPacket::Authentication::OK
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

					case RTC::StunPacket::Authentication::BAD_MESSAGE:
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
				MS_DEBUG_TAG(ice, "STUN Binding Indication received, ignored");

				break;
			}

			case RTC::StunPacket::Class::SUCCESS_RESPONSE:
			{
				// Check authentication.
				switch (packet->CheckAuthentication(
				  this->remoteUsernameFragment, this->usernameFragment, this->remotePassword))
				{
					case RTC::StunPacket::Authentication::OK:
					{
						break;
					}

					case RTC::StunPacket::Authentication::UNAUTHORIZED:
					{
						MS_WARN_TAG(ice, "received STUN Success Response with wrong authentication, discarded");

						return;
					}

					case RTC::StunPacket::Authentication::BAD_MESSAGE:
					{
						MS_WARN_TAG(ice, "cannot check authentication in received STUN Binding Success Response, discarded");

						return;
					}
				}

				// Trick: We only read the fist 4 bytes of the transactionId of the
				// response since we know that we generated 4 bytes followed by 8 zeroed
				// bytes in the transactionId of the sent consent request.
				auto transactionId = Utils::Byte::Get4Bytes(packet->GetTransactionId(), 0);

				// Let's iterate all entries in the queue and remove the consent request
				// associated to this response and all those that were sent before.

				// Find the associated entry first.
				auto it = this->sentConsents.begin();

				for (; it != this->sentConsents.end(); ++it)
				{
					auto& sentConsent = *it;

					if (Utils::Byte::Get4Bytes(sentConsent.transactionId, 0) == transactionId)
					{
						break;
					}
				}

				if (it != this->sentConsents.end())
				{
					// Delete the matching entry and all previous ones.
					this->sentConsents.erase(this->sentConsents.begin(), it + 1);
				}
				else
				{
					MS_WARN_TAG(ice, "received STUN Success Response doesn't match any sent consent request");
				}

				break;
			}

			case RTC::StunPacket::Class::ERROR_RESPONSE:
			{
				// TODO: We may only want to check auth if 403 with MESSAGE_INTEGRITY.

				// Check authentication.
				switch (packet->CheckAuthentication(
				  this->remoteUsernameFragment, this->usernameFragment, this->remotePassword))
				{
					case RTC::StunPacket::Authentication::OK:
					{
						break;
					}

					case RTC::StunPacket::Authentication::UNAUTHORIZED:
					{
						MS_WARN_TAG(ice, "received STUN Error Response with wrong authentication, discarded");

						return;
					}

					case RTC::StunPacket::Authentication::BAD_MESSAGE:
					{
						MS_WARN_TAG(ice, "cannot check authentication in received STUN Binding Error Response");

						return;
					}

					// TODO: Check if this a 403 response to our consent request and do stuff.
				}

				break;
			}
		}
	}

	void IceServer::RestartIce(const std::string& usernameFragment, const std::string& password)
	{
		MS_TRACE();

		if (!this->oldUsernameFragment.empty())
		{
			this->listener->OnIceServerLocalUsernameFragmentRemoved(this, this->oldUsernameFragment);
		}

		this->oldUsernameFragment = this->usernameFragment;
		this->usernameFragment    = usernameFragment;

		this->oldPassword = this->password;
		this->password    = password;

		this->remoteNomination = 0u;

		// Notify the listener.
		this->listener->OnIceServerLocalUsernameFragmentAdded(this, usernameFragment);

		// NOTE: Do not call listener->OnIceServerLocalUsernameFragmentRemoved()
		// yet with old usernameFragment. Wait until we receive a STUN packet
		// with the new one.

		// Restart ICE consent checks to avoid authentication issues.
		MayRestartConsentChecks();
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

				MayStartOrRestartConsentChecks();
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

				MayStopConsentChecks();
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
		auto isNewSelectedTuple = SetSelectedTuple(storedTuple);

		if (isNewSelectedTuple) {
			MayStartOrRestartConsentChecks();
		}
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

					MayStartConsentChecks();
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

						MayStartConsentChecks();
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

					MayStartConsentChecks();
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

						MayStartConsentChecks();
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
						auto isNewSelectedTuple = SetSelectedTuple(storedTuple);

						// Update state.
						this->state = IceState::COMPLETED;

						// Update nomination.
						if (hasNomination && nomination > this->remoteNomination)
						{
							this->remoteNomination = nomination;
						}

						// Notify the listener.
						this->listener->OnIceServerCompleted(this);

						if (isNewSelectedTuple) {
							MayStartOrRestartConsentChecks();
						}
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
						auto isNewSelectedTuple = SetSelectedTuple(storedTuple);

						// Update nomination.
						if (hasNomination && nomination > this->remoteNomination)
						{
							this->remoteNomination = nomination;
						}

						if (isNewSelectedTuple) {
							MayStartOrRestartConsentChecks();
						}
					}
				}

				break;
			}
		}
	}

	RTC::TransportTuple* IceServer::AddTuple(RTC::TransportTuple* tuple)
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

	RTC::TransportTuple* IceServer::HasTuple(const RTC::TransportTuple* tuple) const
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

	bool IceServer::SetSelectedTuple(RTC::TransportTuple* storedTuple)
	{
		MS_TRACE();

		// If already the selected tuple do nothing.
		if (storedTuple == this->selectedTuple)
		{
			return false;
		}

		this->selectedTuple = storedTuple;

		// Notify the listener.
		this->listener->OnIceServerSelectedTuple(this, this->selectedTuple);

		return true;
	}

	void IceServer::MayStartConsentChecks()
	{
		MS_TRACE();

		if (!AreConsentChecksEnabled())
		{
			return;
		}
		else if (AreConsentChecksRunning())
		{
			return;
		}

		// Create the ICE consent checks timer if it doesn't exist.
		if (!this->consentCheckTimer)	{
			this->consentCheckTimer = new TimerHandle(this);
		}

		this->consentCheckTimer->Start(ConsentCheckIntervalMs);
	}

	void IceServer::MayStopConsentChecks()
	{
		MS_TRACE();

		if (this->consentCheckTimer)	{
			this->consentCheckTimer->Stop();
		}

		// Clear queue of ongoing sent ICE consent requests.
		this->sentConsents.clear();
	}

	void IceServer::MayRestartConsentChecks()
	{
		MS_TRACE();

		if (!AreConsentChecksRunning())
		{
			return;
		}

		// Clear queue of ongoing sent ICE consent requests.
		this->sentConsents.clear();

		MayStartConsentChecks();
	}

	void IceServer::MayStartOrRestartConsentChecks()
	{
		MS_TRACE();

		if (!AreConsentChecksRunning())
		{
			MayStartConsentChecks();
		}
		else
		{
			MayRestartConsentChecks();
		}
	}

	void IceServer::SendConsentRequest()
	{
		MS_TRACE();

		// MS_ASSERT(this->state == IceState::CONNECTED || this->state == IceState::COMPLETED, "cannot
		// send consent request in state other than 'connected' or 'completed'");
		MS_ASSERT(this->selectedTuple, "cannot send consent request without a selected tuple");

		// Here we do a trick. We generate a transactionId of 4 bytes and fill the
		// latest 8 bytes of the transactionId with zeroes.
		uint32_t transactionId = Utils::Crypto::GetRandomUInt(0u, UINT32_MAX);

		auto& sentContext = this->sentConsents.emplace_back(transactionId, DepLibUV::GetTimeMs());

		auto* request = new StunPacket(
		  StunPacket::Class::REQUEST,
		  StunPacket::Method::BINDING,
		  sentContext.transactionId,
		  nullptr,
		  0);

		const std::string username = this->remoteUsernameFragment + ":" + this->usernameFragment;

		request->SetUsername(username.c_str(), username.length());
		request->SetPriority(1u);
		request->SetIceControlled(1u);
		request->SetSoftware(SoftwareAttribute.c_str(), SoftwareAttribute.length());
		request->Authenticate(this->remotePassword);
		request->Serialize(StunSerializeBuffer);

		this->listener->OnIceServerSendStunPacket(this, request, this->selectedTuple);

		delete request;
	}

	inline void IceServer::OnTimer(TimerHandle* timer)
	{
		MS_TRACE();

		if (timer == this->consentCheckTimer)
		{
			// State must be 'connected' or 'completed'.
			MS_ASSERT(this->state == IceState::COMPLETED || this->state == IceState::CONNECTED, "ICE consent check timer fired but state is neither 'completed' nor 'connected'");
			// There should be a selected tuple.
			MS_ASSERT(this->selectedTuple, "ICE consent check timer fired but there is not selected tuple");

			auto nowMs = DepLibUV::GetTimeMs();

			// Check if there is at least an ongoing expired ICE consent request
			// and if so move to disconnected state.
			auto it = this->sentConsents.begin();

			for (; it != this->sentConsents.end(); ++it)
			{
				auto& sentConsent = *it;

				if (nowMs - sentConsent.sentAtMs >= ConsentCheckResponseTimeoutMs)
				{
					break;
				}
			}

			if (it == this->sentConsents.end())
			{
					// Send a new ICE consent request.
					SendConsentRequest();

					/*
				   * The interval between ICE consent requests is varied randomly over the
				   * range [0.8, 1.2] times the calculated interval to prevent
				   * synchronization of consent checks.
				   */
					const uint64_t interval = ConsentCheckIntervalMs + static_cast<float>(Utils::Crypto::GetRandomUInt(8, 12)) / 10;

					this->consentCheckTimer->Start(interval);
			}
			else
			{
				MS_WARN_TAG(ice, "ICE consent expired, moving to 'disconnected' state");

				auto* disconnectedSelectedTuple = this->selectedTuple;

				// Update state.
				this->state = IceState::DISCONNECTED;

				// Clear all tuples.
				this->selectedTuple = nullptr;
				this->tuples.clear();

				// Reset remote nomination.
				this->remoteNomination = 0u;

				// Clear queue of ongoing sent ICE consent requests.
				this->sentConsents.clear();

				// Notify the listener.
				this->listener->OnIceServerTupleRemoved(this, disconnectedSelectedTuple);
				this->listener->OnIceServerDisconnected(this);
			}
		}
	}
} // namespace RTC
