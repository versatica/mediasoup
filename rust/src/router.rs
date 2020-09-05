use crate::data_structures::AppData;
use crate::ortc::RtpCapabilities;
use crate::worker::Channel;
use std::mem;
use std::sync::Mutex;
use uuid::Uuid;

// TODO: Router ID new type

#[derive(Default)]
struct Handlers {
    closed: Mutex<Vec<Box<dyn FnOnce()>>>,
}

pub struct Router {
    id: Uuid,
    rtp_capabilities: RtpCapabilities,
    channel: Channel,
    payload_channel: Channel,
    handlers: Handlers,
    app_data: AppData,
}

impl Drop for Router {
    fn drop(&mut self) {
        let callbacks: Vec<_> = mem::take(self.handlers.closed.lock().unwrap().as_mut());
        for callback in callbacks {
            callback();
        }
    }
}

impl Router {
    pub(crate) fn new(
        id: Uuid,
        rtp_capabilities: RtpCapabilities,
        channel: Channel,
        payload_channel: Channel,
        app_data: AppData,
    ) -> Self {
        let handlers = Handlers::default();
        Self {
            id,
            rtp_capabilities,
            channel,
            payload_channel,
            handlers,
            app_data,
        }
    }

    pub fn id(&self) -> Uuid {
        self.id
    }

    pub fn connect_closed<F: FnOnce() + 'static>(&self, callback: F) {
        self.handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}
