import assert = require('node:assert');
import { afterEach, beforeEach, describe, it } from 'node:test';

const buildType = process.env.MEDIASOUP_BUILDTYPE ?? 'Release';

// eslint-disable-next-line @typescript-eslint/no-var-requires
const { WorkerChannel: NativeWorkerChannel } = require(
	`../build/${buildType}/worker-channel.node`
);

const closeRequest = new Uint8Array([
  12, 0,  0, 0, 8,  0, 10, 0, 9, 0,  4,
   0, 8,  0, 0, 0, 16,  0, 0, 0, 0,  1,
  10, 0, 12, 0, 8,  0,  0, 0, 4, 0, 10,
   0, 0,  0, 8, 0,  0,  0, 1, 0, 0,  0,
   0, 0,  0, 0, 0,  0,  0, 0
]);

describe('NativeWorkerChannel constructor', () => {
	it('fails if no argument is passed', () => {
		assert.throws(() => new NativeWorkerChannel(), TypeError);
	});

	it('fails if a single argument is passed', () => {
		const func = () => {};

		assert.throws(() => new NativeWorkerChannel(func), TypeError);
	});

	[true, 1, 'one'].forEach(func => {
		it(`fails if the first argument ("${func}") is not a Function`, () => {
			const version = 'X';

			assert.throws(() => new NativeWorkerChannel(func, version), TypeError);
		});
	});

	[true, 1, () => {}].forEach(version => {
		it(`fails if the second argument ("${version}") is not a String`, () => {
			const func = () => {};

			assert.throws(() => new NativeWorkerChannel(func, version), TypeError);
		});
	});

	[true, 1, 'one', () => {}].forEach(args => {
		it(`fails if the third argument is present ("${args}") and is not an Array`, () => {
			const func = () => {};
			const version = 'X';

			assert.throws(
				() => new NativeWorkerChannel(func, version, args),
				TypeError
			);
		});
	});

	[true, 1, () => {}].forEach(item => {
		it(`fails if the third argument is present and contains a non String item (${item})`, () => {
			const func = () => {};
			const version = 'X';
			const args = ['one', 'two', 'three'];

			// @ts-ignore.
			args.push(item);

			assert.throws(
				() => new NativeWorkerChannel(func, version, args),
				TypeError
			);
		});
	});

	it('succeeds if the given arguments are a Function and a String respectively', () => {
		const func = () => {};
		const version = 'X';

		assert.doesNotThrow(() => new NativeWorkerChannel(func, version));
	});

	it('succeeds if the third argument is an Array of Strings', () => {
		const func = () => {};
		const version = 'X';
		const args = ['one', 'two', 'three'];

		assert.doesNotThrow(() => new NativeWorkerChannel(func, version, args));
	});
});

describe.only('NativeWorkerChannel send', () => {
	const version = 'X';
	const func = () => {};

	let workerChannel: any;

	beforeEach(() => {
		workerChannel = new NativeWorkerChannel(func, version);
	});

	afterEach(() => {
		workerChannel.send(closeRequest);
	});

	[true, 1, 'one', () => {}].forEach(data => {
		it(`fails if send() is called with a value (${data}) different than an Uint8Array`, () => {
			assert.throws(() => workerChannel.send(data), TypeError);
		});
	});

	it(`succeeds if send() is called with a Uint8Array`, () => {
		const data = new Uint8Array(2);

		workerChannel.send(data);
	});
});
