#[cfg(test)]
mod tests;

use crate::consumer::{Consumer, ConsumerId, ConsumerOptions};
use crate::data_consumer::{DataConsumer, DataConsumerId, DataConsumerOptions, DataConsumerType};
use crate::data_producer::{DataProducer, DataProducerId, DataProducerOptions, DataProducerType};
use crate::data_structures::{AppData, ListenIp, SctpState, TransportTuple};
use crate::messages::{
    PlainTransportData, TransportCloseRequest, TransportConnectPlainRequest,
    TransportConnectRequestPlainData, TransportInternal,
};
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::transport::{TransportImpl, TransportType};
use crate::router::Router;
use crate::sctp_parameters::{NumSctpStreams, SctpParameters};
use crate::srtp_parameters::{SrtpCryptoSuite, SrtpParameters};
use crate::transport::{
    ConsumeDataError, ConsumeError, ProduceDataError, ProduceError, RecvRtpHeaderExtensions,
    RtpListener, SctpListener, Transport, TransportGeneric, TransportId, TransportTraceEventData,
    TransportTraceEventType,
};
use crate::worker::{Channel, PayloadChannel, RequestError, SubscriptionHandler};
use async_executor::Executor;
use async_trait::async_trait;
use event_listener_primitives::{Bag, BagOnce, HandlerId};
use log::{debug, error};
use nohash_hasher::IntMap;
use parking_lot::Mutex;
use serde::{Deserialize, Serialize};
use std::fmt;
use std::net::IpAddr;
use std::sync::atomic::{AtomicBool, AtomicUsize, Ordering};
use std::sync::{Arc, Weak};

/// [`PlainTransport`] options.
///
/// # Notes on usage
/// * Note that `comedia` mode just makes sense when the remote endpoint is gonna produce RTP on
///   this plain transport. Otherwise, if the remote endpoint does not send any RTP (or SCTP) packet
///   to mediasoup, there is no way to detect its remote RTP IP and port, so the endpoint won't
///   receive any packet from mediasoup.
/// * In other words, do not use `comedia` mode if the remote endpoint is not going to produce RTP
///   but just consume it. In those cases, do not set `comedia` flag and call
///   [`PlainTransport::connect()`] with the IP and port(s) of the remote endpoint.
#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct PlainTransportOptions {
    /// Listening IP address.
    pub listen_ip: ListenIp,
    /// Fixed port to listen on instead of selecting automatically from Worker's port range.
    pub port: Option<u16>,
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
    /// Create Plain transport options with given listen IP.
    #[must_use]
    pub fn new(listen_ip: ListenIp) -> Self {
        Self {
            listen_ip,
            port: None,
            rtcp_mux: true,
            comedia: false,
            enable_sctp: false,
            num_sctp_streams: NumSctpStreams::default(),
            max_sctp_message_size: 262_144,
            sctp_send_buffer_size: 262_144,
            enable_srtp: false,
            srtp_crypto_suite: SrtpCryptoSuite::default(),
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
#[non_exhaustive]
pub struct PlainTransportDump {
    // Common to all Transports.
    pub id: TransportId,
    pub direct: bool,
    pub producer_ids: Vec<ProducerId>,
    pub consumer_ids: Vec<ConsumerId>,
    pub map_ssrc_consumer_id: IntMap<u32, ConsumerId>,
    pub map_rtx_ssrc_consumer_id: IntMap<u32, ConsumerId>,
    pub data_producer_ids: Vec<DataProducerId>,
    pub data_consumer_ids: Vec<DataConsumerId>,
    pub recv_rtp_header_extensions: RecvRtpHeaderExtensions,
    pub rtp_listener: RtpListener,
    pub max_message_size: usize,
    pub sctp_parameters: Option<SctpParameters>,
    pub sctp_state: Option<SctpState>,
    pub sctp_listener: Option<SctpListener>,
    pub trace_event_types: String,
    // PlainTransport specific.
    pub rtcp_mux: bool,
    pub comedia: bool,
    pub tuple: TransportTuple,
    pub rtcp_tuple: Option<TransportTuple>,
    pub srtp_parameters: Option<SrtpParameters>,
}

/// RTC statistics of the plain transport.
#[derive(Debug, Clone, PartialOrd, PartialEq, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[non_exhaustive]
#[allow(missing_docs)]
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
    #[serde(skip_serializing_if = "Option::is_none")]
    pub rtp_packet_loss_received: Option<f64>,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub rtp_packet_loss_sent: Option<f64>,
    // PlainTransport specific.
    pub rtcp_mux: bool,
    pub comedia: bool,
    pub tuple: Option<TransportTuple>,
    pub rtcp_tuple: Option<TransportTuple>,
}

/// Remote parameters for plain transport.
#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct PlainTransportRemoteParameters {
    /// Remote IPv4 or IPv6.
    /// Required if `comedia` is not set.
    pub ip: Option<IpAddr>,
    /// Remote port.
    /// Required if `comedia` is not set.
    pub port: Option<u16>,
    /// Remote RTCP port.
    /// Required if `comedia` is not set and RTCP-mux is not enabled.
    pub rtcp_port: Option<u16>,
    /// SRTP parameters used by the remote endpoint to encrypt its RTP and RTCP.
    /// The SRTP crypto suite of the local `srtpParameters` gets also updated after connect()
    /// resolves.
    /// Required if enable_srtp was set.
    pub srtp_parameters: Option<SrtpParameters>,
}

