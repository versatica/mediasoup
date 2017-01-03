'use strict';

const gulp = require('gulp');
const eslint = require('gulp-eslint');
const replace = require('gulp-replace');
const touch = require('gulp-touch');
const shell = require('gulp-shell');

let tests =
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

gulp.task('lint', () =>
{
	let src =
	[
		'.eslintrc.js',
		'gulpfile.js',
		'lib/**/*.js',
		'data/**/*.js',
		'test/**/*.js',
		'!node_modules/**'
	];

	return gulp.src(src)
		.pipe(eslint())
		.pipe(eslint.format())
		.pipe(eslint.failAfterError());
});

gulp.task('capabilities', () =>
{
	let capabilities = require('./data/supportedCapabilities');

	return gulp.src('worker/src/RTC/Room.cpp')
		.pipe(replace(/(std::string capabilities =).*/, `$1 R"(${JSON.stringify(capabilities)})";`))
		.pipe(gulp.dest('worker/src/RTC/'))
		.pipe(touch());
});

gulp.task('test', shell.task(
	[ `tap --bail --color --reporter=spec ${tests.join(' ')}` ],
	{
		env : { DEBUG: '*ABORT*' }
	}
));

gulp.task('test-debug', shell.task(
	[ `tap --bail --reporter=tap ${tests.join(' ')}` ],
	{
		env     : { DEBUG: '*WARN* *ERROR* *ABORT*' },
		verbose : true
	}
));

gulp.task('test-worker', shell.task(
	[ 'worker/out/Debug/mediasoup-worker-test' ],
	{
		env     : { DEBUG: '*WARN* *ERROR* *ABORT*' },
		verbose : true
	}
));

gulp.task('default', gulp.series('lint'));
