'use strict';

const tap = require('tap');
const utils = require('../lib/utils');

tap.test('utils.paramToSDP()', { timeout: 1000 }, (t) =>
{
	t.equal(utils.paramToSDP('profileLevelId'), 'profile-level-id');
	t.equal(utils.paramToSDP('packetizationMode'), 'packetization-mode');
	t.equal(utils.paramToSDP('Foo'), 'foo');
	t.equal(utils.paramToSDP('WrongParam'), 'wrong-param');

	// Test SDP codec parameters that don't follow the camelcase convention of ORTC.
	t.equal(utils.paramToSDP('maxPlaybackRate'), 'maxplaybackrate');
	t.equal(utils.paramToSDP('MAXPLAYBACKRATE'), 'maxplaybackrate');
	t.equal(utils.paramToSDP('spropMaxCaptureRate'), 'sprop-maxcapturerate');
	t.equal(utils.paramToSDP('maxAverageBitrate'), 'maxaveragebitrate');
	t.equal(utils.paramToSDP('useInbandFec'), 'useinbandfec');
	t.equal(utils.paramToSDP('useDtx'), 'usedtx');
	t.equal(utils.paramToSDP('spropMaxCaptureRate'), 'sprop-maxcapturerate');
	t.equal(utils.paramToSDP('l'), 'L');
	t.equal(utils.paramToSDP('L'), 'L');
	t.equal(utils.paramToSDP('d'), 'D');
	t.equal(utils.paramToSDP('toP'), 'ToP');

	t.end();
});
