// TODO: This is Unix-specific and doesn't support Windows in any way
mod channel;
mod utils;

use crate::data_structures::{AppData, RouterInternal};
use crate::messages::{
    WorkerCreateRouterRequest, WorkerDumpRequest, WorkerGetResourceRequest,
    WorkerUpdateSettingsRequest,
};
use crate::router::{Router, RouterId, RouterOptions};
use crate::worker::utils::SpawnResult;
use crate::worker_manager::WorkerManager;
use async_executor::Executor;
use async_process::{Child, Command, ExitStatus, Stdio};
use channel::InternalMessage;
pub(crate) use channel::{Channel, RequestError};
use futures_lite::io::BufReader;
use futures_lite::{future, AsyncBufReadExt, StreamExt};
use log::debug;
use log::error;
use log::warn;
use serde::ser::Serializer;
use serde::{Deserialize, Serialize};
use std::cell::Cell;
use std::ffi::OsString;
use std::path::PathBuf;
use std::sync::{Arc, Mutex};
use std::{env, io, mem};

#[derive(Debug, Copy, Clone)]
pub enum WorkerLogLevel {
    Debug,
    Warn,
    Error,
    None,
}

impl Default for WorkerLogLevel {
    fn default() -> Self {
        Self::Error
    }
}

impl Serialize for WorkerLogLevel {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(self.as_str())
    }
}

impl WorkerLogLevel {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::Debug => "debug",
            Self::Warn => "warn",
            Self::Error => "error",
            Self::None => "none",
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub enum WorkerLogTag {
    Info,
    Ice,
    Dtls,
    Rtp,
    Srtp,
    Rtcp,
    Rtx,
    Bwe,
    Score,
    Simulcast,
    Svc,
    Sctp,
    Message,
}

impl Serialize for WorkerLogTag {
    fn serialize<S>(&self, serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        serializer.serialize_str(self.as_str())
    }
}

impl WorkerLogTag {
    pub fn as_str(&self) -> &'static str {
        match self {
            Self::Info => "info",
            Self::Ice => "ice",
            Self::Dtls => "dtls",
            Self::Rtp => "rtp",
            Self::Srtp => "srtp",
            Self::Rtcp => "rtcp",
            Self::Rtx => "rtx",
            Self::Bwe => "bwe",
            Self::Score => "score",
            Self::Simulcast => "simulcast",
            Self::Svc => "svc",
            Self::Sctp => "sctp",
            Self::Message => "message",
        }
    }
}

#[derive(Debug)]
pub struct WorkerSettings {
    pub app_data: AppData,
    /// Logging level for logs generated by the media worker subprocesses (check the Debugging
    /// documentation). Default `WorkerLogLevel::Error`
    pub log_level: WorkerLogLevel,
    /// Log tags for debugging. Check the meaning of each available tag in the Debugging
    /// documentation.
    pub log_tags: Vec<WorkerLogTag>,
    /// Minimum RTC port for ICE, DTLS, RTP, etc. Default 10000.
    pub rtc_min_port: u16,
    /// Maximum RTC port for ICE, DTLS, RTP, etc. Default 59999.
    pub rtc_max_port: u16,
    // TODO: Should these 2 be combined together?
    /// Path to the DTLS public certificate file in PEM format. If unset, a certificate is
    /// dynamically created.
    pub dtls_certificate_file: Option<PathBuf>,
    /// Path to the DTLS certificate private key file in PEM format. If unset, a certificate is
    /// dynamically created.
    pub dtls_private_key_file: Option<PathBuf>,
}

