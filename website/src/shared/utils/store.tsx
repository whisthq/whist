import React, {createContext, useReducer, useContext, useEffect} from "react"
import { deep_copy } from "shared/utils/reducerHelpers"

interface FractalContext {
    state: any,
    dispatch: any
}

const reducer = (
    state: any,
    action: {
        body: any
    }
) => {
    const stateCopy: any = deep_copy(state)
    return Object.assign(stateCopy, action.body)
}

const persistState = (storageKey: string, state: object): void => {
    window.localStorage.setItem(storageKey, JSON.stringify(state));
}

const getInitialState = (storageKey: string): object | undefined => {
    const savedState = window.localStorage.getItem(storageKey);
    try {
        if (!savedState) {
            return undefined
        }
        return JSON.parse(savedState ?? {})
    } catch (e) {
        return undefined
    }
}

export const createProvider = (initialState: object, storageKey: string) => {
  initialState = getInitialState(storageKey) ?? initialState 

  const fractalContext = createContext({})
  const { Provider } = fractalContext

  const FractalProvider = (props: {
      children: JSX.Element | JSX.Element[]
  }) => {
      const { children } = props
      const [state, dispatch] = useReducer(reducer, initialState)

      useEffect(() => persistState(storageKey, state), [state])

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