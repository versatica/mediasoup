use crate::data_producer::DataProducerId;
use crate::data_structures::{AppData, WebRtcMessage};
use crate::messages::{
    DataConsumerCloseRequest, DataConsumerDumpRequest, DataConsumerGetBufferedAmountRequest,
    DataConsumerGetStatsRequest, DataConsumerInternal, DataConsumerSendRequest,
    DataConsumerSendRequestData, DataConsumerSetBufferedAmountLowThresholdData,
    DataConsumerSetBufferedAmountLowThresholdRequest,
};
use crate::sctp_parameters::SctpStreamParameters;
use crate::transport::{Transport, TransportGeneric};
use crate::uuid_based_wrapper_type;
use crate::worker::{
    Channel, NotificationMessage, PayloadChannel, RequestError, SubscriptionHandler,
};
use async_executor::Executor;
use event_listener_primitives::{Bag, HandlerId};
use log::*;
use parking_lot::Mutex as SyncMutex;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use std::fmt::Debug;
use std::sync::Arc;

uuid_based_wrapper_type!(DataConsumerId);

#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct DataConsumerOptions {
    // The id of the DataProducer to consume.
    pub(super) data_producer_id: DataProducerId,
    /// Just if consuming over SCTP.
    /// Whether data messages must be received in order. If true the messages will be sent reliably.
    /// Defaults to the value in the DataProducer if it has type 'Sctp' or to true if it has type
    /// 'Direct'.
    pub(super) ordered: Option<bool>,
    /// Just if consuming over SCTP.
    /// When ordered is false indicates the time (in milliseconds) after which a SCTP packet will
    /// stop being retransmitted.
    /// Defaults to the value in the DataProducer if it has type 'Sctp' or unset if it has type
    /// 'Direct'.
    pub(super) max_packet_life_time: Option<u16>,
    /// Just if consuming over SCTP.
    /// When ordered is false indicates the maximum number of times a packet will be retransmitted.
    /// Defaults to the value in the DataProducer if it has type 'Sctp' or unset if it has type
    /// 'Direct'.
    pub(super) max_retransmits: Option<u16>,
    /// Custom application data.
    pub app_data: AppData,
}

