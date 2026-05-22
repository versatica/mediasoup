use async_io::Timer;
use futures_lite::future;
use hash_hasher::{HashedMap, HashedSet};
use mediasoup::data_consumer::{DataConsumerOptions, DataConsumerType};
use mediasoup::data_producer::{DataProducer, DataProducerOptions};
use mediasoup::direct_transport::DirectTransportOptions;
use mediasoup::plain_transport::PlainTransportOptions;
use mediasoup::prelude::*;
use mediasoup::router::{Router, RouterOptions};
use mediasoup::webrtc_transport::{
    WebRtcTransport, WebRtcTransportListenInfos, WebRtcTransportOptions,
};
use mediasoup::worker::{Worker, WorkerSettings};
use mediasoup::worker_manager::WorkerManager;
use mediasoup_types::data_structures::{AppData, ListenInfo, Protocol};
use mediasoup_types::sctp_parameters::SctpStreamParameters;
use std::env;
use std::net::{IpAddr, Ipv4Addr};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::sync::Arc;
use std::time::Duration;

struct CustomAppData {
    baz: &'static str,
}

struct CustomAppData2 {
    hehe: &'static str,
}

fn sctp_data_producer_options() -> DataProducerOptions {
    let mut options = DataProducerOptions::new_sctp(
        SctpStreamParameters::new_unordered_with_life_time(12345, 5000),
    );

    options.label = "foo".to_string();
    options.protocol = "bar".to_string();

    options
}

async fn init() -> (Worker, Router, WebRtcTransport, DataProducer) {
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

    let webrtc_transport = router
        .create_webrtc_transport({
            let mut transport_options =
                WebRtcTransportOptions::new(WebRtcTransportListenInfos::new(ListenInfo {
                    protocol: Protocol::Udp,
                    ip: IpAddr::V4(Ipv4Addr::LOCALHOST),
                    announced_address: None,
                    expose_internal_ip: false,
                    port: None,
                    port_range: None,
                    flags: None,
                    send_buffer_size: None,
                    recv_buffer_size: None,
                }));

            transport_options.enable_sctp = true;

            transport_options
        })
        .await
        .expect("Failed to create transport1");

    let sctp_data_producer = webrtc_transport
        .produce_data(sctp_data_producer_options())
        .await
        .expect("Failed to create data producer");

    (worker, router, webrtc_transport, sctp_data_producer)
}

#[test]
fn consume_data_succeeds() {
    future::block_on(async move {
        let (_worker, router, _webrtc_transport, sctp_data_producer) = init().await;

        let plain_transport = router
            .create_plain_transport({
                let mut transport_options = PlainTransportOptions::new(ListenInfo {
                    protocol: Protocol::Udp,
                    ip: IpAddr::V4(Ipv4Addr::LOCALHOST),
                    announced_address: None,
                    expose_internal_ip: false,
                    port: None,
                    port_range: None,
                    flags: None,
                    send_buffer_size: None,
                    recv_buffer_size: None,
                });

                transport_options.enable_sctp = true;

                transport_options
            })
            .await
            .expect("Failed to create transport1");

        let new_data_consumer_count = Arc::new(AtomicUsize::new(0));

        plain_transport
            .on_new_data_consumer({
                let new_data_consumer_count = Arc::clone(&new_data_consumer_count);

                Arc::new(move |_data_consumer| {
                    new_data_consumer_count.fetch_add(1, Ordering::SeqCst);
                })
            })
            .detach();

        let data_consumer = plain_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_sctp(sctp_data_producer.id());

                options.subchannels = Some(vec![0, 1, 1, 1, 2, 65535, 100]);
                options.app_data = AppData::new(CustomAppData { baz: "LOL" });

                options
            })
            .await
            .expect("Failed to consume data");

        assert_eq!(data_consumer.data_producer_id(), sctp_data_producer.id());
        assert!(!data_consumer.closed());
        assert_eq!(data_consumer.r#type(), DataConsumerType::Sctp);
        {
            let sctp_stream_parameters = data_consumer.sctp_stream_parameters();
            assert!(sctp_stream_parameters.is_some());
            assert!(!sctp_stream_parameters.unwrap().ordered());
            assert_eq!(
                sctp_stream_parameters.unwrap().max_packet_life_time(),
                Some(5000)
            );
            assert_eq!(sctp_stream_parameters.unwrap().max_retransmits(), None);
        }
        assert_eq!(data_consumer.label().as_str(), "foo");
        assert_eq!(data_consumer.protocol().as_str(), "bar");

        let mut sorted_subchannels = data_consumer.subchannels();
        sorted_subchannels.sort();

        assert_eq!(sorted_subchannels, [0, 1, 2, 100, 65535]);
        assert_eq!(
            data_consumer
                .app_data()
                .downcast_ref::<CustomAppData>()
                .unwrap()
                .baz,
            "LOL",
        );

        {
            let dump = router.dump().await.expect("Failed to dump router");

            assert_eq!(dump.map_data_producer_id_data_consumer_ids, {
                let mut map = HashedMap::default();
                map.insert(sctp_data_producer.id(), {
                    let mut set = HashedSet::default();
                    set.insert(data_consumer.id());
                    set
                });
                map
            });
            assert_eq!(dump.map_data_consumer_id_data_producer_id, {
                let mut map = HashedMap::default();
                map.insert(data_consumer.id(), sctp_data_producer.id());
                map
            });
        }

        {
            let dump = plain_transport
                .dump()
                .await
                .expect("Failed to dump transport");

            assert_eq!(dump.id, plain_transport.id());
            assert_eq!(dump.data_producer_ids, vec![]);
            assert_eq!(dump.data_consumer_ids, vec![data_consumer.id()]);
        }
    });
}

