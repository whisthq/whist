import React, { createContext, useContext, useState } from "react"

const Context = createContext<any>({
  search: "",
  setSearch: () => {},
})

const Provider = (props: { children: React.ReactNode }) => {
  const [search, setSearch] = useState("")

  return (
    <Context.Provider value={{ search, setSearch }}>
      {props.children}
    </Context.Provider>
  )
}

const withContext = () => {
  const context = useContext(Context)
  if (context === undefined) {
    throw new Error("Omnibar Context must be used within Provider")
  }
  return context
}

export { Provider, withContext }
