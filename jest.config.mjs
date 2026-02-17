const config = {
	verbose: true,
	testEnvironment: 'node',
	testRegex: 'node/src/test/test-.*\\.ts',
	transform: {
		// transpilation: true is needed to avoid warnigns. However we lose TS
		// checks. We don't care since we have TS tasks for that.
		// See https://kulshekhar.github.io/ts-jest/docs/getting-started/options/transpilation
		'^.+\\.ts?$': ['ts-jest', { transpilation: true }],
	},
	coveragePathIgnorePatterns: [
		'node/src/Logger.ts',
		'node/src/enhancedEvents.ts',
		'node/src/fbs',
		'node/src/test',
	],
	modulePathIgnorePatterns: ['worker', 'rust', 'target'],
	cacheDirectory: '.cache/jest',
};

export default config;
