//! A direct transport represents a direct connection between the mediasoup Rust process and a
//! [`Router`] instance in a mediasoup-worker thread.
//!
//! A direct transport can be used to directly send and receive data messages from/to Rust by means
//! of [`DataProducer`]s and [`DataConsumer`]s of type `Direct` created on a direct transport.
//! Direct messages sent by a [`DataProducer`] in a direct transport can be consumed by endpoints
//! connected through a SCTP capable transport ([`WebRtcTransport`](crate::webrtc_transport::WebRtcTransport),
//! [`PlainTransport`](crate::plain_transport::PlainTransport), [`PipeTransport`](crate::pipe_transport::PipeTransport)
//! and also by the Rust application by means of a [`DataConsumer`] created on a [`DirectTransport`]
//! (and vice-versa: messages sent over SCTP/DataChannel can be consumed by the Rust application by
//! means of a [`DataConsumer`] created on a [`DirectTransport`]).
//!
//! A direct transport can also be used to inject and directly consume RTP and RTCP packets in Rust
//! by using the [`DirectProducer::send`](crate::producer::DirectProducer::send) and
//! [`Consumer::on_rtp`] API (plus [`DirectTransport::send_rtcp`] and [`DirectTransport::on_rtcp`]
//! API).

#[cfg(test)]
mod tests;

use crate::consumer::{Consumer, ConsumerId, ConsumerOptions};
use crate::data_consumer::{DataConsumer, DataConsumerId, DataConsumerOptions, DataConsumerType};
use crate::data_producer::{DataProducer, DataProducerId, DataProducerOptions, DataProducerType};
use crate::data_structures::{AppData, SctpState};
use crate::messages::{TransportCloseRequest, TransportInternal, TransportSendRtcpNotification};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::{Router, RouterId};
use crate::sctp_parameters::SctpParameters;
use crate::transport::{
    ConsumeDataError, ConsumeError, ProduceDataError, ProduceError, RecvRtpHeaderExtensions,
    RtpListener, SctpListener, Transport, TransportGeneric, TransportId, TransportImpl,
    TransportTraceEventData, TransportTraceEventType, TransportType,
};
use crate::worker::{
    Channel, NotificationError, NotificationMessage, PayloadChannel, RequestError,
    SubscriptionHandler,
};
use async_executor::Executor;
use async_trait::async_trait;
use bytes::Bytes;
use event_listener_primitives::{Bag, BagOnce, HandlerId};
use log::*;
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::fmt;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::{Arc, Weak};

/// Direct transport options.
#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct DirectTransportOptions {
    /// Maximum allowed size for direct messages sent from DataProducers.
    /// Default 262_144.
    pub max_message_size: usize,
    /// Custom application data.
    pub app_data: AppData,
}

