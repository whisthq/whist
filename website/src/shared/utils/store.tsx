import React, {createContext, useReducer, useContext} from "react"
import { deep_copy } from "shared/utils/reducerHelpers"

const reducer = (
    state: any,
    action: {
        body: any
    }
) => {
    const stateCopy: any = deep_copy(state)
    return Object.assign(stateCopy, action.body)
}

interface FractalContext {
    state: any,
    dispatch: any
}

export const createProvider = (initialState: any) => {
  const fractalContext = createContext({})
  const { Provider } = fractalContext

  const FractalProvider = (props: {
      children: JSX.Element | JSX.Element[]
  }) => {
      const { children } = props
      const [state, dispatch] = useReducer(reducer, initialState)

      return <Provider value={{ state, dispatch }}>{children}</Provider>
  }

  const useFractalProvider = () => {
      const context = useContext(fractalContext) as FractalContext
      if (context === undefined) {
          throw new Error("Fractal context is undefined")
      }
      return context
  }

  return { FractalProvider, useFractalProvider }
}