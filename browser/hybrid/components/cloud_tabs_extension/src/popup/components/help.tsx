import * as React from "react"

const Help = (props: { onClick: () => void }) => {
  return (
    <button
      onClick={props.onClick}
      type="button"
      className="text-blue p-3 w-full text-center text-sm font-medium"
    >
      Send Feedback
    </button>
  )
}
export default Help
