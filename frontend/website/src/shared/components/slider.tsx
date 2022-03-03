import React, { useState } from "react"

import "./styles.css"

const Slider = (props: {
  min: number
  max: number
  step: number
  start: number
  onChange: (x: number) => void
}) => {
  const [x, setX] = useState(props.start)

  const onChange = (evt: any) => {
    setX(evt.target.value)
    props.onChange(evt.target.value)
  }

  return (
    <>
      <input
        type="range"
        min={props.min}
        max={props.max}
        step={props.step}
        value={x}
        onChange={onChange}
        className="w-full bg-gray-700 rounded-full h-2"
        style={{
          background: `linear-gradient(to right, #0092FF, #71D9FF)`,
          backgroundSize: `${x}% 100%`,
          backgroundRepeat: "no-repeat",
        }}
      />
    </>
  )
}

export default Slider
