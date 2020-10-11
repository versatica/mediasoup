use crate::data_structures::AppData;
use crate::messages::{
    DataProducerCloseRequest, DataProducerDumpRequest, DataProducerGetStatsRequest,
    DataProducerInternal,
};
use crate::sctp_parameters::SctpStreamParameters;
use crate::transport::Transport;
use crate::uuid_based_wrapper_type;
use crate::worker::{Channel, RequestError};
use async_executor::Executor;
use log::*;
use serde::{Deserialize, Serialize};
use std::mem;
use std::sync::{Arc, Mutex, Weak};

uuid_based_wrapper_type!(DataProducerId);

// TODO: Split into 2 for Direct and others or make an enum
#[derive(Debug)]
pub struct DataProducerOptions {
    /// DataProducer id (just for Router.pipeToRouter() method).
    /// DataProducer id, should most likely not be specified explicitly, specified by pipe transport
    pub(super) id: Option<DataProducerId>,
    /// SCTP parameters defining how the endpoint is sending the data.
    /// Required if SCTP/DataChannel is used.
    /// Must not be given if the data producer is created on a DirectTransport.
    pub(crate) sctp_stream_parameters: Option<SctpStreamParameters>,
    /// A label which can be used to distinguish this DataChannel from others.
    pub label: String,
    /// Name of the sub-protocol used by this DataChannel.
    pub protocol: String,
    /// Custom application data.
    pub app_data: AppData,
}

impl DataProducerOptions {
    pub fn new_sctp(sctp_stream_parameters: SctpStreamParameters) -> Self {
        Self {
            id: None,
            sctp_stream_parameters: Some(sctp_stream_parameters),
            label: "".to_string(),
            protocol: "".to_string(),
            app_data: AppData::default(),
        }
    }

    /// For DirectTransport.
    pub fn new_direct() -> Self {
        Self {
            id: None,
            sctp_stream_parameters: None,
            label: "".to_string(),
            protocol: "".to_string(),
            app_data: AppData::default(),
        }
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq, Ord, PartialOrd, Hash, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum DataProducerType {
    Sctp,
    Direct,
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct DataProducerDump {
    pub id: DataProducerId,
    pub r#type: DataProducerType,
    pub label: String,
    pub protocol: String,
    pub sctp_stream_parameters: Option<SctpStreamParameters>,
}

#[derive(Debug, Deserialize, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct DataProducerStat {
    // `type` field is present in worker, but ignored here
    pub timestamp: u64,
    pub label: String,
    pub protocol: String,
    pub messages_received: usize,
    pub bytes_received: usize,
}

#[derive(Default)]
struct Handlers {
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
    id: DataProducerId,
    r#type: DataProducerType,
    sctp_stream_parameters: Option<SctpStreamParameters>,
    label: String,
    protocol: String,
    executor: Arc<Executor<'static>>,
    channel: Channel,
    payload_channel: Channel,
    handlers: Arc<Handlers>,
    app_data: AppData,
    transport: Box<dyn Transport>,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        let callbacks: Vec<_> = mem::take(self.handlers.closed.lock().unwrap().as_mut());
        for callback in callbacks {
            callback();
        }

        {
            let channel = self.channel.clone();
            let request = DataProducerCloseRequest {
                internal: DataProducerInternal {
                    router_id: self.transport.router_id(),
                    transport_id: self.transport.id(),
                    data_producer_id: self.id,
                },
            };
            self.executor
                .spawn(async move {
                    if let Err(error) = channel.request(request).await {
                        error!("data producer closing failed on drop: {}", error);
                    }
                })
                .detach();
        }
    }
}

#[derive(Clone)]
pub struct DataProducer {
    inner: Arc<Inner>,
}

impl DataProducer {
    pub(super) async fn new(
        id: DataProducerId,
        r#type: DataProducerType,
        sctp_stream_parameters: Option<SctpStreamParameters>,
        label: String,
        protocol: String,
        executor: Arc<Executor<'static>>,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
        transport: Box<dyn Transport>,
    ) -> Self {
        debug!("new()");

        let handlers = Arc::<Handlers>::default();
        let inner = Arc::new(Inner {
            id,
            r#type,
            sctp_stream_parameters,
            label,
            protocol,
            executor,
            channel,
            payload_channel,
            handlers,
            app_data,
            transport,
        });

        Self { inner }
    }