impl DataConsumerOptions {
    /// Inherits parameters of corresponding DataProducer.
    pub fn new_sctp(data_producer_id: DataProducerId) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: None,
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// For DirectTransport.
    pub fn new_direct(data_producer_id: DataProducerId) -> Self {
        Self {
            data_producer_id,
            ordered: Some(true),
            max_packet_life_time: None,
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// Messages will be sent reliably in order.
    pub fn new_sctp_ordered(data_producer_id: DataProducerId) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: None,
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// Messages will be sent unreliably with time (in milliseconds) after which a SCTP packet will
    /// stop being retransmitted.
    pub fn new_sctp_unordered_with_life_time(
        data_producer_id: DataProducerId,
        max_packet_life_time: u16,
    ) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: Some(max_packet_life_time),
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// Messages will be sent unreliably with a limited number of retransmission attempts.
    pub fn new_sctp_unordered_with_retransmits(
        data_producer_id: DataProducerId,
        max_retransmits: u16,
    ) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: None,
            max_retransmits: Some(max_retransmits),
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
#[non_exhaustive]
pub struct DataConsumerDump {
    pub id: DataConsumerId,
    pub data_producer_id: DataProducerId,
    pub r#type: DataConsumerType,
    pub label: String,
    pub protocol: String,
    pub sctp_stream_parameters: Option<SctpStreamParameters>,
    pub buffered_amount: u32,
    pub buffered_amount_low_threshold: u32,
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
pub struct DataConsumerStat {
    // `type` field is present in worker, but ignored here
    pub timestamp: u64,
    pub label: String,
    pub protocol: String,
    pub messages_sent: usize,
    pub bytes_sent: usize,
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum DataConsumerType {
    Sctp,
    Direct,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    DataProducerClose,
    SctpSendBufferFull,
    #[serde(rename_all = "camelCase")]
    BufferedAmountLow {
        buffered_amount: u32,
    },
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum PayloadNotification {
    Message { ppid: u32 },
}

#[derive(Default)]
struct Handlers {
    message: Bag<'static, dyn Fn(&WebRtcMessage) + Send>,
    sctp_send_buffer_full: Bag<'static, dyn Fn() + Send>,
    buffered_amount_low: Bag<'static, dyn Fn(u32) + Send>,
    data_producer_close: Bag<'static, dyn FnOnce() + Send>,
    transport_close: Bag<'static, dyn FnOnce() + Send>,
    close: Bag<'static, dyn FnOnce() + Send>,
}

struct Inner {
    id: DataConsumerId,
    r#type: DataConsumerType,
    sctp_stream_parameters: Option<SctpStreamParameters>,
    label: String,
    protocol: String,
    data_producer_id: DataProducerId,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Option<Box<dyn Transport>>,
    // Drop subscription to consumer-specific notifications when consumer itself is dropped
    _subscription_handlers: Vec<SubscriptionHandler>,
    _on_transport_close_handler: SyncMutex<HandlerId<'static>>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.handlers.close.call_once_simple();

        {
            let channel = self.channel.clone();
            let request = DataConsumerCloseRequest {
                internal: DataConsumerInternal {
                    router_id: self.transport.as_ref().unwrap().router_id(),
                    transport_id: self.transport.as_ref().unwrap().id(),
                    data_consumer_id: self.id,
                    data_producer_id: self.data_producer_id,
                },
            };
            let transport = self.transport.take();
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("consumer closing failed on drop: {}", error);
                    }

                    drop(transport);
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct RegularDataConsumer {
    inner: Arc<Inner>,
}

impl From<RegularDataConsumer> for DataConsumer {
    fn from(producer: RegularDataConsumer) -> Self {
        DataConsumer::Regular(producer)
    }
}

#[derive(Clone)]
pub struct DirectDataConsumer {
    inner: Arc<Inner>,
}

impl From<DirectDataConsumer> for DataConsumer {
    fn from(producer: DirectDataConsumer) -> Self {
        DataConsumer::Direct(producer)
    }
}

#[derive(Clone)]
#[non_exhaustive]
pub enum DataConsumer {
    Regular(RegularDataConsumer),
    Direct(DirectDataConsumer),
}

impl DataConsumer {
    #[allow(clippy::too_many_arguments)]
    pub(super) async fn new<Dump, Stat, Transport>(
        id: DataConsumerId,
        r#type: DataConsumerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
        data_producer_id: DataProducerId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        app_data: AppData,
        transport: Transport,
        direct: bool,
    ) -> Self
    where
        Dump: Debug + DeserializeOwned + 'static,
        Stat: Debug + DeserializeOwned + 'static,
        Transport: TransportGeneric<Dump, Stat> + 'static,
    {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    match serde_json::from_value::<Notification>(notification) {
                        Ok(notification) => match notification {
                            Notification::DataProducerClose => {
                                handlers.data_producer_close.call_once_simple();
                            }
                            Notification::SctpSendBufferFull => {
                                handlers.sctp_send_buffer_full.call_simple();
                            }
                            Notification::BufferedAmountLow { buffered_amount } => {
                                handlers.buffered_amount_low.call(|callback| {
                                    callback(buffered_amount);
                                });
                            }
                        },
                        Err(error) => {
                            error!("Failed to parse notification: {}", error);
                        }
                    }
                })
                .await
        };

        let payload_subscription_handler = {
            let handlers = Arc::clone(&handlers);

            payload_channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    let NotificationMessage { message, payload } = notification;
                    match serde_json::from_value::<PayloadNotification>(message) {
                        Ok(notification) => match notification {
                            PayloadNotification::Message { ppid } => {
                                let message = WebRtcMessage::new(ppid, payload);

                                handlers.message.call(|callback| {
                                    callback(&message);
                                });
                            }
                        },
                        Err(error) => {
                            error!("Failed to parse payload notification: {}", error);
                        }
                    }
                })
                .await
        };

        let on_transport_close_handler = transport.on_close({
            let handlers = Arc::clone(&handlers);

            move || {
                handlers.transport_close.call_once_simple();
                handlers.close.call_once_simple();
            }
        });
        let inner = Arc::new(Inner {
            id,
            r#type,
            sctp_stream_parameters,
            label,
            protocol,
            data_producer_id,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport: Some(Box::new(transport)),
            _subscription_handlers: vec![subscription_handler, payload_subscription_handler],
            _on_transport_close_handler: SyncMutex::new(on_transport_close_handler),
        });

        if direct {
            Self::Direct(DirectDataConsumer { inner })
        } else {
            Self::Regular(RegularDataConsumer { inner })
        }
    }

    /// DataConsumer id.
    pub fn id(&self) -> DataConsumerId {
        self.inner().id
    }

    /// Associated DataProducer id.
    pub fn data_producer_id(&self) -> DataProducerId {
        self.inner().data_producer_id
    }

    /// DataConsumer type.
    pub fn r#type(&self) -> DataConsumerType {
        self.inner().r#type
    }

    /// SCTP stream parameters.
    pub fn sctp_stream_parameters(&self) -> Option<SctpStreamParameters> {
        self.inner().sctp_stream_parameters
    }

    /// DataChannel label.
    pub fn label(&self) -> &String {
        &self.inner().label
    }

    /// DataChannel protocol.
    pub fn protocol(&self) -> &String {
        &self.inner().protocol
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner().app_data
    }

    /// Dump DataConsumer.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<DataConsumerDump, RequestError> {
        debug!("dump()");

        self.inner()
            .channel
            .request(DataConsumerDumpRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Get DataConsumer stats.
    pub async fn get_stats(&self) -> Result<Vec<DataConsumerStat>, RequestError> {
        debug!("get_stats()");

        self.inner()
            .channel
            .request(DataConsumerGetStatsRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Get buffered amount size.
    pub async fn get_buffered_amount(&self) -> Result<u32, RequestError> {
        debug!("get_buffered_amount()");

        let response = self
            .inner()
            .channel
            .request(DataConsumerGetBufferedAmountRequest {
                internal: self.get_internal(),
            })
            .await?;

        Ok(response.buffered_amount)
    }

    /// Set buffered amount low threshold.
    pub async fn set_buffered_amount_low_threshold(
        &self,
        threshold: u32,
    ) -> Result<(), RequestError> {
        debug!(
            "set_buffered_amount_low_threshold() [threshold:{}]",
            threshold
        );

        self.inner()
            .channel
            .request(DataConsumerSetBufferedAmountLowThresholdRequest {
                internal: self.get_internal(),
                data: DataConsumerSetBufferedAmountLowThresholdData { threshold },
            })
            .await
    }

    pub fn on_message<F: Fn(&WebRtcMessage) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner().handlers.message.add(Box::new(callback))
    }

    pub fn on_sctp_send_buffer_full<F: Fn() + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner()
            .handlers
            .sctp_send_buffer_full
            .add(Box::new(callback))
    }

    pub fn on_buffered_amount_low<F: Fn(u32) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner()
            .handlers
            .buffered_amount_low
            .add(Box::new(callback))
    }

    pub fn on_data_producer_close<F: FnOnce() + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner()
            .handlers
            .data_producer_close
            .add(Box::new(callback))
    }

