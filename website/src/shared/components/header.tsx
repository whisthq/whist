import React, { useContext } from "react"
import { connect } from "react-redux"
import { Link } from "react-router-dom"
import { Dropdown, DropdownButton } from "react-bootstrap"

import MainContext from "shared/context/mainContext"
import * as PureAuthAction from "store/actions/auth/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT } from "store/reducers/auth/default"

const Header = (props: {
    dispatch: any
    user: any
    color: string
    account?: boolean
}) => {
    const { width } = useContext(MainContext)

    const { dispatch, user, account, color } = props

    const handleSignOut = () => {
        dispatch(PureAuthAction.updateUser(deepCopy(DEFAULT.user)))
    }

    // Only render navigation links for desktops and tablets
    if (width < 720) {
        return (
            <div
                style={{
                    display: "flex",
                    width: "100%",
                    paddingBottom: 0,
                    paddingTop: 25,
                    borderBottom: "solid 1px #DFDFDF",
                }}
            >
                <Link
                    to="/"
                    style={{
                        outline: "none",
                        textDecoration: "none",
                        marginRight: 100,
                    }}
                >
                    <div
                        className="logo"
                        style={{
                            marginBottom: 20,
                            color: color ? color : "white",
                        }}
                    >
                        Fractal
                    </div>
                </Link>
            </div>
        )
    } else {
        return (
            <div
                style={{
                    display: "flex",
                    justifyContent: "space-between",
                    width: "100%",
                    paddingBottom: 0,
                    paddingTop: 25,
                    borderBottom: "solid 1px #DFDFDF",
                }}
            >
                <div
                    style={{
                        display: "flex",
                    }}
                >
                    <Link
                        to="/"
                        style={{
                            outline: "none",
                            textDecoration: "none",
                            marginRight: 100,
                        }}
                    >
                        <div
                            className="logo"
                            style={{
                                marginBottom: 20,
                                color: color ? color : "white",
                            }}
                        >
                            Fractal
                        </div>
                    </Link>
                    {account ? (
                        <div></div>
                    ) : (
                        <div style={{ display: "flex" }}>
                            <Link to="/about" className="header-link">
                                About
                            </Link>
                            <a
                                href="mailto: support@fractalcomputers.com"
                                className="header-link"
                            >
                                Support
                            </a>
                            <a
                                href="mailto: careers@fractalcomputers.com"
                                className="header-link"
                            >
                                Careers
                            </a>
                        </div>
                    )}
                </div>
                <div>
                    {user.user_id ? (
                        <>
                            <DropdownButton
                                title="My Account"
                                bsPrefix="account-button"
                                menuAlign="right"
                            >
                                <Dropdown.Item href="/dashboard">
                                    Dashboard
                                </Dropdown.Item>
                                <Dropdown.Item href="/profile">
                                    Profile
                                </Dropdown.Item>
                                <Dropdown.Item onClick={handleSignOut}>
                                    Sign Out
                                </Dropdown.Item>
                            </DropdownButton>
                        </>
                    ) : (
                        <Link
                            to="/auth/bypass"
                            className="header-link"
                            style={{
                                fontWeight: "bold",
                                marginRight: 0,
                            }}
                        >
                            Sign In
                        </Link>
                    )}
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: any; authFlow: any }
}) => {
    console.log(state)
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Header)
