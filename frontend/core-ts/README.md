# Whist Core Typescript Library

This project is an internal Whist library for shared React components, Typescript functions, CSS modules, etc., which are used in various other projects within the Whist frontend. If you write TypeScript code that you believe can be reused across various projects, then you should add it to this internal library and use the library within your project, for maximum code reusability.

## Development

Because `/core-ts` is decoupled from Electron or Create React App, we need a manual setup so that we can interact with code as we're developing. When developing, you can either develop directly for this library, or develop within another project and extract the relevant, project-independent TypeScript code pieces to be added to this library. All code part of this library should be fully project-independent, otherwise it belongs better within the specific project(s).

Regardless of how you develop for this library, your additions should be properly unit-tested to ensure that they perform as intended.

### Run Jest in Watch Mode

If you're just making small changes to some functions and want to make sure they pass the test suite, run `yarn test-watch`. This will start `jest --watch`, which runs the tests every time your code changes. It's a little nicer than manually re-triggering them.

### Hot Reload All of Core-TS

If you're making new functions or exploring API responses, you might like to be able to import any module from `/core-ts` and just run as a script. `/core-ts` has `nodemon`, a file watcher, as well as `ts-node`, a TypeScript REPL, installed just for this.

`yarn dev` in your terminal will start up `nodemon` and `ts-node`, and you'll see new console output every time you reload.

### Styling

This project follows the [Whist TypeScript Coding Philosophy](https://www.notion.so/whisthq/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289). Any contributions to this project should be in the form of PRs following the usual Whist TypeScript standards.

## Deployment

This project is not published to any **npm** registry, but is rather built and imported directly into other projects (i.e. `frontend/client-applications`) thanks to our monorepository project structure. For an example of how to import `core-ts` into your project, see [`frontend/client-applications/package.json`](https://github.com/whisthq/whist/blob/dev/frontend/client-applications/package.json).
