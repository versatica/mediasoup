use crate::data_structures::AppData;
use crate::producer::ProducerId;
use crate::uuid_based_wrapper_type;
use crate::worker::RequestError;
use async_trait::async_trait;

uuid_based_wrapper_type!(RtpObserverId);

#[derive(Debug, Clone)]
#[non_exhaustive]
pub struct RtpObserverAddProducerOptions {
    /// The id of the Producer to be added.
    pub producer_id: ProducerId,
}

impl RtpObserverAddProducerOptions {
    pub fn new(producer_id: ProducerId) -> Self {
        Self { producer_id }
    }
}

#[async_trait(?Send)]
pub trait RtpObserver {
    /// RtpObserver id.
    fn id(&self) -> RtpObserverId;

    /// Whether the RtpObserver is paused.
    fn paused(&self) -> bool;

    /// App custom data.
    fn app_data(&self) -> &AppData;

    fn closed(&self) -> bool;

    /// Pause the RtpObserver.
    async fn pause(&self) -> Result<(), RequestError>;

    /// Resume the RtpObserver.
    async fn resume(&self) -> Result<(), RequestError>;

    /// Add a Producer to the RtpObserver.
    async fn add_producer(
        &self,
        rtp_observer_add_producer_options: RtpObserverAddProducerOptions,
    ) -> Result<(), RequestError>;

    /// Remove a Producer from the RtpObserver.
    async fn remove_producer(&self, producer_id: ProducerId) -> Result<(), RequestError>;
}