#[derive(Default)]
struct Handlers {
    new_producer: Bag<Arc<dyn Fn(&Producer) + Send + Sync>, Producer>,
    new_consumer: Bag<Arc<dyn Fn(&Consumer) + Send + Sync>, Consumer>,
    new_data_producer: Bag<Arc<dyn Fn(&DataProducer) + Send + Sync>, DataProducer>,
    new_data_consumer: Bag<Arc<dyn Fn(&DataConsumer) + Send + Sync>, DataConsumer>,
    tuple: Bag<Arc<dyn Fn(&TransportTuple) + Send + Sync>, TransportTuple>,
    rtcp_tuple: Bag<Arc<dyn Fn(&TransportTuple) + Send + Sync>, TransportTuple>,
    sctp_state_change: Bag<Arc<dyn Fn(SctpState) + Send + Sync>>,
    trace: Bag<Arc<dyn Fn(&TransportTraceEventData) + Send + Sync>, TransportTraceEventData>,
    router_close: BagOnce<Box<dyn FnOnce() + Send>>,
    close: BagOnce<Box<dyn FnOnce() + Send>>,
}

#[derive(Debug, Deserialize)]
#[serde(tag = "event", rename_all = "lowercase", content = "data")]
enum Notification {
    Tuple {
        tuple: TransportTuple,
    },
    #[serde(rename_all = "camelCase")]
    RtcpTuple {
        rtcp_tuple: TransportTuple,
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
    used_sctp_stream_ids: Mutex<IntMap<u16, bool>>,
    cname_for_producers: Mutex<Option<String>>,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Arc<Handlers>,
    data: Arc<PlainTransportData>,
    app_data: AppData,
    // Make sure router is not dropped until this transport is not dropped
    router: Router,
    closed: AtomicBool,
    // Drop subscription to transport-specific notifications when transport itself is dropped
    _subscription_handler: Mutex<Option<SubscriptionHandler>>,
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
                        webrtc_server_id: None,
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
}

/// A plain transport represents a network path through which RTP, RTCP (optionally secured with
/// SRTP) and SCTP (DataChannel) is transmitted.
#[derive(Clone)]
#[must_use = "Transport will be closed on drop, make sure to keep it around for as long as needed"]
pub struct PlainTransport {
    inner: Arc<Inner>,
}

impl fmt::Debug for PlainTransport {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("PlainTransport")
            .field("id", &self.inner.id)
            .field("next_mid_for_consumers", &self.inner.next_mid_for_consumers)
            .field("used_sctp_stream_ids", &self.inner.used_sctp_stream_ids)
            .field("cname_for_producers", &self.inner.cname_for_producers)
            .field("router", &self.inner.router)
            .field("closed", &self.inner.closed)
            .finish()
    }
}

#[async_trait]
impl Transport for PlainTransport {
    fn id(&self) -> TransportId {
        self.inner.id
    }

    fn router(&self) -> &Router {
        &self.inner.router
    }

    fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    fn closed(&self) -> bool {
        self.inner.closed.load(Ordering::SeqCst)
    }

    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, ProduceError> {
        debug!("produce()");

        let producer = self
            .produce_impl(producer_options, TransportType::Plain)
            .await?;

        self.inner.handlers.new_producer.call_simple(&producer);

        Ok(producer)
    }

    async fn consume(&self, consumer_options: ConsumerOptions) -> Result<Consumer, ConsumeError> {
        debug!("consume()");

        let consumer = self
            .consume_impl(consumer_options, TransportType::Plain, false)
            .await?;

        self.inner.handlers.new_consumer.call_simple(&consumer);

        Ok(consumer)
    }

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

        self.inner
            .handlers
            .new_data_producer
            .call_simple(&data_producer);

