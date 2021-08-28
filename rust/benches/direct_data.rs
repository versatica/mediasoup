use criterion::async_executor::FuturesExecutor;
use criterion::{criterion_group, criterion_main, Criterion};
use futures_lite::StreamExt;
use mediasoup::prelude::*;
use std::borrow::Cow;

async fn create_data_producer_consumer_pair(
) -> Result<(DataProducer, DataConsumer), Box<dyn std::error::Error>> {
    let worker_manager = WorkerManager::new();
    let worker = worker_manager
        .create_worker(WorkerSettings::default())
        .await?;

    let router = worker.create_router(RouterOptions::default()).await?;
    let direct_transport = router
        .create_direct_transport(DirectTransportOptions::default())
        .await?;

    let data_producer = direct_transport
        .produce_data(DataProducerOptions::new_direct())
        .await?;
    let data_consumer = direct_transport
        .consume_data(DataConsumerOptions::new_direct(data_producer.id()))
        .await?;

    Ok((data_producer, data_consumer))
}

pub fn criterion_benchmark(c: &mut Criterion) {
    let mut group = c.benchmark_group("direct_data");

    let data = std::iter::repeat_with(|| fastrand::u8(..))
        .take(512)
        .collect::<Vec<u8>>();

    {
        let (data_producer, _data_consumer) = futures_lite::future::block_on(async {
            create_data_producer_consumer_pair().await.unwrap()
        });

        let direct_data_producer =
            if let DataProducer::Direct(direct_data_producer) = data_producer.clone() {
                direct_data_producer
            } else {
                unreachable!()
            };

        group.bench_with_input("send", &data, |b, data| {
            b.to_async(FuturesExecutor).iter(|| async {
                let _ = direct_data_producer
                    .send(WebRtcMessage::Binary(Cow::from(data)))
                    .await;
            })
        });
    }
    {
        let (data_producer, data_consumer) = futures_lite::future::block_on(async {
            create_data_producer_consumer_pair().await.unwrap()
        });

        let direct_data_producer =
            if let DataProducer::Direct(direct_data_producer) = data_producer.clone() {
                direct_data_producer
            } else {
                unreachable!()
            };

        group.bench_with_input("recv", &data, |b, data| {
            b.to_async(FuturesExecutor).iter(|| async {
                let (sender, mut receiver) = async_channel::unbounded::<()>();
                let _handler_id = data_consumer.on_message(move |_message| {
                    let _ = sender.try_send(());
                });

                let _ = direct_data_producer
                    .send(WebRtcMessage::Binary(Cow::from(data)))
                    .await;

                let _ = receiver.next().await;
            })
        });
    }

    group.finish();
}

criterion_group!(benches, criterion_benchmark);
criterion_main!(benches);
