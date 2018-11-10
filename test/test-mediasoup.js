const path = require('path');
const tap = require('tap');
const mediasoup = require('../');

tap.test(
	'mediasoup.Server() with no options must succeed', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server();

		server.on('close', () => t.end());

		setTimeout(() => server.close(), 10);
	});

tap.test(
	'mediasoup.Server() with valid options must succeed', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server(
			{
				numWorkers : 1,
				logLevel   : 'warn'
			});

		server.on('close', () => t.end());

		setTimeout(() => server.close(), 100);
	});

tap.test(
	'mediasoup.Server() with valid DTLS certificate must succeed',
	{ timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server(
			{
				numWorkers          : 1,
				dtlsCertificateFile : path.join(__dirname, 'data', 'dtls-cert.pem'),
				dtlsPrivateKeyFile  : path.join(__dirname, 'data', 'dtls-key.pem')
			});

		server.on('close', () => t.end());

		setTimeout(() => server.close(), 100);
	});

tap.test(
	'mediasoup.Server() with wrong options must fail', { timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server({ logLevel: 'WRONG_VALUE' });

		server.on('close', () => t.end());
	});

tap.test(
	'mediasoup.Server() with non existing rtcIPv4 IP must fail',
	{ timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server({ rtcIPv4: '1.2.3.4' });

		server.on('close', () => t.end());
	});

tap.test(
	'mediasoup.Server() with too narrow RTC ports range must fail',
	{ timeout: 2000 }, (t) =>
	{
		const server = mediasoup.Server({ rtcMinPort: 2000, rtcMaxPort: 2050 });

		server.on('close', () => t.end());
	});
