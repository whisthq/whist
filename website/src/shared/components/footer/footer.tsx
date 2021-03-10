import React from "react"
import { HashLink } from "react-router-hash-link"
import {
    MediumIcon,
    LinkedinIcon,
    InstagramIcon,
    TwitterIcon
} from "shared/components/icons"
import {
    AboutLink,
    SupportLink,
    CareersLink,
    SalesLink,
    BlogLink,
    DiscordLink,
    WordmarkLink
} from "shared/components/links"

const FooterLinkList = ({
    title,
    children,
}: {
    title: string
    children: any
}) => (
    <div className="text-left md:text-right">
        {title ? <div className="font-bold mb-2 text-base">{title}</div> : null}
        {children({
            className:
                "font-body hover:text-green-400 hover:outline-none text-sm block border-none no-underline hover:no-underline",
        })}
    </div>
)

const FooterIconList = ({ children }: { children: any }) => (
    <div className="flex text-left space-x-4">
        {children({
            className: "p-2.5 rounded-sm border hover:text-green-400",
            target: "_blank",
            rel: "noopener noreferrer",
        })}
    </div>
)


const Footer = () => {
    return (
        <div className="mx-14 my-4 dark:bg-blue-darkest space-y-8 text-gray-700 dark:text-white">
            <div className="flex flex-col md:flex-row w-full space-y-12 max-w-screen-2xl justify-between">
                <div className="flex-col max-w-sm space-y-6">
                    <WordmarkLink />
                    <div className="font-body text-sm font-light leading-relaxed tracking-widest">
                        Fractal supercharges your applications by streaming them
                        from the cloud.
                    </div>
                    <FooterIconList>
                        {(props: any) => (
                            <>
                                <TwitterIcon {...props} />
                                <MediumIcon {...props} />
                                <LinkedinIcon {...props} />
                                <InstagramIcon {...props} />
                            </>
                        )}
                    </FooterIconList>
                </div>
                <div className="font-body flex space-x-32 text-left pt-0 md:pt-3">
                    <FooterLinkList title="RESOURCES">
                        {(props: any) => (
                            <>
                                <AboutLink {...props} />
                                <BlogLink {...props} />
                                <DiscordLink {...props} />
                            </>
                        )}
                    </FooterLinkList>
                    <FooterLinkList title="CONTACT">
                        {(props: any) => (
                            <>
                                <SalesLink {...props} />
                                <SupportLink {...props} />
                                <CareersLink {...props} />
                            </>
                        )}
                    </FooterLinkList>
                </div>
            </div>
            <div className=" max-w-screen-2xl flex justify-between w-full text-sm mt-1.5">
                <div className="font-body m-0 text-gray-400 overflow-hidden text-xs md:text-sm">
                    &copy; 2021 Fractal Computers, Inc. All Rights Reserved.
                </div>
                <div className="font-body hidden md:block text-gray-400">
                    <HashLink id="tosPage" to="/termsofservice#top">
                        Terms of Service
                    </HashLink>{" "}
                    &amp;{" "}
                    <HashLink id="privacyPage" to="/privacy#top">
                        Privacy Policy
                    </HashLink>
                </div>
            </div>
        </div>
    )
}

export default Footer
