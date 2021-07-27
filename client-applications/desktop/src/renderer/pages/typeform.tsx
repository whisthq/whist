import React from "react"
import { Widget } from "@typeform/embed-react"

const Typeform = (props: { onSubmit: () => void }) => {
    return (
        <Widget
            id="Yfs4GkeN"
            onSubmit={props.onSubmit}
            className="w-screen h-screen"
        />
    )
}

export default Typeform
