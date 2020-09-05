// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use crate::worker::channel::Channel;
use async_executor::Executor;
use async_process::unix::CommandExt;
use async_process::{Child, Command};
use nix::unistd;
use std::fs::File;
use std::io;
use std::os::unix::io::FromRawFd;

pub(super) struct SpawnResult {
    pub(super) child: Child,
    pub(super) channel: Channel,
    pub(super) payload_channel: Channel,
}

pub(super) fn spawn_with_worker_channels(
    executor: &Executor,
    command: &mut Command,
) -> io::Result<SpawnResult> {
    let (producer_fd_read, producer_fd_write) = unistd::pipe().unwrap();
    let (consumer_fd_read, consumer_fd_write) = unistd::pipe().unwrap();
    let (producer_payload_fd_read, producer_payload_fd_write) = unistd::pipe().unwrap();
    let (consumer_payload_fd_read, consumer_payload_fd_write) = unistd::pipe().unwrap();

    unsafe {
        command.pre_exec(move || {
            // Unused in child
            unistd::close(producer_fd_write).unwrap();
            unistd::close(consumer_fd_read).unwrap();
            unistd::close(producer_payload_fd_write).unwrap();
            unistd::close(consumer_payload_fd_read).unwrap();
            // Now duplicate into file descriptor indexes we need
            if producer_fd_read != 3 {
                unistd::dup2(producer_fd_read, 3).unwrap();
                unistd::close(producer_fd_read).unwrap();
            }
            if consumer_fd_write != 4 {
                unistd::dup2(consumer_fd_write, 4).unwrap();
                unistd::close(consumer_fd_write).unwrap();
            }
            if producer_payload_fd_read != 5 {
                unistd::dup2(producer_payload_fd_read, 5).unwrap();
                unistd::close(producer_payload_fd_read).unwrap();
            }
            if consumer_payload_fd_write != 6 {
                unistd::dup2(consumer_payload_fd_write, 6).unwrap();
                unistd::close(consumer_payload_fd_write).unwrap();
            }

            Ok(())
        });
    };

    let producer_file = unsafe { File::from_raw_fd(producer_fd_write) };
    let consumer_file = unsafe { File::from_raw_fd(consumer_fd_read) };
    let producer_payload_file = unsafe { File::from_raw_fd(producer_payload_fd_write) };
    let consumer_payload_file = unsafe { File::from_raw_fd(consumer_payload_fd_read) };

    let child = command.spawn()?;

    // Unused in parent
    unistd::close(producer_fd_read).expect("Failed to close fd");
    unistd::close(consumer_fd_write).expect("Failed to close fd");
    unistd::close(producer_payload_fd_read).expect("Failed to close fd");
    unistd::close(consumer_payload_fd_write).expect("Failed to close fd");

    Ok(SpawnResult {
        child,
        channel: Channel::new(&executor, consumer_file.into(), producer_file.into()),
        payload_channel: Channel::new(
            &executor,
            consumer_payload_file.into(),
            producer_payload_file.into(),
        ),
    })
}
