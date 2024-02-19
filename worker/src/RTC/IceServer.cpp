#define MS_CLASS "RTC::IceServer"
#define MS_LOG_DEV_LEVEL 3

#include "RTC/IceServer.hpp"
#include "DepLibUV.hpp"
#include "Logger.hpp"

namespace RTC
{
	/* Static. */

	static constexpr size_t StunSerializeBufferSize{ 65536 };
	thread_local static uint8_t StunSerializeBuffer[StunSerializeBufferSize];
	static constexpr size_t MaxTuples{ 8 };
	static const std::string SoftwareAttribute{ "mediasoup" };
	static constexpr uint64_t ConsentCheckIntervalMs{ 5000u };
	static constexpr uint8_t ConsentCheckMinTimeoutSec{ 10u };
	static constexpr uint8_t ConsentCheckMaxTimeoutSec{ 60u };
	static constexpr uint64_t MaxOngoingSentConsents{ 20u };

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

	IceServer::IceServer(
	  Listener* listener,
	  const std::string& usernameFragment,
	  const std::string& password,
	  uint8_t consentTimeoutSec)
	  : listener(listener), usernameFragment(usernameFragment), password(password)
	{
		MS_TRACE();

		if (consentTimeoutSec == 0u)
		{
			// 0 means disabled so it's a valid value.
		}
		else if (consentTimeoutSec < ConsentCheckMinTimeoutSec)
		{
			MS_WARN_TAG(
			  ice,
			  "consentTimeoutSec cannot be lower than %" PRIu8 " seconds, fixing it",
			  ConsentCheckMinTimeoutSec);

			consentTimeoutSec = ConsentCheckMinTimeoutSec;
		}
		else if (consentTimeoutSec > ConsentCheckMaxTimeoutSec)
		{
			MS_WARN_TAG(
			  ice,
			  "consentTimeoutSec cannot be higher than %" PRIu8 " seconds, fixing it",
			  ConsentCheckMaxTimeoutSec);

			consentTimeoutSec = ConsentCheckMaxTimeoutSec;
		}

		this->consentTimeoutMs = consentTimeoutSec * 1000;

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

		// Clear queue of ongoing sent ICE consent Requests.
		this->sentConsents.clear();

		// Delete the ICE consent check timer.
		delete this->consentCheckTimer;
		this->consentCheckTimer = nullptr;
	}

	void IceServer::ProcessStunPacket(RTC::StunPacket* packet, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		switch (packet->GetClass())
		{
			case RTC::StunPacket::Class::REQUEST:
			{
				ProcessStunRequest(packet, tuple);

				break;
			}

			case RTC::StunPacket::Class::INDICATION:
			{
				ProcessStunIndication(packet);

				break;
			}

			case RTC::StunPacket::Class::SUCCESS_RESPONSE:
			case RTC::StunPacket::Class::ERROR_RESPONSE:
			{
				ProcessStunResponse(packet);

				break;
			}

			default:
			{
				MS_WARN_TAG(ice, "unknown STUN class %" PRIu16 ", discarded", packet->GetClass());
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

		// Restart ICE consent check to avoid authentication issues.
		MayRestartConsentCheck();
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

				MayStartOrRestartConsentCheck();
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

				MayStopConsentCheck();
			}
		}
	}

