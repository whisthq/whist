import React, { Dispatch, useState, ReactNode } from "react"
import { connect } from "react-redux"
import * as PureAuthAction from "store/actions/auth/pure"
import * as PaymentPureAction from "store/actions/dashboard/payment/pure"
import { deepCopy } from "shared/utils/reducerHelpers"
import { withClass } from "shared/utils/withClass"
import history from "shared/utils/history"
import classNames from "classnames"
import {
    AboutLink,
    SupportLink,
    CareersLink,
    MyAccountLink,
    LogoLink,
    WordmarkLink,
    SignInLink,
    SignOutLink,
    HomeLink,
    SettingsLink,
} from "shared/components/links"
import { BarsIcon } from "shared/components/icons"
import {
    JustifyStartEndRow,
    JustifyStartEndCol,
    ScreenFull,
} from "shared/components/layouts"

import { DEFAULT as AUTH_DEFAULT } from "store/reducers/auth/default"
import { DEFAULT as DASHBOARD_DEFAULT } from "store/reducers/dashboard/default"
import { User, AuthFlow } from "shared/types/reducers"

const mobileHidden = "hidden md:inline"

const handleSignOut = (dispatch: Dispatch<any>) => {
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
const Logo = (props: { className?: string; dark?: boolean }) => (
    <div
        className={classNames(
            "flex items-center space-x-3 mr-10",
            props.className
        )}
    >
        <LogoLink className="w-6" dark={props.dark} />
        <WordmarkLink
            className={classNames(
                "text-xl font-medium text-gray dark:text-gray-100",
                "transition duration-500",
                mobileHidden
            )}
        />
    </div>
)

const startButtonClasses = classNames(
    "text-gray dark:text-gray-300 hover:text-blue dark:hover:text-mint",
    "duration-500 tracking-widest hover:no-underline whitespace-nowrap"
)

const authButtonClasses = classNames(
    "text-white dark:text-black bg-blue dark:bg-mint text-right",
    "px-10 py-2 rounded transition duration-500 whitespace-nowrap"
)

const authTextClasses = classNames("font-bold dark:text-white tracking-tight")

const AboutLinkStyled = withClass(AboutLink, startButtonClasses)
const SupportLinkStyled = withClass(SupportLink, startButtonClasses)
const CareersLinkStyled = withClass(CareersLink, startButtonClasses)
const HomeLinkStyled = withClass(HomeLink, startButtonClasses)
const SettingsLinkStyled = withClass(SettingsLink, startButtonClasses)
const MyAccountLinkStyled = withClass(MyAccountLink, startButtonClasses)
const MyAccountLinkBold = withClass(
    MyAccountLink,
    startButtonClasses,
    authTextClasses
)
const SignInLinkButton = withClass(SignInLink, authButtonClasses)
const SignInLinkBold = withClass(
    SignInLink,
    startButtonClasses,
    authTextClasses
)
const SignOutLinkButton = withClass(SignOutLink, authButtonClasses)

const StartHeaderRow = (props: { children?: ReactNode[] }) => (
    <div className="flex items-center space-x-6">{props.children}</div>
)

const StartHeaderCol = (props: {
    className?: string
    children?: ReactNode[] | ReactNode
}) => (
    <div className={classNames("flex flex-col items-center space-y-2")}>
        {props.children}
    </div>
)

const EndHeaderRow = (props?: any) => (
    <div className="flex items-center">{props.children}</div>
)

const EndHeaderCol = (props?: any) => (
    <div className="flex flex-col items-center space-y-4">{props.children}</div>
)

const Header = (props: {
    dispatch: Dispatch<any>
    user: User
    dark?: boolean
    account?: boolean
}) => {
    let [expanded, setExpanded] = useState(false)

    history.listen(() => setExpanded(false))
    const isSignedIn = props.user.userID
    const onAccountPage = props.account
    const handleSignOutClick = () => handleSignOut(props.dispatch)

    return (
        <div className="relative w-full">
            <div className="flex w-full mt-4 md:h-12 items-center">
                <JustifyStartEndRow
                    start={
                        <StartHeaderRow>
                            <Logo dark={props.dark} />
                            {!onAccountPage && (
                                <>
                                    <AboutLinkStyled className={mobileHidden} />
                                    <SupportLinkStyled
                                        className={mobileHidden}
                                    />
                                    <CareersLinkStyled
                                        className={mobileHidden}
                                    />
                                </>
                            )}
                        </StartHeaderRow>
                    }
                    end={
                        <EndHeaderRow>
                            {isSignedIn ? (
                                !onAccountPage && (
                                    <MyAccountLinkBold
                                        className={mobileHidden}
                                    />
                                )
                            ) : (
                                <SignInLinkBold className={mobileHidden} />
                            )}
                            <BarsIcon
                                className="inline md:hidden text-black dark:text-gray-100"
                                onClick={() => setExpanded(!expanded)}
                            />
                        </EndHeaderRow>
                    }
                />
            </div>
            <ScreenFull
                className={classNames(
                    "md:hidden",
                    props.dark ? "bg-blue-darkest" : "bg-white",
                    !expanded && "hidden"
                )}
            >
                <div className="flex w-full mt-36">
                    <JustifyStartEndCol
                        start={
                            <StartHeaderCol>
                                {isSignedIn && onAccountPage ? (
                                    <>
                                        <HomeLinkStyled />
                                        <SettingsLinkStyled />
                                    </>
                                ) : (
                                    <>
                                        <AboutLinkStyled />
                                        <SupportLinkStyled />
                                        <CareersLinkStyled />
                                    </>
                                )}
                            </StartHeaderCol>
                        }
                        middle={<div className="h-8"></div>}
                        end={
                            <EndHeaderCol>
                                {isSignedIn && <MyAccountLinkStyled />}
                                {isSignedIn ? (
                                    <SignOutLinkButton
                                        onClick={handleSignOutClick}
                                    />
                                ) : (
                                    <SignInLinkButton />
                                )}
                            </EndHeaderCol>
                        }
                    />
                </div>
            </ScreenFull>
        </div>
    )
}

const mapStateToProps = (state: {
    AuthReducer: { user: User; authFlow: AuthFlow }
}) => {
    return {
        user: state.AuthReducer.user,
    }
}

export default connect(mapStateToProps)(Header)
