# Fractal Core Typescript Library

This project is an internal Fractal library for shared React components, Typescript functions, CSS modules, etc., which are used in various other projects within Fractal. If you write TypeScript code that you believe can be reused across various projects, then you should add it to this internal library and use the library within your project, for maximum code reusability.

## Development Workflow

One of the goals of `core-ts` is to extract out common code in the Fractal website and Fractal client application. Those code bases are run in a browser environment and setup with Hot Module Reloading, making it easy to save a file and see how your changes work.

Because `core-ts` is decoupled from Electron or Create React App, we need a manual setup so that we can interact with code as we're developing. Here's a few ways may want to work, depending on the job you're doing.

##### 1. Run Jest in watch mode

If you're just making small changes to some functions and want to make sure they pass the test suite, run `yarn run test-watch`. This will start `jest --watch`, which runs the tests every time your code changes. It's a little nicer than manually re-triggering them.

##### 2. Hot reload all of core-ts

If you're making new functions or exploring API responses, you might like to be able to import any module from `core-ts` and just run as a script. `core-ts` has `nodemon`, a file watcher, as well as `ts-node`, a TypeScript REPL, installed just for this. Currently, `src/index.ts` is empty and is not a bad file to use as a "scratch pad", because it can easily import the rest of the submodules in `core-ts`. Just don't commit your "scratch pad" changes.

You might setup up `src/index.ts` with the boilerplate below to get you started. Note the relative path imports, and the IIFE function attached to `const _ =`, which allows you to use `async/await` syntax.

```js
import { withGet, fetchBase } from "./http"
import { decorate } from "./utilitles"

const get = decorate(fetchBase, withGet)
const url = "www.my-test-server/endpoint"

let x
const _ = (async () => {
    x = await get({ url })
    console.log(x)
})()
```

`yarn run dev` in your terminal will start up `nodemon` and `ts-node`, and you'll see new console output every time you reload.

## Development

When developing, you can either develop directly for this library, or develop within another project and extract the relevant, project-independent TypeScript code pieces to be added to this library. All code part of this library should be fully project-independent, otherwise it belongs better within the specific project(s).

Regardless of how you develop for this library, your additions should be properly unit-tested to ensure that they perform as intended.

## Publishing

This project is not published to any **npm** registry, but is rather built and imported directly into other projects (i.e. `/client-applications/desktop`) thanks to our monorepo project structure. For an example of how to import `core-ts` into your project, see `/client-applications/desktop/package.json`.

## Style

This project follows the [Fractal TypeScript Coding Philosophy](https://www.notion.so/tryfractal/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289). Any contributions to this project should be in the form of PRs following the usual Fractal TypeScript standards.
