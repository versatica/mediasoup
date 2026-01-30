/**
 * Normalize compile_commands.json paths.
 *
 * Converts file paths to be relative to the repo root (cwd).
 * clang-tidy reports warnings using the path from compile_commands.json,
 * and clang-tidy-review's line filter uses paths relative to repo root.
 * These must match for the line filter to work correctly.
 */

import fs from 'node:fs';
import path from 'node:path';

const compileCommandsFile = process.argv.slice(2).join(' ');

if (!compileCommandsFile) {
	logError('compile_commands.json file missing');

	exitWithError();
}

void run();

function run() {
	logInfo(`cleaning up ${compileCommandsFile}`);

	const cwd = process.cwd();

	logInfo(`current working directory: ${cwd}`);

	try {
		const commands = JSON.parse(fs.readFileSync(compileCommandsFile, 'utf8'));

		for (const entry of commands) {
			if (entry.file && entry.directory) {
				// Resolve to absolute path first.
				const absolutePath = path.resolve(entry.directory, entry.file);

				// Convert to relative path from repo root (cwd).
				entry.file = path.relative(cwd, absolutePath);
			}
		}

		fs.writeFileSync(compileCommandsFile, JSON.stringify(commands, null, 2));
	} catch (err) {
		logError(`failed to clean up compile commands: ${err}`);

		exitWithError();
	}
}

function logInfo(message) {
	// eslint-disable-next-line no-console
	console.log(`clean-compile-commands.mjs \x1b[36m[INFO] \x1b[0m`, message);
}

// eslint-disable-next-line no-unused-vars
function logWarn(message) {
	// eslint-disable-next-line no-console
	console.warn(`clean-compile-commands.mjs \x1b[33m[WARN] \x1b[0m`, message);
}

function logError(message) {
	// eslint-disable-next-line no-console
	console.error(`clean-compile-commands.mjs \x1b[31m[ERROR] \x1b[0m`, message);
}

function exitWithError() {
	process.exit(1);
}
