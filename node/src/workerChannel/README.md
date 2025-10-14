## Testing

These tests are to be run locally only.
When running them within a testing environment (jest|node) the native addon
handle will remain open and the test process won't terminate, making any
CI process fail. This is because Node GC does not clean up the native addon handle
right away.

### Running tests

```sh
npx ts-node src/test.ts
```
