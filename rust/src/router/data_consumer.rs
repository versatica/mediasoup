#[cfg(test)]
mod tests;

use crate::data_producer::{DataProducer, DataProducerId, WeakDataProducer};
use crate::data_structures::{AppData, WebRtcMessage};
use crate::messages::{
    DataConsumerCloseRequest, DataConsumerDumpRequest, DataConsumerGetBufferedAmountRequest,
    DataConsumerGetStatsRequest, DataConsumerInternal, DataConsumerSendRequest,
    DataConsumerSendRequestData, DataConsumerSetBufferedAmountLowThresholdData,
    DataConsumerSetBufferedAmountLowThresholdRequest,
};
use crate::sctp_parameters::SctpStreamParameters;
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::{Channel, PayloadChannel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use event_listener_primitives::{Bag, BagOnce, HandlerId};
use log::{debug, error};
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use std::borrow::Cow;
use std::fmt;
use std::fmt::Debug;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Arc, Weak};

uuid_based_wrapper_type!(
    /// [`DataConsumer`] identifier.
    DataConsumerId
);

/// [`DataConsumer`] options.
#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct DataConsumerOptions {
    // The id of the [`DataProducer`](crate::data_producer::DataProducer) to consume.
    pub(super) data_producer_id: DataProducerId,
    /// Just if consuming over SCTP.
    /// Whether data messages must be received in order. If true the messages will be sent reliably.
    /// Defaults to the value in the DataProducer if it has type `Sctp` or to true if it has type
    /// `Direct`.
    pub(super) ordered: Option<bool>,
    /// Just if consuming over SCTP.
    /// When ordered is false indicates the time (in milliseconds) after which a SCTP packet will
    /// stop being retransmitted.
    /// Defaults to the value in the DataProducer if it has type `Sctp` or unset if it has type
    /// `Direct`.
    pub(super) max_packet_life_time: Option<u16>,
    /// Just if consuming over SCTP.
    /// When ordered is false indicates the maximum number of times a packet will be retransmitted.
    /// Defaults to the value in the [`DataProducer`](crate::data_producer::DataProducer) if it
    /// has type `Sctp` or unset if it has type `Direct`.
    pub(super) max_retransmits: Option<u16>,
    /// Custom application data.
    pub app_data: AppData,
}

impl DataConsumerOptions {
    /// Inherits parameters of corresponding
    /// [`DataProducer`](crate::data_producer::DataProducer).
    #[must_use]
    pub fn new_sctp(data_producer_id: DataProducerId) -> Self {
        Self {
            data_producer_id,
            ordered: None,
            max_packet_life_time: None,
            max_retransmits: None,
            app_data: AppData::default(),
        }
    }

    /// For [`DirectTransport`](crate::direct_transport::DirectTransport).
    #[must_use]
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
    #[must_use]
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
    #[must_use]
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
    #[must_use]
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
    pub buffered_amount_low_threshold: u32,
}

/// RTC statistics of the data consumer.
#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
#[allow(missing_docs)]
pub struct DataConsumerStat {
    // `type` field is present in worker, but ignored here
    pub timestamp: u64,
    pub label: String,
    pub protocol: String,
    pub messages_sent: usize,
    pub bytes_sent: usize,
    pub buffered_amount: u32,
}

/// Data consumer type.
#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum DataConsumerType {
    /// The endpoint receives messages using the SCTP protocol.
    Sctp,
    /// Messages are received directly by the Rust process over a direct transport.
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
    message: Bag<Arc<dyn Fn(&WebRtcMessage<'_>) + Send + Sync>>,
    sctp_send_buffer_full: Bag<Arc<dyn Fn() + Send + Sync>>,
    buffered_amount_low: Bag<Arc<dyn Fn(u32) + Send + Sync>>,
    data_producer_close: BagOnce<Box<dyn FnOnce() + Send>>,
    transport_close: BagOnce<Box<dyn FnOnce() + Send>>,
    close: BagOnce<Box<dyn FnOnce() + Send>>,
}

struct Inner {
    id: DataConsumerId,
    r#type: DataConsumerType,
    sctp_stream_parameters: Option<SctpStreamParameters>,
    label: String,
    protocol: String,
    data_producer_id: DataProducerId,
    direct: bool,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Arc<dyn Transport>,
    weak_data_producer: WeakDataProducer,
    closed: Arc<AtomicBool>,
    // Drop subscription to data consumer-specific notifications when data consumer itself is
    // dropped
    _subscription_handlers: Mutex<Vec<Option<SubscriptionHandler>>>,
    _on_transport_close_handler: Mutex<HandlerId>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.close(true);
    }
}

