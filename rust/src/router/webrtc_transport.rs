use crate::consumer::{Consumer, ConsumerId, ConsumerOptions};
use crate::data_consumer::{DataConsumer, DataConsumerId, DataConsumerOptions, DataConsumerType};
use crate::data_producer::{DataProducer, DataProducerId, DataProducerOptions, DataProducerType};
use crate::data_structures::{
    AppData, DtlsParameters, DtlsState, IceCandidate, IceParameters, IceRole, IceState, SctpState,
    TransportListenIp, TransportTuple,
};
use crate::messages::{
    TransportCloseRequest, TransportConnectRequestWebRtc, TransportConnectRequestWebRtcData,
    TransportInternal, TransportRestartIceRequest, WebRtcTransportData,
};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::{Router, RouterId};
use crate::sctp_parameters::{NumSctpStreams, SctpParameters};
use crate::transport::{
    ConsumeDataError, ConsumeError, ProduceDataError, ProduceError, RecvRtpHeaderExtensions,
    RtpListener, SctpListener, Transport, TransportGeneric, TransportId, TransportImpl,
    TransportTraceEventData, TransportTraceEventType, TransportType,
};
use crate::worker::{Channel, PayloadChannel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use async_mutex::Mutex as AsyncMutex;
use async_trait::async_trait;
use event_listener_primitives::{Bag, HandlerId};
use log::*;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;
use std::convert::TryFrom;
use std::ops::Deref;
use std::sync::atomic::AtomicUsize;
use std::sync::Arc;
use thiserror::Error;

/// Struct that protects an invariant of having non-empty list of listen IPs
#[derive(Debug, Clone, Eq, PartialEq, Serialize)]
pub struct TransportListenIps(Vec<TransportListenIp>);

impl TransportListenIps {
    pub fn new(listen_ip: TransportListenIp) -> Self {
        Self(vec![listen_ip])
    }

    pub fn insert(mut self, listen_ip: TransportListenIp) -> Self {
        self.0.push(listen_ip);
        self
    }
}

impl Deref for TransportListenIps {
    type Target = Vec<TransportListenIp>;

    fn deref(&self) -> &Self::Target {
        &self.0
    }
}

#[derive(Error, Debug)]
#[error("Empty list of listen IPs provided, should have at least one element")]
pub struct EmptyListError;

impl TryFrom<Vec<TransportListenIp>> for TransportListenIps {
    type Error = EmptyListError;

    fn try_from(listen_ips: Vec<TransportListenIp>) -> Result<Self, Self::Error> {
        if listen_ips.is_empty() {
            Err(EmptyListError)
        } else {
            Ok(Self(listen_ips))
        }
    }
}

#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct WebRtcTransportOptions {
    /// Listening IP address or addresses in order of preference (first one is the preferred one).
    pub listen_ips: TransportListenIps,
    /// Listen in UDP. Default true.
    pub enable_udp: bool,
    /// Listen in TCP.
    /// Default false.
    pub enable_tcp: bool,
    /// Prefer UDP.
    /// Default false.
    pub prefer_udp: bool,
    /// Prefer TCP.
    /// Default false.
    pub prefer_tcp: bool,
    /// Initial available outgoing bitrate (in bps).
    /// Default 600000.
    pub initial_available_outgoing_bitrate: u32,
    /// Create a SCTP association.
    /// Default false.
    pub enable_sctp: bool,
    /// SCTP streams number.
    pub num_sctp_streams: NumSctpStreams,
    /// Maximum allowed size for SCTP messages sent by DataProducers.
    // 	Default 262144.
    pub max_sctp_message_size: u32,
    /// Maximum SCTP send buffer used by DataConsumers.
    /// Default 262144.
    pub sctp_send_buffer_size: u32,
    /// Custom application data.
    pub app_data: AppData,
}

