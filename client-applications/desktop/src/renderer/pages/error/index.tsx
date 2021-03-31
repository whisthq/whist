import React from "react"

import { FractalButton, FractalButtonState } from "@app/components/html/button"

const Error = (props: {
    title: string,
    text: string,
    onClick: () => void
}) => {
    /*
        Description:
            Error pop-up

        Arguments:
            title (string): Title of error window
            text (string): Body text of error window
            onClick (() => void): Function to execute when window button is pressed
    */

    return (
        <div className="mt-16 font-body text-center">
            <div className="font-semibold text-2xl">An unexpected error has occured</div>
            <div className="mt-2">The protocol stopped working.</div>
            <FractalButton
                contents="Continue"
                className="mt-8 px-12 py-3"
                state={FractalButtonState.DEFAULT}
                onClick={props.onClick}
            />
        </div>
    )
}

export default Error
