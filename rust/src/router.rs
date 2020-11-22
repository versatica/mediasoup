#[cfg(not(doc))]
pub mod audio_level_observer;
#[cfg(not(doc))]
pub mod consumer;
#[cfg(not(doc))]
pub mod data_consumer;
#[cfg(not(doc))]
pub mod data_producer;
#[cfg(not(doc))]
pub mod direct_transport;
#[cfg(not(doc))]
pub mod pipe_transport;
#[cfg(not(doc))]
pub mod plain_transport;
#[cfg(not(doc))]
pub mod producer;
#[cfg(not(doc))]
pub mod rtp_observer;
#[cfg(not(doc))]
pub mod transport;
#[cfg(not(doc))]
pub mod webrtc_transport;

use crate::{ortc, uuid_based_wrapper_type};

use crate::audio_level_observer::{AudioLevelObserver, AudioLevelObserverOptions};
use crate::consumer::{Consumer, ConsumerId, ConsumerOptions};
use crate::data_consumer::{DataConsumer, DataConsumerOptions};
use crate::data_producer::{DataProducer, DataProducerId, DataProducerOptions, WeakDataProducer};
use crate::data_structures::{AppData, TransportListenIp, TransportTuple};
use crate::direct_transport::{DirectTransport, DirectTransportOptions};
use crate::messages::{
    RouterCloseRequest, RouterCreateAudioLevelObserverData, RouterCreateAudioLevelObserverRequest,
    RouterCreateDirectTransportData, RouterCreateDirectTransportRequest,
    RouterCreatePipeTransportData, RouterCreatePipeTransportRequest,
    RouterCreatePlainTransportData, RouterCreatePlainTransportRequest,
    RouterCreateWebrtcTransportData, RouterCreateWebrtcTransportRequest, RouterDumpRequest,
    RouterInternal, RtpObserverInternal, TransportInternal,
};
use crate::pipe_transport::{PipeTransport, PipeTransportOptions, PipeTransportRemoteParameters};
use crate::plain_transport::{PlainTransport, PlainTransportOptions};
use crate::producer::{Producer, ProducerId, ProducerOptions, WeakProducer};
use crate::rtp_observer::RtpObserverId;
use crate::rtp_parameters::{RtpCapabilities, RtpCapabilitiesFinalized, RtpCodecCapability};
use crate::sctp_parameters::NumSctpStreams;
use crate::transport::{
    ConsumeDataError, ConsumeError, ProduceDataError, ProduceError, Transport, TransportGeneric,
    TransportId,
};
use crate::webrtc_transport::{WebRtcTransport, WebRtcTransportOptions};
use crate::worker::{Channel, PayloadChannel, RequestError, Worker};
use async_executor::Executor;
use async_mutex::Mutex as AsyncMutex;
use event_listener_primitives::{Bag, HandlerId};
use futures_lite::future;
use log::*;
use serde::{Deserialize, Serialize};
use std::collections::{HashMap, HashSet};
use std::sync::{Arc, Mutex as SyncMutex};
use thiserror::Error;

uuid_based_wrapper_type!(RouterId);

#[derive(Debug, Default)]
pub struct RouterOptions {
    pub media_codecs: Vec<RtpCodecCapability>,
    pub app_data: AppData,
}

pub struct PipeToRouterOptions {
    /// Target Router instance.
    pub router: Router,
    /// IP used in the PipeTransport pair.
    /// Default '127.0.0.1'.
    listen_ip: TransportListenIp,
    /// Create a SCTP association.
    /// Default true.
    pub enable_sctp: bool,
    /// SCTP streams number.
    pub num_sctp_streams: NumSctpStreams,
    /// Enable RTX and NACK for RTP retransmission.
    /// Default false.
    pub enable_rtx: bool,
    /// Enable SRTP.
    /// Default false.
    pub enable_srtp: bool,
}

impl PipeToRouterOptions {
    pub fn new(router: Router) -> Self {
        Self {
            router,
            listen_ip: TransportListenIp {
                ip: "127.0.0.1".to_string(),
                announced_ip: None,
            },
            enable_sctp: true,
            num_sctp_streams: Default::default(),
            enable_rtx: false,
            enable_srtp: false,
        }
    }
}

