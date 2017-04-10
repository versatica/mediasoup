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
	'test/test-mediasoup.js',
	'test/test-Server.js',
	'test/test-Room.js',
	'test/test-Peer.js',
	'test/test-Transport.js',
	'test/test-RtpReceiver.js',
	'test/test-extra.js'
	// NOTE: Disable this test until adapted.
	// 'test/test-scene-1.js'
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
	let src = workerFiles.concat(
		// Remove Ragel generated files.
		'!worker/src/Utils/IP.cpp'
	);

	return gulp.src(src, { base: '.' })
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
		// Let's generate valid syntax as expected by clang-format rules.
		.pipe(replace(/(const std::string supportedRtpCapabilities =).*\r?\n.*/,
			`$1\n\t\t\t    R"(${JSON.stringify(supportedRtpCapabilities)})";`))
		.pipe(gulp.dest('worker/src/RTC/'))
		.pipe(touch());
});

gulp.task('lint', gulp.series('lint:node', 'lint:worker'));

gulp.task('format', gulp.series('format:worker'));

gulp.task('test', gulp.series('test:node', 'test:worker'));

gulp.task('default', gulp.series('lint', 'test'));
