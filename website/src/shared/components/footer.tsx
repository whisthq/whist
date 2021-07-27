import React from "react"
import { FaLinkedinIn, FaTwitter, FaInstagram, FaMediumM } from "react-icons/fa"

import Logo from "@app/assets/icons/logoWhite.svg"

/* eslint-disable react/display-name */

const navigation = {
    home: [
        { name: "Product", href: "/" },
        { name: "Pricing", href: "/#pricing" },
        { name: "Download", href: "/#download" },
    ],
    about: [
        { name: "Company", href: "/about" },
        { name: "Technology", href: "/technology" },
        { name: "Security", href: "/security" },
        { name: "FAQ", href: "/faq" },
    ],
    resources: [
        { name: "Support", href: "/support" },
        {
            name: "Careers",
            href: "https://www.notion.so/tryfractal/Fractal-Job-Board-a39b64712f094c7785f588053fc283a9",
        },
    ],
    legal: [
        { name: "Privacy", href: "/privacy" },
        { name: "Terms", href: "/termsofservice" },
    ],
    social: [
        {
            name: "Medium",
            href: "https://fractal.medium.com/",
            icon: FaMediumM,
        },
        {
            name: "LinkedIn",
            href: "https://www.linkedin.com/company/fractal/",
            icon: FaLinkedinIn,
        },
        {
            name: "Twitter",
            href: "https://twitter.com/tryfractal",
            icon: FaTwitter,
        },
        {
            name: "Instagram",
            href: "https://www.instagram.com/tryfractal/",
            icon: FaInstagram,
        },
    ],
}

const Footer = () => {
    return (
        <footer className="bg-blue-darker" aria-labelledby="footer-heading">
            <h2 id="footer-heading" className="sr-only">
                Footer
            </h2>
            <div className="max-w-7xl mx-auto py-12 px-4 sm:px-6 lg:py-16 lg:px-8">
                <div className="xl:grid xl:grid-cols-3 xl:gap-8">
                    <div className="space-y-8 xl:col-span-1">
                        <img className="h-6" src={Logo} alt="" />
                        <p className="text-gray-500 text-base">
                            Fractal is a lighter, faster version of Google
                            Chrome that runs in the cloud.
                        </p>
                        <div className="flex space-x-6">
                            {navigation.social.map((item) => (
                                <a
                                    key={item.name}
                                    href={item.href}
                                    className="text-gray-400 hover:text-mint"
                                    target="_blank"
                                    rel="noreferrer"
                                >
                                    <span className="sr-only">{item.name}</span>
                                    <item.icon
                                        className="h-5 w-5"
                                        aria-hidden="true"
                                    />
                                </a>
                            ))}
                        </div>
                    </div>
                    <div className="mt-12 grid grid-cols-2 gap-8 xl:mt-0 xl:col-span-2">
                        <div className="md:grid md:grid-cols-2 md:gap-8">
                            <div>
                                <h3 className="text-sm font-semibold text-gray-400 tracking-wider uppercase">
                                    Home
                                </h3>
                                <ul className="mt-4 space-y-4">
                                    {navigation.home.map((item) => (
                                        <li key={item.name}>
                                            <a
                                                href={item.href}
                                                className="text-base text-gray-500 hover:text-mint"
                                            >
                                                {item.name}
                                            </a>
                                        </li>
                                    ))}
                                </ul>
                            </div>
                            <div className="mt-12 md:mt-0">
                                <h3 className="text-sm font-semibold text-gray-400 tracking-wider uppercase">
                                    About
                                </h3>
                                <ul className="mt-4 space-y-4">
                                    {navigation.about.map((item) => (
                                        <li key={item.name}>
                                            <a
                                                href={item.href}
                                                className="text-base text-gray-500 hover:text-mint"
                                            >
                                                {item.name}
                                            </a>
                                        </li>
                                    ))}
                                </ul>
                            </div>
                        </div>
                        <div className="md:grid md:grid-cols-2 md:gap-8">
                            <div>
                                <h3 className="text-sm font-semibold text-gray-400 tracking-wider uppercase">
                                    Resources
                                </h3>
                                <ul className="mt-4 space-y-4">
                                    {navigation.resources.map((item) => (
                                        <li key={item.name}>
                                            <a
                                                href={item.href}
                                                className="text-base text-gray-500 hover:text-mint"
                                            >
                                                {item.name}
                                            </a>
                                        </li>
                                    ))}
                                </ul>
                            </div>
                            <div className="mt-12 md:mt-0">
                                <h3 className="text-sm font-semibold text-gray-400 tracking-wider uppercase">
                                    Legal
                                </h3>
                                <ul className="mt-4 space-y-4">
                                    {navigation.legal.map((item) => (
                                        <li key={item.name}>
                                            <a
                                                href={item.href}
                                                className="text-base text-gray-500 hover:text-mint"
                                            >
                                                {item.name}
                                            </a>
                                        </li>
                                    ))}
                                </ul>
                            </div>
                        </div>
                    </div>
                </div>
                <div className="mt-12 border-t border-gray-600 pt-8">
                    <p className="text-base text-gray-400 xl:text-center">
                        &copy; 2021 Fractal Computers, Inc. All rights reserved.
                    </p>
                </div>
            </div>
        </footer>
    )
}

export default Footer
