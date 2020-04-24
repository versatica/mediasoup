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
	return gulp.src(workerFiles)
		.pipe(clangFormat.checkFormat('file', null, { verbose: true, fail: true }));
});

gulp.task('format:worker', () =>
{
	return gulp.src(workerFiles, { base: '.' })
		.pipe(clangFormat.format('file'))
		.pipe(gulp.dest('.'));
});