pub struct PipeToRouterSourceProducer {
    /// The id of the Producer to consume.
    pub producer_id: ProducerId,
}

pub struct PipeProducerToRouterValue {
    /// The Consumer created in the current Router.
    pub pipe_consumer: Consumer,
    /// The Producer created in the target Router.
    pub pipe_producer: Producer,
}

#[derive(Debug, Error)]
pub enum PipeProducerToRouterError {
    #[error("Destination router must be different")]
    SameRouter,
    #[error("Producer with id \"{0}\" not found")]
    ProducerNotFound(ProducerId),
    #[error("Failed to create or connect Pipe transport: \"{0}\"")]
    TransportFailed(RequestError),
    #[error("Failed to consume: \"{0}\"")]
    ConsumeFailed(ConsumeError),
    #[error("Failed to produce: \"{0}\"")]
    ProduceFailed(ProduceError),
}

impl From<RequestError> for PipeProducerToRouterError {
    fn from(error: RequestError) -> Self {
        PipeProducerToRouterError::TransportFailed(error)
    }
}

impl From<ConsumeError> for PipeProducerToRouterError {
    fn from(error: ConsumeError) -> Self {
        PipeProducerToRouterError::ConsumeFailed(error)
    }
}

impl From<ProduceError> for PipeProducerToRouterError {
    fn from(error: ProduceError) -> Self {
        PipeProducerToRouterError::ProduceFailed(error)
    }
}

pub struct PipeToRouterSourceDataProducer {
    /// The id of the DataProducer to consume.
    pub data_producer_id: DataProducerId,
}

pub struct PipeDataProducerToRouterValue {
    /// The DataConsumer created in the current Router.
    pub pipe_data_consumer: DataConsumer,
    /// The DataProducer created in the target Router.
    pub pipe_data_producer: DataProducer,
}

#[derive(Debug, Error)]
pub enum PipeDataProducerToRouterError {
    #[error("Destination router must be different")]
    SameRouter,
    #[error("Data producer with id \"{0}\" not found")]
    DataProducerNotFound(DataProducerId),
    #[error("Failed to create or connect Pipe transport: \"{0}\"")]
    TransportFailed(RequestError),
    #[error("Failed to consume: \"{0}\"")]
    ConsumeFailed(ConsumeDataError),
    #[error("Failed to produce: \"{0}\"")]
    ProduceFailed(ProduceDataError),
}

impl From<RequestError> for PipeDataProducerToRouterError {
    fn from(error: RequestError) -> Self {
        PipeDataProducerToRouterError::TransportFailed(error)
    }
}

impl From<ConsumeDataError> for PipeDataProducerToRouterError {
    fn from(error: ConsumeDataError) -> Self {
        PipeDataProducerToRouterError::ConsumeFailed(error)
    }
}

