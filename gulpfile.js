'use strict';

const gulp = require('gulp');
const eslint = require('gulp-eslint');
const replace = require('gulp-replace');
const touch = require('gulp-touch-cmd');
const shell = require('gulp-shell');
const clangFormat = require('gulp-clang-format');

let nodeFiles =
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
	'test/test_mediasoup.js',
	'test/test_Server.js',
	'test/test_Room.js',
	'test/test_Peer.js',
	'test/test_Transport.js',
	'test/test_RtpReceiver.js',
	'test/test_extra.js'
	// NOTE: Disable this test until fixed
	// 'test/test_scene_1.js'
];

gulp.task('lint:node', () =>
{
	return gulp.src(nodeFiles)
		.pipe(eslint())
		.pipe(eslint.format())
		.pipe(eslint.failAfterError());
});

gulp.task('lint:worker', () =>
{
	let src = workerFiles.concat(
		// Remove Ragel generated files.
		'!worker/src/Utils/IP.cpp'
	);

	return gulp.src(src)
		.pipe(clangFormat.checkFormat('file', null, { verbose: true, fail: true }));
});

gulp.task('format:worker', () =>
{
	return gulp.src(workerFiles, { base: '.' })
		.pipe(clangFormat.format('file'))
		.pipe(gulp.dest('.'));
});

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
		verbose : true,
		env     : { DEBUG: '*ABORT* *WARN*' }
	}
));

gulp.task('rtpcapabilities', () =>
{
	let supportedRtpCapabilities = require('./lib/supportedRtpCapabilities');

	return gulp.src('worker/src/RTC/Room.cpp')
		.pipe(replace(/(const std::string supportedRtpCapabilities =).*/,
			`$1 R"(${JSON.stringify(supportedRtpCapabilities)})";`))
		.pipe(gulp.dest('worker/src/RTC/'))
		.pipe(touch());
});

// TODO: Uncomment when C++ syntax and clang-format stuff is done.
// gulp.task('lint', gulp.series('lint:node', 'lint:worker'));
gulp.task('lint', gulp.series('lint:node'));

gulp.task('test', gulp.series('test:node', 'test:worker'));

gulp.task('default', gulp.series('lint', 'test'));
