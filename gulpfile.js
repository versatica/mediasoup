const gulp = require('gulp');
const clangFormat = require('gulp-clang-format');

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
