use crate::ortc::RtpCapabilities;
use crate::worker::Channel;
use async_channel::Receiver;
use std::borrow::BorrowMut;
use std::mem;
use std::sync::{Arc, Mutex};
use uuid::Uuid;

// TODO: Router ID new type

pub struct Router {
    id: Uuid,
    rtp_capabilities: RtpCapabilities,
    channel: Channel,
    payload_channel: Channel,
    closed_handlers: Mutex<Vec<Box<dyn FnOnce(&Router)>>>,
}

impl Router {
    pub(crate) fn new(
        id: Uuid,
        rtp_capabilities: RtpCapabilities,
        channel: Channel,
        payload_channel: Channel,
    ) -> Self {
        let closed_handlers = Mutex::default();
        Self {
            id,
            rtp_capabilities,
            channel,
            payload_channel,
            closed_handlers,
        }
    }

    pub fn id(&self) -> Uuid {
        self.id
    }

    // TODO: Add protection such that methods that most methods can't be called on router from
    //  closed callback
    pub fn connect_closed<F>(&self, callback: F)
    where
        F: FnOnce(&Self) + 'static,
    {
        self.closed_handlers
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}

impl Drop for Router {
    fn drop(&mut self) {
        let callbacks: Vec<_> = mem::take(self.closed_handlers.lock().unwrap().as_mut());
        for callback in callbacks {
            callback(self);
        }
    }
}
