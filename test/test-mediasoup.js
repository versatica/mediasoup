const { toBeType } = require('jest-tobetype');
const pkg = require('../package.json');
const mediasoup = require('../');
const {
	version,
	getSupportedRtpCapabilities,
	parseScalabilityMode
} = mediasoup;

expect.extend({ toBeType });

test('mediasoup.version exposes the package version', () =>
{
	expect(version).toBeType('string');
	expect(version).toBe(pkg.version);
}, 500);

test('mediasoup.getSupportedRtpCapabilities() returns the mediasoup RTP capabilities', () =>
{
	const rtpCapabilities = getSupportedRtpCapabilities();

	expect(rtpCapabilities).toBeType('object');

	// Mangle retrieved codecs to check that, if called again,
	// getSupportedRtpCapabilities() returns a cloned object.
	rtpCapabilities.codecs = 'bar';

	const rtpCapabilities2 = getSupportedRtpCapabilities();

	expect(rtpCapabilities2).not.toEqual(rtpCapabilities);
}, 500);

test('mediasoup.parseScalabilityMode() works', () =>
{
	expect(parseScalabilityMode('L1T3')).toEqual({ spatialLayers: 1, temporalLayers: 3 });
	expect(parseScalabilityMode('L3T2_KEY')).toEqual({ spatialLayers: 3, temporalLayers: 2 });
	expect(parseScalabilityMode('foo')).toEqual({ spatialLayers: 1, temporalLayers: 1 });
	expect(parseScalabilityMode(null)).toEqual({ spatialLayers: 1, temporalLayers: 1 });
}, 500);
