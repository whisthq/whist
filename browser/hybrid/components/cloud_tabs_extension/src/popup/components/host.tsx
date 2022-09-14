import * as React from "react"

const Host = (props: { host: string }) => (
  <div className="text-center py-2">
    <div className="text-gray-900 dark:text-gray-100 font-semibold">
      {props.host}
    </div>
  </div>
)

export default Host
