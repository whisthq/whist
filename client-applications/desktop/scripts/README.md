# Client Application Scripts

This folder contains some general purpose scripts for use in CI and local development. Most scripts are related to building the application or starting up a local development environment.

A goal of this folder is to try and keep scripts "composable" to help eliminate duplicate code. Often, you may want to add a script that is just a variation of another script with different arguments or environment variables. To accomplish this without repeating the entire script, all scripts should have a format like the following:

```javascript
// This example demonstrates how the "start" function, for building a local
// development environment, might be built to be extensible.
// A format like below would allow another script, such as testManual.js, to
// require("../start"), and then call start({ MY_VAR: "my_value"}) to "inject"
// extra environment variables into the start script.

const helpers = require("./build-package-helpers");

// The "main" function for the script should have the signature below.
// The first parameter should be "env", an object of environment variables.
// The remaining parameters can be any positional arguments. Often, these
// will be arguments passed through the command line.
//
// The function below doesn't use any positional arguments, so we prefix
// "args"  with an underscore so the linter doesn't complain.
// It's worth declaring the parameter even if we're not using it so that
// it's clear to a reader that we're adhering to our consistent script syntax.
export default function start(env, ..._args) {
  helpers.buildAndCopyProtocol();
  helpers.buildTailwind();
  // Notice here that we "destructure" the env object before adding the
  // script-specific environment variable VERSION. This way, the environment
  // variables received as the first argument will get merged.
  // If the env object is not passed or is undefined, then no extra
  // environment variables will get merged in.
  helpers.snowpackDev({
    ...env,
    VERSION: helpers.getCurrentClientAppVersion(),
  });
}

// This is a common node directive that runs only if this is the "main" script.
// This module will be the "main" script if it is run directly as a command.
// If we call node start.js from the commmand line, then the block below will
// run our start() function (with no arguments, as no extras are needed).
//
// If we simply import or require this file, the block below will not run,
// allowing us to control how start() is called.
//
// This block is also a good place to do any "argv" command line parsing. This
// helps keep initialization separate from the logic inside the "start" function.
if (require.main === module) {
  start();
}
```

If a script in this folder doesn't adhere to this format, then a comment block at the top of the script should explain why. This commonly happens because a script doesn't get called by "us" directly, but instead is passed directly to a process like snowpack or electron-builder. As we don't control how these functions are called, we can't control their signature.
