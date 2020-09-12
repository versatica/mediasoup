use crate::data_structures::AppData;
use crate::router::RouterId;
use crate::rtp_parameters::{MediaKind, RtpParameters};
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::Channel;
use async_executor::Executor;
use serde::{Deserialize, Serialize};
use std::sync::Arc;

uuid_based_wrapper_type!(ProducerId);

#[derive(Debug)]
#[non_exhaustive]
pub struct ProducerOptions {
    /// Producer id, should most likely not be specified explicitly, specified by plain transport
    #[doc(hidden)]
    pub id: Option<ProducerId>,
    pub kind: MediaKind,
    // TODO: Docs have distinction between RtpSendParameters and RtpReceiveParameters
    pub rtp_parameters: RtpParameters,
    pub paused: bool,
    pub key_frame_request_delay: u32,
    pub app_data: AppData,
}

impl ProducerOptions {
    pub fn new(kind: MediaKind, rtp_parameters: RtpParameters) -> Self {
        Self {
            id: None,
            kind,
            rtp_parameters,
            paused: false,
            key_frame_request_delay: 0,
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum ProducerType {
    None,
    Simple,
    Simulcast,
    SVC,
    Pipe,
}

struct Inner {
    id: ProducerId,
    kind: MediaKind,
    r#type: ProducerType,
    rtp_parameters: RtpParameters,
    consumable_rtp_parameters: RtpParameters,
    paused: bool,
    router_id: RouterId,
    executor: Arc<Executor>,
    channel: Channel,
    payload_channel: Channel,
    app_data: AppData,
    transport: Box<dyn Transport>,
}

#[derive(Clone)]
pub struct Producer {
    inner: Arc<Inner>,
}

impl Producer {
    pub(super) fn new(
        id: ProducerId,
        kind: MediaKind,
        r#type: ProducerType,
        rtp_parameters: RtpParameters,
        consumable_rtp_parameters: RtpParameters,
        paused: bool,
        router_id: RouterId,
        executor: Arc<Executor>,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
        transport: Box<dyn Transport>,
    ) -> Self {
        let inner = Arc::new(Inner {
            id,
            kind,
            r#type,
            rtp_parameters,
            consumable_rtp_parameters,
            paused,
            router_id,
            executor,
            channel,
            payload_channel,
            app_data,
            transport,
        });

        Self { inner }
    }

    /// Producer id.
    pub fn id(&self) -> ProducerId {
        self.inner.id
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }
}
