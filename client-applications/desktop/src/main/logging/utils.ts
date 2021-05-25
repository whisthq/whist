import { truncate, isEmpty, split, includes } from "lodash"
import stringify from "json-stringify-safe"
import chalk from "chalk"

export type LoggingFormat = [string, (val: any) => any]

export type LoggingSchema = {
  [name: string]: {
    [key: string]: LoggingFormat | null
  }
}

const chalkTitle = (s: string) => {
  const parts = split(s, ".")
  const base = chalk.bold
  if (includes(parts, "success")) return base.green(s)
  if (includes(parts, "failure")) return base.red(s)
  return base.yellow(s)
}

const chalkMessage = (s: string) => chalk.cyan(s)

export const logFormat = (title: string, message: string, value: any) => {
  value = truncate(stringify(value, null, 2), {
    length: 1000,
    omission: "...**only printing 1000 characters per log**",
  })

  if (isEmpty(message)) return `${chalkTitle(title)} -- ${value}`
  return `${chalkTitle(title)} -- ${chalkMessage(message)} -- ${value}`
}
