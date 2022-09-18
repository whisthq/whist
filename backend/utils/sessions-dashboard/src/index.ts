import { DeployEnvironment } from "./config"
import { printOutput } from "./output"
import { generateData } from "./generate"
import * as yargs from "yargs"

const title = (extraInfo = "") =>
  `Active sessions for environment "${DeployEnvironment}":${
    extraInfo ? ` (${extraInfo})` : ""
  }`

// (updates every 60 seconds, Ctrl+C to exit)
yargs
  .scriptName("sessions-dashboard")
  .usage("$0 <cmd> [args]")
  .command("show", "Show all active sessions", async (_args) => {
    printOutput(await generateData(), title())
  })
  .command(
    "watch",
    "Watch active sessions",
    {
      interval: {
        alias: "n",
        type: "number",
        description: "Set how often to update the dashboard",
        default: 60,
        required: true,
      },
    },
    async (args) => {
      const minimumInterval = 5
      if (args.interval < minimumInterval) {
        console.log(
          `Interval too low: the minimum interval is ${minimumInterval} seconds`
        )
        process.exit(1)
      }

      // Gracefully handle Ctrl+C
      process.on("SIGINT", () => {
        console.log() // prevent an ugly white square in some terminals
        process.exit(0)
      })

      const tick = async () => {
        printOutput(
          await generateData(),
          title(`updates every ${args.interval} seconds, Ctrl+C to exit`),
          true
        )
      }

      await tick()
      setInterval(tick, args.interval * 1000)
    }
  )
  .demandCommand()
  .strict()
  .wrap((yargs.terminalWidth() * 2) / 3)
  .help("h")
  .help("help").argv