#[test]
fn weak() {
    future::block_on(async move {
        let (_worker, router, _webrtc_transport, sctp_data_producer) = init().await;

        let plain_transport = router
            .create_plain_transport({
                let mut transport_options = PlainTransportOptions::new(ListenInfo {
                    protocol: Protocol::Udp,
                    ip: IpAddr::V4(Ipv4Addr::LOCALHOST),
                    announced_address: None,
                    expose_internal_ip: false,
                    port: None,
                    port_range: None,
                    flags: None,
                    send_buffer_size: None,
                    recv_buffer_size: None,
                });

                transport_options.enable_sctp = true;

                transport_options
            })
            .await
            .expect("Failed to create transport1");

        let data_consumer = plain_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_sctp_unordered_with_life_time(
                    sctp_data_producer.id(),
                    4000,
                );

                options.app_data = AppData::new(CustomAppData { baz: "LOL" });

                options
            })
            .await
            .expect("Failed to consume data");

        let weak_data_consumer = data_consumer.downgrade();

        assert!(weak_data_consumer.upgrade().is_some());

        drop(data_consumer);

        assert!(weak_data_consumer.upgrade().is_none());
    });
}

#[test]
fn dump_succeeds() {
    future::block_on(async move {
        let (_worker, _router, webrtc_transport, sctp_data_producer) = init().await;

        let data_consumer = webrtc_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_sctp_ordered(sctp_data_producer.id());

                options.app_data = AppData::new(CustomAppData { baz: "LOL" });

                options
            })
            .await
            .expect("Failed to consume data");

        let dump = data_consumer
            .dump()
            .await
            .expect("Data consumer dump failed");

        assert_eq!(dump.id, data_consumer.id());
        assert_eq!(dump.data_producer_id, data_consumer.data_producer_id());
        assert_eq!(dump.r#type, DataConsumerType::Sctp);
        {
            let sctp_stream_parameters = dump.sctp_stream_parameters;
            assert!(sctp_stream_parameters.is_some());
            assert_eq!(
                sctp_stream_parameters.unwrap().stream_id(),
                data_consumer.sctp_stream_parameters().unwrap().stream_id(),
            );
            assert!(sctp_stream_parameters.unwrap().ordered());
            assert_eq!(sctp_stream_parameters.unwrap().max_packet_life_time(), None);
            assert_eq!(sctp_stream_parameters.unwrap().max_retransmits(), None);
        }
        assert_eq!(dump.label.as_str(), "foo");
        assert_eq!(dump.protocol.as_str(), "bar");
    });
}

