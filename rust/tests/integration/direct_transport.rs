use futures_lite::future;
use hash_hasher::HashedSet;
use mediasoup::data_consumer::DataConsumerOptions;
use mediasoup::data_producer::{DataProducer, DataProducerOptions};
use mediasoup::data_structures::{AppData, WebRtcMessage};
use mediasoup::direct_transport::{DirectTransport, DirectTransportOptions};
use mediasoup::prelude::*;
use mediasoup::router::{Router, RouterOptions};
use mediasoup::worker::{Worker, WorkerSettings};
use mediasoup::worker_manager::WorkerManager;
use parking_lot::Mutex;
use std::borrow::Cow;
use std::env;
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;

struct CustomAppData {
    foo: &'static str,
}

async fn init() -> (Worker, Router, DirectTransport) {
    {
        let mut builder = env_logger::builder();
        if env::var(env_logger::DEFAULT_FILTER_ENV).is_err() {
            builder.filter_level(log::LevelFilter::Off);
        }
        let _ = builder.is_test(true).try_init();
    }

    let worker_manager = WorkerManager::new();

    let worker = worker_manager
        .create_worker(WorkerSettings::default())
        .await
        .expect("Failed to create worker");

    let router = worker
        .create_router(RouterOptions::default())
        .await
        .expect("Failed to create router");

    let transport = router
        .create_direct_transport(DirectTransportOptions::default())
        .await
        .expect("Failed to create transport1");

    (worker, router, transport)
}

#[test]
fn create_succeeds() {
    future::block_on(async move {
        let (_worker, router, transport) = init().await;

        {
            let dump = router.dump().await.expect("Failed to dump router");

            assert_eq!(dump.transport_ids, {
                let mut set = HashedSet::default();
                set.insert(transport.id());
                set
            });
        }

        let new_transports_count = Arc::new(AtomicUsize::new(0));

        router
            .on_new_transport({
                let new_transports_count = Arc::clone(&new_transports_count);

                move |_transport| {
                    new_transports_count.fetch_add(1, Ordering::SeqCst);
                }
            })
            .detach();

        let transport1 = router
            .create_direct_transport({
                let mut direct_transport_options = DirectTransportOptions::default();
                direct_transport_options.max_message_size = 1024;
                direct_transport_options.app_data = AppData::new(CustomAppData { foo: "bar" });

                direct_transport_options
            })
            .await
            .expect("Failed to create Direct transport");

        assert_eq!(new_transports_count.load(Ordering::SeqCst), 1);
        assert!(!transport1.closed());
        assert_eq!(
            transport1
                .app_data()
                .downcast_ref::<CustomAppData>()
                .unwrap()
                .foo,
            "bar",
        );

        {
            let dump = transport1.dump().await.expect("Failed to dump transport");

            assert_eq!(dump.id, transport1.id());
            assert!(dump.direct);
            assert_eq!(dump.producer_ids, vec![]);
            assert_eq!(dump.consumer_ids, vec![]);
            assert_eq!(dump.data_producer_ids, vec![]);
            assert_eq!(dump.data_consumer_ids, vec![]);
        }
    });
}

#[test]
fn weak() {
    future::block_on(async move {
        let (_worker, _router, transport) = init().await;

        let weak_transport = transport.downgrade();

        assert!(weak_transport.upgrade().is_some());

        drop(transport);

        assert!(weak_transport.upgrade().is_none());
    });
}

#[test]
fn get_stats_succeeds() {
    future::block_on(async move {
        let (_worker, _router, transport) = init().await;

        let stats = transport
            .get_stats()
            .await
            .expect("Failed to get stats on Direct transport");

        assert_eq!(stats.len(), 1);
        assert_eq!(stats[0].transport_id, transport.id());
        assert_eq!(stats[0].bytes_received, 0);
        assert_eq!(stats[0].recv_bitrate, 0);
        assert_eq!(stats[0].bytes_sent, 0);
        assert_eq!(stats[0].send_bitrate, 0);
        assert_eq!(stats[0].rtp_bytes_received, 0);
        assert_eq!(stats[0].rtp_recv_bitrate, 0);
        assert_eq!(stats[0].rtp_bytes_sent, 0);
        assert_eq!(stats[0].rtp_send_bitrate, 0);
        assert_eq!(stats[0].rtx_bytes_received, 0);
        assert_eq!(stats[0].rtx_recv_bitrate, 0);
        assert_eq!(stats[0].rtx_bytes_sent, 0);
        assert_eq!(stats[0].rtx_send_bitrate, 0);
        assert_eq!(stats[0].probation_bytes_sent, 0);
        assert_eq!(stats[0].probation_send_bitrate, 0);
        assert_eq!(stats[0].rtp_packet_loss_received, None);
        assert_eq!(stats[0].rtp_packet_loss_sent, None);
    });
}

