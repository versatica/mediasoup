#!/usr/bin/env node

'use strict';

process.env.DEBUG = '*ERROR* *WARN* mediasoup* mediasoup-tester*';
// process.env.DEBUG = '*ERROR* *WARN* mediasoup* mediasoup-tester* -mediasoup:mediasoup-worker*';

const http = require('http');
const url = require('url');

const debug = require('debug')('mediasoup-tester');
const debugerror = require('debug')('mediasoup-tester:ERROR');

debug.log = console.info.bind(console);
debugerror.log = console.error.bind(console);

const mediasoup = require('../../');
const protoo = require('protoo');
const sdpTransform = require('sdp-transform');

const LISTEN_IP = '0.0.0.0';
const LISTEN_PORT = 8080;

// protoo app
let app = protoo();

// mediasoup server
let server = mediasoup.Server(
	{
		numWorkers : 1,
		logLevel   : 'debug'
	});

// Create a single mediasoup Room
let room = server.Room();

// Node-WebSocket server options
let wsOptions =
{
	maxReceivedFrameSize     : 960000,  // 960 KBytes
	maxReceivedMessageSize   : 960000,
	fragmentOutgoingMessages : true,
	fragmentationThreshold   : 960000
};

// HTTP server
let httpServer = http.createServer((req, res) =>
{
	res.writeHead(404, 'Not Here');
	res.end();
});

httpServer.listen(LISTEN_PORT, LISTEN_IP);

// Handle WebSocket connections
app.websocket(httpServer, wsOptions, (info, accept, reject) =>
{
	// Let the client indicate username and uuid in the URL query
	let u = url.parse(info.req.url, true);
	let username = u.query.username;
	let uuid = u.query.uuid;

	if (username && uuid)
	{
		debug('accepting WebSocket connection [username:%s, uuid:%s, ip:%s, port:%d]',
			username, uuid, info.socket.remoteAddress, info.socket.remotePort);

		accept(username, uuid, null);
	}
	else
	{
		debugerror('rejecting WebSocket connection due to missing username/uuid');

		reject();
	}
});

// Handle new peers
app.on('online', (peer) =>
{
	debug('peer online: %s', peer);

	let mediaPeer;

	// Create a mediasoup Peer instance
	try
	{
		mediaPeer = room.Peer(peer.username);
	}
	catch (error)
	{
		debugerror('error adding a new peer to the room [username:%s, error:%s]', peer.username, error);

		peer.close();
		return;
	}

	mediaPeer.on('close', (error) =>
	{
		if (error)
			peer.close(3500, error.message);
		else
			peer.close();
	});

	// Store the mediasoup Peer instance within the protoo peer
	peer.data.mediaPeer = mediaPeer;
});

// Handle disconnected peers
app.on('offline', (peer) =>
{
	debug('peer offline: %s', peer);

	// Remove from the room
	if (peer.data.mediaPeer)
		peer.data.mediaPeer.close();
});