impl Inner {
    fn close(&self, close_request: bool) {
        if !self.closed.swap(true, Ordering::SeqCst) {
            debug!("close()");

            self.handlers.close.call_simple();

            if close_request {
                let channel = self.channel.clone();
                let request = DataConsumerCloseRequest {
                    internal: DataConsumerInternal {
                        router_id: self.transport.router().id(),
                        transport_id: self.transport.id(),
                        data_consumer_id: self.id,
                    },
                };
                let weak_data_producer = self.weak_data_producer.clone();

                self.executor
                    .spawn(async move {
                        if weak_data_producer.upgrade().is_some() {
                            if let Err(error) = channel.request(request).await {
                                error!("consumer closing failed on drop: {}", error);
                            }
                        }
                    })
                    .detach();
            }
        }
    }
}

/// Data consumer created on transport other than
/// [`DirectTransport`](crate::direct_transport::DirectTransport).
#[derive(Clone)]
#[must_use = "Data consumer will be closed on drop, make sure to keep it around for as long as needed"]
pub struct RegularDataConsumer {
    inner: Arc<Inner>,
}

impl fmt::Debug for RegularDataConsumer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("RegularDataConsumer")
            .field("id", &self.inner.id)
            .field("type", &self.inner.r#type)
            .field("sctp_stream_parameters", &self.inner.sctp_stream_parameters)
            .field("label", &self.inner.label)
            .field("protocol", &self.inner.protocol)
            .field("data_producer_id", &self.inner.data_producer_id)
            .field("transport", &self.inner.transport)
            .field("closed", &self.inner.closed)
            .finish()
    }
}

impl From<RegularDataConsumer> for DataConsumer {
    fn from(producer: RegularDataConsumer) -> Self {
        DataConsumer::Regular(producer)
    }
}

/// Data consumer created on [`DirectTransport`](crate::direct_transport::DirectTransport).
#[derive(Clone)]
#[must_use = "Data consumer will be closed on drop, make sure to keep it around for as long as needed"]
pub struct DirectDataConsumer {
    inner: Arc<Inner>,
}

impl fmt::Debug for DirectDataConsumer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DirectDataConsumer")
            .field("id", &self.inner.id)
            .field("type", &self.inner.r#type)
            .field("sctp_stream_parameters", &self.inner.sctp_stream_parameters)
            .field("label", &self.inner.label)
            .field("protocol", &self.inner.protocol)
            .field("data_producer_id", &self.inner.data_producer_id)
            .field("transport", &self.inner.transport)
            .field("closed", &self.inner.closed)
            .finish()
    }
}

impl From<DirectDataConsumer> for DataConsumer {
    fn from(producer: DirectDataConsumer) -> Self {
        DataConsumer::Direct(producer)
    }
}

/// A data consumer represents an endpoint capable of receiving data messages from a mediasoup
/// [`Router`](crate::router::Router).
///
/// A data consumer can use [SCTP](https://tools.ietf.org/html/rfc4960) (AKA
/// DataChannel) to receive those messages, or can directly receive them in the Rust application if
/// the data consumer was created on top of a
/// [`DirectTransport`](crate::direct_transport::DirectTransport).
#[derive(Clone)]
#[non_exhaustive]
#[must_use = "Data consumer will be closed on drop, make sure to keep it around for as long as needed"]
pub enum DataConsumer {
    /// Data consumer created on transport other than
    /// [`DirectTransport`](crate::direct_transport::DirectTransport).
    Regular(RegularDataConsumer),
    /// Data consumer created on [`DirectTransport`](crate::direct_transport::DirectTransport).
    Direct(DirectDataConsumer),
}

impl fmt::Debug for DataConsumer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match &self {
            DataConsumer::Regular(producer) => f.debug_tuple("Regular").field(&producer).finish(),
            DataConsumer::Direct(producer) => f.debug_tuple("Direct").field(&producer).finish(),
        }
    }
}