impl Default for DirectTransportOptions {
    fn default() -> Self {
        Self {
            max_message_size: 262_144,
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
#[non_exhaustive]
pub struct DirectTransportDump {
    // Common to all Transports.
    pub id: TransportId,
    pub direct: bool,
    pub producer_ids: Vec<ProducerId>,
    pub consumer_ids: Vec<ConsumerId>,
    pub map_ssrc_consumer_id: HashMap<u32, ConsumerId>,
    pub map_rtx_ssrc_consumer_id: HashMap<u32, ConsumerId>,
    pub data_producer_ids: Vec<DataProducerId>,
    pub data_consumer_ids: Vec<DataConsumerId>,
    pub recv_rtp_header_extensions: RecvRtpHeaderExtensions,
    pub rtp_listener: RtpListener,
    pub max_message_size: usize,
    pub sctp_parameters: Option<SctpParameters>,
    pub sctp_state: Option<SctpState>,
    pub sctp_listener: Option<SctpListener>,
    pub trace_event_types: String,
}

/// RTC statistics of the direct transport.
#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
#[allow(missing_docs)]
pub struct DirectTransportStat {
    // Common to all Transports.
    // `type` field is present in worker, but ignored here
    pub transport_id: TransportId,
    pub timestamp: u64,
    pub sctp_state: Option<SctpState>,
    pub bytes_received: usize,
    pub recv_bitrate: u32,
    pub bytes_sent: usize,
    pub send_bitrate: u32,
    pub rtp_bytes_received: usize,
    pub rtp_recv_bitrate: u32,
    pub rtp_bytes_sent: usize,
    pub rtp_send_bitrate: u32,
    pub rtx_bytes_received: usize,
    pub rtx_recv_bitrate: u32,
    pub rtx_bytes_sent: usize,
    pub rtx_send_bitrate: u32,
    pub probation_bytes_sent: usize,
    pub probation_send_bitrate: u32,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub available_outgoing_bitrate: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub available_incoming_bitrate: Option<u32>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub max_incoming_bitrate: Option<u32>,
}

#[derive(Default)]
struct Handlers {
    rtcp: Bag<Box<dyn Fn(&Bytes) + Send + Sync>>,
    new_producer: Bag<Box<dyn Fn(&Producer) + Send + Sync>>,
    new_consumer: Bag<Box<dyn Fn(&Consumer) + Send + Sync>>,
    new_data_producer: Bag<Box<dyn Fn(&DataProducer) + Send + Sync>>,
    new_data_consumer: Bag<Box<dyn Fn(&DataConsumer) + Send + Sync>>,
    trace: Bag<Box<dyn Fn(&TransportTraceEventData) + Send + Sync>>,
    router_close: BagOnce<Box<dyn FnOnce() + Send>>,
    close: BagOnce<Box<dyn FnOnce() + Send>>,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    Trace(TransportTraceEventData),
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum PayloadNotification {
    Rtcp,
}

struct Inner {
    id: TransportId,
    next_mid_for_consumers: AtomicUsize,
    used_sctp_stream_ids: Mutex<HashMap<u16, bool>>,
    cname_for_producers: Mutex<Option<String>>,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    // Make sure router is not dropped until this transport is not dropped
    router: Router,
    closed: AtomicBool,
    // Drop subscription to transport-specific notifications when transport itself is dropped
    _subscription_handlers: Vec<Option<SubscriptionHandler>>,
    _on_router_close_handler: Mutex<HandlerId>,
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
                let request = TransportCloseRequest {
                    internal: TransportInternal {
                        router_id: self.router.id(),
                        transport_id: self.id,
                    },
                };
                let router = self.router.clone();
                self.executor
                    .spawn(async move {
                        if let Err(error) = channel.request(request).await {
                            error!("transport closing failed on drop: {}", error);
                        }

                        drop(router);
                    })
                    .detach();
            }
        }
    }
}

/// A direct transport represents a direct connection between the mediasoup Rust process and a
/// [`Router`] instance in a mediasoup-worker thread.
///
/// A direct transport can be used to directly send and receive data messages from/to Rust by means
/// of [`DataProducer`]s and [`DataConsumer`]s of type `Direct` created on a direct transport.
/// Direct messages sent by a [`DataProducer`] in a direct transport can be consumed by endpoints
/// connected through a SCTP capable transport ([`WebRtcTransport`](crate::webrtc_transport::WebRtcTransport),
/// [`PlainTransport`](crate::plain_transport::PlainTransport), [`PipeTransport`](crate::pipe_transport::PipeTransport)
/// and also by the Rust application by means of a [`DataConsumer`] created on a [`DirectTransport`]
/// (and vice-versa: messages sent over SCTP/DataChannel can be consumed by the Rust application by
/// means of a [`DataConsumer`] created on a [`DirectTransport`]).
///
/// A direct transport can also be used to inject and directly consume RTP and RTCP packets in Rust
/// by using the [`DirectProducer::send`](crate::producer::DirectProducer::send) and
/// [`Consumer::on_rtp`] API (plus [`DirectTransport::send_rtcp`] and [`DirectTransport::on_rtcp`]
/// API).
#[derive(Clone)]
pub struct DirectTransport {
    inner: Arc<Inner>,
}

impl fmt::Debug for DirectTransport {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("DirectTransport")
            .field("id", &self.inner.id)
            .field("next_mid_for_consumers", &self.inner.next_mid_for_consumers)
            .field("used_sctp_stream_ids", &self.inner.used_sctp_stream_ids)
            .field("cname_for_producers", &self.inner.cname_for_producers)
            .field("router", &self.inner.router)
            .field("closed", &self.inner.closed)
            .finish()
    }
}

#[async_trait(?Send)]
impl Transport for DirectTransport {
    /// Transport id.
    fn id(&self) -> TransportId {
        self.inner.id
    }

    fn router_id(&self) -> RouterId {
        self.inner.router.id()
    }

    /// Custom application data.
    fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    fn closed(&self) -> bool {
        self.inner.closed.load(Ordering::SeqCst)
    }

