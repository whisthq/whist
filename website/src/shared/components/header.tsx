/* This example requires Tailwind CSS v2.0+ */
import React, { Fragment } from "react"
import { Popover, Transition } from "@headlessui/react"
import {
  ChartBarIcon,
  CursorClickIcon,
  MenuIcon,
  ShieldCheckIcon,
  ViewGridIcon,
  XIcon,
  ChevronDownIcon,
} from "@heroicons/react/outline"

const about = [
  {
    name: "Company",
    description: "Fractal's company mission, team, and investors.",
    href: "/about",
    icon: ChartBarIcon,
  },
  {
    name: "Technology",
    description: "How Fractal's proprietary technology works.",
    href: "/technology",
    icon: CursorClickIcon,
  },
  {
    name: "Security",
    description: "How Fractal protects your browsing data.",
    href: "/security",
    icon: ShieldCheckIcon,
  },
  {
    name: "FAQ",
    description: "Answers to commonly-asked questions.",
    href: "/faq",
    icon: ViewGridIcon,
  },
]
const resources = [
  {
    name: "Support",
    description: "Contact our support team with any questions.",
    href: "/contact",
  },
  {
    name: "Careers",
    description: "We're looking to recruit brilliant, motivated engineers.",
    href: "/contact#careers",
  },
  {
    name: "Release Log",
    description:
      "See what features are coming next and what we've released so far.",
    href: "https://tryfractal.notion.site/Fractal-Release-Log-c7ea1639eb734d90bd48c34924d72f0a",
    target: "_blank",
  },
]

function classNames(...classes: any[]) {
  return classes.filter(Boolean).join(" ")
}