	void IceServer::ProcessStunRequest(RTC::StunPacket* request, RTC::TransportTuple* tuple)
	{
		MS_TRACE();

		MS_DEBUG_DEV("processing STUN Request");

		// Must be a Binding method.
		if (request->GetMethod() != RTC::StunPacket::Method::BINDING)
		{
			MS_WARN_TAG(
			  ice,
			  "STUN Request with unknown method %#.3x => 400",
			  static_cast<unsigned int>(request->GetMethod()));

			// Reply 400.
			RTC::StunPacket* response = request->CreateErrorResponse(400);

			response->Serialize(StunSerializeBuffer);
			this->listener->OnIceServerSendStunPacket(this, response, tuple);

			delete response;

			return;
		}

		// Must have FINGERPRINT attribute.
		if (!request->HasFingerprint())
		{
			MS_WARN_TAG(ice, "STUN Binding Request without FINGERPRINT attribute => 400");

			// Reply 400.
			RTC::StunPacket* response = request->CreateErrorResponse(400);

			response->Serialize(StunSerializeBuffer);
			this->listener->OnIceServerSendStunPacket(this, response, tuple);

			delete response;

			return;
		}

		// PRIORITY attribute is required.
		if (request->GetPriority() == 0u)
		{
			MS_WARN_TAG(ice, "STUN Binding Request without PRIORITY attribute => 400");

			// Reply 400.
			RTC::StunPacket* response = request->CreateErrorResponse(400);

			response->Serialize(StunSerializeBuffer);
			this->listener->OnIceServerSendStunPacket(this, response, tuple);

			delete response;

			return;
		}

		// Check authentication.
		switch (request->CheckAuthentication(
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
				// We may have changed our usernameFragment and password, so check the
				// old ones.
				// clang-format off
				if (
				  !this->oldUsernameFragment.empty() &&
				  !this->oldPassword.empty() &&
				  request->CheckAuthentication(
				    this->oldUsernameFragment, this->remoteUsernameFragment, this->oldPassword
				  ) == RTC::StunPacket::Authentication::OK
				)
				// clang-format on
				{
					MS_DEBUG_TAG(ice, "using old ICE credentials");

					break;
				}

				MS_WARN_TAG(ice, "wrong authentication in STUN Binding Request => 401");

				// Reply 401.
				RTC::StunPacket* response = request->CreateErrorResponse(401);

				response->Serialize(StunSerializeBuffer);
				this->listener->OnIceServerSendStunPacket(this, response, tuple);

				delete response;

				return;
			}

			case RTC::StunPacket::Authentication::BAD_MESSAGE:
			{
				MS_WARN_TAG(ice, "cannot check authentication in STUN Binding Request => 400");

				// Reply 400.
				RTC::StunPacket* response = request->CreateErrorResponse(400);

				response->Serialize(StunSerializeBuffer);
				this->listener->OnIceServerSendStunPacket(this, response, tuple);

				delete response;

				return;
			}
		}

		// The remote peer must be ICE controlling.
		if (request->GetIceControlled())
		{
			MS_WARN_TAG(ice, "peer indicates ICE-CONTROLLED in STUN Binding Request => 487");

			// Reply 487 (Role Conflict).
			RTC::StunPacket* response = request->CreateErrorResponse(487);

			response->Serialize(StunSerializeBuffer);
			this->listener->OnIceServerSendStunPacket(this, response, tuple);

			delete response;

			return;
		}

		MS_DEBUG_DEV(
		  "valid STUN Binding Request [priority:%" PRIu32 ", useCandidate:%s]",
		  static_cast<uint32_t>(request->GetPriority()),
		  request->HasUseCandidate() ? "true" : "false");

		// Create a success response.
		RTC::StunPacket* response = request->CreateSuccessResponse();

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

		if (request->HasNomination())
		{
			nomination = request->GetNomination();
		}

		// Handle the tuple.
		HandleTuple(tuple, request->HasUseCandidate(), request->HasNomination(), nomination);
	}

	void IceServer::ProcessStunIndication(RTC::StunPacket* indication)
	{
		MS_TRACE();

		MS_DEBUG_DEV("processing STUN Indication");

		// Must be a Binding method.
		if (indication->GetMethod() != RTC::StunPacket::Method::BINDING)
		{
			MS_WARN_TAG(
			  ice,
			  "STUN Indication with unknown method %#.3x, discarded",
			  static_cast<unsigned int>(indication->GetMethod()));

			return;
		}

		// Nothig else to do. We just discard STUN Binding Indications.
	}

