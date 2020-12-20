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
use async_mutex::Mutex as AsyncMutex;
use async_trait::async_trait;
use bytes::Bytes;
use event_listener_primitives::{Bag, HandlerId};
use log::*;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::sync::atomic::AtomicUsize;
use std::sync::Arc;

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

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
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
    rtcp: Bag<'static, dyn Fn(&Bytes) + Send>,
    new_producer: Bag<'static, dyn Fn(&Producer) + Send>,
    new_consumer: Bag<'static, dyn Fn(&Consumer) + Send>,
    new_data_producer: Bag<'static, dyn Fn(&DataProducer) + Send>,
    new_data_consumer: Bag<'static, dyn Fn(&DataConsumer) + Send>,
    trace: Bag<'static, dyn Fn(&TransportTraceEventData) + Send>,
    router_close: Bag<'static, dyn FnOnce() + Send>,
    close: Bag<'static, dyn FnOnce() + Send>,
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
    used_sctp_stream_ids: AsyncMutex<HashMap<u16, bool>>,
    cname_for_producers: AsyncMutex<Option<String>>,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    // Make sure router is not dropped until this transport is not dropped
    router: Router,
    // Drop subscription to transport-specific notifications when transport itself is dropped
    _subscription_handlers: Vec<SubscriptionHandler>,
    _on_router_close_handler: AsyncMutex<HandlerId<'static>>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.handlers.close.call_once_simple();

        {
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

#[derive(Clone)]
pub struct DirectTransport {
    inner: Arc<Inner>,
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

    /// App custom data.
    fn app_data(&self) -> &AppData {
        &self.inner.app_data
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
}

#[async_trait(?Send)]
impl TransportGeneric<DirectTransportDump, DirectTransportStat> for DirectTransport {
    /// Dump Transport.
    #[doc(hidden)]
    async fn dump(&self) -> Result<DirectTransportDump, RequestError> {
        debug!("dump()");

        self.dump_impl().await
    }

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<DirectTransportStat>, RequestError> {
        debug!("get_stats()");

        self.get_stats_impl().await
    }

    async fn enable_trace_event(
        &self,
        types: Vec<TransportTraceEventType>,
    ) -> Result<(), RequestError> {
        debug!("enable_trace_event()");

        self.enable_trace_event_impl(types).await
    }

    fn on_new_producer<F: Fn(&Producer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.new_producer.add(Box::new(callback))
    }

    fn on_new_consumer<F: Fn(&Consumer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.new_consumer.add(Box::new(callback))
    }

    fn on_new_data_producer<F: Fn(&DataProducer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner
            .handlers
            .new_data_producer
            .add(Box::new(callback))
    }

    fn on_new_data_consumer<F: Fn(&DataConsumer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner
            .handlers
            .new_data_consumer
            .add(Box::new(callback))
    }

    fn on_trace<F: Fn(&TransportTraceEventData) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.trace.add(Box::new(callback))
    }

    fn on_router_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.router_close.add(Box::new(callback))
    }

    fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.close.add(Box::new(callback))
    }
}

impl TransportImpl<DirectTransportDump, DirectTransportStat> for DirectTransport {
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

    fn used_sctp_stream_ids(&self) -> &AsyncMutex<HashMap<u16, bool>> {
        &self.inner.used_sctp_stream_ids
    }

    fn cname_for_producers(&self) -> &AsyncMutex<Option<String>> {
        &self.inner.cname_for_producers
    }
}

impl DirectTransport {
    pub(super) async fn new(
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

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
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
                .await
        };

        let payload_subscription_handler = {
            let handlers = Arc::clone(&handlers);

            payload_channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
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
                .await
        };

        let next_mid_for_consumers = AtomicUsize::default();
        let used_sctp_stream_ids = AsyncMutex::new(HashMap::new());
        let cname_for_producers = AsyncMutex::new(None);
        let on_router_close_handler = router.on_close({
            let handlers = Arc::clone(&handlers);

            move || {
                handlers.router_close.call_once_simple();
                handlers.close.call_once_simple();
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
            _subscription_handlers: vec![subscription_handler, payload_subscription_handler],
            _on_router_close_handler: AsyncMutex::new(on_router_close_handler),
        });

        Self { inner }
    }

    /// Send RTCP packet.
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

    pub fn on_rtcp<F: Fn(&Bytes) + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.rtcp.add(Box::new(callback))
    }

    fn get_internal(&self) -> TransportInternal {
        TransportInternal {
            router_id: self.router().id(),
            transport_id: self.id(),
        }
    }
}
