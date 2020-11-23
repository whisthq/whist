import React, { useEffect } from "react"
import { connect } from "react-redux"
import { Redirect } from "react-router"

import Header from "shared/components/header"
import NameForm from "pages/profile/components/nameForm"
import PasswordForm from "pages/profile/components/passwordForm"
import * as PureAuthAction from "store/actions/auth/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT } from "store/reducers/auth/default"

import "styles/profile.css"

const Profile = (props: any) => {
    const { user, dispatch } = props

    const valid_user = user.user_id && user.user_id !== ""

    const handleSignOut = () => {
        dispatch(PureAuthAction.updateUser(deepCopy(DEFAULT.user)))
        dispatch(PureAuthAction.updateAuthFlow(deepCopy(DEFAULT.authFlow)))
    }

    // Clear redux values on page load that will be set when editing password
    useEffect(() => {
        dispatch(
            PureAuthAction.updateAuthFlow({
                resetDone: false,
                passwordVerified: null,
            })
        )
    }, [dispatch])

    if (!valid_user) {
        return <Redirect to="/auth/bypass" />
    } else {
        return (
            <div className="fractalContainer">
                <Header dark={false} account />
                <div
                    style={{
                        width: 500,
                        margin: "auto",
                        marginTop: 70,
                    }}
                >
                    <h2 style={{ marginBottom: "20px" }}>Your Profile</h2>
                    <div className="section-title">Email</div>
                    <div className="section-info">{user.user_id}</div>
                    <NameForm />
                    <PasswordForm />
                    <button
                        className="white-button"
                        style={{
                            width: "100%",
                            marginTop: 25,
                            fontSize: 16,
                        }}
                        onClick={handleSignOut}
                    >
                        Sign Out
                    </button>
                </div>
            </div>
        )
    }
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Profile)
