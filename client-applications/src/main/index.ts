// This is the application's main entry point. No code should live here, this
// file's only purpose is to "initialize" other files in the application by
// importing them.
//
// This file should import every other file in the top-level of the "main
// folder. While many of those files import each other and would run anyways,
// we import everything here first for the sake of being explicit.
//
// If you've created a new file in "main/" and you're not seeing it run, you
// probably haven't imported it here.

// Import the /config folder first to make sure that CONFIG is set in
// process.env, which is required by core-ts. Once all values are moved into
// monorepo config, this will no longer be necessary.

import "@app/config/environment"

// import "@app/main/triggers"
// import "@app/main/flows"
// import "@app/main/effects"

import ndt7 from "@m-lab/ndt7"

ndt7
  .test(
    {
      userAcceptedDataPolicy: true,
    },
    {
      serverChosen: function (server: any) {
        console.log("Testing to:", {
          machine: server.machine,
          locations: server.location,
        })
      },
      downloadComplete: function (data: any) {
        console.log("download", data)
      },
      uploadComplete: function (data: any) {
        console.log("upload", data)
      },
      error: function (err: any) {
        console.log("Error while running the test:", err.message)
      },
    }
  )
  .then(() => {
    console.log("DONE")
  })
