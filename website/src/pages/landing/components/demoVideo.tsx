import React, { useState, useContext } from "react"

import ReactPlayer from "react-player"

// we will want to change this
export const DEMO_URL = "https://www.youtube.com/watch?v=PhhC_N6Bm_s"

// must have url= passed in as a url
function DemoVideo(props: any) {
    return <ReactPlayer url={props.url ? props.url : DEMO_URL} controls />
}

export default DemoVideo
