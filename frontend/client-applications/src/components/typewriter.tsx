import React, { useEffect, useState } from "react"
import classNames from "classnames"
import { last, concat, tail, zip, fill, times } from "lodash"
import NoWorkResult from "postcss/lib/no-work-result"

export const cycle = function* (array: any[]) {
  while (true) for (const i of array) yield i
}

export const orderedCombinations = (str: string) =>
  tail(
    str
      .split("")
      .reduce((acc, s) => concat(acc, `${last(acc) as string}${s}`), [""])
  )

const charStreams = (
  words: string[],
  startTick: number,
  endTick: number,
  forwardTick: number,
  backwardTick: number
) =>
  zip<string | number | boolean>(
    words.flatMap(
      (
        w // Letter stream
      ) => concat(["", ...orderedCombinations(w)])
    ),
    words.flatMap(
      (
        w // Timeout stream
      ) => concat([startTick, ...fill(w.split(""), forwardTick)])
    ),
    words.flatMap(
      (
        w // Blink stream
      ) => concat([...fill(tail(w), false), true, true])
    )
  )

export const TypeWriter = (props: {
  words: string[]
  classNameCursor?: string
  classNameText?: string
  hideCursor?: boolean
  startDelay?: number
  startTick?: number
  endTick?: number
  forwardTick?: number
  backwardTick?: number
  startAt?: number
  repeat?: boolean
}) => {
  const {
    words,
    startTick: start = 500,
    endTick: end = 1500,
    forwardTick: forw = 40,
    backwardTick: back = 50,
    startDelay = 100,
    startAt = 0,
    repeat = false,
  } = props
  const [display, setDisplay] = useState("")
  const [blink, setBlink] = useState(false)

  useEffect(() => {
    const iterate = cycle(charStreams(words, start, end, forw, back))
    times(startAt, () => iterate.next())
    const timeouts: Array<ReturnType<typeof setTimeout>> = []
    const counter = 0
    setTimeout(function run() {
      const [char, ms, blinking] = iterate.next().value
      setDisplay(char)
      setBlink(blinking)
      if (!repeat && char !== words[0]) timeouts.push(setTimeout(run, ms))
    }, startDelay)
    return () => {
      timeouts.map(clearTimeout)
    }
    // eslint-disable-next-line
  }, [])

  return (
    <div className={classNames(props.classNameText)}>
      {display}
      <div
        className={classNames(
          "pl-px inline",
          blink && "animate-blink",
          props.hideCursor !== undefined && "px-0",
          props.classNameCursor
        )}
      ></div>
    </div>
  )
}
