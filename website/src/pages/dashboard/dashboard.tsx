import React from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"
import { DivSpace } from "shared/components/spacingAndText"
import Header from "shared/components/header"
import DownloadBox from "pages/dashboard/components/downloadBox"
//import { CopyToClipboard } from "react-copy-to-clipboard"
// use copy to clipboard functionality when we add back in linux

import "styles/auth.css"
import { Link } from "react-router-dom"
import { updateAuthFlow, updateUser } from "store/actions/auth/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT } from "store/reducers/auth/default"

// Important Note: if you are allowed to login you will be allowed to download
// Basically, all users who are not allowed to download won't be allowed to login
// (and thus get off the waitlist) for now. A more complicated fix would require us
// to use more endpoints or somehow let the server know if the login is on the
// site or on the client
const Dashboard = (props: {
    dispatch: any
    user: {
        user_id: string
    }
}) => {
    const { dispatch, user } = props

    //const [copiedtoClip, setCopiedtoClip] = useState(false)
    //const linuxCommands = "sudo apt-get install libavcodec-dev libavdevice-dev libx11-dev libxtst-dev xclip x11-xserver-utils -y"
    const valid_user = user.user_id && user.user_id !== ""
    const name = valid_user ? user.user_id.split("@")[0] : ""

    const logout = () => {
        // updates the auth state, but not the waitlist state
        // important to use the default and not reset by hand since there are many states and we will modify this
        dispatch(updateUser(deepCopy(DEFAULT.user)))
        dispatch(updateAuthFlow(deepCopy(DEFAULT.authFlow)))
    }

    if (!valid_user) {
        return <Redirect to="/" />
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
                        You've been selected to join our private beta! Click the
                        button below to download Fractal.
                    </div>
                    <DownloadBox />
                    <DivSpace height={40} />

                    <div
                        style={{
                            display: "flex",
                            justifyContent: "center",
                        }}
                    >
                        Want to Log out?
                    </div>
                    <div
                        style={{
                            display: "flex",
                            justifyContent: "center",
                        }}
                    >
                        <Link to="/" onClick={logout}>
                            Log out.
                        </Link>
                    </div>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any; authFlow: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Dashboard)