#[test]
fn send_succeeds() {
    future::block_on(async move {
        let (_worker, _router, transport) = init().await;

        let data_producer = transport
            .produce_data({
                let mut options = DataProducerOptions::new_direct();

                options.label = "foo".to_string();
                options.protocol = "bar".to_string();
                options.app_data = AppData::new(CustomAppData { foo: "bar" });

                options
            })
            .await
            .expect("Failed to produce data");

        let data_consumer = transport
            .consume_data(DataConsumerOptions::new_direct(data_producer.id()))
            .await
            .expect("Failed to consume data");

        let num_messages = 200_usize;
        let mut sent_message_bytes = 0_usize;
        let recv_message_bytes = Arc::new(AtomicUsize::new(0));
        let mut last_sent_message_id = 0_usize;
        let last_recv_message_id = Arc::new(AtomicUsize::new(0));

        let (received_messages_tx, received_messages_rx) = async_oneshot::oneshot::<()>();
        let _handler = data_consumer.on_message({
            let received_messages_tx = Mutex::new(Some(received_messages_tx));
            let recv_message_bytes = Arc::clone(&recv_message_bytes);
            let last_recv_message_id = Arc::clone(&last_recv_message_id);

            move |message| {
                let id: usize = match message {
                    WebRtcMessage::String(string) => {
                        recv_message_bytes.fetch_add(string.as_bytes().len(), Ordering::SeqCst);
                        string.parse().unwrap()
                    }
                    WebRtcMessage::Binary(binary) => {
                        recv_message_bytes.fetch_add(binary.len(), Ordering::SeqCst);
                        String::from_utf8(binary.to_vec()).unwrap().parse().unwrap()
                    }
                    WebRtcMessage::EmptyString => {
                        panic!("Unexpected empty messages!");
                    }
                    WebRtcMessage::EmptyBinary => {
                        panic!("Unexpected empty messages!");
                    }
                };

                if id < num_messages / 2 {
                    assert!(matches!(message, &WebRtcMessage::String(_)));
                } else {
                    assert!(matches!(message, &WebRtcMessage::Binary(_)));
                }

                last_recv_message_id.fetch_add(1, Ordering::SeqCst);
                assert_eq!(id, last_recv_message_id.load(Ordering::SeqCst));

                if id == num_messages {
                    let _ = received_messages_tx.lock().take().unwrap().send(());
                }
            }
        });

        let direct_data_producer = match &data_producer {
            DataProducer::Direct(direct_data_producer) => direct_data_producer,
            _ => {
                panic!("Expected direct data producer")
            }
        };

        loop {
            last_sent_message_id += 1;
            let id = last_sent_message_id;

            let message = if id < num_messages / 2 {
                let content = id.to_string();
                sent_message_bytes += content.len();
                WebRtcMessage::String(content)
            } else {
                let content = id.to_string().into_bytes();
                sent_message_bytes += content.len();
                WebRtcMessage::Binary(Cow::from(content))
            };

            direct_data_producer
                .send(message)
                .expect("Failed to send message");

            if id == num_messages {
                break;
            }
        }

        received_messages_rx
            .await
            .expect("Failed tor receive all messages");

        assert_eq!(last_sent_message_id, num_messages);
        assert_eq!(last_recv_message_id.load(Ordering::SeqCst), num_messages);
        assert_eq!(
            recv_message_bytes.load(Ordering::SeqCst),
            sent_message_bytes,
        );

        {
            let stats = data_producer
                .get_stats()
                .await
                .expect("Failed to get stats on data producer");

            assert_eq!(stats.len(), 1);
            assert_eq!(&stats[0].label, data_producer.label());
            assert_eq!(&stats[0].protocol, data_producer.protocol());
            assert_eq!(stats[0].messages_received, num_messages);
            assert_eq!(stats[0].bytes_received, sent_message_bytes);
        }

        {
            let stats = data_consumer
                .get_stats()
                .await
                .expect("Failed to get stats on data consumer");

            assert_eq!(stats.len(), 1);
            assert_eq!(&stats[0].label, data_consumer.label());
            assert_eq!(&stats[0].protocol, data_consumer.protocol());
            assert_eq!(stats[0].messages_sent, num_messages);
            assert_eq!(
                stats[0].bytes_sent,
                recv_message_bytes.load(Ordering::SeqCst),
            );
        }
    });
}

#[test]
fn close_event() {
    future::block_on(async move {
        let (_worker, _router, transport) = init().await;

        let (mut close_tx, close_rx) = async_oneshot::oneshot::<()>();
        let _handler = transport.on_close(Box::new(move || {
            let _ = close_tx.send(());
        }));

        drop(transport);

        close_rx.await.expect("Failed to receive close event");
    });
}
