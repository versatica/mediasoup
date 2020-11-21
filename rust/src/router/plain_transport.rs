use crate::consumer::{Consumer, ConsumerId, ConsumerOptions};
use crate::data_consumer::{DataConsumer, DataConsumerId, DataConsumerOptions, DataConsumerType};
use crate::data_producer::{DataProducer, DataProducerId, DataProducerOptions, DataProducerType};
use crate::data_structures::{AppData, SctpState, TransportListenIp, TransportTuple};
use crate::messages::{
    PlainTransportData, TransportCloseRequest, TransportConnectRequestPlain,
    TransportConnectRequestPlainData, TransportInternal,
};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::{Router, RouterId};
use crate::sctp_parameters::{NumSctpStreams, SctpParameters};
use crate::srtp_parameters::{SrtpCryptoSuite, SrtpParameters};
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
use std::sync::atomic::AtomicUsize;
use std::sync::Arc;

#[derive(Debug)]
#[non_exhaustive]
pub struct PlainTransportOptions {
    /// Listening IP address.
    pub listen_ip: TransportListenIp,
    /// Use RTCP-mux (RTP and RTCP in the same port).
    /// Default true.
    pub rtcp_mux: bool,
    /// Whether remote IP:port should be auto-detected based on first RTP/RTCP
    /// packet received. If enabled, connect() method must not be called unless
    /// SRTP is enabled. If so, it must be called with just remote SRTP parameters.
    /// Default false.
    pub comedia: bool,
    /// Create a SCTP association.
    /// Default false.
    pub enable_sctp: bool,
    /// SCTP streams number.
    pub num_sctp_streams: NumSctpStreams,
    /// Maximum allowed size for SCTP messages sent by DataProducers.
    /// Default 262144.
    pub max_sctp_message_size: u32,
    /// Maximum SCTP send buffer used by DataConsumers.
    /// Default 262144.
    pub sctp_send_buffer_size: u32,
    /// Enable SRTP. For this to work, connect() must be called with remote SRTP parameters.
    /// Default false.
    pub enable_srtp: bool,
    /// The SRTP crypto suite to be used if enableSrtp is set.
    /// Default 'AesCm128HmacSha180'.
    pub srtp_crypto_suite: SrtpCryptoSuite,
    /// Custom application data.
    pub app_data: AppData,
}

impl PlainTransportOptions {
    pub fn new(listen_ip: TransportListenIp) -> Self {
        Self {
            listen_ip,
            rtcp_mux: true,
            comedia: false,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 262144,
            sctp_send_buffer_size: 262144,
            enable_srtp: false,
            srtp_crypto_suite: SrtpCryptoSuite::default(),
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct PlainTransportDump {
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
    // PlainTransport specific.
    pub rtcp_mux: bool,
    pub comedia: bool,
    pub tuple: Option<TransportTuple>,
    pub rtcp_tuple: Option<TransportTuple>,
    pub srtp_parameters: Option<SrtpParameters>,
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlainTransportStat {
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
    // PlainTransport specific.
    pub rtcp_mux: bool,
    pub comedia: bool,
    pub tuple: Option<TransportTuple>,
    pub rtcp_tuple: Option<TransportTuple>,
}

// TODO: This has all parameters optional, would be useful to somehow make these invariants cleaner
pub struct PlainTransportRemoteParameters {
    /// Remote IPv4 or IPv6.
    /// Required if `comedia` is not set.
    pub ip: Option<String>,
    /// Remote port.
    /// Required if `comedia` is not set.
    pub port: Option<u16>,
    /// Remote RTCP port.
    /// Required if `comedia` is not set and RTCP-mux is not enabled.
    pub rtcp_port: Option<u16>,
    /// SRTP parameters used by the remote endpoint to encrypt its RTP and RTCP.
    /// The SRTP crypto suite of the local `srtpParameters` gets also updated after connect()
    /// resolves.
    /// Required if enableSrtp was set.
    pub srtp_parameters: Option<SrtpParameters>,
}

#[derive(Default)]
struct Handlers {
    new_producer: Bag<'static, dyn Fn(&Producer) + Send>,
    new_consumer: Bag<'static, dyn Fn(&Consumer) + Send>,
    new_data_producer: Bag<'static, dyn Fn(&DataProducer) + Send>,
    new_data_consumer: Bag<'static, dyn Fn(&DataConsumer) + Send>,
    tuple: Bag<'static, dyn Fn(&TransportTuple) + Send>,
    rtcp_tuple: Bag<'static, dyn Fn(&TransportTuple) + Send>,
    sctp_state_change: Bag<'static, dyn Fn(SctpState) + Send>,
    trace: Bag<'static, dyn Fn(&TransportTraceEventData) + Send>,
    close: Bag<'static, dyn FnOnce() + Send>,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    Tuple(TransportTuple),
    RtcpTuple(TransportTuple),
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
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    data: Arc<PlainTransportData>,
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
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("transport closing failed on drop: {}", error);
                    }
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct PlainTransport {
    inner: Arc<Inner>,
}

#[async_trait(?Send)]
impl Transport for PlainTransport {
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

    /// Set maximum incoming bitrate for receiving media.
    async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError> {
        debug!("set_max_incoming_bitrate() [bitrate:{}]", bitrate);

        self.set_max_incoming_bitrate_impl(bitrate).await
    }

    /// Create a Producer.
    ///
    /// Transport will be kept alive as long as at least one producer instance is alive.
    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, ProduceError> {
        debug!("produce()");

        let producer = self
            .produce_impl(producer_options, TransportType::Plain)
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

        let consumer = self.consume_impl(consumer_options).await?;

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
                TransportType::Plain,
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
            .consume_data_impl(DataConsumerType::Sctp, data_consumer_options)
            .await?;

        self.inner.handlers.new_data_consumer.call(|callback| {
            callback(&data_consumer);
        });

        Ok(data_consumer)
    }
}

#[async_trait(?Send)]
impl TransportGeneric<PlainTransportDump, PlainTransportStat> for PlainTransport {
    /// Dump Transport.
    #[doc(hidden)]
    async fn dump(&self) -> Result<PlainTransportDump, RequestError> {
        debug!("dump()");

        self.dump_impl().await
    }

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<PlainTransportStat>, RequestError> {
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

    fn on_new_producer<F: Fn(&Producer) + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.new_producer.add(Box::new(callback))
    }

    fn on_new_consumer<F: Fn(&Consumer) + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.new_consumer.add(Box::new(callback))
    }

    fn on_new_data_producer<F: Fn(&DataProducer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner
            .handlers
            .new_data_producer
            .add(Box::new(callback))
    }

    fn on_new_data_consumer<F: Fn(&DataConsumer) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner
            .handlers
            .new_data_consumer
            .add(Box::new(callback))
    }

    fn on_trace<F: Fn(&TransportTraceEventData) + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.trace.add(Box::new(callback))
    }

    fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.close.add(Box::new(callback))
    }
}

