# Contributing to mediasoup

Thanks for taking the time to contribute to mediasoup! 🎉👍


## License

By contributing to mediasoup, you agree that your contributions will be licensed under its ISC License.


## Reporting Bugs

We primarily use GitHub as an issue tracker. Just open an issue in GitHub if you have encountered a bug in mediasoup.

If you have questions or doubts about mediasoup or need support, please use the mediasoup Discourse Group instead:

* https://mediasoup.discourse.group

If you got a crash in mediasoup, please try to provide a core dump into the issue report:

* https://mediasoup.org/support/#crashes-in-mediasoup-get-a-core-dump


## Pull Request Process

When creating a Pull Request for mediasoup, ensure that you run the following commands to verify that the code in your PR conforms to the code syntax of the project and does not break existing funtionality:

* `npm run lint`: Check JavaScript and C++ linting rules.
* `npm run typescript:build`: Compile TypeScript code (under `src/` folder) into JavaScript code (under `lib/` folder).
* `npm run test`: Run JavaScript and C++ test units.

The full list of `npm` scripts (and `make` tasks) is available in the [doc/Building.md](/doc/Building.md) file.

Once all these commands succeed, wait for the Travis CI checks to complete and verify they run successfully (otherwise the PR won't be accepted).


## Coding Style

In adition to automatic checks performed by commands above, we also enforce other minor things related to coding style:

### Comments in JavaScript and C++

We use `//` for inline comments in both JavaScript and C++ source files.

* Comments must start with upercase letter.
* Comments must not exceed 80 columns (split into different lines if necessary).
* Comments must end with a dot.

Example (good):

```js
// Calculate foo based on bar value.
const foo = bar / 2;
```

Example (bad):

```js
// calculate foo based on bar value
const foo = bar / 2;
```

When adding inline documentation for methods or functions, we use `/** */` syntax. Example:

```js
/**
 * Calculates current score for foo and bar.
 */
function calculateScore(): number
{
  // [...]
}
```
