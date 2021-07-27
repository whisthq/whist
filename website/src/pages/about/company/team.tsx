import React from "react"
import { HashLink } from "react-router-hash-link"

const people = [
    {
        name: "Philippe Noel",
        role: "Founder",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-UPTRX7MGA-51c2ecce0b3b-512",
        linkedinUrl: "https://www.linkedin.com/in/philippemnoel/",
    },
    {
        name: "Ming Ying",
        role: "Founder",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-UPTRXFK98-89ab30c74d35-192",
        linkedinUrl: "https://www.linkedin.com/in/ming-ying/",
    },
    {
        name: "Raviteja Guttula",
        role: "Backend Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U0243QG947L-7e40b38b0d88-512",
        linkedinUrl: "https://www.linkedin.com/in/thisisgvrt/",
    },
    {
        name: "Jannie Zhong",
        role: "Full Stack Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U01J380KFS8-0642358c2177-512",
        linkedinUrl: "https://www.linkedin.com/in/janniezhong/",
    },
    {
        name: "Neil Hansen",
        role: "Full Stack Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U01JS94Q8UF-eb6b84b7179a-192",
        linkedinUrl: "",
    },
    {
        name: "Leor Fishman",
        role: "Backend Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U01997UQJGK-1c4fa8641f69-192",
        linkedinUrl: "https://www.linkedin.com/in/leor-f-63490785/",
    },
    {
        name: "Suriya Kandaswamy",
        role: "Systems Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U018FBSHUC8-e07952d1d5e3-512",
        linkedinUrl: "https://www.linkedin.com/in/suriyak/",
    },
    {
        name: "Roshan Padaki",
        role: "Systems Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U01125TJ6EB-241dcde74e6b-192",
        linkedinUrl: "https://www.linkedin.com/in/roshan-padaki-261a7b132/",
    },
    {
        name: "Savvy Raghuvanshi",
        role: "Systems Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U019K89H3N2-ebcb96e31c54-192",
        linkedinUrl: "https://www.linkedin.com/in/savvy-raghuvanshi/",
    },
    {
        name: "Nicholas Pipitone",
        role: "Systems Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-USEKRDGH1-8672b16c3d84-512",
        linkedinUrl: "https://www.linkedin.com/in/nicholas-pipitone-5736a2107/",
    },
    {
        name: "Serina Hu",
        role: "Systems Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U022KK7URP0-c124ebf8b079-192",
        linkedinUrl: "https://www.linkedin.com/in/serina-h-710776102/",
    },
    {
        name: "Yoel Hawa",
        role: "Systems Engineer",
        imageUrl:
            "https://ca.slack-edge.com/TQ8RU2KE2-U024MGVNKF0-34ae41116f01-192",
        linkedinUrl: "https://www.linkedin.com/in/yoel-hawa/",
    },
].sort((a, b) => {
    const textA = a.name.toUpperCase()
    const textB = b.name.toUpperCase()
    return textA < textB ? -1 : textA > textB ? 1 : 0
})

export const Team = () => {
    return (
        <div className="mx-auto py-12 max-w-7xl lg:py-24">
            <div className="space-y-12">
                <div className="space-y-5 sm:space-y-4 md:max-w-xl lg:max-w-3xl xl:max-w-none m-auto text-center">
                    <div className="text-5xl text-gray-300 tracking-tight">
                        Meet our team
                    </div>
                    <p className="text-lg text-gray-500">
                        We’re engineers passionate about the future of personal
                        computing. To join our team, click{" "}
                        <HashLink
                            to="/contact#careers"
                            className="text-gray-300 hover:text-mint"
                        >
                            here
                        </HashLink>
                        .
                    </p>
                </div>
                <ul className="space-y-4 sm:grid sm:grid-cols-2 sm:gap-6 sm:space-y-0 lg:grid-cols-3 lg:gap-8">
                    {people.map((person) => (
                        <li
                            key={person.name}
                            className="py-10 px-6 bg-gray-900 text-center rounded-lg xl:px-10 xl:text-left"
                        >
                            <div className="space-y-6 xl:space-y-10">
                                <img
                                    className="mx-auto h-40 w-40 rounded-full xl:w-56 xl:h-56 filter grayscale"
                                    src={person.imageUrl}
                                    alt=""
                                />
                                <div className="space-y-2 xl:flex xl:items-center xl:justify-between">
                                    <div className="font-medium text-lg leading-6 space-y-1 text-left">
                                        <h3 className="text-gray-300">
                                            {person.name}
                                        </h3>
                                        <p className="text-gray-500">
                                            {person.role}
                                        </p>
                                    </div>
                                </div>
                            </div>
                        </li>
                    ))}
                    <li className="relative pt-64 pb-10 rounded overflow-hidden">
                        <HashLink to="/contact#careers">
                            <img
                                className="absolute inset-0 h-full w-full object-cover"
                                src="https://images.unsplash.com/photo-1521510895919-46920266ddb3?ixid=MXwxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHw%3D&ixlib=rb-1.2.1&auto=format&fit=crop&fp-x=0.5&fp-y=0.6&fp-z=3&width=1440&height=1440&sat=-100"
                                alt=""
                            />
                            <div className="absolute inset-0 bg-blue mix-blend-multiply" />
                            <div className="absolute inset-0 bg-gradient-to-t from-indigo-600 via-indigo-600 opacity-90" />
                            <div className="relative px-8">
                                <div className="text-gray-100 text-3xl">
                                    Join our team
                                </div>
                                <blockquote className="mt-2">
                                    <div className="relative text-md font-medium text-gray-300 md:flex-grow">
                                        <p className="relative text-sm">
                                            If you see yourself working with us,
                                            you can apply here.
                                        </p>
                                    </div>
                                </blockquote>
                            </div>
                        </HashLink>
                    </li>
                </ul>
            </div>
        </div>
    )
}

export default Team