impl Default for WorkerSettings {
    fn default() -> Self {
        Self {
            app_data: AppData::default(),
            log_level: WorkerLogLevel::default(),
            log_tags: Vec::new(),
            rtc_min_port: 10000,
            rtc_max_port: 59999,
            dtls_certificate_file: None,
            dtls_private_key_file: None,
        }
    }
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
pub struct WorkerUpdateSettings {
    pub log_level: WorkerLogLevel,
    pub log_tags: Vec<WorkerLogTag>,
}

#[derive(Debug, Copy, Clone, Deserialize)]
pub struct WorkerResourceUsage {
    /// User CPU time used (in ms).
    pub ru_utime: u64,
    /// System CPU time used (in ms).
    pub ru_stime: u64,
    /// Maximum resident set size.
    pub ru_maxrss: u64,
    /// Integral shared memory size.
    pub ru_ixrss: u64,
    /// Integral unshared data size.
    pub ru_idrss: u64,
    /// Integral unshared stack size.
    pub ru_isrss: u64,
    /// Page reclaims (soft page faults).
    pub ru_minflt: u64,
    /// Page faults (hard page faults).
    pub ru_majflt: u64,
    /// Swaps.
    pub ru_nswap: u64,
    /// Block input operations.
    pub ru_inblock: u64,
    /// Block output operations.
    pub ru_oublock: u64,
    /// IPC messages sent.
    pub ru_msgsnd: u64,
    /// IPC messages received.
    pub ru_msgrcv: u64,
    /// Signals received.
    pub ru_nsignals: u64,
    /// Voluntary context switches.
    pub ru_nvcsw: u64,
    /// Involuntary context switches.
    pub ru_nivcsw: u64,
}

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
#[doc(hidden)]
pub struct WorkerDumpResponse {
    pub pid: u32,
    pub router_ids: Vec<RouterId>,
}

#[derive(Default)]
struct Handlers {
    new_router: Mutex<Vec<Box<dyn Fn(&Router) + Send>>>,
    died: Mutex<Vec<Box<dyn FnOnce(ExitStatus) + Send>>>,
    closed: Mutex<Vec<Box<dyn FnOnce() + Send>>>,
}

struct Inner {
    channel: Channel,
    payload_channel: Channel,
    child: Child,
    executor: Arc<Executor>,
    pid: u32,
    handlers: Handlers,
    app_data: AppData,
    // Make sure worker is not dropped until this worker manager is not dropped
    _worker_manager: WorkerManager,
}

impl Drop for Inner {
    fn drop(&mut self) {
        debug!("drop()");

        let callbacks: Vec<_> = mem::take(self.handlers.closed.lock().unwrap().as_mut());
        for callback in callbacks {
            callback();
        }

        if matches!(self.child.try_status(), Ok(None)) {
            unsafe {
                libc::kill(self.pid as libc::pid_t, libc::SIGTERM);
            }
        }
    }
}

impl Inner {
    async fn new(
        executor: Arc<Executor>,
        worker_binary: PathBuf,
        WorkerSettings {
            app_data,
            log_level,
            log_tags,
            rtc_min_port,
            rtc_max_port,
            dtls_certificate_file,
            dtls_private_key_file,
        }: WorkerSettings,
        worker_manager: WorkerManager,
    ) -> io::Result<Arc<Self>> {
        debug!("new()");

        let mut spawn_args: Vec<OsString> = Vec::new();
        let spawn_bin: PathBuf = match env::var("MEDIASOUP_USE_VALGRIND") {
            Ok(value) if value.as_str() == "true" => {
                let binary = match env::var("MEDIASOUP_VALGRIND_BIN") {
                    Ok(binary) => binary.into(),
                    _ => "valgrind".into(),
                };

                spawn_args.push(worker_binary.into_os_string());

                binary
            }
            _ => worker_binary,
        };

        spawn_args.push(format!("--logLevel={}", log_level.as_str()).into());
        if !log_tags.is_empty() {
            let log_tags = log_tags
                .iter()
                .map(|log_tag| log_tag.as_str())
                .collect::<Vec<_>>()
                .join(",");
            spawn_args.push(format!("--logTags={}", log_tags).into());
        }
        spawn_args.push(format!("--rtcMinPort={}", rtc_min_port).into());
        spawn_args.push(format!("--rtcMaxPort={}", rtc_max_port).into());

        if let Some(dtls_certificate_file) = dtls_certificate_file {
            let mut arg = OsString::new();
            arg.push("--dtlsCertificateFile=");
            arg.push(dtls_certificate_file);
            spawn_args.push(arg);
        }
        if let Some(dtls_private_key_file) = dtls_private_key_file {
            let mut arg = OsString::new();
            arg.push("--dtlsPrivateKeyFile=");
            arg.push(dtls_private_key_file);
            spawn_args.push(arg);
        }

        debug!(
            "spawning worker process: {} {}",
            spawn_bin.to_string_lossy(),
            spawn_args
                .iter()
                .map(|arg| arg.to_string_lossy())
                .collect::<Vec<_>>()
                .join(" ")
        );

        let mut command = Command::new(spawn_bin);
        command
            .args(spawn_args)
            .stdin(Stdio::null())
            .stdout(Stdio::piped())
            .stderr(Stdio::piped())
            .env("MEDIASOUP_VERSION", env!("CARGO_PKG_VERSION"));

        let SpawnResult {
            child,
            channel,
            payload_channel,
        } = utils::spawn_with_worker_channels(Arc::clone(&executor), &mut command)?;

        let pid = child.id();
        let handlers = Handlers::default();

        let mut inner = Self {
            channel,
            payload_channel,
            child,
            executor,
            pid,
            handlers,
            app_data,
            _worker_manager: worker_manager,
        };

        inner.setup_output_forwarding();

        inner.wait_for_worker_process().await?;

        inner.setup_message_handling();

        let status_fut = inner.child.status();
        let inner = Arc::new(inner);
        {
            let inner_weak = Arc::downgrade(&inner);
            inner
                .executor
                .spawn(async move {
                    let status = status_fut.await;

                    if let Some(inner) = inner_weak.upgrade() {
                        if let Ok(exit_status) = status {
                            warn!("exit status {}", exit_status);

                            // TODO: Probably propagate this down as router/transport/producer
                            //  /consumer events
                            let callbacks: Vec<_> =
                                mem::take(inner.handlers.died.lock().unwrap().as_mut());
                            for callback in callbacks {
                                callback(exit_status);
                            }
                        }
                    }
                })
                .detach();
        }

        Ok(inner)
    }

