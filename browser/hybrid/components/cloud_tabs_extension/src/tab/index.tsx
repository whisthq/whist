import * as React from "react"
import * as ReactDOM from "react-dom"

import WithInterCom from "./help"

const App = () => {
  return (
    <div className="dark">
      <WithInterCom onClose={() => {}} userEmail={undefined} />
    </div>
  )
}

ReactDOM.render(<App />, document.getElementById("root"))
