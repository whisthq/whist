import React from "react"

import useWindowDimensions from "shared/utils/formatting"

const ScreenContext = React.createContext({ width: 1920, height: 1080 })

export const ScreenProvider = ({ children }: any) => {
    const { height, width } = useWindowDimensions()

    // const [width, setWidth] = useState(width)
    // const [height, setHeight] = useState(height)

    // const setWidth = (width) => {
    //     setWidth(width)
    // }

    // const setHeight = (height) => {
    //     setHeight(height)
    // }

    return (
        <ScreenContext.Provider value={{ width: width, height: height }}>
            {children}
        </ScreenContext.Provider>
    )
}

export const ScreenConsumer = ScreenContext.Consumer

export default ScreenContext
