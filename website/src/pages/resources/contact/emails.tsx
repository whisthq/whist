import React from "react"
import { NewspaperIcon, PhoneIcon, SupportIcon } from "@heroicons/react/outline"

const supportLinks = [
    {
        name: "Sales",
        href: "#",
        description:
            "For any business-related inquires including enterprise plans.",
        icon: PhoneIcon,
    },
    {
        name: "Technical Support",
        href: "#",
        description:
            "For bug reports, feature suggestions, and anything product-related.",
        icon: SupportIcon,
    },
    {
        name: "Media Inquiries",
        href: "#",
        description: "For questions about press, partnerships, etc.",
        icon: NewspaperIcon,
    },
]

const Emails = () => (
    <>
        <div className="relative mt-24 max-w-md mx-auto px-4 pb-64 sm:max-w-3xl sm:px-6 md:mt-32 lg:max-w-7xl lg:px-8 text-center">
            <h1 className="text-4xl text-gray-300 md:text-5xl lg:text-6xl mb-6">
                Support
            </h1>
            <p className="m-auto max-w-3xl text-xl text-gray-500">
                Our support team is available over email using the links below.
                For faster responses, you may also join our Discord.
            </p>
        </div>
        <div>
            <section
                className="-mt-48 max-w-md mx-auto relative z-10 px-4 sm:max-w-3xl sm:px-6 lg:max-w-7xl lg:px-8"
                aria-labelledby="contact-heading"
            >
                <h2 className="sr-only" id="contact-heading">
                    Contact us
                </h2>
                <div className="grid grid-cols-1 gap-y-20 lg:grid-cols-3 lg:gap-y-0 lg:gap-x-8">
                    {supportLinks.map((link) => (
                        <div
                            key={link.name}
                            className="flex flex-col rounded bg-gray-900"
                        >
                            <div className="flex-1 relative pt-16 px-6 pb-8 md:px-8">
                                <div className="top-0 p-3 inline-block bg-blue rounded transform -translate-y-1/2">
                                    <link.icon
                                        className="h-6 w-6 text-white"
                                        aria-hidden="true"
                                    />
                                </div>
                                <h3 className="text-xl font-medium text-gray-300">
                                    {link.name}
                                </h3>
                                <p className="mt-4 text-base text-gray-500">
                                    {link.description}
                                </p>
                            </div>
                            <div className="p-6 bg-blue-gray-50 rounded-bl-2xl rounded-br-2xl md:px-8">
                                <a
                                    href={link.href}
                                    className="text-base font-medium text-gray-300 hover:text-mint"
                                >
                                    Contact us
                                    <span aria-hidden="true"> &rarr;</span>
                                </a>
                            </div>
                        </div>
                    ))}
                </div>
            </section>
        </div>
    </>
)

export default Emails
