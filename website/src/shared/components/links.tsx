import React from "react"
import { Link } from "react-router-dom"
import classNames from "classnames"
import LogoBlack from "assets/icons/logoBlack.svg"
import LogoWhite from "assets/icons/logoWhite.svg"

export const AboutLink = (props: { className: string }) => (
    <Link className={props.className} to="/about" id="about">
        About
    </Link>
)

export const SupportLink = (props: { className: string }) => (
    <a
        className={props.className}
        href="mailto: support@fractal.co"
        id="support"
    >
        Support
    </a>
)

export const CareersLink = (props: { className: string }) => (
    <a
        className={props.className}
        href="mailto: careers@fractal.co"
        id="careers"
    >
        Careers
    </a>
)

export const MyAccountLink = (props: { className?: string }) => (
    <Link className={props.className} to="/dashboard">
        My Account
    </Link>
)

export const LogoLink = (props: { className?: string; dark?: boolean }) => (
    <Link
        className={classNames(props.className)}
        style={{ minWidth: "1em" }}
        to="/"
    >
        <img
            src={props.dark ? LogoWhite : LogoBlack}
            className="w-full h-full"
            alt="Logo"
        />
    </Link>
)

export const WordmarkLink = (props: { className?: string }) => (
    <Link
        className={classNames(
            props.className,
            "outline-none tracking-widest hover:no-underline"
        )}
        to="/"
    >
        FRACTAL
    </Link>
)

export const SignInLink = (props: { className?: string }) => (
    <Link id="signin" to="/auth" className={props.className}>
        Sign In
    </Link>
)

export const SignOutLink = (props: {
    className?: string
    onClick?: Function
}) => (
    <button
        className={props.className}
        onClick={() => props.onClick && props.onClick()}
    >
        Sign Out
    </button>
)

export const HomeLink = (props: { className?: string; onClick?: Function }) => (
    <Link className={props.className} to="/dashboard">
        Home
    </Link>
)

export const SettingsLink = (props: {
    className?: string
    onClick?: Function
}) => (
    <Link to="/dashboard/settings" className={props.className}>
        Settings
    </Link>
)

export const SalesLink = (props: any) => (
    <a {...props} href="mailto: sales@fractal.co">
        Sales
    </a>
)

export const BlogLink = (props: any) => (
    /* Temporary eslint-disable due to eslint bug */
    /* Can remove on eslint-plugin-react@7.21.6 release */
    /* eslint-disable react/jsx-no-target-blank */
    <a
        {...props}
        href="fractal.medium.com"
        target="_blank"
        rel="noopener noreferror"
    >
        Blog
    </a>
)

export const DiscordLink = (props: any) => (
    /* Temporary eslint-disable due to eslint bug */
    /* Can remove on eslint-plugin-react@7.21.6 release */
    /* eslint-disable react/jsx-no-target-blank */
    <a
        {...props}
        href="https://discord.gg/HjPpDGvEeA"
        target="_blank"
        rel="noopener noreferror"
    >
        Join our Discord
    </a>
)