#[test]
fn get_stats_succeeds() {
    future::block_on(async move {
        let (_worker, _router, webrtc_transport, sctp_data_producer) = init().await;

        let data_consumer = webrtc_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_sctp_unordered_with_life_time(
                    sctp_data_producer.id(),
                    4000,
                );

                options.app_data = AppData::new(CustomAppData { baz: "LOL" });

                options
            })
            .await
            .expect("Failed to consume data");

        let stats = data_consumer
            .get_stats()
            .await
            .expect("Failed to get data consumer stats");

        assert_eq!(stats.len(), 1);
        assert_eq!(&stats[0].label, data_consumer.label());
        assert_eq!(&stats[0].protocol, data_consumer.protocol());
        assert_eq!(stats[0].messages_sent, 0);
        assert_eq!(stats[0].bytes_sent, 0);
    });
}

#[test]
fn set_subchannels() {
    future::block_on(async move {
        let (_worker, _router, transport1, sctp_data_producer) = init().await;

        let data_consumer = transport1
            .consume_data(DataConsumerOptions::new_sctp_unordered_with_life_time(
                sctp_data_producer.id(),
                4000,
            ))
            .await
            .expect("Failed to consume data");

        data_consumer
            .set_subchannels([999, 999, 998, 0].to_vec())
            .await
            .expect("Failed to set data consumer subchannels");

        let mut sorted_subchannels = data_consumer.subchannels();
        sorted_subchannels.sort();

        assert_eq!(sorted_subchannels, [0, 998, 999]);
    });
}

#[test]
fn add_and_remove_subchannel() {
    future::block_on(async move {
        let (_worker, _router, transport1, sctp_data_producer) = init().await;

        let data_consumer = transport1
            .consume_data(DataConsumerOptions::new_sctp_unordered_with_life_time(
                sctp_data_producer.id(),
                4000,
            ))
            .await
            .expect("Failed to consume data");

        data_consumer
            .set_subchannels([].to_vec())
            .await
            .expect("Failed to set data consumer subchannels");

        assert_eq!(data_consumer.subchannels(), []);

        data_consumer
            .add_subchannel(5)
            .await
            .expect("Failed to add data consumer subchannel");

        assert_eq!(data_consumer.subchannels(), [5]);

        data_consumer
            .add_subchannel(10)
            .await
            .expect("Failed to add data consumer subchannel");

        let mut sorted_subchannels = data_consumer.subchannels();
        sorted_subchannels.sort();

        assert_eq!(sorted_subchannels, [5, 10]);

        data_consumer
            .add_subchannel(5)
            .await
            .expect("Failed to add data consumer subchannel");

        sorted_subchannels = data_consumer.subchannels();
        sorted_subchannels.sort();

        assert_eq!(sorted_subchannels, [5, 10]);

        data_consumer
            .remove_subchannel(666)
            .await
            .expect("Failed to remove data consumer subchannel");

        sorted_subchannels = data_consumer.subchannels();
        sorted_subchannels.sort();

        assert_eq!(sorted_subchannels, [5, 10]);

        data_consumer
            .remove_subchannel(5)
            .await
            .expect("Failed to remove data consumer subchannel");

        sorted_subchannels = data_consumer.subchannels();
        sorted_subchannels.sort();

        assert_eq!(sorted_subchannels, [10]);

        data_consumer
            .add_subchannel(5)
            .await
            .expect("Failed to add data consumer subchannel");

        sorted_subchannels = data_consumer.subchannels();
        sorted_subchannels.sort();

        assert_eq!(sorted_subchannels, [5, 10]);

        data_consumer
            .set_subchannels([].to_vec())
            .await
            .expect("Failed to set data consumer subchannels");

        assert_eq!(data_consumer.subchannels(), []);
    });
}

