const gulp = require('gulp');
const shell = require('gulp-shell');
const clangFormat = require('gulp-clang-format');
const os = require('os');

const workerFiles =
[
	'worker/src/**/*.cpp',
	'worker/include/**/*.hpp',
	'worker/test/src/**/*.cpp',
	'worker/test/include/helpers.hpp',
	'worker/fuzzer/src/**/*.cpp',
	'worker/fuzzer/include/**/*.hpp'
];

gulp.task('lint:worker', () =>
{
	const src = workerFiles.concat(
		// Ignore Ragel generated files.
		'!worker/src/Utils/IP.cpp',
		// Ignore json.hpp.
		'!worker/include/json.hpp'
	);

	return gulp.src(src)
		.pipe(clangFormat.checkFormat('file', null, { verbose: true, fail: true }));
});

gulp.task('format:worker', () =>
{
	const src = workerFiles.concat(
		// Ignore Ragel generated files.
		'!worker/src/Utils/IP.cpp',
		// Ignore json.hpp.
		'!worker/include/json.hpp'
	);

	return gulp.src(src, { base: '.' })
		.pipe(clangFormat.format('file'))
		.pipe(gulp.dest('.'));
});

gulp.task('win:build', shell.task(
	[
		'echo build for windows',
		'cd worker && python ./scripts/configure.py --format=msvs',
		`MSBuild ./worker/out/mediasoup-worker.sln /p:Configuration=${process.env.MEDIASOUP_BUILDTYPE === 'Debug' ?'Debug' : 'Release'} -t:mediasoup-worker `
	],
	{
		verbose : true
	}
));

gulp.task('make:build', shell.task(
	[
		'make -C worker'
	],
	{
		verbose : true
	}
));

gulp.task('build', gulp.series(os.platform() === 'win32'? 'win:build' : 'make:build'));
