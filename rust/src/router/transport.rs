use crate::data_structures::{AppData, TransportInternal};
use crate::messages::{
    TransportDumpRequest, TransportGetStatsRequest, TransportSetMaxIncomingBitrateData,
    TransportSetMaxIncomingBitrateRequest,
};
use crate::ortc::RtpParametersError;
use crate::producer::{Producer, ProducerId, ProducerOptions};
use crate::router::RouterId;
use crate::worker::{Channel, RequestError};
use crate::{ortc, uuid_based_wrapper_type};
use async_trait::async_trait;
use log::debug;
use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fmt::Debug;
use std::marker::PhantomData;
use thiserror::Error;

uuid_based_wrapper_type!(TransportId);

#[derive(Debug, Copy, Clone, Deserialize, Serialize)]
#[serde(rename_all = "lowercase")]
pub enum Direction {
    In,
    Out,
}

#[derive(Debug, Clone, Deserialize, Serialize)]
#[serde(tag = "type", rename_all = "lowercase")]
pub enum TransportTraceEventData {
    Probation {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: Direction,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
    Bwe {
        /// Event timestamp.
        timestamp: u64,
        /// Event direction.
        direction: Direction,
        // TODO: Clarify value structure
        /// Per type information.
        info: Value,
    },
}

#[async_trait(?Send)]
pub trait Transport<Dump, Stat, RemoteParameters> {
    /// Transport id.
    fn id(&self) -> TransportId;

    /// App custom data.
    fn app_data(&self) -> &AppData;

    /// Dump Transport.
    async fn dump(&self) -> Result<Dump, RequestError>;

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<Stat>, RequestError>;

    /// Provide the Transport remote parameters.
    async fn connect(&self, remote_parameters: RemoteParameters) -> Result<(), RequestError>;

    async fn set_max_incoming_bitrate(&self, bitrate: u32) -> Result<(), RequestError>;

    async fn produce(&self, producer_options: ProducerOptions) -> Result<Producer, ProduceError>;

    fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F);
    // TODO
}

#[derive(Debug, Error)]
pub enum ProduceError {
    #[error("Producer with ID {0} already exists")]
    AlreadyExists(ProducerId),
    #[error("Incorrect RTP parameters: {0}")]
    RtpParametersError(RtpParametersError),
    #[error("Request to worker failed: {0}")]
    RequestError(RequestError),
}

#[async_trait(?Send)]
pub(super) trait TransportImpl<Dump, Stat, RemoteParameters>:
    Transport<Dump, Stat, RemoteParameters>
where
    Dump: Debug + DeserializeOwned + 'static,
    Stat: Debug + DeserializeOwned + 'static,
    RemoteParameters: 'static,
{
    fn router_id(&self) -> RouterId;

    fn channel(&self) -> &Channel;

    fn has_producer(&self, id: &ProducerId) -> bool;

    async fn dump_impl(&self) -> Result<Dump, RequestError> {
        self.channel()
            .request(TransportDumpRequest {
                internal: TransportInternal {
                    router_id: self.router_id(),
                    transport_id: self.id(),
                },
                phantom_data: PhantomData {},
            })
            .await
    }

    async fn get_stats_impl(&self) -> Result<Vec<Stat>, RequestError> {
        self.channel()
            .request(TransportGetStatsRequest {
                internal: TransportInternal {
                    router_id: self.router_id(),
                    transport_id: self.id(),
                },
                phantom_data: PhantomData {},
            })
            .await
    }

    async fn set_max_incoming_bitrate_impl(&self, bitrate: u32) -> Result<(), RequestError> {
        self.channel()
            .request(TransportSetMaxIncomingBitrateRequest {
                internal: TransportInternal {
                    router_id: self.router_id(),
                    transport_id: self.id(),
                },
                data: TransportSetMaxIncomingBitrateData { bitrate },
            })
            .await
    }

    async fn produce_impl(
        &self,
        producer_options: ProducerOptions,
    ) -> Result<Producer, ProduceError> {
        if let Some(id) = &producer_options.id {
            if self.has_producer(id) {
                return Err(ProduceError::AlreadyExists(*id));
            }
        }

        ortc::validate_rtp_parameters(&producer_options.rtp_parameters)
            .map_err(ProduceError::RtpParametersError)?;

        todo!()
    }
}
