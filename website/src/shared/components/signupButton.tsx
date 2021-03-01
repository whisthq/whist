import React from "react"
import { connect } from "react-redux"
import { HashLink } from "react-router-hash-link"


export const SignupButton = () => {
    return (
        <div>
            <HashLink to="/auth/bypass/#top">
                <button
                    className="px-4 text-lg outline-none tracking-wide bg-black text-black border-1 border-gray-100"
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
