const { toBeType } = require('jest-tobetype');
const pkg = require('../package.json');
const mediasoup = require('../');
const { version, getSupportedRtpCapabilities } = mediasoup;

expect.extend({ toBeType });

test('mediasoup.version exposes the package version', () =>
{
	expect(version).toBeType('string');
	expect(version).toBe(pkg.version);
}, 2000);

test('mediasoup.getSupportedRtpCapabilities() returns the mediasoup RTP capabilities', () =>
{
	const rtpCapabilities = getSupportedRtpCapabilities();

	expect(rtpCapabilities).toBeType('object');

	// Mangle retrieved codecs to check that, if called again,
	// getSupportedRtpCapabilities() returns a cloned object.
	rtpCapabilities.codecs = 'bar';

	const rtpCapabilities2 = getSupportedRtpCapabilities();

	expect(rtpCapabilities2).not.toEqual(rtpCapabilities);
}, 2000);
