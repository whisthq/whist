import React from "react"
import { FaLinkedinIn, FaTwitter, FaDiscord } from "react-icons/fa"

import URLs from "@app/shared/constants/urls"

/* eslint-disable react/display-name */

const navigation = {
  home: [
    { name: "Product", href: "/" },
    { name: "Download", href: "/download#top" },
  ],
  about: [
    { name: "Company", href: "/about" },
    { name: "Technology", href: "/technology" },
    { name: "Security", href: "/security" },
    { name: "FAQ", href: "/faq" },
  ],
  resources: [
    { name: "Join our Discord", href: {URLs.DISCORD} },
    { name: "Blog", href: "https://whisthq.medium.com" },
    { name: "Support", href: "/contact" },
    {
      name: "Careers",
      href: "/contact#careers",
    },
  ],
  legal: [
    { name: "Privacy", href: "/privacy" },
    { name: "Terms", href: "/termsofservice" },
  ],
  social: [
    {
      name: "Discord",
      href: {URLs.DISCORD},
      icon: FaDiscord,
    },
    {
      name: "Twitter",
      href: {URLs.TWITTER},
      icon: FaTwitter,
    },
    {
      name: "LinkedIn",
      href: {URLs.LINKEDIN},
      icon: FaLinkedinIn,
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
            <img
              className="h-12 w-12 rounded relative right-3"
              src="https://fractal-brand-assets.s3.amazonaws.com/png/new+logo/FullColorShaded_blue.png"
              alt=""
            />
            <p className="text-gray-500 text-base">
              Whist is a lighter, faster version of Google Chrome that runs in
              the cloud.
            </p>
            <div className="flex space-x-6">
              {navigation.social.map((item) => (
                <a
                  key={item.name}
                  href={item.href}
                  className="text-gray-400 hover:text-blue-light"
                  target="_blank"
                  rel="noreferrer"
                >
                  <span className="sr-only">{item.name}</span>
                  <item.icon className="h-5 w-5" aria-hidden="true" />
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
                        className="text-base text-gray-500 hover:text-blue-light"
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
                        className="text-base text-gray-500 hover:text-blue-light"
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
                        className="text-base text-gray-500 hover:text-blue-light"
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
                        target="_blank"
                        href={item.href}
                        className="text-base text-gray-500 hover:text-blue-light"
                        rel="noreferrer"
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
        <div className="w-full text-center mt-4">
          <a href="/download#top">
            <button className="rounded bg-blue-light text-gray-900 px-12 py-3">
              Get Started
            </button>
          </a>
        </div>
        <div className="mt-12 border-t border-gray-600 pt-8">
          <p className="text-base text-gray-400 xl:text-center">
            &copy; 2022 Whist Technologies, Inc. All rights reserved.
          </p>
        </div>
      </div>
    </footer>
  )
}

export default Footer
