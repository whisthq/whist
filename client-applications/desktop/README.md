# Fractal Desktop Client

This repository contains the code for the Fractal client application, which is the end user's gateway into Fractal's technology. The user downloads and installs the client into `Applications` (MacOS) or `Program Files` (Windows), and launches it anytime they want to open up the Fractal browser.

At a high-level, this repository has two main functions. The first is that it's the home for all the source code related to the client application's GUI and background process. The second is that it's home to all the scripts and configuration involved in "packaging" the application for the user. This "packaging" involves bundling dependencies, notarization/certificates, and moving files to the correct place on the user's OS. Below, we'll discuss each of these functions in more detail.

## Setting Up for Development

We use `yarn` as the package manager for this project. All of the commands required for development are aliased in `package.json`, and can be called with `yarn`. For example, `yarn start` will boot up the development environment. We don't write commands out directly in the `scripts` section of `package.json`. Instead, each command has a corresponding file in the `desktop/scripts` folder. This allows us to more carefully comment our `yarn` commands, and it makes diffs more visible in PRs. You shouldn't `cd scripts` to run anything in the scripts folder. They should all be run with `desktop` as the working directory, and you should only need to interact with them using `yarn`.

1. Go to the `/protocol/` README to install all the necessary prerequisites.

2. Make sure you have `yarn` installed on your computer. You can install it [here](https://classic.yarnpkg.com/en/docs/install/#mac-stable).

3. `cd` into the `desktop` folder.

4. Run `yarn` to install all dependencies on your local machine. If dependency issues occur, try running `rm -rf node_modules/` and then `yarn` again.

5. To start the application in the `dev` environment, run `yarn start`. `yarn start` will install
   the protocol for you if it's not already installed.

## How To Contribute

Before making a pull request, ensure that the following steps are taken:

1. Make sure the application is fully working both in `dev` (via `yarn start`) and packaged (see **Packaging for Production**). This is important because the Electron app can behave differently when packaged. For example, file paths will change.

2. Make sure that your code follows the guidelines outlined in our [React coding philsophy](https://www.notion.so/tryfractal/Typescript-Coding-Philosophy-984288f157fa47f7894c886c6a95e289).

3. Lint your code by running `yarn lint:fix`. If this does not pass, your code will fail Github CI.

4. Rebase against `dev` by pulling `dev` and running `git rebase dev`.

Finally, you can open PR to `dev`.

### App State and Logging

Application data are written and stored in the following locations:

- On macOS, look in `~/Library/Application\ Support/{Electron,fractal}/{dev,staging,prod}`
- On Linux, look in `~/.config/{Electron,fractal}/{dev,staging,prod}`
- On Windows, look in `%APPDATA%\{Electron,fractal}\{dev,staging,prod}`. Note that `%APPDATA%` usually corresponds to `C:\Users\<user>\AppData\Roaming`.
  The unpackaged app will have `Electron` in the path while the packaged app will have `fractal`. You can clear these files to re-trigger behavior, such as a re-running the login flow, by deleting them via `yarn cache:clear`.

Some pieces of state, such as the user's authentication token, are stored in the file system and persist between app launches. This state is persisted in the file `config.json`.

Similarly, logs from the client app will be stored in `logs/client.log` and logs from the protocol will be stored in `logs/protocol.log` within the application data folder.

## Client Application Source Code

Our client application is built on [Electron](https://www.electronjs.org), which is a cross-platform GUI framework. It provides a consistent interface for interacting with different operating system functions (like window management), as well as a GUI rendering engine based on Chromium. This means that user interface work with Electron is very similar to modern web design. Electron windows simply render HTML pages, which are able to link to external CSS, images, and JavaScript.

Electron applications are divided into two major parts, the "main" and the "renderer" processes. While the two processes are both JavaScript environments, they have completely different capabilities and responsibilities. Even though they're both JS-based, they are different CPU processes and can only communicate via serializable data. Essentially, the main/renderer communication is limited to anything that is serializable into JSON.

We use TypeScript for both processes, which is bundled and compiled to JavaScript with [Snowpack](https://www.snowpack.dev).

### Client Renderer Process

The Electron "renderer" process runs in a web browser environment. You'll feel right at home if you have web dev experience, as the renderer process has an identical API to Google Chrome. The renderer process is fully capable of performing HTTP requests and bundling NodeJS dependencies, and many applications rely on it for much of their functionality.

However, the Fractal client application uses the renderer very minimally. As an architectural decision, we've decided to keep the renderer process as "dumb" as possible. It holds very little logic or state, and is essentially a "view" layer for the main process.

Applications can become tricky to manage when there's two "brains". It becomes unclear what computation should happen where, and keeping state in sync becomes a major challenge. Because the renderer process has no ability to launch child process, manage windows, or work with the filesystem, we decided to make the main process the "brain" of the app.

We use a single Electron IPC channel to communicate between the main process and the renderer windows. The main process always "broadcasts" an object to all windows across this IPC channel. This object represents the "state" of the application, which the renderers will use to decide what to display.

To simplify interaction with this IPC channel, we've created a React hook called `useMainState`, which listens for new state and re-renders the window. `useMainState` also provides a way to send a state update back to the main process, which then "round-trips" and once again triggers a re-render. The API is nearly identical to React's built-in `useState` to make it feel familiar to use.

We've designed the renderer process to be easy to reason about. There's little that can go wrong, because it doesn't do very much. Since IPC communication is a little slow, the renderer still uses some local state (like for storing keystrokes), but if we could, we'd make it completely stateless. Its purpose is to accept user input and render the main process state into JSX/HTML, nothing more.

Files related to the renderer process mostly live in `src/renderer`, with some shared React components located in `src/components`, and a few helper functions in `src/utils`.

### Client Main Process

The Electron "main" process runs in a NodeJS environment, and has full access to the NodeJS standard library and ecosystem. HTTP requests, process management, and file system interaction are handled with standard NodeJS APIs, while GUI windows, communication with the renderer process, and application lifecycle are managed with Electron's API.

Work begins in the main process when events are triggered. Events might be built in Electron EventEmitters like `appReady`, or they might be our custom `triggers`, which emit based on the success or failure of API calls, etc.

Our main process "reacts" to events using RXJS observable subcriptions. We organize these observables into "flows", which are the main unit of work in the main process. Flows are just a function with the following signature:

```js
const FlowA = (t: Obervable<T>) => { success: Observable<F>, failure: Observable<F>, warning: Observable<F> }
```

The function above accepts of Observable and returns an JS object of observables. The keys you see above are `success`, `failure`, `warning`, but you can choose any number of keys with any name to emit from a flow. As a convention, try and use names you see elsewhere in the app, like `success` and `failure`.

When you call `FlowA(ObservableA)`, the return value will be an object with observable values. However, work inside `FlowA` will not happen right away. Calling `FlowA(ObservableA)` just "wires up" the flow of data, which causes observables and flows within `FlowA` to start subscribing to `ObservableA`. The actual work will be started when `ObservableA` _actually emits some data_.

What causes `ObservableA` to emit? `ObservableA` itself is subscribed to some observable "upstream", maybe `ObservableB`. It might also be subscribe to a NodeJS EventEmitter, but from the perspective of `FlowA` it doesn't matter. When `ObservableA` emits, `FlowA` does some work, and the results of the work are emitted through one of the `success`, `failure`, `warning` observables returned by `FlowA`.

This is a functional reactive programming model. We're not change of the timing of the work. We're basically saying "when we have new data from x, start doing y" across the entire application. This frees us from managing asynchronous vs. synchronous functions. All work triggers other work "when the work is done".

Here's a more concrete example using the `mandelboxCreate` flow in the client app:

```js
// A "flow" is created with the "flow" function, which accepts
// a string name as its first argument and the actual definition as the
// second argument. This allows us to wrap the defintion with other context,
// such as information for logging.
const mandelboxFlow  = flow(
  "mandelboxFlow", // flow name usually matches the variable
  (
    trigger: Observable<{ // flows take a single argument, which is always
      accessToken: string // We need to "unwrap" this observable inside
      configToken: string // of the flow to make use of its arguments.
      region?: AWSRegion
    }>
  ) => {
    // Rxjs has its own library of operators to use on observables, such
    // "map", which is like Array.map() in that it applies a function to
    // each element in the sequence.
    const create = mandelboxCreateFlow(
      trigger.pipe(map((t) => pick(t, ["accessToken", "region"])))
    ) // create will have a value of:
      // {
      //   success: Observable<ServerResponse>
      //   failure: Observable<ServerResponse>
      // }
      //
      // If the response was successful, the "success" observable will emit.
      // If it's not successful, only the "failure" observable will emit.
      // Both observables should not emit for the same input.

    // As with mandelboxCreateFlow above, here we are  initializing another
    // flow (child flow) using the result of another flow.
    const polling = mandelboxPollingFlow(
      zip(create.success, trigger).pipe( // rxjs "zip" allows us to make an
        map(([c, t]) => ({               // observavble containing the contents
          ...pick(c, ["mandelboxID"]),   // of multiple source observables.
          ...pick(t, ["accessToken"]),   // The result observable will only
        }))                              // emit when all source observables
      )                                  // have new emissions.
    )

    const host = hostServiceFlow(
      zip([trigger, create.success]).pipe(
        map(([t, _c]) => pick(t, ["accessToken", "configToken"]))
      ) // We sometimes do these "map" calls when we're passing inputs
    )   // to flows when we need to "shuffle around" arguments to fit
        // a flow's function signature.

    return {
      success: fromSignal(polling.success, host.success),
      failure: merge(create.failure, polling.failure, host.failure),
    } // just like "create" above, the output of the mandelboxFlow will be
  }   // an object of { success, failure }, with values that will emit a result
)     // once all the processes we've "wired up" in the flow complete.

```

#### Observables and RxJS

An observable is not much more complicated than a function. If I'm an observable, I might "subscribe" to an Event, so that when the Event is triggered, I run my function with the data created by the event. It's possible that I may have subscribers of my own. When I have my function result, I'll pass it on to my subscribers, who may themselves be observables.

The interaction of observables creates room for rich expression of control flow. Observables can accept "upstream" data with a fine degree of control over timing and parallelism. They can transform, filter, and join other observables to create new data structures. They are a higher-level abstraction of asynchrony than Promises, and allow us to focus on the "rules" that drive the order of computation within our system.

Observables are a concept of functional reactive programming, and are the main structure introduced by RxJS. Rx can be intimidating. The library has a huge API of utilities for creating and manipulating observables, and there's a natual learning curve that comes with starting to think about time-based streams of data. Once things start to click, the Rx standard library becomes a powerful tool, and it becomes very fast to implement complex behavior that can is otherwise unwiedly to write in an imperative style.

There are some great tutorials for RxJS out there, like [this one](https://www.learnrxjs.io) and [this one](https://gist.github.com/staltz/868e7e9bc2a7b8c1f754). Try starting out by thinking about how a "stream of data across time" is similar to a simple "list of data", and how you might `map`, `filter`, and `reduce` each one.

#### Config

All config variables are stored in various files in the `config` folder in the application root. The configuration files are used in both the application itself and our build scripts.

- `build.js` contains variables used in the build process
- `environment.js` exports environment-specific variables used while the application is running
- `paths.js` exports relevant OS-dependent paths

In the electron app itself, these variables are re-exported by the files in `src/config`.

## Packaging

### MacOS Notarizing

Before you can package the MacOS application it needs to be notarized. The application will get notarized as part of the regular build script. This means that it needs to be uploaded to Apple's servers and scanned for viruses and malware.

Notarizing is done in Github CI. In the event you want to notarize locally:

1. Download the Fractal Apple Developer Certificate, which is `fractal-apple-codesigning-certificate.p12` in the `fractal-dev-secrets` bucket. The password is `Fractalcomputers!`.

2. Make sure you have the latest version of Xcode and have opened it at least once. We recommend downloading Xcode from the App Store, and ensuring that you have the MacOSX SDK at `/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk`.

3. Run `xcode-select --install`.

4. Run `yarn package:notarize`.

### Publishing New Versions

Fractal runs two update channels, `production` and `testing`. The `dev` branch should be published automatically to `testing`, while `production` should match `prod`. The build script has a special `noupdates` channel which should be used for any builds that aren't on one of these branches.

Any CI generated builds are also stored in GitHub Releases which can be manually downloaded and used.

### Common Auto-Update Errors

#### Unable to find `altool`

First try to run `xcode-select --reset` in your Terminal. This will reset `xcode-select` to use the default CLI tools path. If this doesn't work, uninstall Xcode altogether, and install the FULL version from the Mac App Store.

#### 403 error access denied

This is probably not an S3 bucket policy issue; rather, you're probably trying to read a resource that doesn't exist. Right click the packaged app and select `Show Package Contents`. Then open `Contents` > `Resources` > `app-update.yml`. Ensure that the bucket and channel are correct. The dev channel is `dev-rc`, staging is `staging-rc`, prod is `latest`.

#### Zip file not found

When you packaged the app, or when the app currently in S3 was published, the ZIP file was likely not included. Check `electron-builder.config.js` to see if "zip" is one of the targets.

## Continous Integration

This repository has basic continuous integration through GitHub Actions. For every PR to `dev`, `staging`, or `prod`, GitHub Actions will attempt to build the bundled application on Windows-64bit, macOS-64bit, and Linux-64bit. These will be uploaded to their respective s3 buckets: `s3://fractal-chromium-{windows,macos,ubuntu}-{dev,staging,prod}`. Each s3 bucket functions as a release channel and only stores the latest version. A YAML-formatted metadata file is present detailing the version and other info. See [electron-builder's publish documentation](https://www.electron.build/configuration/publish) for more info.

Changes in the `protocol/` subrepo will also trigger the client-apps to be rebuilt.

## Traps!

Abstractions leak. Bugs hide. Things don't work the way they seem. This is programming, and sometimes we have to get our hands dirty with implementation details that we shouldn't have to know about. Here's some for this project.

### Always use take(1)/takeLast(1) instead of first()/last()

These are pairs of observable operators that seem like they should do the same thing. Often they do, when observables are emitting consistently. The subtle difference is that `first()` will error after a certain amount of time if its upstream observable never emits. `take(1)` will not error and simply be silent. Similarly, `last()` will error, and `takeLast(1)` will not.

This is important to know, because you won't see the `EmptyError` caused by `first` and `last` until runtime. When you do, it will appear as a cryptic, hard to read Electron pop-up. This is a difficult error, so the takeaway is that you should never use `first()` or `last()`. Always use `take(1)` and `takeLast(1)`.

[More info here](https://swalsh.org/blog/rxjs-first-vs-take1).
