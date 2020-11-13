import React, { useEffect, useState } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { useQuery } from "@apollo/client"
import { HashLink } from "react-router-hash-link"

import Header from "shared/components/header"
import DownloadBox from "pages/dashboard/components/downloadBox"
import { GET_USER } from "pages/dashboard/constants/graphql"
import { PuffAnimation } from "shared/components/loadingAnimations"
import { updateUser } from "store/actions/auth/pure"
import { DEFAULT } from "store/reducers/auth/default"
import { deepCopy } from "shared/utils/reducerHelpers"

//import { CopyToClipboard } from "react-copy-to-clipboard"
// use copy to clipboard functionality when we add back in linux

import "styles/auth.css"

// Important Note: if you are allowed to login you will be allowed to download
// Basically, all users who are not allowed to download won't be allowed to login
// (and thus get off the waitlist) for now. A more complicated fix would require us
// to use more endpoints or somehow let the server know if the login is on the
// site or on the client
const Dashboard = (props: {
    dispatch: any
    user: {
        user_id: string
        canLogin: boolean
        accessToken: string
        waitlistToken: string
        emailVerificationToken: string
    }
}) => {
    const { user, dispatch } = props
    const [canLogin, setCanLogin] = useState(false)

    //const [copiedtoClip, setCopiedtoClip] = useState(false)
    //const linuxCommands = "sudo apt-get install libavcodec-dev libavdevice-dev libx11-dev libxtst-dev xclip x11-xserver-utils -y"
    const valid_user = user.user_id && user.user_id !== ""
    const name = user.user_id ? user.user_id.split("@")[0] : ""

    const { data, loading } = useQuery(GET_USER, {
        variables: { user_id: user.user_id },
        context: {
            headers: {
                Authorization: `Bearer ${user.accessToken}`,
            },
        },
    })

    const logout = () => {
        dispatch(updateUser(deepCopy(DEFAULT.user)))
    }

    useEffect(() => {
        if (
            data &&
            data.users &&
            data.users[0] &&
            (data.users[0].can_login ||
                user.user_id.includes("@tryfractal.com") ||
                user.waitlistToken === user.emailVerificationToken)
        ) {
            setCanLogin(true)
            dispatch(
                updateUser({
                    canLogin: data.users[0].can_login,
                })
            )
        }
    }, [
        data,
        dispatch,
        user.user_id,
        user.waitlistToken,
        user.emailVerificationToken,
    ])

    if (loading) {
        return (
            <div
                style={{
                    position: "relative",
                    height: "100vh",
                    marginTop: "-35%",
                }}
            >
                <PuffAnimation />
            </div>
        )
    } else {
        if (!valid_user) {
            return <Redirect to="/auth" />
        } else if (!user.canLogin && !canLogin) {
            return (
                <div className="fractalContainer">
                    <Header color="black" />
                    <div
                        style={{
                            width: 400,
                            margin: "auto",
                            marginTop: 70,
                        }}
                    >
                        <div
                            style={{
                                color: "#111111",
                                fontSize: 32,
                                fontWeight: "bold",
                            }}
                        >
                            You're still on the waitlist!
                        </div>
                        <div
                            style={{
                                marginTop: 15,
                            }}
                        >
                            The more friends you refer, the faster we'll take
                            you off the waitlist.
                        </div>
                        <HashLink to="/#top">
                            <button
                                className="white-button"
                                style={{
                                    width: "100%",
                                    marginTop: 25,
                                    color: "white",
                                    background: "rgba(66, 133, 244, 0.9)",
                                    border: "none",
                                    fontSize: 16,
                                }}
                            >
                                Back to Home
                            </button>
                        </HashLink>
                        <button
                            className="white-button"
                            style={{
                                width: "100%",
                                marginTop: 25,
                                fontSize: 16,
                            }}
                            onClick={logout}
                        >
                            Log Out
                        </button>
                    </div>
                </div>
            )
        } else {
            // for now it wil lalways be loading
            return (
                <div className="fractalContainer">
                    <Header color="black" />
                    <div
                        style={{
                            width: 400,
                            margin: "auto",
                            marginTop: 70,
                        }}
                    >
                        <div
                            style={{
                                color: "#111111",
                                fontSize: 32,
                                fontWeight: "bold",
                            }}
                        >
                            Congratulations{name.length < 7 ? " " + name : ""}!
                        </div>
                        <div
                            style={{
                                marginTop: 20,
                                color: "#333333",
                            }}
                        >
                            You've been selected to join our private beta! Click
                            the button below to download Fractal.
                        </div>
                        <DownloadBox />
                        <div
                            style={{
                                textAlign: "center",
                                marginTop: 25,
                                width: "100%",
                                height: 1,
                                background: "#DFDFDF",
                            }}
                        ></div>
                        <button
                            className="white-button"
                            style={{
                                width: "100%",
                                marginTop: 25,
                                fontSize: 16,
                            }}
                            onClick={logout}
                        >
                            Log Out
                        </button>
                    </div>
                </div>
            )
        }
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Dashboard)