    /// Create a Producer.
    ///
    /// Transport will be kept alive as long as at least one producer instance is alive.
    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, ProduceError> {
        debug!("produce()");

        let producer = self
            .produce_impl(producer_options, TransportType::Direct)
            .await?;

        self.inner.handlers.new_producer.call(|callback| {
            callback(&producer);
        });

        Ok(producer)
    }

    /// Create a Consumer.
    ///
    /// Transport will be kept alive as long as at least one consumer instance is alive.
    async fn consume(&self, consumer_options: ConsumerOptions) -> Result<Consumer, ConsumeError> {
        debug!("consume()");

        let consumer = self
            .consume_impl(consumer_options, TransportType::Direct, false)
            .await?;

        self.inner.handlers.new_consumer.call(|callback| {
            callback(&consumer);
        });

        Ok(consumer)
    }

    /// Create a DataProducer.
    ///
    /// Transport will be kept alive as long as at least one data producer instance is alive.
    async fn produce_data(
        &self,
        data_producer_options: DataProducerOptions,
    ) -> Result<DataProducer, ProduceDataError> {
        debug!("produce_data()");

        let data_producer = self
            .produce_data_impl(
                DataProducerType::Direct,
                data_producer_options,
                TransportType::Direct,
            )
            .await?;

        self.inner.handlers.new_data_producer.call(|callback| {
            callback(&data_producer);
        });

        Ok(data_producer)
    }

    /// Create a DataConsumer.
    ///
    /// Transport will be kept alive as long as at least one data consumer instance is alive.
    async fn consume_data(
        &self,
        data_consumer_options: DataConsumerOptions,
    ) -> Result<DataConsumer, ConsumeDataError> {
        debug!("consume_data()");

        let data_consumer = self
            .consume_data_impl(
                DataConsumerType::Direct,
                data_consumer_options,
                TransportType::Direct,
            )
            .await?;

        self.inner.handlers.new_data_consumer.call(|callback| {
            callback(&data_consumer);
        });

        Ok(data_consumer)
    }

    async fn enable_trace_event(
        &self,
        types: Vec<TransportTraceEventType>,
    ) -> Result<(), RequestError> {
        debug!("enable_trace_event()");

        self.enable_trace_event_impl(types).await
    }

