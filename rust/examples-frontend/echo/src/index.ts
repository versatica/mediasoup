/* eslint-disable no-console */
import { Device } from 'mediasoup-client';
import { MediaKind, RtpCapabilities, RtpParameters } from 'mediasoup-client/lib/RtpParameters';
import { DtlsParameters, TransportOptions, Transport } from 'mediasoup-client/lib/Transport';
import { ConsumerOptions } from 'mediasoup-client/lib/Consumer';

type Brand<K, T> = K & { __brand: T };

type ConsumerId = Brand<string, 'ConsumerId'>;
type ProducerId = Brand<string, 'ProducerId'>;

interface ServerInit {
    action: 'Init';
    consumerTransportOptions: TransportOptions;
    producerTransportOptions: TransportOptions;
    routerRtpCapabilities: RtpCapabilities;
}

interface ServerConnectedProducerTransport {
	action: 'ConnectedProducerTransport';
}

interface ServerProduced {
	action: 'Produced';
	id: ProducerId;
}

interface ServerConnectedConsumerTransport {
	action: 'ConnectedConsumerTransport';
}

interface ServerConsumed {
	action: 'Consumed';
	id: ConsumerId;
	kind: MediaKind;
	rtpParameters: RtpParameters;
}

type ServerMessage =
	ServerInit |
	ServerConnectedProducerTransport |
	ServerProduced |
	ServerConnectedConsumerTransport |
	ServerConsumed;

interface ClientInit {
    action: 'Init';
    rtpCapabilities: RtpCapabilities;
}

interface ClientConnectProducerTransport {
    action: 'ConnectProducerTransport';
	dtlsParameters: DtlsParameters;
}

interface ClientConnectConsumerTransport {
    action: 'ConnectConsumerTransport';
    dtlsParameters: DtlsParameters;
}

interface ClientProduce {
    action: 'Produce';
    kind: MediaKind;
    rtpParameters: RtpParameters;
}

interface ClientConsume {
    action: 'Consume';
    producerId: ProducerId;
}

interface ClientConsumerResume {
    action: 'ConsumerResume';
    id: ConsumerId;
}

type ClientMessage =
	ClientInit |
	ClientConnectProducerTransport |
	ClientProduce |
	ClientConnectConsumerTransport |
	ClientConsume |
	ClientConsumerResume;

async function init()
{
	const sendPreview = document.querySelector('#preview-send') as HTMLVideoElement;
	const receivePreview = document.querySelector('#preview-receive') as HTMLVideoElement;

	sendPreview.onloadedmetadata = () =>
	{
		sendPreview.play();
	};
	receivePreview.onloadedmetadata = () =>
	{
		receivePreview.play();
	};

	const receiveMediaStream = new MediaStream();

	const ws = new WebSocket('ws://localhost:3000/ws');

	function send(message: ClientMessage)
	{
		ws.send(JSON.stringify(message));
	}

	const device = new Device();
	let producerTransport: Transport | undefined;
	let consumerTransport: Transport | undefined;

	{
		const waitingForResponse: Map<ServerMessage['action'], Function> = new Map();

		ws.onmessage = async (message) =>
		{
			const decodedMessage: ServerMessage = JSON.parse(message.data);

			switch (decodedMessage.action)
			{
				case 'Init': {
					// It is expected that server will send initialization message right after
					// WebSocket connection is established
					await device.load({
						routerRtpCapabilities : decodedMessage.routerRtpCapabilities
					});

					// Send client-side initialization message back right away
					send({
						action          : 'Init',
						rtpCapabilities : device.rtpCapabilities
					});

					// Producer transport is needed to send audio and video to SFU
					producerTransport = device.createSendTransport(
						decodedMessage.producerTransportOptions
					);

					producerTransport
						.on('connect', ({ dtlsParameters }, success) =>
						{
							// Send request to establish producer transport connection
							send({
								action : 'ConnectProducerTransport',
								dtlsParameters
							});
							// And wait for confirmation, but, obviously, no error handling,
							// which you should definitely have in real-world applications
							waitingForResponse.set('ConnectedProducerTransport', () =>
							{
								success();
								console.log('Producer transport connected');
							});
						})
						.on('produce', ({ kind, rtpParameters }, success) =>
						{
							// Once connection is established, send request to produce
							// audio or video track
							send({
								action : 'Produce',
								kind,
								rtpParameters
							});
							// And wait for confirmation, but, obviously, no error handling,
							// which you should definitely have in real-world applications
							waitingForResponse.set('Produced', ({ id }: {id: string}) =>
							{
								success({ id });
							});
						});

					// Request microphone and camera access, in real-world apps you may want
					// to do this separately so that audio-only and video-only cases are
					// handled nicely instead of failing completely
					const mediaStream = await navigator.mediaDevices.getUserMedia({
						audio : true,
						video : {
							width : {
								ideal : 1270
							},
							height : {
								ideal : 720
							},
							frameRate : {
								ideal : 60
							}
						}
					});

					sendPreview.srcObject = mediaStream;

					const producers = [];

					// And create producers for all tracks that were previously requested
					for (const track of mediaStream.getTracks())
					{
						const producer = await producerTransport.produce({ track });

						producers.push(producer);
						console.log(`${track.kind} producer created:`, producer);
					}

					// Consumer transport is now needed to receive previously produced
					// tracks back
					consumerTransport = device.createRecvTransport(
						decodedMessage.consumerTransportOptions
					);

					consumerTransport
						.on('connect', ({ dtlsParameters }, success) =>
						{
							// Send request to establish consumer transport connection
							send({
								action : 'ConnectConsumerTransport',
								dtlsParameters
							});
							// And wait for confirmation, but, obviously, no error handling,
							// which you should definitely have in real-world applications
							waitingForResponse.set('ConnectedConsumerTransport', () =>
							{
								success();
								console.log('Consumer transport connected');
							});
						});

					// For simplicity of this example producers were stored in an array
					// and are now all consumed one at a time
					for (const producer of producers)
					{
						await new Promise((resolve) =>
						{
							// Send request to consume producer
							send({
								action     : 'Consume',
								producerId : producer.id as ProducerId
							});
							// And wait for confirmation, but, obviously, no error handling,
							// which you should definitely have in real-world applications
							waitingForResponse.set('Consumed', async (consumerOptions: ConsumerOptions) =>
							{
								// Once confirmation is received, corresponding consumer
								// can be created client-side
								const consumer = await (consumerTransport as Transport).consume(
									consumerOptions
								);

								console.log(`${consumer.kind} consumer created:`, consumer);

								// Consumer needs to be resumed after being created in
								// paused state (see official documentation about why:
								// https://mediasoup.org/documentation/v3/mediasoup/api/#transport-consume)
								send({
									action : 'ConsumerResume',
									id     : consumer.id as ConsumerId
								});

								receiveMediaStream.addTrack(consumer.track);
								receivePreview.srcObject = receiveMediaStream;

								resolve(undefined);
							});
						});
					}

					break;
				}
				default: {
					// All messages other than initialization go here and are assumed
					// to be notifications that correspond to previously sent requests
					const callback = waitingForResponse.get(decodedMessage.action);

					if (callback)
					{
						waitingForResponse.delete(decodedMessage.action);
						callback(decodedMessage);
					}
					else
					{
						console.error('Received unexpected message', decodedMessage);
					}
				}
			}
		};
	}
	ws.onerror = console.error;
}

init();
