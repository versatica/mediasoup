use crate::data_structures::AppData;
use crate::uuid_based_wrapper_type;
use crate::worker::RequestError;
use async_trait::async_trait;

uuid_based_wrapper_type!(TransportId);

#[async_trait]
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

    fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F);
    // TODO
}
