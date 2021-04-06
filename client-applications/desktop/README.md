# Fractal Desktop Client

This repository contains the code for the Fractal client application, which is the end user's gateway into Fractal's technology. The user downloads and installs the client into `Applications` (MacOS) or `Program Files` (Windows), and launches it anytime they want to open up the Fractal browser.

At a high-level, this repository has two main functions. The first is that it's the home for all the source code related to the client application's GUI and background process. The second is that it's home to all the scripts and configuration involved in "packaging" the application for the user. This "packaging" involves bundling dependencies, notarization/certificates, and moving files to the correct place on the user's OS. Below, we'll discuss each of these functions in more detail.

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

#### The Event Loop

The backbone of our main process is a reactive event loop managed by [RxJS](https://rxjs-dev.firebaseapp.com/guide/overview). It's useful to visualize each cycle of the event loop as a "stream" of data flowing in one direction. It's also useful to think of it as a story with a beginning, a middle, and an end.

"Events" are the beginning of the story. Events create the data that flows through the event loop. Major events include Electron lifecycle events (`appReady`,  `appQuit`, etc.), and events related to user input (button clicks, input submit, etc). As the user interacts with the renderer process, data is sent through IPC back to the main process, where the new data is processed as a "event". Anytime there is a new event with data, the application event loop comes to life and reacts to it.

"Effects" are the end of the story. Effects take incoming data, and perform some sort of side-effect that changes the state of the world. This might include creating GUI windows, sending state over IPC, or quitting the application. Nothing is done with the return value of an Effect function. When it receives new "upstream" data, it shouts a command out into the void and waits for new data.

"Observables" are the middle of the story. Observables represent the chain of computation between Events and Effects, and are the most important part of the main process architecture. Events and Effects have no state, little control flow, and generally don't do very much. This makes them easily testable and highly resuable. Our architecture places all of our key logic and state within observables, which provide a high-level abstraction over the scheduling of asynchronous events.

#### Observables and RxJS

An observable is not much more complicated than a function. If I'm an observable, I might "subscribe" to an Event, so that when the Event is triggered, I run my function with the data created by the event. It's possible that I may have subscribers of my own. When I have my function result, I'll pass it on to my subscribers, who may themselves be observables. Data flows through this "chain" of computation until it gets to an Effect, which uses the data to change something in the environment.

The interaction of observables creates room for rich expression of control flow. Observables can accept "upstream" data with a fine degree of control over timing and parallelism. They can transform, filter, and join other observables to create new data structures. They are a higher-level abstraction of asynchrony than Promises, and allow us to focus on the "rules" that drive the order of computation within our system. 

Observables are a concept of functional reactive programming, and are the main structure introduced by RxJS. Rx can be intimidating. The library has a huge API of utilities for creating and manipulating observables, and there's a natual learning curve that comes with starting to think about time-based streams of data. Once things start to click, the Rx standard library becomes a powerful tool, and it becomes very fast to implement complex behavior that can is otherwise unwiedly to write in an imperative style.

There are some great tutorials for RxJS out there, like [this one](https://www.learnrxjs.io) and [this one](https://gist.github.com/staltz/868e7e9bc2a7b8c1f754). Try starting out by thinking about how a "stream of data across time" is similar to a simple "list of data", and how you might `map`, `filter`, and `reduce` each one.

#### Organization

Main process functions are grouped primarily by level of abstraction, and then sub-grouped based on their functionality. This means that we have several files named, for example, `container.ts`, in locations like `utils/container.ts`, `effects/container.ts`, and `observables/container.ts`. It should feel natural to choose a place to put a new function, or where to look for an existing one.

You can identify a level of abstraction by "what the functions know about". Generally, the `utils` folder is for the lowest level of abstraction. `utils/container.ts` knows about the NodeJS standard library, `core-ts`, and HTTP responses. The functions inside it are tiny, only a few lines each. Files like this tend to be bags of functions that all have a similar return type, like an HTTP response, and provide helpers to extract data or perform validation. For example, `containerInfo` returns an HTTP response, and comes with `containerInfoValid` to check if the response is successful. It also brings along `containerInfoIP` and `containerInfoPorts` to extract the IP address and ports from the response.

This format makes function names verbose, but it allows for higher-levels of abstraction to be expressed much more concisely. `observables/container.ts` now doesn't need to know that it's dealing with an HTTP response. It can be entirely devoted to scheduling `containerInfo` requests and managing relationships with other observables. This becomes especially useful when you realize that `containerInfo`, when triggered by an event, needs to poll the webserver every second until certain other parts of application state receive certain data. This requires complex business logic that's much easier to manage when you're not complecting it with details like HTTP status codes.

Consistency is a key design goal of this project. It should be easy to choose a name for a function or observable when you need to. Names tend to be prefixed by their functionality, like `containerInfo` and `containerAssign`, and are often trailed by some sort of state like `Request`, `Warning`, `Success`, `Failure`, or `Loading`. Try and re-use these key terms as often as possible.

A good indication of well-organized functions is visual consistency. Functions on the same level of abstraction that return similar data will often look similar, especially if they don't do too much and use consistent parameter names. In fact, visual coherence is so important that we often carefully pick names that have the same letter count. For example:

```js
import {
    containerInfoRequest,
    containerInfoSuccess,
    containerInfoFailure,
    containerInfoLoading,
    containerInfoWarning,
    containerInfoProcess,
    containerInfoPolling,
} from "@app/observables/conainer"
```

This stuff matters. It's a design detail, but in a large file it can make code significantly more readable. It also helps with spelling errors, because you immediately notice that the alignment is off. This is one of many dimensions of program legibility, and it's one worth optimizing if you're picking between a few possible names.

#### Debugging
Subscribers to observables can live anywhere in your codebase, which allows for complete decoupling of logging and logic. By "spying" on the emissions of each observable, we can implement sophisticated logging without peppering every function with `log` statements.

You can even set up loggers based on the the behavior of multiple observables to test your expectations about the program. You might subscribe to both `containerInfoRequest` and `containerInfoSuccess`, and fire a `log.warning` if you see two requests before a success. That would be a pretty difficult task with traditional, imperative logging.

A handy file during development is `main/debug.ts`. When the environemnt variable `DEBUG` is `true`, `debug.ts` will print out the value of almost any observable when that observable emits a new value. It has a simple schema to control which observables print and what their ouput looks like. You might find keeping it on all the time because it adds so much visibility into the program.
