import React, { useContext, useState } from "react"
import { connect } from "react-redux"
import { Link } from "react-router-dom"
import { Dropdown, DropdownButton, Collapse } from "react-bootstrap"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faBars } from "@fortawesome/free-solid-svg-icons"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"
import * as PureAuthAction from "store/actions/auth/pure"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import history from "shared/utils/history"
import { DEFAULT as AUTH_DEFAULT } from "store/reducers/auth/default"
import { DEFAULT as DASHBOARD_DEFAULT } from "store/reducers/dashboard/default"

const Header = (props: {
    dispatch: any
    user: any
    dark: boolean
    account?: boolean
    waitlistUser: any
}) => {
    const { width } = useContext(MainContext)

    const { dispatch, user, account, dark } = props

    const [expanded, setExpanded] = useState(false)

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
        history.push("/auth/bypass")
    }

    // Only render navigation links for desktops and tablets
    if (width >= ScreenSize.MEDIUM) {
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
                    {!account && (
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
                                href="mailto: support@fractal.co"
                                className="header-link"
                            >
                                Support
                            </a>
                            <a
                                href="mailto: careers@fractal.co"
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
    } else {
        return (
            <div
                style={{
                    display: "flex",
                    flexDirection: "row",
                    justifyContent: "space-between",
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
                <div style={{ display: "flex", flexDirection: "column" }}>
                    <button
                        type="button"
                        onClick={() => {
                            setExpanded(!expanded)
                        }}
                        style={{
                            background: "none",
                            outline: "none !important",
                            border: "none",
                            position: "relative",
                            top: 5,
                            textAlign: "right",
                        }}
                    >
                        <FontAwesomeIcon
                            icon={faBars}
                            style={{
                                color: "black",
                            }}
                        />
                    </button>
                    <Collapse in={expanded}>
                        <div>
                            <div
                                style={{
                                    display: "flex",
                                    flexDirection: "column",
                                    textAlign: "right",
                                    padding: "12px 0 12px 0",
                                }}
                            >
                                {account ? (
                                    <>
                                        <Link
                                            to="/dashboard"
                                            className="mobile-header-link"
                                        >
                                            Dashboard
                                        </Link>
                                        <Link
                                            to="/profile"
                                            className="mobile-header-link"
                                        >
                                            Profile
                                        </Link>
                                        <button
                                            className="mobile-header-link"
                                            onClick={handleSignOut}
                                            style={{
                                                background: "none",
                                                border: "none",
                                                padding: 0,
                                                textAlign: "right",
                                            }}
                                        >
                                            Sign Out
                                        </button>
                                    </>
                                ) : (
                                    <>
                                        <Link
                                            to="/about"
                                            className="mobile-header-link"
                                        >
                                            About
                                        </Link>
                                        <a
                                            href="mailto: support@fractal.co"
                                            className="mobile-header-link"
                                        >
                                            Support
                                        </a>
                                        <a
                                            href="mailto: careers@fractal.co"
                                            className="mobile-header-link"
                                        >
                                            Careers
                                        </a>
                                        <>
                                            {user.userID ? (
                                                <>
                                                    <Link
                                                        to="/dashboard"
                                                        className="mobile-header-link"
                                                    >
                                                        My Account
                                                    </Link>
                                                    <button
                                                        className="mobile-header-link"
                                                        onClick={handleSignOut}
                                                        style={{
                                                            background: "none",
                                                            border: "none",
                                                            padding: 0,
                                                            textAlign: "right",
                                                        }}
                                                    >
                                                        Sign Out
                                                    </button>
                                                </>
                                            ) : (
                                                <Link
                                                    to="/auth/bypass"
                                                    className="mobile-header-link"
                                                >
                                                    Sign In
                                                </Link>
                                            )}
                                        </>
                                    </>
                                )}
                            </div>
                        </div>
                    </Collapse>
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: any; authFlow: any }
    WaitlistReducer: { waitlistUser: any }
}) => {
    return {
        user: state.AuthReducer.user,
        waitlistUser: state.WaitlistReducer.waitlistUser,
    }
}

export default connect(mapStateToProps)(Header)
