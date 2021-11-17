/* eslint-disable no-console */
import { Device } from 'mediasoup-client';
import { MediaKind, RtpCapabilities, RtpParameters } from 'mediasoup-client/lib/RtpParameters';
import { DtlsParameters, TransportOptions, Transport } from 'mediasoup-client/lib/Transport';
import { ConsumerOptions } from 'mediasoup-client/lib/Consumer';
import { getExtendedRtpCapabilities } from 'mediasoup-client/lib/ortc';

type Brand<K, T> = K & { __brand: T };

type ConsumerId = Brand<string, 'ConsumerId'>;
type ProducerId = Brand<string, 'ProducerId'>;

interface ServerInit {
    action: 'Init';
    consumerTransportOptions: TransportOptions;
    routerRtpCapabilities: RtpCapabilities;
	rtpProducerId: ProducerId;
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
	ServerConnectedConsumerTransport |
	ServerConsumed;

interface ClientInit {
    action: 'Init';
    rtpCapabilities: RtpCapabilities;
}

interface ClientConnectConsumerTransport {
    action: 'ConnectConsumerTransport';
    dtlsParameters: DtlsParameters;
}

interface ClientConsume {
    action: 'Consume';
    producerId: ProducerId;
}

type ClientMessage =
	ClientInit |
	ClientConnectConsumerTransport |
	ClientConsume;

async function init()
{
	const receivePreview = document.querySelector('#preview-receive') as HTMLAudioElement;

	receivePreview.onloadedmetadata = () =>
	{
		console.log('onloadedmetadata');
		receivePreview.play();
	};

	const receiveMediaStream = new MediaStream();

	const ws = new WebSocket('ws://localhost:3000/ws');

	function send(message: ClientMessage)
	{
		ws.send(JSON.stringify(message));
	}

	const device = new Device();
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

					// TODO: Here we hardcode RTP capabilities, as Chromium doesn't really expose
					//  multiopus support in a meaningful way, so we need to hack it like this for
					//  now until `mediasoup-client` is updated to handle this case properly
					{
						// @ts-ignore
						device._recvRtpCapabilities = {
							codecs : [
								{
									kind       : 'audio',
									mimeType   : 'audio/multiopus',
									clockRate  : 48000,
									channels   : 6,
									parameters : {
										useinbandfec      : 1,
										'channel_mapping' : '0,1,4,5,2,3',
										'num_streams'     : 4,
										'coupled_streams' : 2
									},
									rtcpFeedback : [
										{
											type      : 'transport-cc',
											parameter : ''
										}
									]
								}
							],
							headerExtensions : device.rtpCapabilities.headerExtensions
						};
						// @ts-ignore
						device._extendedRtpCapabilities = getExtendedRtpCapabilities(
							device.rtpCapabilities,
							decodedMessage.routerRtpCapabilities
						);
					}

					// Send client-side initialization message back right away.
					send({
						action          : 'Init',
						rtpCapabilities : device.rtpCapabilities
					});

					// Consumer transport is now needed to receive producer created on the server
					consumerTransport = device.createRecvTransport(
						decodedMessage.consumerTransportOptions
					);

					// TODO: Here we patch RTCPeerConnection in such a way that we can intercept
					//  and munge SDP answer until until `mediasoup-client` is updated to handle
					//  this case properly
					{
						// @ts-ignore
						const pc = consumerTransport._handler._pc;
						const setLocalDescription = pc.setLocalDescription;

						pc.setLocalDescription = (answer: {type: 'answer'; sdp: string}) =>
						{
							answer.sdp = answer.sdp.replace(
								'm=audio 9 UDP/TLS/RTP/SAVPF 0',
								`m=audio 9 UDP/TLS/RTP/SAVPF 100
a=rtpmap:100 multiopus/48000/6
a=fmtp:100 channel_mapping=0,4,1,2,3,5;coupled_streams=2;num_streams=4;useinbandfec=1`
							);

							return setLocalDescription.call(pc, answer);
						};
					}

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

					// We have just one producer and it is already created, so consuming is easy
					await new Promise((resolve) =>
					{
						// Send request to consume producer
						send({
							action     : 'Consume',
							producerId : decodedMessage.rtpProducerId
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

							receiveMediaStream.addTrack(consumer.track);
							receivePreview.srcObject = receiveMediaStream;

							resolve(undefined);
						});
					});

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

document.querySelector('#connect')!
	.addEventListener('click', () =>
	{
		const container = document.querySelector('#container')!;

		container.removeAttribute('waiting-for-click');

		init();
	});