    pub fn on_transport_close<F: FnOnce() + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner()
            .handlers
            .transport_close
            .add(Box::new(callback))
    }

    pub fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner().handlers.close.add(Box::new(callback))
    }

    fn inner(&self) -> &Arc<Inner> {
        match self {
            DataConsumer::Regular(data_consumer) => &data_consumer.inner,
            DataConsumer::Direct(data_consumer) => &data_consumer.inner,
        }
    }

    fn get_internal(&self) -> DataConsumerInternal {
        DataConsumerInternal {
            router_id: self.inner().transport.as_ref().unwrap().router_id(),
            transport_id: self.inner().transport.as_ref().unwrap().id(),
            data_consumer_id: self.inner().id,
            data_producer_id: self.inner().data_producer_id,
        }
    }
}

impl DirectDataConsumer {
    /// Send data.
    pub async fn send(&self, message: WebRtcMessage) -> Result<(), RequestError> {
        let (ppid, payload) = message.into_ppid_and_payload();

        self.inner
            .payload_channel
            .request(
                DataConsumerSendRequest {
                    internal: DataConsumerInternal {
                        router_id: self.inner.transport.as_ref().unwrap().router_id(),
                        transport_id: self.inner.transport.as_ref().unwrap().id(),
                        data_consumer_id: self.inner.id,
                        data_producer_id: self.inner.data_producer_id,
                    },
                    data: DataConsumerSendRequestData { ppid },
                },
                payload,
            )
            .await
    }
}