impl WebRtcTransportOptions {
    pub fn new(listen_ips: TransportListenIps) -> Self {
        Self {
            listen_ips,
            enable_udp: true,
            enable_tcp: false,
            prefer_udp: false,
            prefer_tcp: false,
            initial_available_outgoing_bitrate: 600_000,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 262144,
            sctp_send_buffer_size: 262144,
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct WebRtcTransportDump {
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
    pub sctp_listener: Option<SctpListener>,
    pub trace_event_types: String,
    // WebRtcTransport specific.
    pub dtls_parameters: DtlsParameters,
    pub dtls_state: DtlsState,
    pub ice_candidates: Vec<IceCandidate>,
    pub ice_parameters: IceParameters,
    pub ice_role: IceRole,
    pub ice_state: IceState,
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct WebRtcTransportStat {
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
    // WebRtcTransport specific.
    pub ice_role: IceRole,
    pub ice_state: IceState,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub ice_selected_tuple: Option<TransportTuple>,
    pub dtls_state: DtlsState,
}

#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
pub struct WebRtcTransportRemoteParameters {
    pub dtls_parameters: DtlsParameters,
}

#[derive(Default)]
struct Handlers {
    new_producer: Bag<'static, dyn Fn(&Producer) + Send>,
    new_consumer: Bag<'static, dyn Fn(&Consumer) + Send>,
    new_data_producer: Bag<'static, dyn Fn(&DataProducer) + Send>,
    new_data_consumer: Bag<'static, dyn Fn(&DataConsumer) + Send>,
    ice_state_change: Bag<'static, dyn Fn(IceState) + Send>,
    ice_selected_tuple_change: Bag<'static, dyn Fn(&TransportTuple) + Send>,
    dtls_state_change: Bag<'static, dyn Fn(DtlsState) + Send>,
    sctp_state_change: Bag<'static, dyn Fn(SctpState) + Send>,
    trace: Bag<'static, dyn Fn(&TransportTraceEventData) + Send>,
    close: Bag<'static, dyn FnOnce() + Send>,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    #[serde(rename_all = "camelCase")]
    IceStateChange {
        ice_state: IceState,
    },
    #[serde(rename_all = "camelCase")]
    IceSelectedTupleChange {
        ice_selected_tuple: TransportTuple,
    },
    #[serde(rename_all = "camelCase")]
    DtlsStateChange {
        dtls_state: DtlsState,
        dtls_remote_cert: Option<String>,
    },
    #[serde(rename_all = "camelCase")]
    SctpStateChange {
        sctp_state: SctpState,
    },
    Trace(TransportTraceEventData),
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
    data: Arc<WebRtcTransportData>,
    app_data: AppData,
    // Make sure router is not dropped until this transport is not dropped
    router: Router,
    // Drop subscription to transport-specific notifications when transport itself is dropped
    _subscription_handler: SubscriptionHandler,
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
pub struct WebRtcTransport {
    inner: Arc<Inner>,
}

#[async_trait(?Send)]
impl Transport for WebRtcTransport {
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
            .produce_impl(producer_options, TransportType::WebRtc)
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
            .consume_impl(consumer_options, TransportType::WebRtc, false)
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
                DataProducerType::Sctp,
                data_producer_options,
                TransportType::WebRtc,
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
                DataConsumerType::Sctp,
                data_consumer_options,
                TransportType::WebRtc,
            )
            .await?;

        self.inner.handlers.new_data_consumer.call(|callback| {
            callback(&data_consumer);
        });

        Ok(data_consumer)
    }
}

#[async_trait(?Send)]
impl TransportGeneric<WebRtcTransportDump, WebRtcTransportStat> for WebRtcTransport {
    /// Dump Transport.
    #[doc(hidden)]
    async fn dump(&self) -> Result<WebRtcTransportDump, RequestError> {
        debug!("dump()");

        self.dump_impl().await
    }

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<WebRtcTransportStat>, RequestError> {
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

    fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId<'static> {
        self.inner.handlers.close.add(Box::new(callback))
    }
}

impl TransportImpl<WebRtcTransportDump, WebRtcTransportStat> for WebRtcTransport {
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

impl WebRtcTransport {
    pub(super) async fn new(
        id: TransportId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        data: WebRtcTransportData,
        app_data: AppData,
        router: Router,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let data = Arc::new(data);

        let subscription_handler = {
            let handlers = Arc::clone(&handlers);
            let data = Arc::clone(&data);

            channel
                .subscribe_to_notifications(id.to_string(), move |notification| {
                    match serde_json::from_value::<Notification>(notification) {
                        Ok(notification) => match notification {
                            Notification::IceStateChange { ice_state } => {
                                *data.ice_state.lock() = ice_state;
                                handlers.ice_state_change.call(|callback| {
                                    callback(ice_state);
                                });
                            }
                            Notification::IceSelectedTupleChange { ice_selected_tuple } => {
                                data.ice_selected_tuple
                                    .lock()
                                    .replace(ice_selected_tuple.clone());
                                handlers.ice_selected_tuple_change.call(|callback| {
                                    callback(&ice_selected_tuple);
                                });
                            }
                            Notification::DtlsStateChange {
                                dtls_state,
                                dtls_remote_cert,
                            } => {
                                *data.dtls_state.lock() = dtls_state;

                                if let Some(dtls_remote_cert) = dtls_remote_cert {
                                    data.dtls_remote_cert.lock().replace(dtls_remote_cert);
                                }

                                handlers.dtls_state_change.call(|callback| {
                                    callback(dtls_state);
                                });
                            }
                            Notification::SctpStateChange { sctp_state } => {
                                data.sctp_state.lock().replace(sctp_state);

                                handlers.sctp_state_change.call(|callback| {
                                    callback(sctp_state);
                                });
                            }
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

        let next_mid_for_consumers = AtomicUsize::default();
        let used_sctp_stream_ids = AsyncMutex::new({
            let mut used_used_sctp_stream_ids = HashMap::new();
            if let Some(sctp_parameters) = &data.sctp_parameters {
                for i in 0..sctp_parameters.mis {
                    used_used_sctp_stream_ids.insert(i, false);
                }
            }
            used_used_sctp_stream_ids
        });
        let cname_for_producers = AsyncMutex::new(None);
        let inner = Arc::new(Inner {
            id,
            next_mid_for_consumers,
            used_sctp_stream_ids,
            cname_for_producers,
            executor,
            channel,
            payload_channel,
            handlers,
            data,
            app_data,
            router,
            _subscription_handler: subscription_handler,
        });

        Self { inner }
    }

    /// Provide the WebRtcTransport remote parameters.
    pub async fn connect(
        &self,
        remote_parameters: WebRtcTransportRemoteParameters,
    ) -> Result<(), RequestError> {
        debug!("connect()");

        let response = self
            .inner
            .channel
            .request(TransportConnectRequestWebRtc {
                internal: self.get_internal(),
                data: TransportConnectRequestWebRtcData {
                    dtls_parameters: remote_parameters.dtls_parameters,
                },
            })
            .await?;

        self.inner.data.dtls_parameters.lock().role = response.dtls_local_role;

        Ok(())
    }

    /// Set maximum incoming bitrate for receiving media.
    pub async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError> {
        debug!("set_max_incoming_bitrate() [bitrate:{}]", bitrate);

        self.set_max_incoming_bitrate_impl(bitrate).await
    }

    /// ICE role.
    pub fn ice_role(&self) -> IceRole {
        self.inner.data.ice_role
    }

    /// ICE parameters.
    pub fn ice_parameters(&self) -> IceParameters {
        self.inner.data.ice_parameters.clone()
    }

    /// ICE candidates.
    pub fn ice_candidates(&self) -> Vec<IceCandidate> {
        self.inner.data.ice_candidates.clone()
    }

    /// ICE state.
    pub fn ice_state(&self) -> IceState {
        *self.inner.data.ice_state.lock()
    }

    /// ICE selected tuple.
    pub fn ice_selected_tuple(&self) -> Option<TransportTuple> {
        self.inner.data.ice_selected_tuple.lock().clone()
    }

    /// DTLS parameters.
    pub fn dtls_parameters(&self) -> DtlsParameters {
        self.inner.data.dtls_parameters.lock().clone()
    }

    /// DTLS state.
    pub fn dtls_state(&self) -> DtlsState {
        *self.inner.data.dtls_state.lock()
    }

    /// Remote certificate in PEM format.
    pub fn dtls_remote_cert(&self) -> Option<String> {
        self.inner.data.dtls_remote_cert.lock().clone()
    }

    /// SCTP parameters.
    pub fn sctp_parameters(&self) -> Option<SctpParameters> {
        self.inner.data.sctp_parameters
    }

    /// SCTP state.
    pub fn sctp_state(&self) -> Option<SctpState> {
        *self.inner.data.sctp_state.lock()
    }

    /// Restart ICE.
    pub async fn restart_ice(&self) -> Result<IceParameters, RequestError> {
        debug!("restart_ice()");

        let response = self
            .inner
            .channel
            .request(TransportRestartIceRequest {
                internal: self.get_internal(),
            })
            .await?;

        Ok(response.ice_parameters)
    }

    pub fn on_ice_state_change<F: Fn(IceState) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner.handlers.ice_state_change.add(Box::new(callback))
    }

    pub fn on_ice_selected_tuple_change<F: Fn(&TransportTuple) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner
            .handlers
            .ice_selected_tuple_change
            .add(Box::new(callback))
    }

    pub fn on_dtls_state_change<F: Fn(DtlsState) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner
            .handlers
            .dtls_state_change
            .add(Box::new(callback))
    }

    pub fn on_sctp_state_change<F: Fn(SctpState) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId<'static> {
        self.inner
            .handlers
            .sctp_state_change
            .add(Box::new(callback))
    }

    fn get_internal(&self) -> TransportInternal {
        TransportInternal {
            router_id: self.router().id(),
            transport_id: self.id(),
        }
    }
}
