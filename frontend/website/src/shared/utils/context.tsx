import React, { useState, useContext } from "react"

import useWindowDimensions from "@app/shared/utils/formatting"

interface MainContextInterface {
  width: number // current browser window width
  height: number // current browser window height
  dark: boolean // dark mode true/false
  setDark: (dark: boolean) => void // toggle dark mode
}

const MainContext = React.createContext<MainContextInterface>({
  width: 1920,
  height: 1080,
  dark: true,
  setDark: () => {},
})

export const MainProvider = ({ children }: any) => {
  const { height, width } = useWindowDimensions()
  const [dark, setDark] = useState(true)

  return (
    <MainContext.Provider
      value={{
        width: width,
        height: height,
        dark: dark,
        setDark: setDark,
      }}
    >
      {children}
    </MainContext.Provider>
  )
}

export const withContext = () => {
  const context = useContext(MainContext)
  if (context === undefined) throw new Error("No React context found")
  return context
}
