const gulp = require('gulp');
const clangFormat = require('gulp-clang-format');

const workerFiles =
[
	'../src/**/*.cpp',
	'../include/**/*.hpp',
	'../test/src/**/*.cpp',
	'../test/include/helpers.hpp',
	'../fuzzer/src/**/*.cpp',
	'../fuzzer/include/**/*.hpp'
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
