'use strict';

const gulp = require('gulp');
const jshint = require('gulp-jshint');
const jscs = require('gulp-jscs');
const stylish = require('gulp-jscs-stylish');
const replace = require('gulp-replace');
const touch = require('gulp-touch');
const shell = require('gulp-shell');

let tests =
[
	// 'test/test_mediasoup.js',
	// 'test/test_Server.js',
	'test/test_Room.js',
	'test/test_Peer.js',
	'test/test_Transport.js',
	'test/test_RtpReceiver.js',
	// 'test/test_extra.js'
	// NOTE: Disable this test until fixed
	// 'test/test_scene_1.js'
];

gulp.task('lint', () =>
{
	let src = [ 'gulpfile.js', 'lib/**/*.js', 'data/**/*.js', 'test/**/*.js' ];

	return gulp.src(src)
		.pipe(jshint('.jshintrc'))
		.pipe(jscs('.jscsrc'))
		.pipe(stylish.combineWithHintResults())
		.pipe(jshint.reporter('jshint-stylish', { verbose: true }))
		.pipe(jshint.reporter('fail'));
});

gulp.task('capabilities', () =>
{
	let capabilities = require('./data/supportedCapabilities');

	return gulp.src('worker/src/RTC/Room.cpp')
		.pipe(replace(/(std::string capabilities =).*/, `$1 R"(${JSON.stringify(capabilities)})";`))
		.pipe(gulp.dest('worker/src/RTC/'))
		.pipe(touch());
});

gulp.task('worker', shell.task(
	[ `make` ]
));

gulp.task('test', shell.task(
	[ `tap --bail --color --reporter=spec ${tests.join(' ')}` ],
	{
		env : { DEBUG: '*ABORT*' }
	}
));

gulp.task('test-debug', shell.task(
	[ `tap --bail --reporter=tap ${tests.join(' ')}` ],
	{
		env     : { DEBUG: '*ERROR* *WARN*' },
		verbose : true
	}
));

gulp.task('t', gulp.series('test'));

gulp.task('td', gulp.series('test-debug'));

gulp.task('make', gulp.series('capabilities', 'worker'));

gulp.task('default', gulp.series('lint'));