#[test]
fn consume_data_from_a_direct_data_producer_succeeds() {
    future::block_on(async move {
        let (_worker, router, webrtc_transport, sctp_data_producer) = init().await;

        let direct_transport = router
            .create_direct_transport(DirectTransportOptions::default())
            .await
            .expect("Failed to create Direct transport");

        let direct_data_producer = direct_transport
            .produce_data(DataProducerOptions::new_direct())
            .await
            .expect("Failed to create data producer");

        let data_consumer = webrtc_transport
            .consume_data(DataConsumerOptions::new_direct(
                direct_data_producer.id(),
                None,
            ))
            .await
            .expect("Failed to consume data");

        assert_eq!(data_consumer.data_producer_id(), direct_data_producer.id());
        assert!(!data_consumer.closed());
        assert_eq!(data_consumer.r#type(), DataConsumerType::Sctp);
        {
            let sctp_stream_parameters = data_consumer.sctp_stream_parameters();
            assert!(sctp_stream_parameters.is_some());
            assert!(sctp_stream_parameters.unwrap().ordered());
            assert_eq!(sctp_stream_parameters.unwrap().max_packet_life_time(), None);
            assert_eq!(sctp_stream_parameters.unwrap().max_retransmits(), None);
        }
        assert_eq!(data_consumer.label().as_str(), "");
        assert_eq!(data_consumer.protocol().as_str(), "");

        {
            let dump = webrtc_transport
                .dump()
                .await
                .expect("Failed to dump transport");

            assert_eq!(dump.id, webrtc_transport.id());
            assert_eq!(dump.data_producer_ids, vec![sctp_data_producer.id()]);
            assert_eq!(dump.data_consumer_ids, vec![data_consumer.id()]);
        }
    });
}

#[test]
fn dump_consuming_from_a_direct_data_producer_succeeds() {
    future::block_on(async move {
        let (_worker, router, webrtc_transport, _sctp_data_producer) = init().await;

        let direct_transport = router
            .create_direct_transport(DirectTransportOptions::default())
            .await
            .expect("Failed to create Direct transport");

        let direct_data_producer = direct_transport
            .produce_data(DataProducerOptions::new_direct())
            .await
            .expect("Failed to create data producer");

        let data_consumer = webrtc_transport
            .consume_data(DataConsumerOptions::new_sctp_unordered_with_retransmits(
                direct_data_producer.id(),
                2,
            ))
            .await
            .expect("Failed to consume data");

        let dump = data_consumer
            .dump()
            .await
            .expect("Data consumer dump failed");

        assert_eq!(dump.id, data_consumer.id());
        assert_eq!(dump.data_producer_id, data_consumer.data_producer_id());
        assert_eq!(dump.r#type, DataConsumerType::Sctp);
        {
            let sctp_stream_parameters = data_consumer.sctp_stream_parameters();
            assert!(sctp_stream_parameters.is_some());
            assert!(!sctp_stream_parameters.unwrap().ordered());
            assert_eq!(sctp_stream_parameters.unwrap().max_packet_life_time(), None);
            assert_eq!(sctp_stream_parameters.unwrap().max_retransmits(), Some(2));
        }
        assert_eq!(dump.label.as_str(), "");
        assert_eq!(dump.protocol.as_str(), "");
    });
}

#[test]
fn consume_data_on_direct_transport_succeeds() {
    future::block_on(async move {
        let (_worker, router, _webrtc_transport, sctp_data_producer) = init().await;

        let direct_transport = router
            .create_direct_transport(DirectTransportOptions::default())
            .await
            .expect("Failed to create Direct transport");

        let new_data_consumer_count = Arc::new(AtomicUsize::new(0));

        direct_transport
            .on_new_data_consumer({
                let new_data_consumer_count = Arc::clone(&new_data_consumer_count);

                Arc::new(move |_data_consumer| {
                    new_data_consumer_count.fetch_add(1, Ordering::SeqCst);
                })
            })
            .detach();

        let data_consumer = direct_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_direct(sctp_data_producer.id(), None);

                options.app_data = AppData::new(CustomAppData2 { hehe: "HEHE" });

                options
            })
            .await
            .expect("Failed to consume data");

        assert_eq!(new_data_consumer_count.load(Ordering::SeqCst), 1);
        assert_eq!(data_consumer.data_producer_id(), sctp_data_producer.id());
        assert!(!data_consumer.closed());
        assert_eq!(data_consumer.r#type(), DataConsumerType::Direct);
        assert_eq!(data_consumer.sctp_stream_parameters(), None);
        assert_eq!(data_consumer.label().as_str(), "foo");
        assert_eq!(data_consumer.protocol().as_str(), "bar");
        assert_eq!(
            data_consumer
                .app_data()
                .downcast_ref::<CustomAppData2>()
                .unwrap()
                .hehe,
            "HEHE",
        );

        {
            let dump = direct_transport
                .dump()
                .await
                .expect("Failed to dump transport");

            assert_eq!(dump.id, direct_transport.id());
            assert_eq!(dump.data_producer_ids, vec![]);
            assert_eq!(dump.data_consumer_ids, vec![data_consumer.id()]);
        }
    });
}

#[test]
fn dump_on_direct_transport_succeeds() {
    future::block_on(async move {
        let (_worker, router, _webrtc_transport, sctp_data_producer) = init().await;

        let direct_transport = router
            .create_direct_transport(DirectTransportOptions::default())
            .await
            .expect("Failed to create Direct transport");

        let data_consumer = direct_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_direct(sctp_data_producer.id(), None);

                options.app_data = AppData::new(CustomAppData2 { hehe: "HEHE" });

                options
            })
            .await
            .expect("Failed to consume data");

        let dump = data_consumer
            .dump()
            .await
            .expect("Data consumer dump failed");

        assert_eq!(dump.id, data_consumer.id());
        assert_eq!(dump.data_producer_id, data_consumer.data_producer_id());
        assert_eq!(dump.r#type, DataConsumerType::Direct);
        assert_eq!(dump.sctp_stream_parameters, None);
        assert_eq!(dump.label.as_str(), "foo");
        assert_eq!(dump.protocol.as_str(), "bar");
    });
}

