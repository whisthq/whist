import React, { useContext } from "react"
import { connect } from "react-redux"
import { Link } from "react-router-dom"
import { Dropdown, DropdownButton } from "react-bootstrap"

import MainContext from "shared/context/mainContext"
import * as PureAuthAction from "store/actions/auth/pure"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { DEFAULT as AUTH_DEFAULT } from "store/reducers/auth/default"
import { DEFAULT as DASHBOARD_DEFAULT } from "store/reducers/dashboard/default"

const Header = (props: {
    dispatch: any
    user: any
    dark: boolean
    account?: boolean
}) => {
    const { width } = useContext(MainContext)

    const { dispatch, user, account, dark } = props

    const handleSignOut = () => {
        dispatch(PureAuthAction.updateUser(deepCopy(AUTH_DEFAULT.user)))
        dispatch(
            PaymentPureAction.updateStripeInfo(
                deepCopy(DASHBOARD_DEFAULT.stripeInfo)
            )
        )
        dispatch(
            PaymentPureAction.updatePaymentFlow(
                deepCopy(DASHBOARD_DEFAULT.paymentFlow)
            )
        )
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
                    background: dark ? "black" : "white",
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
                            color: dark ? "white" : "black",
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
                    background: dark ? "black" : "white",
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
                                color: dark ? "white" : "black",
                            }}
                        >
                            Fractal
                        </div>
                    </Link>
                    {account ? (
                        <div></div>
                    ) : (
                        <div
                            style={{
                                display: "flex",
                                color: dark ? "white" : "black",
                            }}
                        >
                            <Link
                                to="/about"
                                className={
                                    dark ? "header-link-light" : "header-link"
                                }
                                style={{ color: dark ? "white" : "black" }}
                            >
                                About
                            </Link>
                            <a
                                href="mailto: support@tryfractal.com"
                                className="header-link"
                            >
                                Support
                            </a>
                            <a
                                href="mailto: careers@tryfractal.com"
                                className="header-link"
                            >
                                Careers
                            </a>
                        </div>
                    )}
                </div>
                <div>
                    {user.userID ? (
                        <>
                            <DropdownButton
                                title="My Account"
                                bsPrefix={
                                    dark
                                        ? "account-button-light"
                                        : "account-button"
                                }
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
                            className={
                                dark ? "header-link-light" : "header-link"
                            }
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
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Header)
