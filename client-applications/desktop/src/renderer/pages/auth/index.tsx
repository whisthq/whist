import React, { useState, useReducer } from "react"
import { Route } from "react-router-dom"

import Login from "@app/renderer/pages/auth/login"
import Signup from "@app/renderer/pages/auth/signup"
import { useMainState } from "@app/utils/state"
import { string } from "prop-types"

type AuthState = {
    loginEmail: string
    loginPassword: string
    signupEmail: string
    signupPassword: string
    signupConfirmPassword: string
}

const initialState: AuthState = {
    loginEmail: "",
    loginPassword: "",
    signupEmail: "",
    signupPassword: "",
    signupConfirmPassword: "",
}

const AuthReducer = (state: AuthState, action: Record<any, any>) => {
    const isValidType = (value: string): value is keyof AuthState => {
        return !Object.keys(initialState).includes(action.type)
    }

    if (isValidType(action.type)) {
        state[action.type] = action.body
        console.log(state)
        return state
    } else {
        throw new Error("Invalid reducer key")
    }
}

const Auth = () => {
    /*
        Description:
            Router for auth-related pages (e.g. login and signup)
    */

    const [mainState, setMainState] = useMainState()
    const [state, dispatch] = useReducer(AuthReducer, initialState)

    const onLogin = () => {
        setMainState({
            email: state.loginEmail,
            loginRequest: {
                email: state.loginEmail,
                password: state.loginPassword,
            },
        })
    }

    const onSignup = () => {
        setMainState({
            email: state.signupEmail,
            signupRequest: {
                email: state.signupEmail,
                password: state.signupPassword,
                confirmPassword: state.signupConfirmPassword,
            },
        })
    }

    return (
        <>
            <Route
                exact
                path="/"
                render={() => (
                    <Login
                        email={state.loginEmail}
                        password={state.loginPassword}
                        warning={mainState.loginWarning}
                        loading={mainState.loginLoading}
                        onLogin={onLogin}
                        onChangeEmail={(s: string) =>
                            dispatch({ type: "loginEmail", body: s })
                        }
                        onChangePassword={(s: string) =>
                            dispatch({ type: "loginPassword", body: s })
                        }
                    />
                )}
            />
            <Route
                path="/auth/login"
                render={() => (
                    <Login
                        email={state.loginEmail}
                        password={state.loginPassword}
                        warning={mainState.loginWarning}
                        loading={mainState.loginLoading}
                        onLogin={onLogin}
                        onChangeEmail={(s: string) =>
                            dispatch({ type: "loginEmail", body: s })
                        }
                        onChangePassword={(s: string) =>
                            dispatch({ type: "loginPassword", body: s })
                        }
                    />
                )}
            />
            <Route
                path="/auth/signup"
                render={() => (
                    <Signup
                        onSignup={onSignup}
                        email={state.signupEmail}
                        password={state.signupPassword}
                        confirmPassword={state.signupConfirmPassword}
                        onChangeEmail={(s: string) =>
                            dispatch({ type: "signupEmail", body: s })
                        }
                        onChangePassword={(s: string) =>
                            dispatch({ type: "signupPassword", body: s })
                        }
                        onChangeConfirmPassword={(s: string) =>
                            dispatch({ type: "signupConfirmPassword", body: s })
                        }
                    />
                )}
            />
        </>
    )
}

export default Auth