#[test]
fn get_stats_on_direct_transport_succeeds() {
    future::block_on(async move {
        let (_worker, router, _webrtc_transport, sctp_data_producer) = init().await;

        let direct_transport = router
            .create_direct_transport(DirectTransportOptions::default())
            .await
            .expect("Failed to create Direct transport");

        let data_consumer = direct_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_direct(sctp_data_producer.id(), None);

                options.app_data = AppData::new(CustomAppData2 { hehe: "HEHE" });

                options
            })
            .await
            .expect("Failed to consume data");

        let stats = data_consumer
            .get_stats()
            .await
            .expect("Failed to get data consumer stats");

        assert_eq!(stats.len(), 1);
        assert_eq!(&stats[0].label, data_consumer.label());
        assert_eq!(&stats[0].protocol, data_consumer.protocol());
        assert_eq!(stats[0].messages_sent, 0);
        assert_eq!(stats[0].bytes_sent, 0);
    });
}

#[test]
fn close_event() {
    future::block_on(async move {
        let (_worker, router, _webrtc_transport, sctp_data_producer) = init().await;

        let plain_transport = router
            .create_plain_transport({
                let mut transport_options = PlainTransportOptions::new(ListenInfo {
                    protocol: Protocol::Udp,
                    ip: IpAddr::V4(Ipv4Addr::LOCALHOST),
                    announced_address: None,
                    expose_internal_ip: false,
                    port: None,
                    port_range: None,
                    flags: None,
                    send_buffer_size: None,
                    recv_buffer_size: None,
                });

                transport_options.enable_sctp = true;

                transport_options
            })
            .await
            .expect("Failed to create transport1");

        let data_consumer = plain_transport
            .consume_data({
                let mut options = DataConsumerOptions::new_sctp_unordered_with_life_time(
                    sctp_data_producer.id(),
                    4000,
                );

                options.app_data = AppData::new(CustomAppData { baz: "LOL" });

                options
            })
            .await
            .expect("Failed to consume data");

        {
            let (mut tx, rx) = async_oneshot::oneshot::<()>();
            let _handler = data_consumer.on_close(move || {
                let _ = tx.send(());
            });
            drop(data_consumer);

            rx.await.expect("Failed to receive close event");
        }

        // Drop is async, give consumer a bit of time to finish
        Timer::after(Duration::from_millis(200)).await;

        {
            let dump = router.dump().await.expect("Failed to dump router");

            assert_eq!(dump.map_data_producer_id_data_consumer_ids, {
                let mut map = HashedMap::default();
                map.insert(sctp_data_producer.id(), HashedSet::default());
                map
            });
            assert_eq!(
                dump.map_data_consumer_id_data_producer_id,
                HashedMap::default()
            );
        }

        {
            let dump = plain_transport
                .dump()
                .await
                .expect("Failed to dump transport");

            assert_eq!(dump.data_producer_ids, vec![]);
            assert_eq!(dump.data_consumer_ids, vec![]);
        }
    });
}
