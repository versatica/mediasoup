'use strict';

const gulp = require('gulp');
const eslint = require('gulp-eslint');
const replace = require('gulp-replace');
const rename = require('gulp-rename');
const touch = require('gulp-touch-cmd');
const shell = require('gulp-shell');
const clangFormat = require('gulp-clang-format');
const os = require('os');

const nodeFiles =
[
	'.eslintrc.js',
	'gulpfile.js',
	'lib/**/*.js',
	'test/**/*.js'
];
const workerFiles =
[
	'worker/src/**/*.cpp',
	'worker/include/**/*.hpp'
];
const nodeTests =
[
	// 'test/test-mediasoup.js',
	'test/test-Server.js'
	// 'test/test-Room.js',
	// 'test/test-Peer.js',
	// 'test/test-Transport.js',
	// 'test/test-Producer.js',
	// 'test/test-utils.js'
];
const workerCompilationDatabaseTemplate = 'worker/compile_commands_template.json';
const workerHeaderFilterRegex =
	'(common.hpp|DepLibSRTP.hpp|DepLibUV.hpp|DepOpenSSL.hpp|LogLevel.hpp|Logger.hpp' +
	'|Loop.hpp|MediaSoupError.hpp|Settings.hpp|Utils.hpp' +
	'|handles/*.hpp|Channel/*.hpp|RTC/**/*.hpp)';
const numCpus = os.cpus().length;

gulp.task('lint:node', () =>
{
	return gulp.src(nodeFiles)
		.pipe(eslint())
		.pipe(eslint.format())
		.pipe(eslint.failAfterError());
});

gulp.task('lint:worker', () =>
{
	const src = workerFiles.concat(
		// Remove Ragel generated files.
		'!worker/src/Utils/IP.cpp'
	);

	return gulp.src(src)
		.pipe(clangFormat.checkFormat('file', null, { verbose: true, fail: true }));
});

gulp.task('format:worker', () =>
{
	const src = workerFiles.concat(
		// Remove Ragel generated files.
		'!worker/src/Utils/IP.cpp'
	);

	return gulp.src(src, { base: '.' })
		.pipe(clangFormat.format('file'))
		.pipe(gulp.dest('.'));
});

gulp.task('tidy:worker:prepare', () =>
{
	return gulp.src(workerCompilationDatabaseTemplate)
		.pipe(replace(/PATH/gm, __dirname))
		.pipe(rename('compile_commands.json'))
		.pipe(gulp.dest('worker'))
		.pipe(touch());
});

gulp.task('tidy:worker:run', shell.task(
	[
		'cd worker && ' +
		'./scripts/clang-tidy.py ' +
		'-clang-tidy-binary=../node_modules/.bin/clang-tidy ' +
		'-clang-apply-replacements-binary=' +
		'../node_modules/.bin/clang-apply-replacements ' +
		`-header-filter='${workerHeaderFilterRegex}' ` +
		'-p=. ' +
		`-j=${numCpus} ` +
		`-checks=${process.env.MEDIASOUP_TIDY_CHECKS || ''} ` +
		'-quiet ' +
		`${process.env.MEDIASOUP_TIDY_FIX === '1' ? '-fix -format' : ''}`
	],
	{
		verbose : true
	}
));

gulp.task('tidy:worker', gulp.series('tidy:worker:prepare', 'tidy:worker:run'));

gulp.task('test:node', shell.task(
	[
		'if type make &> /dev/null; then make; fi',
		`tap --bail --color --reporter=spec ${nodeTests.join(' ')}`
	],
	{
		verbose : true,
		env     : { DEBUG: '*ABORT* *WARN*' }
	}
));

gulp.task('test:worker', shell.task(
	[
		'if type make &> /dev/null; then make test; fi',
		`cd worker && ./out/${process.env.MEDIASOUP_BUILDTYPE === 'Debug' ?
			'Debug' : 'Release'}/mediasoup-worker-test --invisibles --use-colour=yes`
	],
	{
		verbose : true
	}
));

gulp.task('lint', gulp.series('lint:node', 'lint:worker'));

gulp.task('format', gulp.series('format:worker'));

gulp.task('tidy', gulp.series('tidy:worker'));

gulp.task('test', gulp.series('test:node', 'test:worker'));

gulp.task('default', gulp.series('lint', 'test'));