        Ok(data_producer)
    }

    async fn consume_data(
        &self,
        data_consumer_options: DataConsumerOptions,
    ) -> Result<DataConsumer, ConsumeDataError> {
        debug!("consume_data()");

        let data_consumer = self
            .consume_data_impl(
                DataConsumerType::Sctp,
                data_consumer_options,
                TransportType::Plain,
            )
            .await?;

        self.inner
            .handlers
            .new_data_consumer
            .call_simple(&data_consumer);

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
        callback: Arc<dyn Fn(&Producer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_producer.add(callback)
    }

    fn on_new_consumer(
        &self,
        callback: Arc<dyn Fn(&Consumer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_consumer.add(callback)
    }

    fn on_new_data_producer(
        &self,
        callback: Arc<dyn Fn(&DataProducer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_data_producer.add(callback)
    }

    fn on_new_data_consumer(
        &self,
        callback: Arc<dyn Fn(&DataConsumer) + Send + Sync + 'static>,
    ) -> HandlerId {
        self.inner.handlers.new_data_consumer.add(callback)
    }

    fn on_trace(
        &self,
        callback: Arc<dyn Fn(&TransportTraceEventData) + Send + Sync + 'static>,
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

#[async_trait]
impl TransportGeneric for PlainTransport {
    type Dump = PlainTransportDump;
    type Stat = PlainTransportStat;

    #[doc(hidden)]
    async fn dump(&self) -> Result<Self::Dump, RequestError> {
        debug!("dump()");

        serde_json::from_value(self.dump_impl().await?).map_err(|error| {
            RequestError::FailedToParse {
                error: error.to_string(),
            }
        })
    }

    async fn get_stats(&self) -> Result<Vec<Self::Stat>, RequestError> {
        debug!("get_stats()");

        serde_json::from_value(self.get_stats_impl().await?).map_err(|error| {
            RequestError::FailedToParse {
                error: error.to_string(),
            }
        })
    }
}

impl TransportImpl for PlainTransport {
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

    fn used_sctp_stream_ids(&self) -> &Mutex<IntMap<u16, bool>> {
        &self.inner.used_sctp_stream_ids
    }

    fn cname_for_producers(&self) -> &Mutex<Option<String>> {
        &self.inner.cname_for_producers
    }
}

impl PlainTransport {
    pub(super) fn new(
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

            channel.subscribe_to_notifications(id.into(), move |notification| {
                match serde_json::from_slice::<Notification>(notification) {
                    Ok(notification) => match notification {
                        Notification::Tuple { tuple } => {
                            *data.tuple.lock() = tuple;

                            handlers.tuple.call_simple(&tuple);
                        }
                        Notification::RtcpTuple { rtcp_tuple } => {
                            data.rtcp_tuple.lock().replace(rtcp_tuple);

                            handlers.rtcp_tuple.call_simple(&rtcp_tuple);
                        }
                        Notification::SctpStateChange { sctp_state } => {
                            data.sctp_state.lock().replace(sctp_state);

                            handlers.sctp_state_change.call(|callback| {
                                callback(sctp_state);
                            });
                        }
                        Notification::Trace(trace_event_data) => {
                            handlers.trace.call_simple(&trace_event_data);
                        }
                    },
                    Err(error) => {
                        error!("Failed to parse notification: {}", error);
                    }
                }
            })
        };

        let next_mid_for_consumers = AtomicUsize::default();
        let used_sctp_stream_ids = Mutex::new({
            let mut used_used_sctp_stream_ids = IntMap::default();
            if let Some(sctp_parameters) = &data.sctp_parameters {
                for i in 0..sctp_parameters.mis {
                    used_used_sctp_stream_ids.insert(i, false);
                }
            }
            used_used_sctp_stream_ids
        });
        let cname_for_producers = Mutex::new(None);
        let inner_weak = Arc::<Mutex<Option<Weak<Inner>>>>::default();
        let on_router_close_handler = router.on_close({
            let inner_weak = Arc::clone(&inner_weak);

            move || {
                let maybe_inner = inner_weak.lock().as_ref().and_then(Weak::upgrade);
                if let Some(inner) = maybe_inner {
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
            data,
            app_data,
            router,
            closed: AtomicBool::new(false),
            _subscription_handler: Mutex::new(subscription_handler),
            _on_router_close_handler: Mutex::new(on_router_close_handler),
        });

        inner_weak.lock().replace(Arc::downgrade(&inner));

        Self { inner }
    }

    /// Provide the [`PlainTransport`] with remote parameters.
    ///
    /// # Notes on usage
    /// * If `comedia` is enabled in this plain transport and SRTP is not, `connect()` must not be
    ///   called.
    /// * If `comedia` is enabled and SRTP is also enabled (`enable_srtp` was set in the
    ///   [`Router::create_plain_transport`] options) then `connect()` must be called with just the
    ///   remote `srtp_parameters`.
    /// * If `comedia` is disabled, `connect()` must be eventually called with remote `ip`, `port`,
    ///   optional `rtcp_port` (if RTCP-mux is not enabled) and optional `srtp_parameters` (if SRTP
    ///   is enabled).
    ///
    /// # Examples
    /// ```rust
    /// use mediasoup::plain_transport::PlainTransportRemoteParameters;
    ///
    /// # async fn f(
    /// #     plain_transport: mediasoup::plain_transport::PlainTransport,
    /// # ) -> Result<(), Box<dyn std::error::Error>> {
    /// // Calling connect() on a PlainTransport created with comedia and rtcp_mux set.
    /// plain_transport
    ///     .connect(PlainTransportRemoteParameters {
    ///         ip: Some("1.2.3.4".parse().unwrap()),
    ///         port: Some(9998),
    ///         rtcp_port: None,
    ///         srtp_parameters: None,
    ///     })
    ///     .await?;
    /// # Ok(())
    /// # }
    /// ```
    /// ```rust
    /// use mediasoup::plain_transport::PlainTransportRemoteParameters;
    ///
    /// # async fn f(
    /// #     plain_transport: mediasoup::plain_transport::PlainTransport,
    /// # ) -> Result<(), Box<dyn std::error::Error>> {
    /// // Calling connect() on a PlainTransport created with comedia unset and rtcp_mux
    /// // also unset.
    /// plain_transport
    ///     .connect(PlainTransportRemoteParameters {
    ///         ip: Some("1.2.3.4".parse().unwrap()),
    ///         port: Some(9998),
    ///         rtcp_port: Some(9999),
    ///         srtp_parameters: None,
    ///     })
    ///     .await?;
    /// # Ok(())
    /// # }
    /// ```
    /// ```rust
    /// use mediasoup::plain_transport::PlainTransportRemoteParameters;
    /// use mediasoup::srtp_parameters::{SrtpParameters, SrtpCryptoSuite};
    ///
    /// # async fn f(
    /// #     plain_transport: mediasoup::plain_transport::PlainTransport,
    /// # ) -> Result<(), Box<dyn std::error::Error>> {
    /// // Calling connect() on a PlainTransport created with comedia set and
    /// // enable_srtp enabled.
    /// plain_transport
    ///     .connect(PlainTransportRemoteParameters {
    ///         ip: None,
    ///         port: None,
    ///         rtcp_port: None,
    ///         srtp_parameters: Some(SrtpParameters {
    ///             crypto_suite: SrtpCryptoSuite::AesCm128HmacSha180,
    ///             key_base64: "ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv".to_string(),
    ///         }),
    ///     })
    ///     .await?;
    /// # Ok(())
    /// # }
    /// ```
    /// ```rust
    /// use mediasoup::plain_transport::PlainTransportRemoteParameters;
    /// use mediasoup::srtp_parameters::{SrtpParameters, SrtpCryptoSuite};
    ///
    /// # async fn f(
    /// #     plain_transport: mediasoup::plain_transport::PlainTransport,
    /// # ) -> Result<(), Box<dyn std::error::Error>> {
    /// // Calling connect() on a PlainTransport created with comedia unset, rtcpMux
    /// // set and enableSrtp enabled.
    /// plain_transport
    ///     .connect(PlainTransportRemoteParameters {
    ///         ip: Some("1.2.3.4".parse().unwrap()),
    ///         port: Some(9998),
    ///         rtcp_port: None,
    ///         srtp_parameters: Some(SrtpParameters {
    ///             crypto_suite: SrtpCryptoSuite::AesCm128HmacSha180,
    ///             key_base64: "ZnQ3eWJraDg0d3ZoYzM5cXN1Y2pnaHU5NWxrZTVv".to_string(),
    ///         }),
    ///     })
    ///     .await?;
    /// # Ok(())
    /// # }
    /// ```
    pub async fn connect(
        &self,
        remote_parameters: PlainTransportRemoteParameters,
    ) -> Result<(), RequestError> {
        debug!("connect()");

        let response = self
            .inner
            .channel
            .request(TransportConnectPlainRequest {
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
            *self.inner.data.tuple.lock() = tuple;
        }

        if let Some(rtcp_tuple) = response.rtcp_tuple {
            self.inner.data.rtcp_tuple.lock().replace(rtcp_tuple);
        }

        if let Some(srtp_parameters) = response.srtp_parameters {
            self.inner
                .data
                .srtp_parameters
                .lock()
                .replace(srtp_parameters);
        }

        Ok(())
    }

    /// Set maximum incoming bitrate for media streams sent by the remote endpoint over this
    /// transport.
    pub async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError> {
        debug!("set_max_incoming_bitrate() [bitrate:{}]", bitrate);

        self.set_max_incoming_bitrate_impl(bitrate).await
    }

    /// The transport tuple. If RTCP-mux is enabled (`rtcp_mux` is set), this tuple refers to both
    /// RTP and RTCP.
    ///
    /// # Notes on usage
    /// * Once the plain transport is created, `transport.tuple()` will contain information about
    ///   its `local_ip`, `local_port` and `protocol`.
    /// * Information about `remote_ip` and `remote_port` will be set:
    ///   * after calling `connect()` method, or
    ///   * via dynamic remote address detection when using `comedia` mode.
    #[must_use]
    pub fn tuple(&self) -> TransportTuple {
        *self.inner.data.tuple.lock()
    }

    /// The transport tuple for RTCP. If RTCP-mux is enabled (`rtcp_mux` is set), its value is
    /// `None`.
    ///
    /// # Notes on usage
    /// * Once the plain transport is created (with RTCP-mux disabled), `transport.rtcp_tuple()`
    ///   will contain information about its `local_ip`, `local_port` and `protocol`.
    /// * Information about `remote_ip` and `remote_port` will be set:
    ///   * after calling `connect()` method, or
    ///   * via dynamic remote address detection when using `comedia` mode.
    #[must_use]
    pub fn rtcp_tuple(&self) -> Option<TransportTuple> {
        *self.inner.data.rtcp_tuple.lock()
    }

    /// Current SCTP state. Or `None` if SCTP is not enabled.
    #[must_use]
    pub fn sctp_parameters(&self) -> Option<SctpParameters> {
        self.inner.data.sctp_parameters
    }

    /// Current SCTP state. Or `None` if SCTP is not enabled.
    #[must_use]
    pub fn sctp_state(&self) -> Option<SctpState> {
        *self.inner.data.sctp_state.lock()
    }

    /// Local SRTP parameters representing the crypto suite and key material used to encrypt sending
    /// RTP and SRTP. Note that, if `comedia` mode is set, these local SRTP parameters may change
    /// after calling `connect()` with the remote SRTP parameters (to override the local SRTP crypto
    /// suite with the one given in `connect()`).
    #[must_use]
    pub fn srtp_parameters(&self) -> Option<SrtpParameters> {
        self.inner.data.srtp_parameters.lock().clone()
    }

    /// Callback is called after the remote RTP origin has been discovered. Only if `comedia` mode
    /// was set.
    pub fn on_tuple<F: Fn(&TransportTuple) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner.handlers.tuple.add(Arc::new(callback))
    }

    /// Callback is called after the remote RTCP origin has been discovered. Only if `comedia` mode
    /// was set and `rtcp_mux` was not.
    pub fn on_rtcp_tuple<F: Fn(&TransportTuple) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner.handlers.rtcp_tuple.add(Arc::new(callback))
    }

    /// Callback is called when the transport SCTP state changes.
    pub fn on_sctp_state_change<F: Fn(SctpState) + Send + Sync + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner
            .handlers
            .sctp_state_change
            .add(Arc::new(callback))
    }

    /// Downgrade `PlainTransport` to [`WeakPlainTransport`] instance.
    #[must_use]
    pub fn downgrade(&self) -> WeakPlainTransport {
        WeakPlainTransport {
            inner: Arc::downgrade(&self.inner),
        }
    }

    fn get_internal(&self) -> TransportInternal {
        TransportInternal {
            router_id: self.router().id(),
            transport_id: self.id(),
            webrtc_server_id: None,
        }
    }
}

/// [`WeakPlainTransport`] doesn't own pipe transport instance on mediasoup-worker and will not
/// prevent one from being destroyed once last instance of regular [`PlainTransport`] is dropped.
///
/// [`WeakPlainTransport`] vs [`PlainTransport`] is similar to [`Weak`] vs [`Arc`].
#[derive(Clone)]
pub struct WeakPlainTransport {
    inner: Weak<Inner>,
}

impl fmt::Debug for WeakPlainTransport {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.debug_struct("WeakPlainTransport").finish()
    }
}

impl WeakPlainTransport {
    /// Attempts to upgrade `WeakPlainTransport` to [`PlainTransport`] if last instance of one
    /// wasn't dropped yet.
    #[must_use]
    pub fn upgrade(&self) -> Option<PlainTransport> {
        let inner = self.inner.upgrade()?;

        Some(PlainTransport { inner })
    }
}
