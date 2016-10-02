'use strict';

const tap = require('tap');
const extra = require('../').extra;

const sdpFingerprint1 = '75:1B:81:93:B7:ED:27:7E:42:BE:D6:C4:8E:F7:04:3A:49:CE:3F:EE';
const rawFingerprint1 = '751b8193b7ed277e42bed6c48ef7043a49ce3fee';
const sdpFingerprint2 = '46:6D:6F:3D:09:A6:19:B2:27:C3:37:08:A7:CD:91:AE:0F:67:C4:BB:9A:A7:97:FD:40:BA:DF:3B:2C:C2:38:8A:38:5F:61:C2:A6:70:01:0C:7F:4D:C6:27:FF:31:5A:20:21:4B:F2:1D:B5:60:E2:4A:2E:67:31:B2:B0:2D:48:47';
const rawFingerprint2 = '466d6f3d09a619b227c33708a7cd91ae0f67c4bb9aa797fd40badf3b2cc2388a385f61c2a670010c7f4dc627ff315a20214bf21db560e24a2e6731b2b02d4847';

tap.test('extra.fingerprintFromSDP()', { timeout: 1000 }, (t) =>
{
	t.equal(extra.fingerprintFromSDP(sdpFingerprint1), rawFingerprint1);
	t.equal(extra.fingerprintFromSDP(sdpFingerprint2), rawFingerprint2);

	t.end();
});

tap.test('extra.fingerprintToSDP()', { timeout: 1000 }, (t) =>
{
	t.equal(extra.fingerprintToSDP(rawFingerprint1), sdpFingerprint1);
	t.equal(extra.fingerprintToSDP(rawFingerprint2), sdpFingerprint2);

	t.end();
});

tap.test('extra.paramFromSDP()', { timeout: 1000 }, (t) =>
{
	t.equal(extra.paramFromSDP('profile-level-id'), 'profileLevelId');
	t.equal(extra.paramFromSDP('Packetization-MODE'), 'packetizationMode');
	t.equal(extra.paramFromSDP('foo'), 'foo');
	t.equal(extra.paramFromSDP('wrong-param-'), 'wrongParam');

	t.end();
});

tap.test('extra.paramToSDP()', { timeout: 1000 }, (t) =>
{
	t.equal(extra.paramToSDP('profileLevelId'), 'profile-level-id');
	t.equal(extra.paramToSDP('packetizationMode'), 'packetization-mode');
	t.equal(extra.paramToSDP('Foo'), 'foo');
	t.equal(extra.paramToSDP('WrongParam'), 'wrong-param');

	t.end();
});
