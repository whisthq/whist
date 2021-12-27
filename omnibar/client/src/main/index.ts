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
import "@app/main/triggers";
import "@app/main/effects";

import { io } from "socket.io-client";
import { WhistServer } from "@whist/shared";

const socket = io(WhistServer.DEV, {
  reconnectionDelayMax: 10000,
});

socket.emit("Client connected", {
  id: 1234,
  from: "client",
});
