use std::error::Error;
use thiserror::Error;

#[derive(Debug, Error)]
pub enum RequestError {
    #[error("Channel already closed")]
    ChannelClosed,
    #[error("Message is too long")]
    MessageTooLong,
    #[error("Payload is too long")]
    PayloadTooLong,
    #[error("Request timed out")]
    TimedOut,
    // TODO: Enum?
    #[error("Received response error: {reason}")]
    Response { reason: String },
    #[error("Failed to parse response from worker: {error}")]
    FailedToParse {
        #[from]
        error: Box<dyn Error>,
    },
    #[error("Worker did not return any data in response")]
    NoData,
}