// Handle PUT requests to /test-transport
app.put('/test-transport', function(req)
{
	debug('/test-transport started');

	// Retrieve the mediasoup Peer associated to the protoo peer who sent the request
	let mediaPeer = req.peer.data.mediaPeer;

	// TODO: not sure why this happens (when connecting with same username:uuid in other tab)
	// NOTE: This may be a protoo bug. It seems that upon reconnection the peer.data is not "cloned".
	if (!mediaPeer)
	{
		debugerror('/test-transport error due to non existing mediasoup Peer for this peer [username:%s]', req.peer.username);

		req.reply(500, 'no mediasoup Peer for this peer');

		req.peer.close();
		return;
	}

	let sdpOffer = req.data.sdp;
	let offer = sdpTransform.parse(sdpOffer);
	let answer = {};
	let numTransports = 0;
	let transportPromises = [];

	answer.version = 0;
	answer.origin =
	{
		address        : '0.0.0.0',
		ipVer          : offer.origin.ipVer,
		netType        : offer.origin.netType,
		sessionId      : offer.origin.sessionId,
		sessionVersion : offer.origin.sessionVersion,
		username       : 'mediasoup'
	};
	answer.name = 'mediasoup';
	answer.timing = { start: 0, stop: 0 };
	answer.groups = offer.groups;
	answer.msidSemantic = offer.msidSemantic;
	answer.icelite = 'ice-lite';
	answer.media = [];

	// Initially assume as many transports as m= lines
	// NOTE: We assume rtcpmux, PERIOD
	numTransports = offer.media.length;

	// If bundle is used, decrease the number of required transports
	if (offer.groups && offer.groups.length > 0)
	{
		// Assume "bundle" is for all the transports
		if (offer.groups[0].type === 'BUNDLE')
			numTransports = 1;
	}

	debug('/test-transport [numTransports:%d]', numTransports);

	// Create transports promises
	for (let i = 0; i < numTransports; i++)
	{
		let promise = mediaPeer.createTransport(
			{
				udp        : true,
				tcp        : true,
				preferIPv6 : true,
				preferTcp  : true
			})
			.then((transport) =>
			{
				transport.on('close', () =>
				{
					debug('transport "close" event');
				});

				transport.on('iceselectedtuplechange', (data) =>
				{
					debug('transport "iceselectedtuplechange" event [data.iceSelectedTuple:%o, transport.iceSelectedTuple:%o]', data.iceSelectedTuple, transport.iceSelectedTuple);
				});

				transport.on('icestatechange', (data) =>
				{
					debug('transport "icestatechange" event [data.iceState:%s, transport.iceState:%s]', data.iceState, transport.iceState);
				});

				transport.on('dtlsstatechange', (data) =>
				{
					debug('transport "dtlsstatechange" event [data.dtlsState:%s, transport.dtlsState:%s]', data.dtlsState, transport.dtlsState);
				});

				return transport;
			})
			.catch((error) => debugerror('SOMETHING FAILED: %s]', error));

		// Add the transport promise to the array of promises
		transportPromises.push(promise);
	}

	// Once all the transports are created, continue
	Promise.all(transportPromises)
		.catch((error) =>
		{
			req.reply(400, error.message);
			return;
		})
		.then((transports) =>
		{
			// Here we need to call setRemoteDtlsParameters() for each local transport, so will collect
			// those promises and return a single one (via Promise.all) to handle it in the next .then()
			let dtlsPromises = [];

			// Inspect each m= section in the SDP offer and create a m= section for the SDP response
			// Also create a RtpReceiver for each m= section
			offer.media.forEach((om, i) =>
			{
				let am = {};
				let transport;

				// Take the corresponding transport
				if (numTransports === 1)
					transport = transports[0];
				else
					transport = transports[i];

				am.mid = om.mid;
				am.connection = { ip: '1.2.3.4', version: 4 };  // TODO: fuck you SDP
				am.port = 12345;  // TODO: fuck you SDP
				am.protocol = om.protocol;
				am.iceUfrag = transport.iceLocalParameters.usernameFragment;
				am.icePwd = transport.iceLocalParameters.password;
				am.candidates = [];

				transport.iceLocalCandidates.forEach((candidate) =>
				{
					let ac = {};

					ac.component = transport.iceComponent === 'RTP' ? 1 : 2;
					ac.foundation = candidate.foundation;
					ac.ip = candidate.ip;
					ac.port = candidate.port;
					ac.priority = candidate.priority;
					ac.transport = candidate.protocol;
					ac.type = candidate.type;
					if (candidate.tcpType)
						ac.tcpType = candidate.tcpType;

					am.candidates.push(ac);
				});

				am.fingerprint =
				{
					type : 'sha-224',
					hash : mediasoup.extra.fingerprintToSDP(transport.dtlsLocalParameters.fingerprints['sha-224'])
				};

				// TODO: TEST
				let rtpReceiver = mediaPeer.RtpReceiver(transport);

				rtpReceiver.on('close', () =>
				{
					debug('rtpReceiver "close" event');
				});

				let codecs = [];

				om.rtp.forEach((rtp) =>
				{
					let codecName = rtp.codec;
					let codecPayloadType = rtp.payload;
					let codecClockRate = rtp.rate || null;
					let codecParameters = {};

					if (Array.isArray(om.fmtp))
					{
						om.fmtp.filter((fmtp) => fmtp.payload === codecPayloadType).forEach((fmtp) =>
						{
							let params = sdpTransform.parseFmtpConfig(fmtp.config);

							for (let k in params)
								codecParameters[k] = params[k];
						});
					}

					codecs.push(
						{
							name        : codecName,
							kind        : om.type,
							payloadType : codecPayloadType,
							clockRate   : codecClockRate,
							parameters  : codecParameters
						});
				});

				let encodings = [];

				if (Array.isArray(om.ssrcs))
				{
					let ssrc = om.ssrcs[0].id;

					encodings.push(
						{
							ssrc : ssrc
						});
				}

				rtpReceiver.receive(
					{
						codecs    : codecs,
						encodings : encodings
					});

				// rtpReceiver.dump()
				// 	.then((data) => debug('RTP_RECEIVER DUMP:\n%s', JSON.stringify(data, null, '\t')))
				// 	.catch((error) => debugerror('RTP_RECEIVER DUMP ERROR: %s', error));

				am.type = om.type;
				am.rtp = om.rtp;
				am.fmtp = om.fmtp;
				am.direction = om.direction === 'recvonly' ? 'sendonly' : 'sendrecv';
				am.payloads = om.payloads;
				am.rtcpMux = 'rtcp-mux';
				am.rtcp = om.rtcp;
				am.rtcpFb = om.rtcpFb;

				// Add the m= section to the SDP answer
				answer.media.push(am);

				// Set remote DTLS parameters into the transport
				// NOTE: Don't do twice for the same transport!
				if (transports[i])
				{
					// NOTE: Firefox uses global fingerprint attribute
					let remoteFingerprint = om.fingerprint || offer.fingerprint;

					// Create a promise
					let promise = transport.setRemoteDtlsParameters(
						{
							role        : 'client',  // SDP offer MUST always have a=setup:actpass so we can choose whatever we want
							fingerprint :
							{
								algorithm : remoteFingerprint.type,
								value     : mediasoup.extra.fingerprintFromSDP(remoteFingerprint.hash)
							}
						})
						.then(() =>
						{
							// Set the a=setup attribute for the SDP response
							am.setup = transport.dtlsLocalParameters.role === 'client' ? 'active' : 'passive';
						});

					// Add the promise to the array of promises
					dtlsPromises.push(promise);
				}
			});

			// Return a new promise for the next then()
			return Promise.all(dtlsPromises);
		})
		.then(() =>
		{
			debug('/test-transport succeeded, replying 200 with SDP answer');

			req.reply(200, 'OK',
				{
					type : 'answer',
					sdp  : sdpTransform.write(answer)
				});

			mediaPeer.dump()
				.then((data) => debug('PEER DUMP:\n%s', JSON.stringify(data, null, '\t')))
				.catch((error) => debugerror('PEER DUMP ERROR: %s', error));
		})
		.catch((error) =>
		{
			debugerror('/test-transport failed: %s', error);
			debugerror('stack:\n' + error.stack);

			req.reply(500, error.message);
		});
});

setInterval(() =>
{
	room.dump()
		.then((data) => debug('ROOM DUMP:\n%s', JSON.stringify(data, null, '\t')))
		.catch((error) => debugerror('ROOM DUMP ERROR: %s', error));
}, 20000);
