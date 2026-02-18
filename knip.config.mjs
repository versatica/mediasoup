const config = {
	$schema: 'https://unpkg.com/knip@5/schema.json',
	entry: ['node/src/index.ts', 'node/src/types.ts', 'node/src/extras.ts'],
	project: ['node/src/**/*.ts'],
	ignore: [
		'node/src/fbsUtils.ts',
		'node/src/rtpParametersFbsUtils.ts',
		'node/src/rtpStreamStatsFbsUtils.ts',
		'node/src/srtpParametersFbsUtils.ts',
	],
	ignoreDependencies: ['open-cli', 'supports-color'],
	typescript: {
		config: ['tsconfig.json'],
	},
	jest: {
		config: ['jest.config.mjs'],
		entry: ['node/src/test/**/*.ts'],
	},
};

export default config;
