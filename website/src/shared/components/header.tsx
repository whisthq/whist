import React, { Dispatch, useState, ReactNode } from "react"
import { withClass } from "@app/shared/utils/withClass"
import history from "@app/shared/utils/history"
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
} from "@app/shared/components/links"
import { BarsIcon } from "@app/shared/components/icons"
import {
    JustifyStartEndRow,
    JustifyStartEndCol,
    ScreenFull,
} from "@app/shared/components/layouts"

const mobileHidden = "hidden md:inline"

const handleSignOut = (dispatch: Function) => {
    console.log("SIGNED Out")
}
const Logo = (props: { className?: string; dark?: boolean }) => (
    <div
        className={classNames(
            "flex items-center translate-x-1 space-x-6 mr-10",
            props.className
        )}
    >
        <LogoLink className="w-6 transform md:-translate-y-0.5" dark={props.dark} />
        <WordmarkLink
            className={classNames(
                "text-xl font-medium text-gray dark:text-gray-100",
                mobileHidden
            )}
        />
    </div>
)

const startButtonClasses = classNames(
    "text-gray-500 dark:text-gray-300 hover:text-blue dark:hover:text-mint",
    "duration-500 font-body tracking-widest hover:no-underline whitespace-nowrap"
)

const authButtonClasses = classNames(
    "font-body text-gray-100 dark:text-black bg-blue dark:bg-mint text-right",
    "px-10 py-2 rounded transition duration-500 whitespace-nowrap"
)

const authTextClasses = classNames(
    "font-bold text-gray-900 dark:text-white whitespace-nowrap tracking-wide")

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
    onAccountPage?: boolean | false
    isSignedIn?: boolean | false
    dark?: boolean
}) => {
    let [expanded, setExpanded] = useState(false)

    history.listen(() => setExpanded(false))
    const handleSignOutClick = () => handleSignOut(() => null)

    return (
        <div className="relative w-full">
            <div className="flex w-full mt-4 md:h-12 items-center">
                <JustifyStartEndRow
                    start={
                        <StartHeaderRow>
                            <Logo dark={props.dark} />
                            {!props.onAccountPage && (
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
                            {props.isSignedIn ? (
                                !props.onAccountPage && (
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
                                {props.isSignedIn && props.onAccountPage ? (
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
                                {props.isSignedIn && <MyAccountLinkStyled />}
                                {props.isSignedIn ? (
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

export default (Header)