impl TransportImpl<PlainTransportDump, PlainTransportStat> for PlainTransport {
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
}

impl PlainTransport {
    pub(super) async fn new(
        id: TransportId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        data: PlainTransportData,
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
                            Notification::Tuple(tuple) => {
                                *data.tuple.lock().unwrap() = tuple.clone();

                                handlers.tuple.call(|callback| {
                                    callback(&tuple);
                                });
                            }
                            Notification::RtcpTuple(rtcp_tuple) => {
                                data.rtcp_tuple.lock().unwrap().replace(rtcp_tuple.clone());

                                handlers.rtcp_tuple.call(|callback| {
                                    callback(&rtcp_tuple);
                                });
                            }
                            Notification::SctpStateChange { sctp_state } => {
                                data.sctp_state.lock().unwrap().replace(sctp_state);

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
                .unwrap()
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
        let inner = Arc::new(Inner {
            id,
            next_mid_for_consumers,
            used_sctp_stream_ids,
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

    /// Provide the PlainTransport remote parameters.
    pub async fn connect(
        &self,
        remote_parameters: PlainTransportRemoteParameters,
    ) -> Result<(), RequestError> {
        debug!("connect()");

        let response = self
            .inner
            .channel
            .request(TransportConnectRequestPlain {
                internal: self.get_internal(),
                data: TransportConnectRequestPlainData {
                    ip: remote_parameters.ip,
                    port: remote_parameters.port,
                    rtcp_port: remote_parameters.rtcp_port,
                    srtp_parameters: remote_parameters.srtp_parameters,
                },
            })
            .await?;

        if let Some(tuple) = response.tuple {
            *self.inner.data.tuple.lock().unwrap() = tuple;
        }

        if let Some(rtcp_tuple) = response.rtcp_tuple {
            self.inner
                .data
                .rtcp_tuple
                .lock()
                .unwrap()
                .replace(rtcp_tuple);
        }

        if let Some(srtp_parameters) = response.srtp_parameters {
            self.inner
                .data
                .srtp_parameters
                .lock()
                .unwrap()
                .replace(srtp_parameters);
        }

        Ok(())
    }

    /// Transport tuple.
    pub fn tuple(&self) -> TransportTuple {
        self.inner.data.tuple.lock().unwrap().clone()
    }

    /// Transport RTCP tuple.
    pub fn rtcp_tuple(&self) -> Option<TransportTuple> {
        self.inner.data.rtcp_tuple.lock().unwrap().clone()
    }

    /// SCTP parameters.
    pub fn sctp_parameters(&self) -> Option<SctpParameters> {
        self.inner.data.sctp_parameters
    }

    /// SCTP state.
    pub fn sctp_state(&self) -> Option<SctpState> {
        *self.inner.data.sctp_state.lock().unwrap()
    }

    /// SRTP parameters.
    pub fn srtp_parameters(&self) -> Option<SrtpParameters> {
        self.inner.data.srtp_parameters.lock().unwrap().clone()
    }

    pub fn on_tuple<F: Fn(&TransportTuple) + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.tuple.add(Box::new(callback))
    }

    pub fn on_rtcp_tuple<F: Fn(&TransportTuple) + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.rtcp_tuple.add(Box::new(callback))
    }

    pub fn on_sctp_state_change<F: Fn(SctpState) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
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