	void IceServer::ProcessStunResponse(RTC::StunPacket* response)
	{
		MS_TRACE();

		const std::string responseType = response->GetClass() == RTC::StunPacket::Class::SUCCESS_RESPONSE
		                                   ? "Success"
		                                   : std::to_string(response->GetErrorCode()) + " Error";

		MS_DEBUG_DEV("processing STUN %s Response", responseType.c_str());

		// Must be a Binding method.
		if (response->GetMethod() != RTC::StunPacket::Method::BINDING)
		{
			MS_WARN_TAG(
			  ice,
			  "STUN %s Response with unknown method %#.3x, discarded",
			  responseType.c_str(),
			  static_cast<unsigned int>(response->GetMethod()));

			return;
		}

		// Must use FINGERPRINT (optional for ICE STUN indications).
		if (!response->HasFingerprint())
		{
			MS_WARN_TAG(
			  ice,
			  "STUN Binding %s Response without FINGERPRINT attribute, discarded",
			  responseType.c_str());

			return;
		}

		// Check authentication.
		switch (response->CheckAuthentication(
		  this->remoteUsernameFragment, this->usernameFragment, this->remotePassword))
		{
			case RTC::StunPacket::Authentication::OK:
			{
				break;
			}

			case RTC::StunPacket::Authentication::UNAUTHORIZED:
			{
				MS_WARN_TAG(
				  ice, "STUN %s Response with wrong authentication, discarded", responseType.c_str());

				return;
			}

			case RTC::StunPacket::Authentication::BAD_MESSAGE:
			{
				MS_WARN_TAG(
				  ice,
				  "cannot check authentication in STUN Binding %s Response, discarded",
				  responseType.c_str());

				return;
			}
		}

		if (!IsConsentCheckSupported())
		{
			MS_WARN_DEV(
			  "ignoring STUN Binding %s Response because ICE consent check is not supported",
			  responseType.c_str());

			return;
		}
		// Ignore if right now ICE consent check is not running since it means that
		// ICE is no longer established.
		else if (!IsConsentCheckRunning())
		{
			MS_DEBUG_DEV(
			  "ignoring STUN Binding %s Response because ICE consent check is not running",
			  responseType.c_str());

			return;
		}

		// Trick: We only read the fist 4 bytes of the transactionId of the
		// response since we know that we generated 4 bytes followed by 8 zeroed
		// bytes in the transactionId of the sent consent Request.
		auto transactionId = Utils::Byte::Get4Bytes(response->GetTransactionId(), 0);

		// Let's iterate all entries in the queue and remove the consent Request
		// associated to this response and all those that were sent before.

		// Find the associated entry first.
		auto it = std::find_if(
		  this->sentConsents.begin(),
		  this->sentConsents.end(),
		  [transactionId](auto& sentConsent)
		  { return Utils::Byte::Get4Bytes(sentConsent.transactionId, 0) == transactionId; });

		if (it != this->sentConsents.end())
		{
			// If a success response or any non 403 error, let's behave the same way
			// by deleting the matching sent consent Request and all previous ones.
			// This way, if a strict ICE endpoint replies 400 to our consent Requests
			// (because indeed mediasoup as ICE Lite server should not send requests)
			// we know that the endpoint is alive, which is what ICE consent mechanism
			// is about anyway.
			if (response->GetErrorCode() != 403)
			{
				this->sentConsents.erase(this->sentConsents.begin(), it + 1);
			}
			// 403 means that the endpoint has revoked the consent so we should
			// disconnect ICE.
			else
			{
				MS_WARN_TAG(
				  ice,
				  "ICE consent revoked by the endpoint by means of a 403 response to our ICE consent Request, moving to 'disconnected' state");

				ConsentTerminated();
			}
		}
		else
		{
			MS_WARN_TAG(
			  ice,
			  "STUN %s Response doesn't match any sent consent Request, discarded",
			  responseType.c_str());
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

		if (isNewSelectedTuple)
		{
			MayStartOrRestartConsentCheck();
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

					MayStartConsentCheck();
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

						MayStartConsentCheck();
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

					MayStartConsentCheck();
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

						MayStartConsentCheck();
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

						if (isNewSelectedTuple)
						{
							MayStartOrRestartConsentCheck();
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

						if (isNewSelectedTuple)
						{
							MayStartOrRestartConsentCheck();
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
			MS_DEBUG_DEV("tuple already exists");

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

	void IceServer::MayStartConsentCheck()
	{
		MS_TRACE();

		if (!IsConsentCheckSupported())
		{
			return;
		}
		else if (IsConsentCheckRunning())
		{
			return;
		}

		// Create the ICE consent check timer if it doesn't exist.
		if (!this->consentCheckTimer)
		{
			this->consentCheckTimer = new TimerHandle(this);
		}

		this->consentCheckTimer->Start(ConsentCheckIntervalMs);
	}

	void IceServer::MayStopConsentCheck()
	{
		MS_TRACE();

		if (this->consentCheckTimer)
		{
			this->consentCheckTimer->Stop();
		}

		// Clear queue of ongoing sent ICE consent Requests.
		this->sentConsents.clear();
	}

	void IceServer::MayRestartConsentCheck()
	{
		MS_TRACE();

		if (!IsConsentCheckRunning())
		{
			return;
		}

		// Clear queue of ongoing sent ICE consent Requests.
		this->sentConsents.clear();

		MayStartConsentCheck();
	}

	void IceServer::MayStartOrRestartConsentCheck()
	{
		MS_TRACE();

		if (!IsConsentCheckRunning())
		{
			MayStartConsentCheck();
		}
		else
		{
			MayRestartConsentCheck();
		}
	}

	void IceServer::SendConsentRequest()
	{
		MS_TRACE();

		MS_ASSERT(
		  this->state == IceState::CONNECTED || this->state == IceState::COMPLETED,
		  "cannot send ICE consent Request in state other than 'connected' or 'completed'");

		MS_ASSERT(this->selectedTuple, "cannot send ICE consent Request without a selected tuple");

		// Here we do a trick. We generate a transactionId of 4 bytes and fill the
		// latest 8 bytes of the transactionId with zeroes.
		uint32_t transactionId = Utils::Crypto::GetRandomUInt(0u, UINT32_MAX);

		auto& sentContext = this->sentConsents.emplace_back(transactionId, DepLibUV::GetTimeMs());

		// Limit max number of entries in the queue.
		if (this->sentConsents.size() > MaxOngoingSentConsents)
		{
			MS_WARN_TAG(ice, "too many entries in the sent consents queue, removing the oldest one");

			this->sentConsents.pop_front();
		}

		auto* request = new StunPacket(
		  StunPacket::Class::REQUEST, StunPacket::Method::BINDING, sentContext.transactionId, nullptr, 0);

		const std::string username = this->remoteUsernameFragment + ":" + this->usernameFragment;

		request->SetUsername(username.c_str(), username.length());
		request->SetPriority(1u);
		request->SetIceControlled(1u);
		request->SetSoftware(SoftwareAttribute.c_str(), SoftwareAttribute.length());
		request->Authenticate(this->remotePassword);
		request->Serialize(StunSerializeBuffer);

		MS_DEBUG_DEV("sending ICE consent Request");

		this->listener->OnIceServerSendStunPacket(this, request, this->selectedTuple);

		delete request;
	}

	void IceServer::ConsentTerminated()
	{
		MS_TRACE();

		// There should be a selected tuple.
		MS_ASSERT(this->selectedTuple, "ICE consent terminated but there is not selected tuple");

		auto* disconnectedSelectedTuple = this->selectedTuple;

		// Update state.
		this->state = IceState::DISCONNECTED;

		// Clear all tuples.
		this->selectedTuple = nullptr;
		this->tuples.clear();

		// Reset remote nomination.
		this->remoteNomination = 0u;

		// Stop ICE consent check.
		MayStopConsentCheck();

		// Notify the listener.
		this->listener->OnIceServerTupleRemoved(this, disconnectedSelectedTuple);
		this->listener->OnIceServerDisconnected(this);
	}

	inline void IceServer::OnTimer(TimerHandle* timer)
	{
		MS_TRACE();

		if (timer == this->consentCheckTimer)
		{
			MS_ASSERT(IsConsentCheckSupported(), "ICE consent check not supported");

			// State must be 'connected' or 'completed'.
			MS_ASSERT(
			  this->state == IceState::COMPLETED || this->state == IceState::CONNECTED,
			  "ICE consent check timer fired but state is neither 'completed' nor 'connected'");

			// There should be a selected tuple.
			MS_ASSERT(this->selectedTuple, "ICE consent check timer fired but there is not selected tuple");

			// Check if there is at least an ongoing expired ICE Consent Request
			// and if so move to disconnected state.

			auto nowMs            = DepLibUV::GetTimeMs();
			auto consentTimeoutMs = this->consentTimeoutMs;

			auto it = std::find_if(
			  this->sentConsents.begin(),
			  this->sentConsents.end(),
			  [nowMs, consentTimeoutMs](auto& sentConsent)
			  { return nowMs - sentConsent.sentAtMs >= consentTimeoutMs; });

			if (it == this->sentConsents.end())
			{
				// Send a new ICE Consent Request.
				SendConsentRequest();

				/*
				 * The interval between ICE Consent Requests is varied randomly over the
				 * range [0.8, 1.2] times the calculated interval to prevent
				 * synchronization of consent checks.
				 */
				const uint64_t interval =
				  ConsentCheckIntervalMs + static_cast<float>(Utils::Crypto::GetRandomUInt(8, 12)) / 10;

				this->consentCheckTimer->Start(interval);
			}
			else
			{
				MS_WARN_TAG(ice, "ICE consent expired due to timeout, moving to 'disconnected' state");

				ConsentTerminated();
			}
		}
	}
} // namespace RTC
