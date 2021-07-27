import React from "react"
import { Link } from "react-router-dom"
import classNames from "classnames"
import LogoWhite from "@app/assets/icons/logoWhite.svg"

export const AboutLink = (props: { className: string }) => (
    <Link className={props.className} to="/about" id="about">
        About
    </Link>
)

export const SupportLink = (props: { className: string }) => (
    <a
        className={props.className}
        href="mailto:support@fractal.co"
        id="support"
    >
        Support
    </a>
)

export const CareersLink = (props: { className: string }) => (
    <a
        className={props.className}
        href="https://www.notion.so/Fractal-Job-Board-a39b64712f094c7785f588053fc283a9"
        id="careers"
    >
        Careers
    </a>
)

export const LogoLink = (props: { className?: string; dark?: boolean }) => (
    <Link
        className={classNames(props.className)}
        style={{ minWidth: "1em" }}
        to="/"
    >
        <img
            src={LogoWhite}
            className="w-full h-full"
            alt="Logo"
        />
    </Link>
)

export const WordmarkLink = (props: { className?: string }) => (
    <Link
        className={classNames(
            props.className,
            "outline-none font-semibold hover:no-underline tracking-fractal"
        )}
        to="/"
    >
        FRACTAL
    </Link>
)

export const HomeLink = (props: { className?: string; onClick?: Function }) => (
    <Link className={props.className} to="/">
        Home
    </Link>
)

export const SalesLink = (props: any) => (
    <a {...props} href="mailto:sales@fractal.co">
        Sales
    </a>
)

export const SecurityLink = (props: any) => (
    <a {...props} href="mailto:security@fractal.co">
        Security
    </a>
)

export const BlogLink = (props: any) => (
    /* Temporary eslint-disable due to eslint bug */
    /* Can remove on eslint-plugin-react@7.21.6 release */
    /* eslint-disable react/jsx-no-target-blank */
    <a
        {...props}
        href="https://medium.com/@fractal"
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
