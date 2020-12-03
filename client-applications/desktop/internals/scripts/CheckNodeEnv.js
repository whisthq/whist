import chalk from 'chalk'
import { debugLog } from '../../utils/logging.js'

export default function CheckNodeEnv(expectedEnv) {
    if (!expectedEnv) {
        throw new Error('"expectedEnv" not set')
    }

    if (process.env.NODE_ENV !== expectedEnv) {
        debugLog(
            chalk.whiteBright.bgRed.bold(
                `"process.env.NODE_ENV" must be "${expectedEnv}" to use this webpack config`
            )
        )
        process.exit(2)
    }
}
