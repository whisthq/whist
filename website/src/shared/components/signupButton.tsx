import React from "react"
import { connect } from "react-redux"

import { HashLink } from "react-router-hash-link"

import sharedStyles from "styles/shared.module.css"

export const SignupButton = () => {
    return (
        <div>
            <HashLink to="/auth/bypass/#top">
                <button
                    className={sharedStyles.whiteButton}
                    style={{ textTransform: "uppercase" }}
                >
                    GET STARTED
                </button>
            </HashLink>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: {
        user: {
            userID: string
        }
    }
}) => {
    return {
        userID: state.AuthReducer.user.userID,
    }
}

export default connect(mapStateToProps)(SignupButton)
