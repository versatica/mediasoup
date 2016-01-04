#!/usr/bin/env node

'use strict';

process.env.DEBUG = '*ERROR* *WARN* mediasoup* mediasoup-tester*';

const http = require('http');
const url = require('url');

const debug = require('debug')('mediasoup-tester');
const debugerror = require('debug')('mediasoup-tester:ERROR');
const mediasoup = require('../../');
const protoo = require('protoo');
const sdpTransform = require('sdp-transform');

const LISTEN_IP = '0.0.0.0';
const LISTEN_PORT = 8080;

// protoo app
let app = protoo();

// mediasoup server
let server = mediasoup.Server({ numWorkers: 1 });
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

	// Create a mediasoup Peer instance
	let mediaPeer = room.Peer(peer.username);

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
	peer.data.mediaPeer.close();
});

// Handle PUT requests to /test-transport
app.put('/test-transport', function(req)
{
	// Retrieve the mediasoup Peer associated to the protoo peer who sent the request
	let mediaPeer = req.peer.data.mediaPeer;
	let sdpOffer = req.data.sdp;
	let offer = sdpTransform.parse(sdpOffer);
	let answer = {};

	answer.version = 0;
	answer.origin = offer.origin;
	answer.name = 'mediasoup';
	answer.timing = { start: 0, stop: 0 };
	answer.groups = offer.groups;
	answer.msidSemantic = offer.msidSemantic;
	answer.icelite = 'ice-lite';
	answer.media = [];

	mediaPeer.createTransport({ udp: true, tcp: false })
		.then((transport) =>
		{
			offer.media.forEach((om) =>
			{
				let am = {};

				am.mid = om.mid;
				am.connection = { ip: '1.2.3.4', version: 4 };
				am.port = 12345;
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
					type : 'sha-256',
					hash : mediasoup.extra.dtlsFingerprintToSDP(transport.dtlsLocalParameters.fingerprints['sha-256'])
				};
				// SDP offer is always 'a=setup:actpass'
				am.setup = 'passive';
				am.type = om.type;
				am.rtcpMux = 'rtcp-mux';
				am.rtcp = om.rtcp;
				am.rtcpFb = om.rtcpFb;
				am.rtp = om.rtp;
				am.fmtp = om.fmtp;
				am.direction = om.direction === 'recvonly' ? 'sendonly' : 'sendrecv';
				am.payloads = om.payloads;

				// Update the transport with the remote DTLS parameters
				setTimeout(() =>
				{
					transport.setRemoteDtlsParameters(
						{
							role        : 'client',
							fingerprint :
							{
								algorithm : om.fingerprint.type,
								value     : mediasoup.extra.dtlsFingerprintFromSDP(om.fingerprint.hash)
							}
						})
						.then((data) => debug('transport.start() success [data:%o]', data))
						.catch((error) => debugerror('transport.start() failed: %o', error));
				}, 0);

				answer.media.push(am);
			});

			let data =
			{
				type : 'answer',
				sdp  : sdpTransform.write(answer)
			};

			req.reply(200, 'OK', data);
		})
		.catch((error) =>
		{
			req.reply(400, error.message);
			return;
		});
});

setTimeout(() =>
{
	room.dump()
		.then((data) => debug('ROOM DUMP:\n%s', JSON.stringify(data, null, '\t')))
		.catch((error) => debugerror('SERVER DUMP ERROR: [status:%s, error:"%s"]', error.status, error));
}, 7000);
