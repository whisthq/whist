import React from "react"
import { Button } from "react-bootstrap"
import { connect } from "react-redux"
import "styles/auth.css"

const SignoutButton = (props: any) => {
    return <Button className="signout-button">Sign out</Button>
}

const mapStateToProps = () => {}

export default connect(mapStateToProps)(SignoutButton)
