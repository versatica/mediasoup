/* eslint-disable no-console */
import { Device } from 'mediasoup-client';
import { MediaKind, RtpCapabilities, RtpParameters } from 'mediasoup-client/lib/RtpParameters';
import { DtlsParameters, TransportOptions, Transport } from 'mediasoup-client/lib/Transport';
import { ConsumerOptions } from 'mediasoup-client/lib/Consumer';

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
	id: string;
}

interface ServerConnectedConsumerTransport {
	action: 'ConnectedConsumerTransport';
}

interface ServerConsumed {
	action: 'Consumed';
	id: string;
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
    producerId: string;
}

interface ClientConsumerResume {
    action: 'ConsumerResume';
    id: string;
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

	let receiveMediaStream: MediaStream | undefined;

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
					await device.load({
						routerRtpCapabilities : decodedMessage.routerRtpCapabilities
					});

					send({
						action          : 'Init',
						rtpCapabilities : device.rtpCapabilities
					});

					producerTransport = device.createSendTransport(
						decodedMessage.producerTransportOptions
					);

					producerTransport
						.on('connect', ({ dtlsParameters }, success) =>
						{
							send({
								action : 'ConnectProducerTransport',
								dtlsParameters
							});
							waitingForResponse.set('ConnectedProducerTransport', () =>
							{
								success();
								console.log('Producer transport connected');
							});
						})
						.on('produce', ({ kind, rtpParameters }, success) =>
						{
							send({
								action : 'Produce',
								kind,
								rtpParameters
							});
							waitingForResponse.set('Produced', ({ id }: {id: string}) =>
							{
								success({ id });
							});
						});

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

					for (const track of mediaStream.getTracks())
					{
						const producer = await producerTransport.produce({ track });

						producers.push(producer);
						console.log(`${track.kind} producer created:`, producer);
					}

					consumerTransport = device.createRecvTransport(
						decodedMessage.consumerTransportOptions
					);

					consumerTransport
						.on('connect', ({ dtlsParameters }, success) =>
						{
							send({
								action : 'ConnectConsumerTransport',
								dtlsParameters
							});
							waitingForResponse.set('ConnectedConsumerTransport', () =>
							{
								success();
								console.log('Consumer transport connected');
							});
						});

					for (const producer of producers)
					{
						await new Promise((resolve) =>
						{
							send({
								action     : 'Consume',
								producerId : producer.id
							});
							waitingForResponse.set('Consumed', async (consumerOptions: ConsumerOptions) =>
							{
								const consumer = await (consumerTransport as Transport).consume(
									consumerOptions
								);

								console.log(`${consumer.kind} producer created:`, consumer);

								send({
									action : 'ConsumerResume',
									id     : consumer.id
								});

								if (receiveMediaStream)
								{
									receiveMediaStream.addTrack(consumer.track);
									receivePreview.srcObject = receiveMediaStream;
								}
								else
								{
									receiveMediaStream = new MediaStream([ consumer.track ]);
									receivePreview.srcObject = receiveMediaStream;
								}
								resolve(undefined);
							});
						});
					}

					break;
				}
				default: {
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
