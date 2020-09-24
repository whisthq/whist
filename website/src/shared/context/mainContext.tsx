import React, { useState } from "react"

import useWindowDimensions from "shared/utils/formatting"

interface MainContext {
    width: number
    height: number
    referralCode: string
    setReferralCode: (referralCode: string) => void
    appHighlight: string
    setAppHighlight: (appHighlight: string) => void
}

const MainContext = React.createContext<MainContext>({
    width: 1920,
    height: 1080,
    referralCode: "",
    setReferralCode: () => {},
    appHighlight: "",
    setAppHighlight: () => {},
})

export const MainProvider = ({ children }: any) => {
    const { height, width } = useWindowDimensions()
    const [referralCode, setReferralCode] = useState("")
    const [appHighlight, setAppHighlight] = useState("")

    // const [width, setWidth] = useState(width)
    // const [height, setHeight] = useState(height)

    // const setWidth = (width) => {
    //     setWidth(width)
    // }

    // const setHeight = (height) => {
    //     setHeight(height)
    // }

    return (
        <MainContext.Provider
            value={{
                width: width,
                height: height,
                referralCode: referralCode,
                setReferralCode: setReferralCode,
                appHighlight: appHighlight,
                setAppHighlight: setAppHighlight,
            }}
        >
            {children}
        </MainContext.Provider>
    )
}

export const MainConsumer = MainContext.Consumer

export default MainContext
