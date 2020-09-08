use crate::data_structures::AppData;
use crate::uuid_based_wrapper_type;
use crate::worker::RequestError;
use async_trait::async_trait;

uuid_based_wrapper_type!(TransportId);

#[async_trait]
pub trait Transport<Dump, Stat> {
    /// Transport id.
    fn id(&self) -> TransportId;

    /// App custom data.
    fn app_data(&self) -> &AppData;

    /// Dump Transport.
    async fn dump(&self) -> Result<Dump, RequestError>;

    /// Get Transport stats.
    async fn get_stats(&self) -> Result<Vec<Stat>, RequestError>;

    fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F);
    // TODO
}
