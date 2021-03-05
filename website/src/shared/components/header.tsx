import React, { Dispatch, useContext, useState } from "react"
import { connect } from "react-redux"
import { Link } from "react-router-dom"
import { FaBars } from "react-icons/fa"

import { ScreenSize } from "shared/constants/screenSizes"
import MainContext from "shared/context/mainContext"
import * as PureAuthAction from "store/actions/auth/pure"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import history from "shared/utils/history"
import LogoBlack from "assets/icons/logoBlack.svg"
import LogoWhite from "assets/icons/logoWhite.svg"

import { DEFAULT as AUTH_DEFAULT } from "store/reducers/auth/default"
import { DEFAULT as DASHBOARD_DEFAULT } from "store/reducers/dashboard/default"
import { User, AuthFlow } from "store/reducers/auth/default"

import styles from "styles/shared.module.css"

const Header = (props: {
    dispatch: Dispatch<any>
    user: User
    dark?: boolean
    account?: boolean
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
        history.push("/auth")
    }

    // Only render navigation links for desktops and tablets
    if (width >= ScreenSize.MEDIUM) {
        return (
            <div className="flex justify-between w-full pt-8 bg-none">
                <div className="flex">
                    <Link
                        to="/"
                        className="outline-none no-underline mr-16 flex"
                    >
                        <img
                            src={dark ? LogoWhite : LogoBlack}
                            className="w-6 h-6 mr-3 mt-1"
                            alt="Logo"
                        />
                        <div className="text-xl text-gray dark:text-gray-100 font-medium mt-0.5 tracking-widest">
                            FRACTAL
                        </div>
                    </Link>
                    {!account && (
                        <div className="flex mt-1">
                            <Link
                                to="/about"
                                id="about"
                                className="text-gray dark:text-gray-300 no-underline mr-5 hover:text-blue dark:hover:text-mint duration-500 tracking-widest"
                            >
                                About
                            </Link>
                            <a
                                href="mailto: support@fractal.co"
                                className="text-gray dark:text-gray-300 no-underline mr-5 hover:text-blue dark:hover:text-mint duration-500 tracking-widest"
                            >
                                Support
                            </a>
                            <a
                                href="mailto: careers@fractal.co"
                                className="text-gray dark:text-gray-300 no-underline mr-5 hover:text-blue dark:hover:text-mint duration-500 tracking-widest"
                            >
                                Careers
                            </a>
                        </div>
                    )}
                </div>
                <div>
                    {user.userID ? (
                        !account ? (
                            <>
                                <Link
                                    to="/dashboard"
                                    className="text-gray dark:text-gray-100 tracking-wider font-medium text-lg no-underline dark:text-white dark:hover:text-mint duration-500"
                                >
                                    My Account
                                </Link>
                            </>
                        ) : (
                            <></>
                        )
                    ) : (
                        <Link
                            id="signin"
                            to="/auth"
                            className={
                                dark
                                    ? styles.headerLinkLight
                                    : styles.headerLink
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
            <div>
                <div className="flex justify-between w-full pt-4 bg-white dark:bg-blue-darkest">
                    <Link to="/" className="outline-none no-underline flex">
                        <img
                            src={dark ? LogoWhite : LogoBlack}
                            className="w-6 h-6 mr-3 mt-1"
                            alt="Logo"
                        />
                    </Link>
                    <div>
                        <button
                            type="button"
                            onClick={() => {
                                setExpanded(!expanded)
                            }}
                            className="bg-none outline-none relative top-1 text-right dark:text-gray-100"
                        >
                            <FaBars className="text-black dark:text-gray-100" />
                        </button>
                    </div>
                </div>
                <div
                    className="text-center w-full h-screen duration-500 relative"
                    style={{ display: expanded ? "block" : "none" }}
                >
                    {account ? (
                        <div className="absolute top-1/3 left-1/2 transform -translate-y-1/2 -translate-x-1/2">
                            <div
                                className="mb-2 tracking-wider"
                                onClick={() => setExpanded(false)}
                            >
                                <Link
                                    to="/dashboard"
                                    className="text-gray dark:text-gray-300"
                                >
                                    Home
                                </Link>
                            </div>
                            <div
                                className="mb-2 tracking-wider"
                                onClick={() => setExpanded(false)}
                            >
                                <Link
                                    to="/dashboard/settings"
                                    className="text-gray dark:text-gray-300"
                                >
                                    Settings
                                </Link>
                            </div>
                            <div
                                className="mt-10 b-2 tracking-wider"
                                onClick={() => setExpanded(false)}
                            >
                                <button
                                    className="text-white dark:text-black bg-blue dark:bg-mint text-right px-10 py-2 rounded"
                                    onClick={handleSignOut}
                                >
                                    Sign Out
                                </button>
                            </div>
                        </div>
                    ) : (
                        <div className="absolute top-1/3 left-1/2 transform -translate-y-1/2 -translate-x-1/2">
                            <div className="mb-2 tracking-wider">
                                <Link
                                    to="/about"
                                    className="text-gray dark:text-gray-300"
                                >
                                    About
                                </Link>
                            </div>
                            <div className="mb-2 tracking-wider">
                                <a
                                    href="mailto: support@fractal.co"
                                    className="text-gray dark:text-gray-300"
                                >
                                    Support
                                </a>
                            </div>
                            <div className="mb-2 tracking-wider">
                                <a
                                    href="mailto: careers@fractal.co"
                                    className="text-gray dark:text-gray-300"
                                >
                                    Careers
                                </a>
                            </div>
                            <>
                                {user.userID ? (
                                    <div className="mt-10">
                                        <div className="mb-2 tracking-wider">
                                            <Link
                                                to="/dashboard"
                                                className="text-gray dark:text-gray-300"
                                            >
                                                My Account
                                            </Link>
                                        </div>
                                        <div className="pt-2 mb-2 tracking-wider">
                                            <button
                                                className="text-white dark:text-black bg-blue dark:bg-mint text-right px-10 py-2 rounded"
                                                onClick={handleSignOut}
                                            >
                                                Sign Out
                                            </button>
                                        </div>
                                    </div>
                                ) : (
                                    <div
                                        className="mt-10 pt-2 mb-2 tracking-wider"
                                        onClick={() => setExpanded(false)}
                                    >
                                        <Link
                                            id="signin"
                                            to="/auth"
                                            className="text-white dark:text-black bg-blue dark:bg-mint text-right px-10 py-2 rounded"
                                        >
                                            Sign In
                                        </Link>
                                    </div>
                                )}
                            </>
                        </div>
                    )}
                </div>
            </div>
        )
    }
}

const mapStateToProps = (state: {
    AuthReducer: { user: User; authFlow: AuthFlow }
}) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Header)
