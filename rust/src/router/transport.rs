use crate::data_structures::AppData;
use crate::uuid_based_wrapper_type;

uuid_based_wrapper_type!(TransportId);

pub trait Transport {
    /// Transport id.
    fn id(&self) -> TransportId;

    /// App custom data.
    fn app_data(&self) -> &AppData;

    fn connect_closed<F: FnOnce() + 'static>(&self, callback: F)
    where
        Self: Transport;
    // TODO
}
