// Contents of this module is inspired by https://github.com/Srinivasa314/alcro/tree/master/src/chrome
use async_process::unix::CommandExt;
use async_process::Command;
use std::error::Error;
use std::fs::File;
use std::io::{BufRead, BufReader, Write};
use std::os::raw::c_int;
use std::os::unix::io::FromRawFd;

pub struct WorkerChannel {
    reader_pipe: BufReader<File>,
    writer_pipe: File,
}

impl WorkerChannel {
    fn new(reader: File, writer: File) -> Self {
        Self {
            reader_pipe: BufReader::new(reader),
            writer_pipe: writer,
        }
    }

    pub fn read(&mut self) -> Result<String, Box<dyn Error>> {
        let mut bytes_to_read = vec![];
        self.reader_pipe.read_until(0, &mut bytes_to_read)?;
        bytes_to_read.pop();
        Ok(String::from_utf8(bytes_to_read)?)
    }

    pub fn write(&mut self, message: String) -> Result<usize, Box<dyn Error>> {
        Ok(self
            .writer_pipe
            .write(std::ffi::CString::new(message)?.as_bytes_with_nul())?)
    }
}

pub struct WorkerChannels {
    pub channel: WorkerChannel,
    pub payload_channel: WorkerChannel,
}

pub fn setup_worker_channels(command: &mut Command) -> WorkerChannels {
    const READ_END: usize = 0;
    const WRITE_END: usize = 1;

    let mut producer_channel: [c_int; 2] = [0; 2];
    let mut consumer_channel: [c_int; 2] = [0; 2];
    let mut producer_payload_channel: [c_int; 2] = [0; 2];
    let mut consumer_payload_channel: [c_int; 2] = [0; 2];

    unsafe {
        libc::pipe(producer_channel.as_mut_ptr());
        libc::pipe(consumer_channel.as_mut_ptr());
        libc::pipe(producer_payload_channel.as_mut_ptr());
        libc::pipe(consumer_payload_channel.as_mut_ptr());
    }

    unsafe {
        command.pre_exec(move || {
            libc::dup2(producer_channel[READ_END], 3);
            libc::dup2(consumer_channel[WRITE_END], 4);
            libc::dup2(producer_payload_channel[READ_END], 5);
            libc::dup2(consumer_payload_channel[WRITE_END], 6);

            Ok(())
        });
    };

    let channel;
    let payload_channel;
    unsafe {
        libc::close(producer_channel[READ_END]);
        libc::close(consumer_channel[WRITE_END]);
        libc::close(producer_payload_channel[READ_END]);
        libc::close(consumer_payload_channel[WRITE_END]);
        channel = WorkerChannel::new(
            File::from_raw_fd(producer_channel[WRITE_END]),
            File::from_raw_fd(consumer_channel[READ_END]),
        );
        payload_channel = WorkerChannel::new(
            File::from_raw_fd(producer_payload_channel[WRITE_END]),
            File::from_raw_fd(consumer_payload_channel[READ_END]),
        );
    }

    WorkerChannels {
        channel,
        payload_channel,
    }
}