    fn setup_output_forwarding(&mut self) {
        let stdout = self.child.stdout.take().unwrap();
        self.executor
            .spawn(async move {
                let mut lines = BufReader::new(stdout).lines();
                while let Some(Ok(line)) = lines.next().await {
                    debug!("(stdout) {}", line);
                }
            })
            .detach();

        let stderr = self.child.stderr.take().unwrap();
        self.executor
            .spawn(async move {
                let mut lines = BufReader::new(stderr).lines();
                while let Some(Ok(line)) = lines.next().await {
                    error!("(stderr) {}", line);
                }
            })
            .detach();
    }

    async fn wait_for_worker_process(&mut self) -> io::Result<()> {
        let status = self.child.status();
        future::or(
            async move {
                status.await?;
                Err(io::Error::new(
                    io::ErrorKind::NotFound,
                    "worker process exited before being ready",
                ))
            },
            self.wait_for_worker_ready(),
        )
        .await
    }

    async fn wait_for_worker_ready(&mut self) -> io::Result<()> {
        let (sender, receiver) = async_oneshot::oneshot();
        let pid = self.pid;
        let sender = Cell::new(Some(sender));
        let _handler = self
            .channel
            .subscribe_to_notifications(self.pid.to_string(), move |notification| {
                let result = match serde_json::from_value::<String>(notification.event.clone()) {
                    Ok(event) => {
                        if event == "running".to_string() {
                            debug!("worker process running [pid:{}]", pid);
                            Ok(())
                        } else {
                            Err(io::Error::new(
                                io::ErrorKind::Other,
                                format!(
                                    "unexpected first notification from worker [pid:{}]: {:?}",
                                    pid, notification
                                ),
                            ))
                        }
                    }
                    Err(error) => Err(io::Error::new(
                        io::ErrorKind::Other,
                        format!(
                            "unexpected first notification from worker [pid:{}]: {:?}; error = {}",
                            pid, notification, error
                        ),
                    )),
                };
                drop(
                    sender
                        .take()
                        .take()
                        .expect("Receiving more than one worker notification")
                        .send(result),
                );
            })
            .await
            .unwrap();

        receiver.await.unwrap()
    }

    fn setup_message_handling(&mut self) {
        let channel_receiver = self.channel.get_internal_message_receiver();
        let payload_channel_receiver = self.payload_channel.get_internal_message_receiver();
        let pid = self.pid;
        self.executor
            .spawn(async move {
                while let Ok(message) = channel_receiver.recv().await {
                    match message {
                        InternalMessage::Debug(text) => debug!("[pid:{}] {}", pid, text),
                        InternalMessage::Warn(text) => warn!("[pid:{}] {}", pid, text),
                        InternalMessage::Error(text) => error!("[pid:{}] {}", pid, text),
                        InternalMessage::Dump(text) => println!("{}", text),
                        InternalMessage::Unexpected(data) => error!(
                            "worker[pid:{}] unexpected data: {}",
                            pid,
                            String::from_utf8_lossy(&data)
                        ),
                    }
                }
            })
            .detach();

        self.executor
            .spawn(async move {
                while let Ok(message) = payload_channel_receiver.recv().await {
                    println!("Message {:?}", message);
                }
            })
            .detach();
    }
}

#[derive(Clone)]
pub struct Worker {
    inner: Arc<Inner>,
}

impl Worker {
    pub(super) async fn new(
        executor: Arc<Executor>,
        worker_binary: PathBuf,
        worker_settings: WorkerSettings,
        worker_manager: WorkerManager,
    ) -> io::Result<Self> {
        let inner = Inner::new(executor, worker_binary, worker_settings, worker_manager).await?;

        Ok(Self { inner })
    }

