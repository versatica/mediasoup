const { toBeType } = require('jest-tobetype');
const pkg = require('../package.json');
const mediasoup = require('../');
const { version, getSupportedMediaCodecs } = mediasoup;

expect.extend({ toBeType });

test('mediasoup.version exposes the package version', () =>
{
	expect(version).toBeType('string');
	expect(version).toBe(pkg.version);
}, 2000);

test('mediasoup.getSupportedMediaCodecs() provides cloned supported media codecs', () =>
{
	const mediaCodecs1 = getSupportedMediaCodecs();

	expect(mediaCodecs1).toBeType('array');

	// Mangle retrieved codecs to check that, if called again, getSupportedMediaCodecs()
	// returns cloned media codecs.
	mediaCodecs1.foo = 'bar';

	const mediaCodecs2 = getSupportedMediaCodecs();

	expect(mediaCodecs2).not.toEqual(mediaCodecs1);
}, 2000);
