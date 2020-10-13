import React, { useState, useContext } from "react"
import MainContext from "shared/context/mainContext"

import ReactPlayer from "react-player"

// must have url= passed in as a url
function DemoVideo(props: any) {
    const {} = useContext(MainContext)

    return (
        <div>
            <ReactPlayer
                url={props.url}
            />
        </div>
    )
}

export default DemoVideo