const Header = () => {
  return (
    <Popover className="relative z-50">
      {({ open }) => (
        <>
          <div className="flex justify-between items-center px-4 py-6 sm:px-6 md:justify-start md:space-x-10">
            <div className="flex justify-start lg:w-0 lg:flex-1">
              <a href="/">
                <img
                  className="h-9 w-9 relative md:bottom-1 rounded"
                  src="https://fractal-brand-assets.s3.amazonaws.com/png/icon+logo/icon_color2_borderless.png"
                  alt=""
                />
              </a>
            </div>
            <div className="-mr-2 -my-2 md:hidden">
              <Popover.Button className="rounded-md p-2 inline-flex items-center justify-center text-gray-300 focus:outline-none">
                <span className="sr-only">Open menu</span>
                <MenuIcon className="h-6 w-6" aria-hidden="true" />
              </Popover.Button>
            </div>
            <Popover.Group as="nav" className="hidden md:flex space-x-10">
              <Popover className="relative">
                {({ open }) => (
                  <>
                    <Popover.Button
                      className={classNames(
                        open ? "text-mint" : "text-gray-300",
                        "group rounded-md inline-flex items-center text-base font-medium hover:text-mint"
                      )}
                    >
                      <span>About</span>
                      <ChevronDownIcon className="h-5 w-4 ml-2 mb-0.5" />
                    </Popover.Button>

                    <Transition
                      show={open}
                      as={Fragment}
                      enter="transition ease-out duration-200"
                      enterFrom="opacity-0 translate-y-1"
                      enterTo="opacity-100 translate-y-0"
                      leave="transition ease-in duration-150"
                      leaveFrom="opacity-100 translate-y-0"
                      leaveTo="opacity-0 translate-y-1"
                    >
                      <Popover.Panel
                        static
                        className="absolute z-10 -ml-4 mt-3 transform w-screen max-w-md lg:max-w-2xl lg:ml-0 lg:left-1/2 lg:-translate-x-1/2"
                      >
                        <div className="rounded-lg shadow-lg ring-1 ring-black ring-opacity-5 overflow-hidden">
                          <div className="relative grid gap-6 bg-white px-5 py-6 sm:gap-8 sm:p-8 lg:grid-cols-2">
                            {about.map((item) => (
                              <a
                                key={item.name}
                                href={item.href}
                                className="-m-3 p-3 flex items-start rounded-lg hover:bg-gray-50"
                              >
                                <div className="flex-shrink-0 flex items-center justify-center h-10 w-10 rounded-md bg-indigo-500 text-white sm:h-12 sm:w-12">
                                  <item.icon
                                    className="h-6 w-6"
                                    aria-hidden="true"
                                  />
                                </div>
                                <div className="ml-4">
                                  <p className="text-base font-medium text-gray-900">
                                    {item.name}
                                  </p>
                                  <p className="-mt-2 text-sm text-gray-500">
                                    {item.description}
                                  </p>
                                </div>
                              </a>
                            ))}
                          </div>
                        </div>
                      </Popover.Panel>
                    </Transition>
                  </>
                )}
              </Popover>
              <Popover className="relative">
                {({ open }) => (
                  <>
                    <Popover.Button
                      className={classNames(
                        open ? "text-mint" : "text-gray-300",
                        "group rounded-md inline-flex items-center text-base font-medium hover:text-mint focus:outline-none"
                      )}
                    >
                      <span>Resources</span>
                      <ChevronDownIcon className="h-5 w-4 ml-2 mb-0.5" />
                    </Popover.Button>

                    <Transition
                      show={open}
                      as={Fragment}
                      enter="transition ease-out duration-200"
                      enterFrom="opacity-0 translate-y-1"
                      enterTo="opacity-100 translate-y-0"
                      leave="transition ease-in duration-150"
                      leaveFrom="opacity-100 translate-y-0"
                      leaveTo="opacity-0 translate-y-1"
                    >
                      <Popover.Panel
                        static
                        className="absolute z-10 left-1/2 transform -translate-x-1/2 mt-3 px-2 w-screen max-w-xs sm:px-0"
                      >
                        <div className="rounded-lg shadow-lg ring-1 ring-black ring-opacity-5 overflow-hidden">
                          <div className="relative grid gap-6 bg-white px-5 py-6 sm:gap-8 sm:p-8">
                            {resources.map((item) => (
                              <a
                                key={item.name}
                                href={item.href}
                                className="-m-3 p-3 block rounded-md hover:bg-gray-50"
                                target={item?.target ?? "_self"}
                                rel="noreferrer"
                              >
                                <p className="text-base font-medium text-gray-900">
                                  {item.name}
                                </p>
                                <p className="mt-1 text-sm text-gray-500">
                                  {item.description}
                                </p>
                              </a>
                            ))}
                          </div>
                        </div>
                      </Popover.Panel>
                    </Transition>
                  </>
                )}
              </Popover>
              <a href="/download#top" className="hover:no-underline">
                <span className="text-base font-medium text-gray-300 hover:text-mint no-underline cursor-pointer">
                  Download
                </span>
              </a>
              <a
                href="https://discord.com/invite/HjPpDGvEeA"
                className="hover:no-underline"
              >
                <span className="text-base font-medium text-gray-300 hover:text-mint no-underline cursor-pointer">
                  Join our Discord
                </span>
              </a>
            </Popover.Group>
            <div className="hidden md:flex items-center justify-end md:flex-1 lg:w-0"></div>
          </div>

          <Transition
            show={open}
            as={Fragment}
            enter="duration-200 ease-out"
            enterFrom="opacity-0 scale-95"
            enterTo="opacity-100 scale-100"
            leave="duration-100 ease-in"
            leaveFrom="opacity-100 scale-100"
            leaveTo="opacity-0 scale-95"
          >
            <Popover.Panel
              focus
              static
              className="absolute top-0 inset-x-0 p-2 transition transform origin-top-right md:hidden"
            >
              <div className="rounded-lg shadow-lg ring-1 ring-black ring-opacity-5 bg-white divide-y-2 divide-gray-50">
                <div className="pt-5 pb-6 px-5">
                  <div className="flex items-center justify-between">
                    <div></div>
                    <div className="-mr-2">
                      <Popover.Button className="bg-white rounded-md p-2 inline-flex items-center justify-center text-gray-600 focus:outline-none">
                        <span className="sr-only">Close menu</span>
                        <XIcon className="h-6 w-6" aria-hidden="true" />
                      </Popover.Button>
                    </div>
                  </div>
                  <div className="mt-6">
                    <nav className="grid grid-cols-1 gap-7">
                      {about.map((item) => (
                        <a
                          key={item.name}
                          href={item.href}
                          className="-m-3 p-3 flex items-center rounded-lg hover:bg-gray-50"
                        >
                          <div className="flex-shrink-0 flex items-center justify-center h-10 w-10 rounded-md bg-indigo-500 text-white">
                            <item.icon className="h-6 w-6" aria-hidden="true" />
                          </div>
                          <div className="ml-4 text-base font-medium text-gray-900">
                            {item.name}
                          </div>
                        </a>
                      ))}
                    </nav>
                  </div>
                </div>
                <div className="py-6 px-5">
                  <div className="grid grid-cols-2 gap-4">
                    {resources.map((item) => (
                      <a
                        key={item.name}
                        href={item.href}
                        className="text-base font-medium text-gray-900 hover:text-gray-700"
                      >
                        {item.name}
                      </a>
                    ))}
                  </div>
                </div>
              </div>
            </Popover.Panel>
          </Transition>
        </>
      )}
    </Popover>
  )
}

export default Header
