import applescript from "applescript"

import { getAllBrowserWindows } from "@app/main/utils/applescript"

getAllBrowserWindows("Safari").then((out) => console.log("applemyapple", out))
