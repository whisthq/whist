import React from "react"
import { connect } from "react-redux"
import "styles/landing.css"

const GoogleButton = (props: any) => {
    return <div></div>
}

function mapStateToProps(state: { AuthReducer: { user: any } }) {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(GoogleButton)
