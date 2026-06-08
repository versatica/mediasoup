#ifndef MS_RTC_DATA_CONSUMER_HPP
#define MS_RTC_DATA_CONSUMER_HPP

#include "common.hpp"
#include "Channel/ChannelRequest.hpp"
#include "Channel/ChannelSocket.hpp"
#include "RTC/SCTP/public/Message.hpp"
#include "RTC/SctpDictionaries.hpp"
#include "SharedInterface.hpp"
#include <ankerl/unordered_dense.h>
#include <string>

namespace RTC
{
	class DataConsumer : public Channel::ChannelSocket::RequestHandler
	{
	protected:
		using onQueuedCallback = const std::function<void(bool queued, bool sctpSendBufferFull)>;

	public:
		class Listener
		{
		public:
			virtual ~Listener() = default;

		public:
			virtual void OnDataConsumerSendMessage(
			  RTC::DataConsumer* dataConsumer, RTC::SCTP::Message message, onQueuedCallback* cb) = 0;
			virtual void OnDataConsumerNeedBufferedAmount(
			  const RTC::DataConsumer* dataConsumer, uint32_t& bufferedAmount) const = 0;
			virtual void OnDataConsumerNeedBufferedAmountLowThreshold(
			  const RTC::DataConsumer* dataConsumer, uint32_t& bufferedAmountLowThreshold) const = 0;
			virtual void OnDataConsumerSetBufferedAmountLowThreshold(
			  const RTC::DataConsumer* dataConsumer, uint32_t bytes) const                 = 0;
			virtual void OnDataConsumerDataProducerClosed(RTC::DataConsumer* dataConsumer) = 0;
		};

	public:
		enum class Type : uint8_t
		{
			SCTP = 0,
			DIRECT
		};

	public:
		DataConsumer(
		  SharedInterface* shared,
		  const std::string& id,
		  const std::string& dataProducerId,
		  RTC::DataConsumer::Listener* listener,
		  const FBS::Transport::ConsumeDataRequest* data,
		  size_t maxMessageSize);
		~DataConsumer() override;

	public:
		flatbuffers::Offset<FBS::DataConsumer::DumpResponse> FillBuffer(
		  flatbuffers::FlatBufferBuilder& builder) const;
		flatbuffers::Offset<FBS::DataConsumer::GetStatsResponse> FillBufferStats(
		  flatbuffers::FlatBufferBuilder& builder) const;
		Type GetType() const
		{
			return this->type;
		}
		const RTC::SctpStreamParameters& GetSctpStreamParameters() const
		{
			return this->sctpStreamParameters;
		}
		bool IsActive() const
		{
			// It's active it DataConsumer and DataProducer are not paused and the transport
			// is connected.
			// clang-format off
			return (
				this->transportConnected &&
				(this->type == DataConsumer::Type::DIRECT || this->sctpAssociationConnected) &&
				!this->paused &&
				!this->dataProducerPaused &&
				!this->dataProducerClosed
			);
			// clang-format on
		}
		void TransportConnected();
		void TransportDisconnected();
		bool IsPaused() const
		{
			return this->paused;
		}
		bool IsDataProducerPaused() const
		{
			return this->dataProducerPaused;
		}
		void DataProducerPaused();
		void DataProducerResumed();
		void SctpAssociationConnected();
		void SctpAssociationClosed();
		void SctpBufferedAmountLow(uint32_t bufferedAmount) const;
		void SctpSendBufferFull() const;
		void DataProducerClosed();
		bool SendMessage(
		  RTC::SCTP::Message message,
		  std::vector<uint16_t>& subchannels,
		  std::optional<uint16_t> requiredSubchannel,
		  const onQueuedCallback* cb = nullptr);

		/* Methods inherited from Channel::ChannelSocket::RequestHandler. */
	public:
		void HandleRequest(Channel::ChannelRequest* request) override;

	public:
		// Passed by argument.
		std::string id;
		std::string dataProducerId;

	private:
		// Passed by argument.
		SharedInterface* shared{ nullptr };
		RTC::DataConsumer::Listener* listener{ nullptr };
		size_t maxMessageSize{ 0u };
		// Others.
		Type type;
		RTC::SctpStreamParameters sctpStreamParameters;
		std::string label;
		std::string protocol;
		ankerl::unordered_dense::set<uint16_t> subchannels;
		bool transportConnected{ false };
		bool sctpAssociationConnected{ false };
		bool paused{ false };
		bool dataProducerPaused{ false };
		bool dataProducerClosed{ false };
		size_t messagesSent{ 0u };
		size_t bytesSent{ 0u };
	};
} // namespace RTC

#endif