    /// Worker process identifier (PID).
    pub fn pid(&self) -> u32 {
        self.inner.pid
    }

    /// App custom data.
    pub fn app_data(&self) -> &AppData {
        &self.inner.app_data
    }

    /// Dump Worker.
    #[doc(hidden)]
    pub async fn dump(&self) -> Result<WorkerDumpResponse, RequestError> {
        debug!("dump()");

        self.inner.channel.request(WorkerDumpRequest {}).await
    }

    /// Get mediasoup-worker process resource usage.
    pub async fn get_resource_usage(&self) -> Result<WorkerResourceUsage, RequestError> {
        debug!("get_resource_usage()");

        self.inner
            .channel
            .request(WorkerGetResourceRequest {})
            .await
    }

    /// Update settings.
    pub async fn update_settings(&self, data: WorkerUpdateSettings) -> Result<(), RequestError> {
        debug!("update_settings()");

        self.inner
            .channel
            .request(WorkerUpdateSettingsRequest { data })
            .await
    }

    /// Create a Router.
    ///
    /// Worker will be kept alive as long as at least one router is alive.
    pub async fn create_router(
        &self,
        router_options: RouterOptions,
    ) -> Result<Router, RequestError> {
        debug!("create_router()");

        let router_id = RouterId::new();
        let internal = RouterInternal { router_id };

        self.inner
            .channel
            .request(WorkerCreateRouterRequest { internal })
            .await?;

        let router = Router::new(
            router_id,
            Arc::clone(&self.inner.executor),
            self.inner.channel.clone(),
            self.inner.payload_channel.clone(),
            router_options,
            self.clone(),
        );

        for callback in self.inner.handlers.new_router.lock().unwrap().iter() {
            callback(&router);
        }

        Ok(router)
    }

    pub fn connect_new_router<F: Fn(&Router) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .new_router
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_died<F: FnOnce(ExitStatus) + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .died
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }

    pub fn connect_closed<F: FnOnce() + Send + 'static>(&self, callback: F) {
        self.inner
            .handlers
            .closed
            .lock()
            .unwrap()
            .push(Box::new(callback));
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::data_structures::TransportListenIp;
    use crate::transport::Transport;
    use crate::webrtc_transport::{TransportListenIps, WebRtcTransportOptions};
    use futures_lite::future;
    use std::thread;

    fn init() {
        let _ = env_logger::builder().is_test(true).try_init();
    }

    #[test]
    fn worker_test() {
        init();

        let executor = Arc::new(Executor::new());
        let (_stop_sender, stop_receiver) = async_oneshot::oneshot::<()>();
        {
            let executor = Arc::clone(&executor);
            thread::spawn(move || {
                let _ = future::block_on(executor.run(stop_receiver));
            });
        }
        let worker_settings = WorkerSettings::default();
        let worker_binary: PathBuf = env::var("MEDIASOUP_WORKER_BIN")
            .map(Into::into)
            .unwrap_or_else(|_| "../worker/out/Release/mediasoup-worker".into());
        future::block_on(async move {
            let worker = Worker::new(
                executor,
                worker_binary.clone(),
                worker_settings,
                WorkerManager::new(worker_binary),
            )
            .await
            .unwrap();

            println!("Worker dump: {:#?}", worker.dump().await);
            println!("Resource usage: {:#?}", worker.get_resource_usage().await);
            println!(
                "Update settings: {:?}",
                worker
                    .update_settings(WorkerUpdateSettings {
                        log_level: WorkerLogLevel::Debug,
                        log_tags: Vec::new(),
                    })
                    .await
            );

            let router = worker
                .create_router(RouterOptions::default())
                .await
                .unwrap();
            println!("Router created: {:?}", router.id());
            println!("Router dump: {:#?}", router.dump().await.unwrap());
            let webrtc_transport = router
                .create_webrtc_transport(WebRtcTransportOptions::new(TransportListenIps::new(
                    TransportListenIp {
                        ip: "127.0.0.1".to_string(),
                        announced_ip: None,
                    },
                )))
                .await
                .unwrap();
            println!("WebRTC transport created: {:?}", webrtc_transport.id());
            println!(
                "WebRTC transport stats: {:#?}",
                webrtc_transport.get_stats().await.unwrap()
            );
            println!(
                "WebRTC transport dump: {:#?}",
                webrtc_transport.dump().await.unwrap()
            );

            // Just to give it time to finish everything with router destruction
            thread::sleep(std::time::Duration::from_millis(200));
        });

        // Just to give it time to finish everything
        thread::sleep(std::time::Duration::from_millis(200));
    }
}
