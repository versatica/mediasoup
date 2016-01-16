'use strict';

const debug = require('debug')('mediasoup-tester');
const debugerror = require('debug')('mediasoup-tester:ERROR');

debugerror.log = console.error.bind(console);

const domready = require('domready');
const rtcninja = require('rtcninja');
const randomString = require('random-string');
const Peer = require('./Peer');

let protooWsUrl;

if (global.location.protocol === 'https:')
	protooWsUrl = 'wss://' + global.location.hostname + ':8443';
else
	protooWsUrl = 'ws://' + global.location.hostname + ':8080';

domready(() =>
{
	debug('DOM ready');

	// Ensure this is a WebRTC capable browser
	rtcninja();
	if (!rtcninja.hasWebRTC())
		throw new Error('non a WebRTC capable browser');

	run();
});

function run()
{
	debug('run()');

	let username = randomString();
	let uuid = randomString();
	// let username = 'alice';
	// let uuid = 'aaaa';

	let peer = new Peer(protooWsUrl, username, uuid);

	// Call the 'test-transport' service
	peer.once('ready', () =>
	{
		peer.testTransport();
	});

	// global.document.addEventListener('click', () => peer.close());

	document.querySelector('.click2closeProtoo').addEventListener('click', () =>
		{
			debug('closing protoo-client');
			peer.closeProtoo();
		});

	document.querySelector('.click2closePeerConnection').addEventListener('click', () =>
		{
			debug('closing RTCPeerConnection');
			peer.closePeerConnection();
		});
}
