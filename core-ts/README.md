# Whist Core Typescript Library

This project is an internal Whist library for shared React components, Typescript functions, CSS modules, etc., which are used in various other projects within Whist. If you write TypeScript code that you believe can be reused across various projects, then you should add it to this internal library and use the library within your project, for maximum code reusability.

It's important to note that development in this repository works a little differently than the rest of the Whist organization. In `core-ts`, we try and follow standard practices for `npm` package development. This means that we do not have `dev` and `staging` branches, and PRs are merged directly into `prod`. This means that every push will trigger a new `npm` package release with an updated version number.

If you have `core-ts` as a dependency in another `fractal` repository, you should "pin" your dependency version. This means your `package.json` dependencies should contain `@fractal/core-ts: 1.0.1`, not `@fractal/core-ts: ^1.0.1`. Install a specific version with `yarn add @fractal/core-ts@1.0.1`, after following the authentication instructions below.

## Development Workflow

One of the goals of `core-ts` is to extract out common code in the Whist website and Whist client application. Those code bases are run in a browser environment and setup with Hot Module Reloading, making it easy to save a file and see how your changes work.

Because `core-ts` is decoupled from Electron or Create React App, we need a manual setup so that we can interact with code as we're developing. Here's a few ways may want to work, depending on the job you're doing.

##### 1. Run Jest in watch mode

If you're just making small changes to some functions and want to make sure they pass the test suite, run `yarn test-watch`. This will start `jest --watch`, which runs the tests every time your code changes. It's a little nicer than manually re-triggering them.

##### 2. Hot reload all of core-ts

If you're making new functions or exploring API responses, you might like to be able to import any module from `core-ts` and just run as a script. `core-ts` has `nodemon`, a file watcher, as well as `ts-node`, a TypeScript REPL, installed just for this. Currently, `src/index.ts` is empty and is not a bad file to use as a "scratch pad", because it can easily import the rest of the submodules in `core-ts`. Just don't commit your "scratch pad" changes.

You might setup up `src/index.ts` with the boilerplate below to get you started. Note the relative path imports, and the IIFE function attached to `const _ =`, which allows you to use `async/await` syntax.

```javascript
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

`yarn dev` in your terminal will start up `nodemon` and `ts-node`, and you'll see new console output every time you reload.

##### 3. Link your local core-ts repos to other projects

The best kind of testing is using `core-ts` in working projects. If you're working on `core-ts/http`, you might want to import your functions into `fractal/website` so you can make sure they work in a real environment. You don't want to have to commit and publish for every change, however, so we use `yarn link`. This command will create symlinks in `fractal/website`, pointing the corresponding `import` statements to your local `core-ts` repo.

There's a few things that have to happen to make this work. First of all, you can't `link` your source TypeScript files. You have to link the JavaScript output from TypeScript compilation. Our `package.json` has a script set up to automatically compile your `.ts` files every time you change them: `yarn tsc-watch`. This will place the compiled `.js` files in `core-ts/dist`.

Make sure to keep the `yarn tsc-watch` process running, so that anytime you change your `core-ts/src`, it is compiled to `core-ts/dist`.

After you run `yarn tsc-watch` for the first time, you need to `cd` into the new `dist` folder in `core-ts` (same level as `src`). Here, run `yarn link` in your terminal. It's important that you do this in your `dist` folder and not one level up, or else `npm link` will be linking your `.ts` source code instead of the compiled `.js` output.

Now, you need to navigate over to `fractal/website` (or wherever you're linking to). Where we'd usually run `yarn add @fractal/core-ts`, we're going to run `yarn link @fractal/core-ts`. This will create a symlink, so that when `fractal/website` calls `import { ... } from @fractal/core-ts/...`, it will import the latest changes for your local `@fractal/core-ts`.

If that sounded confusing, that's because it is. Here's a recap of the steps:

1. In your local `@fractal/core-ts`, run `yarn tsc-watch`. Leave this process running in an open terminal.
2. Still in `@fractal/core-ts`, `cd dist` to navigate to the new `dist` folder. Run `yarn link`.
3. Navigate to `fractal/website` (or wherever you'd like to import `core-ts`), and run `yarn link @fractal/core-ts`.

If all goes well, `import @fractal/core-ts` will work in `fractal/website` just like any other `npm` module. It will even trigger a hot reload of the `fractal/website` development server every time you change your local `@fractal/core-ts`.

## Development

When developing, you can either develop directly for this library, or develop within another project and extract the relevant, project-independent TypeScript code pieces to be added to this library. All code part of this library should be fully project-independent, otherwise it belongs better within the specific project(s).

Regardless of how you develop for this library, your additions should be properly unit-tested to ensure that they perform as intended.

## Publishing

This project is not published to any **npm** registry, but is rather built and imported directly into other projects (i.e. `/client-applications`) thanks to our monorepo project structure. For an example of how to import `core-ts` into your project, see `/client-applications/package.json`.

## Style

This project follows the [Whist TypeScript Coding Philosophy](https://www.notion.so/whisthq/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289). Any contributions to this project should be in the form of PRs following the usual Whist TypeScript standards.
