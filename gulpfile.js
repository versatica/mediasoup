'use strict';

const gulp = require('gulp');
const jshint = require('gulp-jshint');
const jscs = require('gulp-jscs');
const stylish = require('gulp-jscs-stylish');

gulp.task('lint', () =>
{
	let src = [ 'gulpfile.js', 'lib/**/*.js' ];

	return gulp.src(src)
		.pipe(jshint('.jshintrc'))
		.pipe(jscs('.jscsrc'))
		.pipe(stylish.combineWithHintResults())
		.pipe(jshint.reporter('jshint-stylish', { verbose: true }))
		.pipe(jshint.reporter('fail'));
});

gulp.task('default', gulp.series('lint'));