    /// DataProducer id.
    pub fn id(&self) -> DataProducerId {
        self.inner.id
    }

    /// DataProducer type.
    pub fn r#type(&self) -> DataProducerType {
        self.inner.r#type
    }

    /// SCTP stream parameters.
    pub fn sctp_stream_parameters(&self) -> Option<SctpStreamParameters> {
        self.inner.sctp_stream_parameters
    }

    /// DataChannel label.
    pub fn label(&self) -> &String {
        &self.inner.label
    }

    /// DataChannel protocol.
    pub fn protocol(&self) -> &String {
        &self.inner.protocol
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// Dump DataProducer.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<DataProducerDump, RequestError> {
        debug!("dump()");

        self.inner
            .channel
            .request(DataProducerDumpRequest {
                internal: self.get_internal(),
            })
            .await
    }

    /// Get DataProducer stats.
    pub async fn get_stats(&self) -> Result<Vec<DataProducerStat>, RequestError> {
        debug!("get_stats()");

        self.inner
            .channel
            .request(DataProducerGetStatsRequest {
                internal: self.get_internal(),
            })
            .await
    }

    // TODO: Probably create a generic parameter on producer to make sure this method is only
    //  available when it should
    // /**
    //  * Send data (just valid for DataProducers created on a DirectTransport).
    //  */
    // send(message: string | Buffer, ppid?: number): void
    // {
    // 	if (typeof message !== 'string' && !Buffer.isBuffer(message))
    // 	{
    // 		throw new TypeError('message must be a string or a Buffer');
    // 	}
    //
    // 	/*
    // 	 * +-------------------------------+----------+
    // 	 * | Value                         | SCTP     |
    // 	 * |                               | PPID     |
    // 	 * +-------------------------------+----------+
    // 	 * | WebRTC String                 | 51       |
    // 	 * | WebRTC Binary Partial         | 52       |
    // 	 * | (Deprecated)                  |          |
    // 	 * | WebRTC Binary                 | 53       |
    // 	 * | WebRTC String Partial         | 54       |
    // 	 * | (Deprecated)                  |          |
    // 	 * | WebRTC String Empty           | 56       |
    // 	 * | WebRTC Binary Empty           | 57       |
    // 	 * +-------------------------------+----------+
    // 	 */
    //
    // 	if (typeof ppid !== 'number')
    // 	{
    // 		ppid = (typeof message === 'string')
    // 			? message.length > 0 ? 51 : 56
    // 			: message.length > 0 ? 53 : 57;
    // 	}
    //
    // 	// Ensure we honor PPIDs.
    // 	if (ppid === 56)
    // 		message = ' ';
    // 	else if (ppid === 57)
    // 		message = Buffer.alloc(1);
    //
    // 	const notifData = { ppid };
    //
    // 	this._payloadChannel.notify(
    // 		'dataProducer.send', this._internal, notifData, message);
    // }

    pub fn on_closed<F: FnOnce() + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub(super) fn downgrade(&self) -> WeakDataProducer {
        WeakDataProducer {
            inner: Arc::downgrade(&self.inner),
        }
    }

    fn get_internal(&self) -> DataProducerInternal {
        DataProducerInternal {
            router_id: self.inner.transport.router_id(),
            transport_id: self.inner.transport.id(),
            data_producer_id: self.inner.id,
        }
    }
}

#[derive(Clone)]
pub(super) struct WeakDataProducer {
    inner: Weak<Inner>,
}

impl WeakDataProducer {
    pub(super) fn upgrade(&self) -> Option<DataProducer> {
        Some(DataProducer {
            inner: self.inner.upgrade()?,
        })
    }
}
