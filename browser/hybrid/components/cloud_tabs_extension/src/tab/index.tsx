import * as React from "react"
import * as ReactDOM from "react-dom"

import WithInterCom from "./help"

const App = () => {
  return (
    <>
      <WithInterCom onClose={() => {}} userEmail={undefined} />
    </>
  )
}

ReactDOM.render(<App />, document.getElementById("root"))
