import React, { useState, useContext } from "react"
import { connect } from "react-redux"
import { FormControl } from "react-bootstrap"

import "styles/auth.css"

import MainContext from "shared/context/mainContext"
import * as AuthPureAction from "store/actions/auth/pure"

import GoogleButton from "pages/auth/components/googleButton"

const SignupView = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const { width } = useContext(MainContext)
    const { dispatch } = props

    return (
        <div>
            <div
                style={{
                    width: 400,
                    margin: "auto",
                    marginTop: 150,
                }}
            >
                <h2
                    style={{
                        color: "#111111",
                        textAlign: "center",
                    }}
                >
                    Let's get started.
                </h2>
                <FormControl
                    type="email"
                    aria-label="Default"
                    aria-describedby="inputGroup-sizing-default"
                    placeholder="Email Address"
                    className="input-form"
                    style={{
                        marginTop: 40,
                    }}
                />
                <FormControl
                    type="password"
                    aria-label="Default"
                    aria-describedby="inputGroup-sizing-default"
                    placeholder="Password"
                    className="input-form"
                    style={{
                        marginTop: 15,
                    }}
                />
                <FormControl
                    type="password"
                    aria-label="Default"
                    aria-describedby="inputGroup-sizing-default"
                    placeholder="Confirm Password"
                    className="input-form"
                    style={{
                        marginTop: 15,
                    }}
                />
                <button
                    className="white-button"
                    style={{
                        width: "100%",
                        marginTop: 15,
                        background: "#3930b8",
                        border: "none",
                        color: "white",
                        fontSize: 16,
                        paddingTop: 20,
                        paddingBottom: 20,
                    }}
                >
                    Sign up
                </button>
                <div
                    style={{
                        height: 1,
                        width: "100%",
                        marginTop: 30,
                        marginBottom: 30,
                        background: "#EFEFEF",
                    }}
                ></div>
                <GoogleButton />
            </div>
        </div>
    )
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(SignupView)
