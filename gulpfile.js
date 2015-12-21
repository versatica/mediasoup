'use strict';

const gulp = require('gulp');
const jshint = require('gulp-jshint');
const jscs = require('gulp-jscs');
const stylish = require('gulp-jscs-stylish');
const shell = require('gulp-shell');

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
	[ 'node_modules/.bin/tap --bail --color --reporter=spec test/test_*.js' ]
));

gulp.task('test-debug', shell.task(
	[ 'node_modules/.bin/tap --bail --reporter=tap test/test_*.js' ],
	{
		env     : { DEBUG: '*ERROR* *WARN*' },
		verbose : true
	}
));

gulp.task('default', gulp.series('lint'));
