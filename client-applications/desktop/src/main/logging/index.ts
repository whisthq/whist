import host from "./host"

type LoggingMap = Record<string, Record<string, any>>

const loggingMap: LoggingMap = {
  ...host
}

export default loggingMap
