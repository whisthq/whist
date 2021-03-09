import { createLogger, format, transports, config } from "winston"
import { FractalNodeEnvironment } from "shared/types/config"
import { config as FractalConfig } from "shared/constants/config"
import Logzio from "winston-logzio"

const { combine, timestamp, json } = format

/* eslint-disable no-console */
export const debugLog = <T>(callback: T) => {
    /*
    Description:
        console.log's only in development
    */
    console.log(process.env.NODE_ENV)
    if (process.env.NODE_ENV === FractalNodeEnvironment.DEVELOPMENT) {
        console.log(callback)
    }
}

export class FractalLogger {
    maxFileSize = 10000000

    token = FractalConfig.keys.LOGZ_API_KEY

    /*
        Description:
            Uses the winston logging module to log to console, local .log file, and DataDog

        Usage:
            const logger = new FractalLogger()
            logger.logInfo("Example info log")
            logger.logError("Example error log")

        Methods:
            logInfo(logs: string, userID: string)
                Info-level log. In both dev and prod, will log to console and a .log file. In prod,
                will also log to DataDog.
            logError(logs: string, userID: string)
                Error-level log. In both dev and prod, will log to console and a .log file. In prod,
                will also log to DataDog.
    */

    private createLogger = () => {
        // Where to send the logs
        const developmentTransport = [
            new transports.Console(),
            new transports.File({ filename: "combined.log" }),
        ]

        const productionTransport = [new transports.Console()]
        const transport =
            process.env.NODE_ENV === FractalNodeEnvironment.PRODUCTION
                ? productionTransport
                : developmentTransport

        const logger = createLogger({
            levels: config.syslog.levels,
            format: combine(
                timestamp({
                    format: "YYYY-MM-DD HH:mm:ss",
                }),
                json()
            ),
            transports: transport,
            exceptionHandlers: transport,
        })

        if (process.env.NODE_ENV === FractalNodeEnvironment.PRODUCTION) {
            logger.add(
                new Logzio({
                    token: this.token as string,
                })
            )
        }

        return logger
    }

    private formatuserID = (userID: string) => {
        if (!userID) {
            userID = "No user ID"
        }

        return userID
    }

    logInfo = (logs: string, userID = "", callback?: () => void) => {
        const logger = this.createLogger()
        logger.info(`${this.formatuserID(userID)} | ${logs}`)
        logger.end()
        if (callback) {
            logger.on("finish", () => {
                callback()
            })
        }
    }

    logError = (logs: string, userID = "", callback?: () => void) => {
        const logger = this.createLogger()
        logger.error(`${this.formatuserID(userID)} | ${logs}`)
        logger.end()
        if (callback) {
            logger.on("finish", () => {
                callback()
            })
        }
    }
}