    fn on_new_producer(
        &self,
        callback: Box<dyn Fn(&Producer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_producer.add(callback)
    }

    fn on_new_consumer(
        &self,
        callback: Box<dyn Fn(&Consumer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_consumer.add(callback)
    }

    fn on_new_data_producer(
        &self,
        callback: Box<dyn Fn(&DataProducer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_data_producer.add(callback)
    }

    fn on_new_data_consumer(
        &self,
        callback: Box<dyn Fn(&DataConsumer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_data_consumer.add(callback)
    }

    fn on_trace(
        &self,
        callback: Box<dyn Fn(&TransportTraceEventData) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.trace.add(callback)
    }

    fn on_router_close(&self, callback: Box<dyn FnOnce() + Send + 'static>) -> HandlerId {
        self.inner.handlers.router_close.add(callback)
    }

    fn on_close(&self, callback: Box<dyn FnOnce() + Send + 'static>) -> HandlerId {
        let handler_id = self.inner.handlers.close.add(Box::new(callback));
        if self.inner.closed.load(Ordering::Relaxed) {
            self.inner.handlers.close.call_simple();
        }
        handler_id
    }
}

#[async_trait(?Send)]
impl TransportGeneric for DirectTransport {
    type Dump = DirectTransportDump;
    type Stat = DirectTransportStat;

    /// Dump Transport.
    #[doc(hidden)]
    async fn dump(&self) -> Result<Self::Dump, RequestError> {
        debug!("dump()");

        self.dump_impl().await
    }

    /// Returns current RTC statistics of the transport.
    ///
    /// Check the [RTC Statistics](https://mediasoup.org/documentation/v3/mediasoup/rtc-statistics/)
    /// section for more details (TypeScript-oriented, but concepts apply here as well).
    async fn get_stats(&self) -> Result<Vec<Self::Stat>, RequestError> {
        debug!("get_stats()");

        self.get_stats_impl().await
    }
}

impl TransportImpl for DirectTransport {
    fn router(&self) -> &Router {
        &self.inner.router
    }

    fn channel(&self) -> &Channel {
        &self.inner.channel
    }

    fn payload_channel(&self) -> &PayloadChannel {
        &self.inner.payload_channel
    }

    fn executor(&self) -> &Arc<Executor<'static>> {
        &self.inner.executor
    }

    fn next_mid_for_consumers(&self) -> &AtomicUsize {
        &self.inner.next_mid_for_consumers
    }

    fn used_sctp_stream_ids(&self) -> &Mutex<HashMap<u16, bool>> {
        &self.inner.used_sctp_stream_ids
    }

    fn cname_for_producers(&self) -> &Mutex<Option<String>> {
        &self.inner.cname_for_producers
    }
}

impl DirectTransport {
    pub(super) fn new(
        id: TransportId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        app_data: AppData,
        router: Router,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);

            channel.subscribe_to_notifications(id.into(), move |notification| {
                match serde_json::from_value::<Notification>(notification) {
                    Ok(notification) => match notification {
                        Notification::Trace(trace_event_data) => {
                            handlers.trace.call(|callback| {
                                callback(&trace_event_data);
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

            payload_channel.subscribe_to_notifications(id.into(), move |notification| {
                let NotificationMessage { message, payload } = notification;
                match serde_json::from_value::<PayloadNotification>(message) {
                    Ok(notification) => match notification {
                        PayloadNotification::Rtcp => {
                            handlers.rtcp.call(|callback| {
                                callback(&payload);
                            });
                        }
                    },
                    Err(error) => {
                        error!("Failed to parse payload notification: {}", error);
                    }
                }
            })
        };

        let next_mid_for_consumers = AtomicUsize::default();
        let used_sctp_stream_ids = Mutex::new(HashMap::new());
        let cname_for_producers = Mutex::new(None);
        let inner_weak = Arc::<Mutex<Option<Weak<Inner>>>>::default();
        let on_router_close_handler = router.on_close({
            let inner_weak = Arc::clone(&inner_weak);

            move || {
                if let Some(inner) = inner_weak
                    .lock()
                    .as_ref()
                    .and_then(|weak_inner| weak_inner.upgrade())
                {
                    inner.handlers.router_close.call_simple();
                    inner.close(false);
                }
            }
        });
        let inner = Arc::new(Inner {
            id,
            next_mid_for_consumers,
            used_sctp_stream_ids,
            cname_for_producers,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            router,
            closed: AtomicBool::new(false),
            _subscription_handlers: vec![subscription_handler, payload_subscription_handler],
            _on_router_close_handler: Mutex::new(on_router_close_handler),
        });

        inner_weak.lock().replace(Arc::downgrade(&inner));

        Self { inner }
    }

    /// Send a RTCP packet from the Rust process.
    ///
    /// * `rtcp_packet` - Bytes containing a valid RTCP packet (can be a compound packet).
    pub async fn send_rtcp(&self, rtcp_packet: Bytes) -> Result<(), NotificationError> {
        self.inner
            .payload_channel
            .notify(
                TransportSendRtcpNotification {
                    internal: self.get_internal(),
                },
                rtcp_packet,
            )
            .await
    }

    /// Callback is called when the direct transport receives a RTCP packet from its router.
    pub fn on_rtcp<F: Fn(&Bytes) + Send + Sync + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.rtcp.add(Box::new(callback))
    }

    /// Downgrade `DirectTransport` to [`WeakDirectTransport`] instance.
    pub fn downgrade(&self) -> WeakDirectTransport {
        WeakDirectTransport {
            inner: Arc::downgrade(&self.inner),
        }
    }

    fn get_internal(&self) -> TransportInternal {
        TransportInternal {
            router_id: self.router().id(),
            transport_id: self.id(),
        }
    }
}

/// [`WeakDirectTransport`] doesn't own direct transport instance on mediasoup-worker and will not
/// prevent one from being destroyed once last instance of regular [`DirectTransport`] is dropped.
///
/// [`WeakDirectTransport`] vs [`DirectTransport`] is similar to [`Weak`] vs [`Arc`].
#[derive(Clone)]
pub struct WeakDirectTransport {
    inner: Weak<Inner>,
}

impl fmt::Debug for WeakDirectTransport {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("WeakDirectTransport").finish()
    }
}

impl WeakDirectTransport {
    /// Attempts to upgrade `WeakDirectTransport` to [`DirectTransport`] if last instance of one
    /// wasn't dropped yet.
    pub fn upgrade(&self) -> Option<DirectTransport> {
        let inner = self.inner.upgrade()?;

        Some(DirectTransport { inner })
    }
}
