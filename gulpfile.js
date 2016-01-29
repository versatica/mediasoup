'use strict';

const gulp = require('gulp');
const jshint = require('gulp-jshint');
const jscs = require('gulp-jscs');
const stylish = require('gulp-jscs-stylish');
const shell = require('gulp-shell');

let tests =
[
	'test/test_mediasoup.js',
	'test/test_Server.js',
	'test/test_Room.js',
	'test/test_Peer.js',
	'test/test_Transport.js',
	'test/test_RtpReceiver.js',
	'test/test_RtpListener.js',
	'test/test_extra.js'
];

gulp.task('lint', () =>
{
	let src = [ 'gulpfile.js', 'lib/**/*.js', 'test/**/*.js' ];

	return gulp.src(src)
		.pipe(jshint('.jshintrc'))
		.pipe(jscs('.jscsrc'))
		.pipe(stylish.combineWithHintResults())
		.pipe(jshint.reporter('jshint-stylish', { verbose: true }))
		.pipe(jshint.reporter('fail'));
});

gulp.task('test', shell.task(
	[ `tap --bail --color --reporter=spec ${tests.join(' ')}` ]
));

gulp.task('test-debug', shell.task(
	[ `tap --bail --reporter=tap ${tests.join(' ')}` ],
	{
		env     : { DEBUG: '*ERROR* *WARN*' },
		verbose : true
	}
));

gulp.task('default', gulp.series('lint'));
