// TODO: This is Unix-specific and doesn't support Windows in any way
mod channel;
mod utils;

use crate::data_structures::{WorkerLogLevel, WorkerLogTag};
use crate::worker::channel::{Channel, EventMessage, NotificationEvent};
use crate::worker::utils::SpawnResult;
use async_executor::Executor;
use async_process::{Child, Command, Stdio};
use futures_lite::io::BufReader;
use futures_lite::{future, AsyncBufReadExt, StreamExt};
use log::debug;
use log::error;
use log::warn;
use std::ffi::OsString;
use std::path::PathBuf;
use std::sync::Arc;
use std::{env, io};

#[derive(Debug, Clone)]
pub struct WorkerSettings {
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
            log_level: WorkerLogLevel::default(),
            log_tags: Vec::new(),
            rtc_min_port: 10000,
            rtc_max_port: 59999,
            dtls_certificate_file: None,
            dtls_private_key_file: None,
        }
    }
}

#[derive(Debug, Copy, Clone)]
struct WorkerResourceUsage {
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

// TODO: Drop impl
pub struct Worker {
    channel: Channel,
    payload_channel: Channel,
    child: Child,
    executor: Arc<Executor>,
    pid: u32,
}

impl Worker {
    pub(super) async fn new(
        executor: Arc<Executor>,
        worker_binary: PathBuf,
        WorkerSettings {
            log_level,
            log_tags,
            rtc_min_port,
            rtc_max_port,
            dtls_certificate_file,
            dtls_private_key_file,
        }: WorkerSettings,
    ) -> io::Result<Self> {
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
            .kill_on_drop(true)
            .env("MEDIASOUP_VERSION", env!("CARGO_PKG_VERSION"));

        let SpawnResult {
            child,
            channel,
            payload_channel,
        } = utils::spawn_with_worker_channels(&executor, &mut command)?;

        let pid = child.id();

        let mut worker = Self {
            channel,
            payload_channel,
            child,
            executor,
            pid,
        };

        worker.setup_output_forwarding();

        // TODO: Event for this
        let status = worker.child.status();
        worker
            .executor
            .spawn(async move {
                match status.await {
                    Ok(exit_status) => {
                        println!("exit status {}", exit_status);
                    }
                    Err(error) => {
                        error!("failed to spawn worker: {}", error);
                    }
                }
            })
            .detach();

        worker.wait_for_worker_process().await?;

        worker.setup_message_handling();

        Ok(worker)
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
        future::race::<io::Result<()>, _, _>(
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
        match self.channel.get_receiver().next().await {
            Some(EventMessage::Notification {
                target_id,
                event: NotificationEvent::Running,
                data: _,
            }) if target_id == self.pid.to_string() => {
                debug!("worker process running [pid:{}]", self.pid);
                Ok(())
            }
            message => Err(io::Error::new(
                io::ErrorKind::Other,
                format!("unexpected first message from worker: {:?}", message),
            )),
        }
    }

    fn setup_message_handling(&mut self) {
        let channel_receiver = self.channel.get_receiver();
        let payload_channel_receiver = self.payload_channel.get_receiver();
        let pid = self.pid;
        // TODO: Make sure these are dropped with worker
        self.executor
            .spawn(async move {
                while let Ok(message) = channel_receiver.recv().await {
                    println!("Message {:?}", message);
                    match message {
                        EventMessage::Notification {
                            target_id,
                            event,
                            data: _,
                        } => {
                            if target_id == pid.to_string() {
                                match event {
                                    NotificationEvent::Running => {
                                        error!("unexpected Running message for")
                                    }
                                }
                            } else {
                                error!("unexpected target ID {} event {:?}", target_id, event);
                            }
                        }
                        EventMessage::Debug(text) => debug!("[pid:{}] {}", pid, text),
                        EventMessage::Warn(text) => warn!("[pid:{}] {}", pid, text),
                        EventMessage::Error(text) => error!("[pid:{}] {}", pid, text),
                        EventMessage::Dump(text) => println!("{}", text),
                        EventMessage::Unexpected(data) => error!(
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

#[cfg(test)]
mod tests {
    use super::*;
    use futures_lite::future;
    use std::thread;

    fn init() {
        let _ = env_logger::builder().is_test(true).try_init();
    }

    #[test]
    fn worker_test() {
        init();

        let executor = Arc::new(Executor::new());
        let (stop_sender, stop_receiver) = async_oneshot::oneshot::<()>();
        {
            let executor = Arc::clone(&executor);
            thread::spawn(move || {
                let _ = future::block_on(executor.run(stop_receiver));
            });
        }
        let worker_settings = WorkerSettings::default();
        future::block_on(async move {
            let worker = Worker::new(
                executor,
                env::var("MEDIASOUP_WORKER_BIN")
                    .map(|path| path.into())
                    .unwrap_or_else(|_| "../worker/out/Release/mediasoup-worker".into()),
                worker_settings,
            )
            .await
            .unwrap();
        });
    }
}
