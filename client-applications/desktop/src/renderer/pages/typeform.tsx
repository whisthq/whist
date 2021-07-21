import React from "react"
import { ReactTypeformEmbed } from "react-typeform-embed"

const Typeform = (props: { onSubmit: () => void }) => {
    return (
        <div className="w-screen h-screen">
            <ReactTypeformEmbed
                url="https://form.typeform.com/to/Yfs4GkeN"
                onSubmit={props.onSubmit}
            />
        </div>
    )
}

export default Typeform
