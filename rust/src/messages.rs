use serde::de::DeserializeOwned;
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::fmt::Debug;
use uuid::Uuid;

pub(crate) trait Request: Debug + Serialize {
    type Response: DeserializeOwned;

    fn as_method(&self) -> &'static str;
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct WorkerDumpResponse {
    pub pid: u32,
    pub router_ids: Vec<Uuid>,
}

#[derive(Debug, Serialize)]
pub struct WorkerDumpRequest;

impl Request for WorkerDumpRequest {
    type Response = WorkerDumpResponse;

    fn as_method(&self) -> &'static str {
        "worker.dump"
    }
}