impl DataConsumer {
    #[allow(clippy::too_many_arguments)]
    pub(super) fn new(
        id: DataConsumerId,
        r#type: DataConsumerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
        data_producer: DataProducer,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        app_data: AppData,
        transport: Arc<dyn Transport>,
        direct: bool,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let closed = Arc::new(AtomicBool::new(false));

        let inner_weak = Arc::<Mutex<Option<Weak<Inner>>>>::default();
        let subscription_handler = {
            let handlers = Arc::clone(&handlers);
            let closed = Arc::clone(&closed);
            let inner_weak = Arc::clone(&inner_weak);

            channel.subscribe_to_notifications(id.into(), move |notification| {
                match serde_json::from_slice::<Notification>(notification) {
                    Ok(notification) => match notification {
                        Notification::DataProducerClose => {
                            if !closed.load(Ordering::SeqCst) {
                                handlers.data_producer_close.call_simple();

                                let maybe_inner =
                                    inner_weak.lock().as_ref().and_then(Weak::upgrade);
                                if let Some(inner) = maybe_inner {
                                    inner
                                        .executor
                                        .clone()
                                        .spawn(async move {
                                            // Potential drop needs to happen from a different
                                            // thread to prevent potential deadlock
                                            inner.close(false);
                                        })
                                        .detach();
                                }
                            }
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
        };

        let payload_subscription_handler = {
            let handlers = Arc::clone(&handlers);

            payload_channel.subscribe_to_notifications(id.into(), move |message, payload| {
                match serde_json::from_slice::<PayloadNotification>(message) {
                    Ok(notification) => match notification {
                        PayloadNotification::Message { ppid } => {
                            match WebRtcMessage::new(ppid, Cow::from(payload)) {
                                Ok(message) => {
                                    handlers.message.call(|callback| {
                                        callback(&message);
                                    });
                                }
                                Err(ppid) => {
                                    error!("Bad ppid {}", ppid);
                                }
                            }
                        }
                    },
                    Err(error) => {
                        error!("Failed to parse payload notification: {}", error);
                    }
                }
            })
        };

        let on_transport_close_handler = transport.on_close({
            let inner_weak = Arc::clone(&inner_weak);

            Box::new(move || {
                let maybe_inner = inner_weak.lock().as_ref().and_then(Weak::upgrade);
                if let Some(inner) = maybe_inner {
                    inner.handlers.transport_close.call_simple();
                    inner.close(false);
                }
            })
        });
        let inner = Arc::new(Inner {
            id,
            r#type,
            sctp_stream_parameters,
            label,
            protocol,
            data_producer_id: data_producer.id(),
            direct,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport,
            weak_data_producer: data_producer.downgrade(),
            closed,
            _subscription_handlers: Mutex::new(vec![
                subscription_handler,
                payload_subscription_handler,
            ]),
            _on_transport_close_handler: Mutex::new(on_transport_close_handler),
        });

        inner_weak.lock().replace(Arc::downgrade(&inner));

        if direct {
            Self::Direct(DirectDataConsumer { inner })
        } else {
            Self::Regular(RegularDataConsumer { inner })
        }
    }

    /// Data consumer identifier.
    #[must_use]
    pub fn id(&self) -> DataConsumerId {
        self.inner().id
    }

    /// The associated data producer identifier.
    #[must_use]
    pub fn data_producer_id(&self) -> DataProducerId {
        self.inner().data_producer_id
    }

    /// Transport to which data consumer belongs.
    pub fn transport(&self) -> &Arc<dyn Transport> {
        &self.inner().transport
    }

    /// The type of the data consumer.
    #[must_use]
    pub fn r#type(&self) -> DataConsumerType {
        self.inner().r#type
    }

    /// The SCTP stream parameters (just if the data consumer type is `Sctp`).
    #[must_use]
    pub fn sctp_stream_parameters(&self) -> Option<SctpStreamParameters> {
        self.inner().sctp_stream_parameters
    }

    /// The data consumer label.
    #[must_use]
    pub fn label(&self) -> &String {
        &self.inner().label
    }

    /// The data consumer sub-protocol.
    #[must_use]
    pub fn protocol(&self) -> &String {
        &self.inner().protocol
    }

    /// Custom application data.
    #[must_use]
    pub fn app_data(&self) -> &AppData {
        &self.inner().app_data
    }

    /// Whether the data consumer is closed.
    #[must_use]
    pub fn closed(&self) -> bool {
        self.inner().closed.load(Ordering::SeqCst)
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

    /// Returns current statistics of the data consumer.
    ///
    /// Check the [RTC Statistics](https://mediasoup.org/documentation/v3/mediasoup/rtc-statistics/)
    /// section for more details (TypeScript-oriented, but concepts apply here as well).
    pub async fn get_stats(&self) -> Result<Vec<DataConsumerStat>, RequestError> {
        debug!("get_stats()");

        self.inner()
            .channel
            .request(DataConsumerGetStatsRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Returns the number of bytes of data currently buffered to be sent over the underlying SCTP
    /// association.
    ///
    /// # Notes on usage
    /// The underlying SCTP association uses a common send buffer for all data consumers, hence the
    /// value given by this method indicates the data buffered for all data consumers in the
    /// transport.
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

    /// Whenever the underlying SCTP association buffered bytes drop to this value,
    /// `on_buffered_amount_low` callback is called.
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

    /// Callback is called when a message has been received from the corresponding data producer.
    ///
    /// # Notes on usage
    /// Just available in direct transports, this is, those created via
    /// [`Router::create_direct_transport`](crate::router::Router::create_direct_transport).
    pub fn on_message<F: Fn(&WebRtcMessage<'_>) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner().handlers.message.add(Arc::new(callback))
    }

    /// Callback is called when a message could not be sent because the SCTP send buffer was full.
    pub fn on_sctp_send_buffer_full<F: Fn() + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner()
            .handlers
            .sctp_send_buffer_full
            .add(Arc::new(callback))
    }

    /// Emitted when the underlying SCTP association buffered bytes drop down to the value set with
    /// [`DataConsumer::set_buffered_amount_low_threshold`].
    ///
    /// # Notes on usage
    /// Only applicable for consumers of type `Sctp`.
    pub fn on_buffered_amount_low<F: Fn(u32) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner()
            .handlers
            .buffered_amount_low
            .add(Arc::new(callback))
    }

    /// Callback is called when the associated data producer is closed for whatever reason. The data
    /// consumer itself is also closed.
    pub fn on_data_producer_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner()
            .handlers
            .data_producer_close
            .add(Box::new(callback))
    }

    /// Callback is called when the transport this data consumer belongs to is closed for whatever
    /// reason. The data consumer itself is also closed.
    pub fn on_transport_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner()
            .handlers
            .transport_close
            .add(Box::new(callback))
    }

    /// Callback is called when the data consumer is closed for whatever reason.
    ///
    /// NOTE: Callback will be called in place if data consumer is already closed.
    pub fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        let handler_id = self.inner().handlers.close.add(Box::new(callback));
        if self.inner().closed.load(Ordering::Relaxed) {
            self.inner().handlers.close.call_simple();
        }
        handler_id
    }

    /// Downgrade `DataConsumer` to [`WeakDataConsumer`] instance.
    #[must_use]
    pub fn downgrade(&self) -> WeakDataConsumer {
        WeakDataConsumer {
            inner: Arc::downgrade(self.inner()),
        }
    }

    fn inner(&self) -> &Arc<Inner> {
        match self {
            DataConsumer::Regular(data_consumer) => &data_consumer.inner,
            DataConsumer::Direct(data_consumer) => &data_consumer.inner,
        }
    }

    fn get_internal(&self) -> DataConsumerInternal {
        DataConsumerInternal {
            router_id: self.inner().transport.router().id(),
            transport_id: self.inner().transport.id(),
            data_consumer_id: self.inner().id,
        }
    }
}

impl DirectDataConsumer {
    /// Sends direct messages from the Rust process.
    pub async fn send(&self, message: WebRtcMessage<'_>) -> Result<(), RequestError> {
        let (ppid, payload) = message.into_ppid_and_payload();

        self.inner
            .payload_channel
            .request(
                DataConsumerSendRequest {
                    internal: DataConsumerInternal {
                        router_id: self.inner.transport.router().id(),
                        transport_id: self.inner.transport.id(),
                        data_consumer_id: self.inner.id,
                    },
                    data: DataConsumerSendRequestData { ppid },
                },
                payload.into_owned(),
            )
            .await
    }
}

/// [`WeakDataConsumer`] doesn't own data consumer instance on mediasoup-worker and will not prevent
/// one from being destroyed once last instance of regular [`DataConsumer`] is dropped.
///
/// [`WeakDataConsumer`] vs [`DataConsumer`] is similar to [`Weak`] vs [`Arc`].
#[derive(Clone)]
pub struct WeakDataConsumer {
    inner: Weak<Inner>,
}

impl fmt::Debug for WeakDataConsumer {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("WeakDataConsumer").finish()
    }
}

impl WeakDataConsumer {
    /// Attempts to upgrade `WeakDataConsumer` to [`DataConsumer`] if last instance of one wasn't
    /// dropped yet.
    #[must_use]
    pub fn upgrade(&self) -> Option<DataConsumer> {
        let inner = self.inner.upgrade()?;

        let data_consumer = if inner.direct {
            DataConsumer::Direct(DirectDataConsumer { inner })
        } else {
            DataConsumer::Regular(RegularDataConsumer { inner })
        };

        Some(data_consumer)
    }
}
