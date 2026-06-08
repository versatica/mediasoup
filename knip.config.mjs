const config = {
	$schema: 'https://unpkg.com/knip@5/schema.json',
	project: ['node/src/**/*.ts'],
	ignore: [
		'node/src/fbsUtils.ts',
		'node/src/rtpParametersFbsUtils.ts',
		'node/src/rtpStreamStatsFbsUtils.ts',
		'node/src/srtpParametersFbsUtils.ts',
	],
	ignoreDependencies: ['open-cli', 'supports-color'],
	ignoreBinaries: ['python3'],
	typescript: {
		config: ['tsconfig.json'],
	},
	jest: {
		config: ['jest.config.mjs'],
		entry: ['node/src/test/**/*.ts'],
	},
};

export default config;
