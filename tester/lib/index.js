'use strict';

const debug = require('debug')('mediasoup-tester');
const debugerror = require('debug')('mediasoup-tester:ERROR');

debugerror.log = console.error.bind(console);

const domready = require('domready');
const rtcninja = require('rtcninja');
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

	// Create peers
	let alice = new Peer(protooWsUrl, 'alice', 'aaaa');

	// Call the 'test-transport' service
	alice.once('online', () => alice.testTransport());
}