impl From<ProduceDataError> for PipeDataProducerToRouterError {
    fn from(error: ProduceDataError) -> Self {
        PipeDataProducerToRouterError::ProduceFailed(error)
    }
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct RouterDump {
    pub id: RouterId,
    pub map_consumer_id_producer_id: HashMap<ConsumerId, ProducerId>,
    pub map_data_consumer_id_data_producer_id: HashMap<ConsumerId, ProducerId>,
    pub map_data_producer_id_data_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    pub map_producer_id_consumer_ids: HashMap<ProducerId, HashSet<ConsumerId>>,
    pub map_producer_id_observer_ids: HashMap<ProducerId, HashSet<RtpObserverId>>,
    pub rtp_observer_ids: HashSet<RtpObserverId>,
    pub transport_ids: HashSet<TransportId>,
}

pub enum NewTransport<'a> {
    Direct(&'a DirectTransport),
    Pipe(&'a PipeTransport),
    Plain(&'a PlainTransport),
    WebRtc(&'a WebRtcTransport),
}

pub enum NewRtpObserver<'a> {
    AudioLevel(&'a AudioLevelObserver),
}

struct PipeTransportPair {
    local: PipeTransport,
    remote: PipeTransport,
}

#[derive(Default)]
struct Handlers {
    new_transport: Bag<'static, dyn Fn(NewTransport) + Send>,
    new_rtp_observer: Bag<'static, dyn Fn(NewRtpObserver) + Send>,
    close: Bag<'static, dyn FnOnce() + Send>,
}

struct Inner {
    id: RouterId,
    executor: Arc<Executor<'static>>,
    rtp_capabilities: RtpCapabilitiesFinalized,
    channel: Channel,
    payload_channel: PayloadChannel,
    handlers: Handlers,
    app_data: AppData,
    // TODO: RwLock instead
    producers: Arc<SyncMutex<HashMap<ProducerId, WeakProducer>>>,
    data_producers: Arc<SyncMutex<HashMap<DataProducerId, WeakDataProducer>>>,
    mapped_pipe_transports:
        Arc<SyncMutex<HashMap<RouterId, Arc<AsyncMutex<Option<PipeTransportPair>>>>>>,
    // Make sure worker is not dropped until this router is not dropped
    worker: Option<Worker>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        self.handlers.close.call_once_simple();

        {
            let channel = self.channel.clone();
            let request = RouterCloseRequest {
                internal: RouterInternal { router_id: self.id },
            };
            let worker = self.worker.take();
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("router closing failed on drop: {}", error);
                    }

                    drop(worker);
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct Router {
    inner: Arc<Inner>,
}

impl Router {
    pub(super) fn new(
        id: RouterId,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: PayloadChannel,
        rtp_capabilities: RtpCapabilitiesFinalized,
        app_data: AppData,
        worker: Worker,
    ) -> Self {
        debug!("new()");

        let producers = Arc::<SyncMutex<HashMap<ProducerId, WeakProducer>>>::default();
        let data_producers = Arc::<SyncMutex<HashMap<DataProducerId, WeakDataProducer>>>::default();
        let mapped_pipe_transports = Arc::<
            SyncMutex<HashMap<RouterId, Arc<AsyncMutex<Option<PipeTransportPair>>>>>,
        >::default();
        let handlers = Handlers::default();
        let inner = Arc::new(Inner {
            id,
            executor,
            rtp_capabilities,
            channel,
            payload_channel,
            handlers,
            producers,
            data_producers,
            mapped_pipe_transports,
            app_data,
            worker: Some(worker),
        });

        Self { inner }
    }

    /// Router id.
    pub fn id(&self) -> RouterId {
        self.inner.id
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// RTC capabilities of the Router.
    pub fn rtp_capabilities(&self) -> &RtpCapabilitiesFinalized {
        &self.inner.rtp_capabilities
    }

    /// Dump Router.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<RouterDump, RequestError> {
        debug!("dump()");

        self.inner
            .channel
            .request(RouterDumpRequest {
                internal: RouterInternal {
                    router_id: self.inner.id,
                },
            })
            .await
    }

    /// Create a DirectTransport.
    ///
    /// Router will be kept alive as long as at least one transport instance is alive.
    pub async fn create_direct_transport(
        &self,
        direct_transport_options: DirectTransportOptions,
    ) -> Result<DirectTransport, RequestError> {
        debug!("create_direct_transport()");

        let transport_id = TransportId::new();
        self.inner
            .channel
            .request(RouterCreateDirectTransportRequest {
                internal: TransportInternal {
                    router_id: self.inner.id,
                    transport_id,
                },
                data: RouterCreateDirectTransportData::from_options(&direct_transport_options),
            })
            .await?;

        let transport_fut = DirectTransport::new(
            transport_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            direct_transport_options.app_data,
            self.clone(),
        );
        let transport = transport_fut.await;

        self.inner.handlers.new_transport.call(|callback| {
            callback(NewTransport::Direct(&transport));
        });

        self.after_transport_creation(&transport);

        Ok(transport)
    }

    /// Create a WebRtcTransport.
    ///
    /// Router will be kept alive as long as at least one transport instance is alive.
    pub async fn create_webrtc_transport(
        &self,
        webrtc_transport_options: WebRtcTransportOptions,
    ) -> Result<WebRtcTransport, RequestError> {
        debug!("create_webrtc_transport()");

        let transport_id = TransportId::new();
        let data = self
            .inner
            .channel
            .request(RouterCreateWebrtcTransportRequest {
                internal: TransportInternal {
                    router_id: self.inner.id,
                    transport_id,
                },
                data: RouterCreateWebrtcTransportData::from_options(&webrtc_transport_options),
            })
            .await?;

        let transport_fut = WebRtcTransport::new(
            transport_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            data,
            webrtc_transport_options.app_data,
            self.clone(),
        );
        let transport = transport_fut.await;

        self.inner.handlers.new_transport.call(|callback| {
            callback(NewTransport::WebRtc(&transport));
        });

        self.after_transport_creation(&transport);

        Ok(transport)
    }

    /// Create a PipeTransport.
    ///
    /// Router will be kept alive as long as at least one transport instance is alive.
    pub async fn create_pipe_transport(
        &self,
        pipe_transport_options: PipeTransportOptions,
    ) -> Result<PipeTransport, RequestError> {
        debug!("create_pipe_transport()");

        let transport_id = TransportId::new();
        let data = self
            .inner
            .channel
            .request(RouterCreatePipeTransportRequest {
                internal: TransportInternal {
                    router_id: self.inner.id,
                    transport_id,
                },
                data: RouterCreatePipeTransportData::from_options(&pipe_transport_options),
            })
            .await?;

        let transport_fut = PipeTransport::new(
            transport_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            data,
            pipe_transport_options.app_data,
            self.clone(),
        );
        let transport = transport_fut.await;

        self.inner.handlers.new_transport.call(|callback| {
            callback(NewTransport::Pipe(&transport));
        });

        self.after_transport_creation(&transport);

        Ok(transport)
    }

    /// Create a PlainTransport.
    ///
    /// Router will be kept alive as long as at least one transport instance is alive.
    pub async fn create_plain_transport(
        &self,
        plain_transport_options: PlainTransportOptions,
    ) -> Result<PlainTransport, RequestError> {
        debug!("create_plain_transport()");

        let transport_id = TransportId::new();
        let data = self
            .inner
            .channel
            .request(RouterCreatePlainTransportRequest {
                internal: TransportInternal {
                    router_id: self.inner.id,
                    transport_id,
                },
                data: RouterCreatePlainTransportData::from_options(&plain_transport_options),
            })
            .await?;

        let transport_fut = PlainTransport::new(
            transport_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            data,
            plain_transport_options.app_data,
            self.clone(),
        );
        let transport = transport_fut.await;

        self.inner.handlers.new_transport.call(|callback| {
            callback(NewTransport::Plain(&transport));
        });

        self.after_transport_creation(&transport);

        Ok(transport)
    }

    /**
     * Create an AudioLevelObserver.
     */
    pub async fn create_audio_level_observer(
        &self,
        audio_level_observer_options: AudioLevelObserverOptions,
    ) -> Result<AudioLevelObserver, RequestError> {
        debug!("create_audio_level_observer()");

        let rtp_observer_id = RtpObserverId::new();
        self.inner
            .channel
            .request(RouterCreateAudioLevelObserverRequest {
                internal: RtpObserverInternal {
                    router_id: self.inner.id,
                    rtp_observer_id,
                },
                data: RouterCreateAudioLevelObserverData::from_options(
                    &audio_level_observer_options,
                ),
            })
            .await?;

        let audio_level_observer_fut = AudioLevelObserver::new(
            rtp_observer_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            audio_level_observer_options.app_data,
            self.clone(),
        );

        let audio_level_observer = audio_level_observer_fut.await;

        self.inner.handlers.new_rtp_observer.call(|callback| {
            callback(NewRtpObserver::AudioLevel(&audio_level_observer));
        });

        Ok(audio_level_observer)
    }

    /// Pipes `Producer` with the given `producer_id` into another `Router` on same host.
    pub async fn pipe_producer_to_router(
        &self,
        producer_id: ProducerId,
        pipe_to_router_options: PipeToRouterOptions,
    ) -> Result<PipeProducerToRouterValue, PipeProducerToRouterError> {
        debug!("pipe_producer_to_router()");

        let remote_router_id = pipe_to_router_options.router.id();

        if remote_router_id == self.id() {
            return Err(PipeProducerToRouterError::SameRouter);
        }

        let producer = match self
            .inner
            .producers
            .lock()
            .unwrap()
            .get(&producer_id)
            .map(|weak_producer| weak_producer.upgrade())
            .flatten()
        {
            Some(producer) => producer,
            None => return Err(PipeProducerToRouterError::ProducerNotFound(producer_id)),
        };

        // Here we may have to create a new PipeTransport pair to connect source and
        // destination Routers. We just want to keep a PipeTransport pair for each
        // pair of Routers. Since this operation is async, it may happen that two
        // simultaneous calls piping to the same router would end up generating two pairs of
        // `PipeTransport`. To prevent that, we have `HashMap` with mapping behind `Mutex` and
        // another `Mutex` on the pair of `PipeTransport`.

        let mut mapped_pipe_transports = self.inner.mapped_pipe_transports.lock().unwrap();

        let pipe_transport_pair_mutex = mapped_pipe_transports
            .entry(remote_router_id)
            .or_default()
            .clone();

        let mut pipe_transport_pair = pipe_transport_pair_mutex.lock().await;

        drop(mapped_pipe_transports);

        let pipe_transport_pair = match pipe_transport_pair.as_ref() {
            Some(pipe_transport_pair) => pipe_transport_pair,
            None => {
                pipe_transport_pair.replace(
                    self.create_pipe_transport_pair(pipe_to_router_options)
                        .await?,
                );

                pipe_transport_pair.as_ref().unwrap()
            }
        };

        let pipe_consumer = pipe_transport_pair
            .local
            .consume(ConsumerOptions::new(
                producer_id,
                RtpCapabilities::default(),
            ))
            .await?;

        let pipe_producer = pipe_transport_pair
            .remote
            .produce({
                let mut producer_options = ProducerOptions::new(
                    pipe_consumer.kind(),
                    pipe_consumer.rtp_parameters().clone(),
                );
                producer_options.id.replace(producer_id);
                producer_options.paused = pipe_consumer.producer_paused();
                producer_options.app_data = producer.app_data().clone();

                producer_options
            })
            .await?;

        // Pipe events from the pipe Consumer to the pipe Producer.
        pipe_consumer
            .on_pause({
                let executor = Arc::clone(&self.inner.executor);
                let pipe_producer_weak = pipe_producer.downgrade();

                move || {
                    if let Some(pipe_producer) = pipe_producer_weak.upgrade() {
                        executor
                            .spawn(async move {
                                let _ = pipe_producer.pause().await;
                            })
                            .detach();
                    }
                }
            })
            .detach();
        pipe_consumer
            .on_resume({
                let executor = Arc::clone(&self.inner.executor);
                let pipe_producer_weak = pipe_producer.downgrade();

                move || {
                    if let Some(pipe_producer) = pipe_producer_weak.upgrade() {
                        executor
                            .spawn(async move {
                                let _ = pipe_producer.resume().await;
                            })
                            .detach();
                    }
                }
            })
            .detach();

        // Pipe events from the pipe Producer to the pipe Consumer.
        pipe_producer
            .on_close({
                let pipe_consumer = pipe_consumer.clone();

                move || {
                    // Just ot make sure consumer on local router outlives producer on the other
                    // router
                    drop(pipe_consumer);
                }
            })
            .detach();

        Ok(PipeProducerToRouterValue {
            pipe_consumer,
            pipe_producer,
        })
    }

    /// Pipes `DataProducer` with the given `data_producer_id` into another `Router` on same host.
    pub async fn pipe_data_producer_to_router(
        &self,
        data_producer_id: DataProducerId,
        pipe_to_router_options: PipeToRouterOptions,
    ) -> Result<PipeDataProducerToRouterValue, PipeDataProducerToRouterError> {
        debug!("pipe_producer_to_router()");

        let remote_router_id = pipe_to_router_options.router.id();

        if remote_router_id == self.id() {
            return Err(PipeDataProducerToRouterError::SameRouter);
        }

        let producer = match self
            .inner
            .data_producers
            .lock()
            .unwrap()
            .get(&data_producer_id)
            .map(|weak_producer| weak_producer.upgrade())
            .flatten()
        {
            Some(producer) => producer,
            None => {
                return Err(PipeDataProducerToRouterError::DataProducerNotFound(
                    data_producer_id,
                ))
            }
        };

        // Here we may have to create a new PipeTransport pair to connect source and
        // destination Routers. We just want to keep a PipeTransport pair for each
        // pair of Routers. Since this operation is async, it may happen that two
        // simultaneous calls piping to the same router would end up generating two pairs of
        // `PipeTransport`. To prevent that, we have `HashMap` with mapping behind `Mutex` and
        // another `Mutex` on the pair of `PipeTransport`.

        let mut mapped_pipe_transports = self.inner.mapped_pipe_transports.lock().unwrap();

        let pipe_transport_pair_mutex = mapped_pipe_transports
            .entry(remote_router_id)
            .or_default()
            .clone();

        let mut pipe_transport_pair = pipe_transport_pair_mutex.lock().await;

        drop(mapped_pipe_transports);

        let pipe_transport_pair = match pipe_transport_pair.as_ref() {
            Some(pipe_transport_pair) => pipe_transport_pair,
            None => {
                pipe_transport_pair.replace(
                    self.create_pipe_transport_pair(pipe_to_router_options)
                        .await?,
                );

                pipe_transport_pair.as_ref().unwrap()
            }
        };

        let pipe_data_consumer = pipe_transport_pair
            .local
            .consume_data(DataConsumerOptions::new_sctp(data_producer_id))
            .await?;

        let pipe_data_producer = pipe_transport_pair
            .remote
            .produce_data({
                let mut producer_options = DataProducerOptions::new_sctp(
                    pipe_data_consumer.sctp_stream_parameters().unwrap(),
                );
                producer_options.id.replace(data_producer_id);
                producer_options.label = pipe_data_consumer.label().clone();
                producer_options.protocol = pipe_data_consumer.protocol().clone();
                producer_options.app_data = producer.app_data().clone();

                producer_options
            })
            .await?;

        // Pipe events from the pipe DataProducer to the pipe Consumer.
        pipe_data_producer
            .on_close({
                let pipe_data_consumer = pipe_data_consumer.clone();

                move || {
                    // Just ot make sure consumer on local router outlives producer on the other
                    // router
                    drop(pipe_data_consumer);
                }
            })
            .detach();

        Ok(PipeDataProducerToRouterValue {
            pipe_data_consumer,
            pipe_data_producer,
        })
    }

    /// Check whether the given RTP capabilities can consume the given Producer.
    pub fn can_consume(
        &self,
        producer_id: &ProducerId,
        rtp_capabilities: &RtpCapabilities,
    ) -> bool {
        match self.get_producer(producer_id) {
            Some(producer) => {
                match ortc::can_consume(producer.consumable_rtp_parameters(), rtp_capabilities) {
                    Ok(result) => result,
                    Err(error) => {
                        error!("can_consume() | unexpected error: {}", error);
                        false
                    }
                }
            }
            None => {
                error!(
                    "can_consume() | Producer with id \"{}\" not found",
                    producer_id
                );
                false
            }
        }
    }

    pub fn on_new_transport<F: Fn(NewTransport) + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.new_transport.add(Box::new(callback))
    }

    pub fn on_new_rtp_observer<F: Fn(NewRtpObserver) + Send + 'static>(
        &self,
        callback: F,
    ) -> HandlerId {
        self.inner.handlers.new_rtp_observer.add(Box::new(callback))
    }

    pub fn on_close<F: FnOnce() + Send + 'static>(&self, callback: F) -> HandlerId {
        self.inner.handlers.close.add(Box::new(callback))
    }

    async fn create_pipe_transport_pair(
        &self,
        pipe_to_router_options: PipeToRouterOptions,
    ) -> Result<PipeTransportPair, RequestError> {
        let PipeToRouterOptions {
            router,
            listen_ip,
            enable_sctp,
            num_sctp_streams,
            enable_rtx,
            enable_srtp,
        } = pipe_to_router_options;

        let remote_router_id = router.id();

        let transport_options = PipeTransportOptions {
            enable_sctp,
            num_sctp_streams,
            enable_rtx,
            enable_srtp,
            app_data: Default::default(),
            ..PipeTransportOptions::new(listen_ip.clone())
        };
        let local_pipe_transport_fut = self.create_pipe_transport(transport_options.clone());

        let remote_pipe_transport_fut = router.create_pipe_transport(transport_options);

        let (local_pipe_transport, remote_pipe_transport) =
            future::try_zip(local_pipe_transport_fut, remote_pipe_transport_fut).await?;

        let local_connect_fut = local_pipe_transport.connect({
            let (ip, port) = match remote_pipe_transport.tuple() {
                TransportTuple::LocalOnly {
                    local_ip,
                    local_port,
                    ..
                } => (local_ip, local_port),
                TransportTuple::WithRemote {
                    local_ip,
                    local_port,
                    ..
                } => (local_ip, local_port),
            };

            PipeTransportRemoteParameters {
                ip,
                port,
                srtp_parameters: remote_pipe_transport.srtp_parameters(),
            }
        });

        let remote_connect_fut = remote_pipe_transport.connect({
            let (ip, port) = match local_pipe_transport.tuple() {
                TransportTuple::LocalOnly {
                    local_ip,
                    local_port,
                    ..
                } => (local_ip, local_port),
                TransportTuple::WithRemote {
                    local_ip,
                    local_port,
                    ..
                } => (local_ip, local_port),
            };

            PipeTransportRemoteParameters {
                ip,
                port,
                srtp_parameters: local_pipe_transport.srtp_parameters(),
            }
        });

        future::try_zip(local_connect_fut, remote_connect_fut).await?;

        local_pipe_transport
            .on_close({
                let mapped_pipe_transports = Arc::clone(&self.inner.mapped_pipe_transports);

                move || {
                    mapped_pipe_transports
                        .lock()
                        .unwrap()
                        .remove(&remote_router_id);
                }
            })
            .detach();

        remote_pipe_transport
            .on_close({
                let mapped_pipe_transports = Arc::clone(&self.inner.mapped_pipe_transports);

                move || {
                    mapped_pipe_transports
                        .lock()
                        .unwrap()
                        .remove(&remote_router_id);
                }
            })
            .detach();

        Ok(PipeTransportPair {
            local: local_pipe_transport,
            remote: remote_pipe_transport,
        })
    }

    fn after_transport_creation<Dump, Stat, T>(&self, transport: &T)
    where
        T: TransportGeneric<Dump, Stat>,
    {
        {
            let producers_weak = Arc::downgrade(&self.inner.producers);
            transport
                .on_new_producer(move |producer| {
                    let producer_id = producer.id();
                    if let Some(producers) = producers_weak.upgrade() {
                        producers
                            .lock()
                            .unwrap()
                            .insert(producer_id, producer.downgrade());
                    }
                    {
                        let producers_weak = producers_weak.clone();
                        producer
                            .on_close(move || {
                                if let Some(producers) = producers_weak.upgrade() {
                                    producers.lock().unwrap().remove(&producer_id);
                                }
                            })
                            .detach();
                    }
                })
                .detach();
        }
        {
            let data_producers_weak = Arc::downgrade(&self.inner.data_producers);
            transport
                .on_new_data_producer(move |data_producer| {
                    let data_producer_id = data_producer.id();
                    if let Some(data_producers) = data_producers_weak.upgrade() {
                        data_producers
                            .lock()
                            .unwrap()
                            .insert(data_producer_id, data_producer.downgrade());
                    }
                    {
                        let data_producers_weak = data_producers_weak.clone();
                        data_producer
                            .on_close(move || {
                                if let Some(data_producers) = data_producers_weak.upgrade() {
                                    data_producers.lock().unwrap().remove(&data_producer_id);
                                }
                            })
                            .detach();
                    }
                })
                .detach();
        }
    }

    fn has_producer(&self, producer_id: &ProducerId) -> bool {
        self.inner
            .producers
            .lock()
            .unwrap()
            .contains_key(producer_id)
    }

    fn get_producer(&self, producer_id: &ProducerId) -> Option<Producer> {
        self.inner
            .producers
            .lock()
            .unwrap()
            .get(producer_id)?
            .upgrade()
    }

    fn has_data_producer(&self, data_producer_id: &DataProducerId) -> bool {
        self.inner
            .data_producers
            .lock()
            .unwrap()
            .contains_key(data_producer_id)
    }

    fn get_data_producer(&self, data_producer_id: &DataProducerId) -> Option<DataProducer> {
        self.inner
            .data_producers
            .lock()
            .unwrap()
            .get(data_producer_id)?
            .upgrade()
    }
}
