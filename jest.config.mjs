const config = {
	verbose: true,
	testEnvironment: 'node',
	testRegex: 'node/src/test/test-.*\\.ts',
	transform: {
		'^.+\\.ts?$': ['ts-jest'],
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
