use crate::data_structures::{AppData, TransportId};

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
