import React, { KeyboardEvent } from "react"
import FractalKey from "shared/types/input"

const { shell } = require("electron")

const loadingMessages = [
    <div>
        Remember to save your files to cloud storage. Your cookies are erased at
        logoff to preserve your privacy.
    </div>,
    <div>
        Fractal never tracks your streaming session and all streamed data is
        fully end-to-end encrypted.
    </div>,
    <div>
        Join our Discord to chat with Fractal users and engineers:
        <div
            onClick={() => shell.openExternal("https://discord.gg/PDNpHjy")}
            /*
             * accessibility enforcement
             * - role="link" tells client machines how to use this div
             * - tabIndex allows link to be selected with tab key
             * - onKeyDown allows link to be opened with enter key
             */
            role="link"
            tabIndex={0}
            onKeyDown={(event: KeyboardEvent) => {
                if (event.key === FractalKey.ENTER) {
                    shell.openExternal("https://discord.gg/PDNpHjy")
                }
            }}
            style={{ cursor: "pointer" }}
        >
            https://discord.gg/PDNpHjy
        </div>
        .
    </div>,
    <div>
        If you make cool artwork on Fractal, tweet us @tryfractal and we’ll
        share it!
    </div>,
    <div>
        You can launch Fractal apps directly by downloading them from the
        Fractal App Store.
    </div>,
    <div>You can zoom in with Ctrl+/Cmd+.</div>,
]

export const generateMessage = (): string => {
    /*
    Description:
        Returns a random loading message (defined above)

    Arguments:
    
    Returns:
        message (string) : Loading message
    */
    return _.shuffle(loadingMessages)[0]
}

// default export until we have multiple exports
export default generateMessage
