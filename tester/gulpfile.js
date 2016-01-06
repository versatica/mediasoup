'use strict';

const path = require('path');

const gulp = require('gulp');
const rename = require('gulp-rename');
const browserify = require('browserify');
const vinylSourceStream = require('vinyl-source-stream');
const del = require('del');
const mkdirp = require('mkdirp');
const ncp = require('ncp');
const jshint = require('gulp-jshint');
const jscs = require('gulp-jscs');
const stylish = require('gulp-jscs-stylish');
const browserSync = require('browser-sync').create();

const PKG = require('./package.json');
const OUTPUT_DIR = 'output';
const OUTPUT =
{
	HTML      : 'index.html',
	JS        : PKG.name + '.js',
	CSS       : PKG.name + '.css',
	RESOURCES : 'resources'
};

// gulp.task('clean:css', () => del(path.join(OUTPUT_DIR, OUTPUT.CSS)));

gulp.task('clean:html', () => del(path.join(OUTPUT_DIR, OUTPUT.HTML)));

gulp.task('clean:js', () => del([path.join(OUTPUT_DIR, OUTPUT.JS), 'build']));

gulp.task('clean:resources', () => del(path.join(OUTPUT_DIR, OUTPUT.RESOURCES)));

gulp.task('clean', () => del(OUTPUT_DIR));

gulp.task('lint:js', () =>
{
	let src = [ 'gulpfile.js', 'lib/**/*.js', 'server/**/*.js' ];

	return gulp.src(src)
		.pipe(jshint('.jshintrc'))
		.pipe(jscs('.jscsrc'))
		.pipe(stylish.combineWithHintResults())
		.pipe(jshint.reporter('jshint-stylish', { verbose: true }))
		.pipe(jshint.reporter('fail'));
});

gulp.task('lint', gulp.series('lint:js'));

gulp.task('build:html:copy', () =>
{
	return gulp.src('index.html')
		.pipe(rename(OUTPUT.HTML))
		.pipe(gulp.dest(OUTPUT_DIR));
});

gulp.task('build:html', gulp.series('clean:html', 'build:html:copy'));

gulp.task('build:resources:copy', (done) =>
{
	let dst = path.join(OUTPUT_DIR, OUTPUT.RESOURCES);

	mkdirp.sync(dst);

	ncp('resources', dst, { stopOnErr: true }, (error) =>
	{
		if (error && error[0].code !== 'ENOENT')
			throw new Error(`resources copy failed: ${error}`);

		done();
	});
});

gulp.task('build:resources', gulp.series('clean:resources', 'build:resources:copy'));

gulp.task('build:js:browserify', () =>
{
	return browserify([path.join(__dirname, PKG.main)])
		.bundle()
			.pipe(vinylSourceStream(PKG.name + '.js'))
			.pipe(rename(PKG.name + '.js'))
			.pipe(gulp.dest(OUTPUT_DIR));
});

gulp.task('build:js', gulp.series('clean:js', 'build:js:browserify'));

gulp.task('build', gulp.parallel(
	'build:js',
	'build:html',
	// 'build:css',
	'build:resources'
));

gulp.task('live:sync', (done) =>
{
	browserSync.init(
		{
			server :
			{
				baseDir   : OUTPUT_DIR,
				open      : false,
				directory : false
			},
			https : false
		});

	done();
});

gulp.task('live:reload', (done) =>
{
	browserSync.reload();

	done();
});

gulp.task('live:watch', (done) =>
{
	gulp.watch([ 'index.html' ], gulp.series(
		'build:html', 'live:reload'
	));

	gulp.watch(['resources/**/*'], gulp.series(
		'build:resources', 'live:reload'
	));

	gulp.watch(['lib/**/*.js'], gulp.series(
		'lint:js', 'build:js:browserify', 'live:reload'
	));

	done();
});

gulp.task('live', gulp.parallel(
	'live:watch',
	'live:sync'
));

gulp.task('default', gulp.series('lint', 'build', 'live'));
