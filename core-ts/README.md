# Fractal Core Typescript Library

This project is an internal Fractal library for shared React components, Typescript functions, CSS modules, etc., which are used in various other projects within Fractal. If you write TypeScript code that you believe can be reused across various projects, then you should add it to this internal library and use the library within your project, for maximum code reusability.

It's important to note that development in this repository works a little differently than the rest of the Fractal organization. In `core-ts`, we try and follow standard practices for `npm` package development. This means that we do not have `dev` and `staging` branches, and PRs are merged directly into `prod`. This means that every push will trigger a new `npm` package release with an updated version number.

If you have `core-ts` as a dependency in another `fractal` repository, you should "pin" your dependency version. This means your `package.json` dependencies should contain `@fractal/core-ts: 1.0.1`, not `@fractal/core-ts: ^1.0.1`. Install a specific version with `npm install @fractal/core-ts@1.0.1`, after following the authentication instructions below.

## Installation

##### 1. Authenticate your GitHub account through `npm`

To authenticate with GitHub packages, you need a [GitHub personal access token](https://docs.github.com/en/github/authenticating-to-github/creating-a-personal-access-token). These are meant to be generated per use, so you likely need to make a new one in your GitHub Profile > Settings > Developer Settings. Keep the page open after you generate it, as you won't be able to see the token again when you leave.

You can authenticate your user account in your terminal with the `npm login` command in the example below. You'll be prompted one at a time for your credentials.

    # Paste this command into your terminal.
    npm login --scope=@fractal --registry=https://npm.pkg.github.com

    > Username: USERNAME (github username)
    > Password: TOKEN    (personal access token)
    > Email:    EMAIL    (@fractal.co email)

It's also possible to create a per-user `~.npmrc` file that contains your token. However, to avoid storing authentication tokens in plain text, use the method above to enter your credentials. More about authenticating with GitHub packages at [this link](https://docs.github.com/en/packages/guides/configuring-npm-for-use-with-github-packages#authenticating-to-github-packages).

##### 2. Add the Fractal GitHub registry to your project's `.npmrc`

If you don't already have a `.npmrc` in the root of your project, create one. You only need to add this line:

    @fractal:registry=https://npm.pkg.github.com

This adds a new npm "registry", which is a place that `npm` will check for packages during `npm install`. Make sure you commit this `.npmrc` to version control. This will make `@fractal/*` packages available for installation. If you already have an `.npmrc` in your project, you might need a slightly different syntax to add more than one registry. See [these docs](https://docs.github.com/en/packages/guides/configuring-npm-for-use-with-github-packages#publishing-multiple-packages-to-the-same-repository) for info.

Now you're ready to install `@fractal` npm packages! They work exactly the same as any other `npm` package. Remember to install a specific version number. To install `fractal/core-ts` (replacing the version number with latest):

    npm install @fractal/core-ts@1.0.1  --save-exact

    or

    yarn add @fractal/core-ts@1.0.1 --exact

## Install a branch for development

A slightly different process is required to install a specific branch of `core-ts`. Trying to install `@fractal/core-ts` with the `@` prefix will always try and pull the latest "released" package. This should be done when you want to use the stable release of the library. If you need an unreleased feature or are working on `core-ts`, you may want to install from a specific branch or commit. You need to do this using npm's git protocol.

The easiest way is just to `npm install fractal/core-ts`, without the `@` prefix. The `@` symbol signifies to `npm` that you're installing a "package", which is pre-built and published in an npm registry. Without `@`, `npm` recognizes the command as `npm install github-username/repo-name`, and you can install from a specfic branch or commit with the `#`separator. This means the full command to install a branch of `core-ts` is:

```bash
npm install fractal/core-ts#branch-or-commit
```

Sometimes, this can lead to installation difficulties that you may not be used to. When installing with the git protocol, `npm` needs to do some bundling of the source files that is usually done for you when installing a released package, and you may run into some errors if your system isn't set up for this. A known problem (and solution) involving `xcode` installation on MacOS is discussed in [this blog post](https://medium.com/@mrjohnkilonzi/how-to-resolve-no-xcode-or-clt-version-detected-d0cf2b10a750) and [this documentation](https://github.com/nodejs/node-gyp/blob/master/macOS_Catalina.md) from NodeJS.

## Development Workflow

One of the goals of `core-ts` is to extract out common code in the Fractal website and Fractal client application. Those code bases are run in a browser environment and setup with Hot Module Reloading, making it easy to save a file and see how your changes work.

Because `core-ts` is decoupled from Electron or Create React App, we need a manual setup so that we can interact with code as we're developing. Here's a few ways may want to work, depending on the job you're doing.

##### 1. Run Jest in watch mode

If you're just making small changes to some functions and want to make sure they pass the test suite, run `npm run test-watch`. This will start `jest --watch`, which runs the tests every time your code changes. It's a little nicer than manually re-triggering them.

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

`npm run dev` in your terminal will start up `nodemon` and `ts-node`, and you'll see new console output every time you reload.

##### 3. Link your local core-ts repos to other projects

The best kind of testing is using `core-ts` in working projects. If you're working on `core-ts/http`, you might want to import your functions into `fractal/website` so you can make sure they work in a real environment. You don't want to have to commit and publish for every change, however, so we use `npm link`. This command will create symlinks in `fractal/website`, pointing the corresponding `import` statements to your local `core-ts` repo.

There's a few things that have to happen to make this work. First of all, you can't `link` your source TypeScript files. You have to link the JavaScript output from TypeScript compilation. Our `package.json` has a script set up to automatically compile your `.ts` files every time you change them: `npm run tsc-watch`. This will place the compiled `.js` files in `core-ts/dist`.

Make sure to keep the `npm run tsc-watch` process running, so that anytime you change your `core-ts/src`, it is compiled to `core-ts/dist`.

After you run `npm run tsc-watch` for the first time, you need to `cd` into the new `dist` folder in `core-ts` (same level as `src`). Here, run `npm link` in your terminal. It's important that you do this in your `dist` folder and not one level up, or else `npm link` will be linking your `.ts` source code instead of the compiled `.js` output.

Now, you need to navigate over to `fractal/website` (or wherever you're linking to). Where we'd usually run `npm install @fractal/core-ts`, we're going to run `npm link @fractal/core-ts`. This will create a symlink, so that when `fractal/website` calls `import { ... } from @fractal/core-ts/...`, it will import the latest changes for your local `@fractal/core-ts`.

If that sounded confusing, that's because it is. Here's a recap of the steps:

1. In your local `@fractal/core-ts`, run `npm run tsc-watch`. Leave this process running in an open terminal.
2. Still in `@fractal/core-ts`, `cd dist` to navigate to the new `dist` folder. Run `npm link`.
3. Navigate to `fractal/website` (or wherever you'd like to import `core-ts`), and run `npm link @fractal/core-ts`.

If all goes well, `import @fractal/core-ts` will work in `fractal/website` just like any other `npm` module. It will even trigger a hot reload of the `fractal/website` development server every time you change your local `@fractal/core-ts`.

## Development

When developing, you can either develop directly for this library, or develop within another project and extract the relevant, project-independent TypeScript code pieces to be added to this library. All code part of this library should be fully project-independent, otherwise it belongs better within the specific project(s).

Regardless of how you develop for this library, your additions should be properly unit-tested to ensure that they perform as intended.

## Publishing

This project is not published to any **npm** registry, but is rather built and imported directly into other projects (i.e. `/client-applications/desktop`) thanks to our monorepo project structure. For an example of how to import `core-ts` into your project, see `/client-applications/desktop/package.json`.

## Style

This project follows the [Fractal TypeScript Coding Philosophy](https://www.notion.so/tryfractal/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289). Any contributions to this project should be in the form of PRs following the usual Fractal TypeScript standards.
