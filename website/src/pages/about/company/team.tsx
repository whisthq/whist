import React from "react"

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
    const textA = a.name.toUpperCase();
    const textB = b.name.toUpperCase();
    return (textA < textB) ? -1 : (textA > textB) ? 1 : 0;
})

export const Team = () => {
    return (
        <div className="mx-auto py-12 max-w-7xl lg:py-24">
            <div className="space-y-12">
                <div className="space-y-5 sm:space-y-4 md:max-w-xl lg:max-w-3xl xl:max-w-none text-center">
                    <div className="text-5xl text-gray-300 tracking-tight">
                        Meet our team
                    </div>
                    <p className="text-lg text-gray-500">
                        We’re engineers passionate about the future of personal
                        computing.
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
                                    className="mx-auto h-40 w-40 rounded-full xl:w-56 xl:h-56"
                                    src={person.imageUrl}
                                    alt=""
                                />
                                <div className="space-y-2 xl:flex xl:items-center xl:justify-between">
                                    <div className="font-medium text-lg leading-6 space-y-1 text-left">
                                        <h3 className="text-white">
                                            {person.name}
                                        </h3>
                                        <p className="text-indigo-500">
                                            {person.role}
                                        </p>
                                    </div>

                                    <ul className="flex justify-center space-x-5">
                                        {person.linkedinUrl !== "" && (
                                            <li>
                                                <a
                                                    href={person.linkedinUrl}
                                                    className="text-gray-400 hover:text-gray-300"
                                                >
                                                    <span className="sr-only">
                                                        LinkedIn
                                                    </span>
                                                    <svg
                                                        className="w-5 h-5"
                                                        aria-hidden="true"
                                                        fill="currentColor"
                                                        viewBox="0 0 20 20"
                                                    >
                                                        <path
                                                            fillRule="evenodd"
                                                            d="M16.338 16.338H13.67V12.16c0-.995-.017-2.277-1.387-2.277-1.39 0-1.601 1.086-1.601 2.207v4.248H8.014v-8.59h2.559v1.174h.037c.356-.675 1.227-1.387 2.526-1.387 2.703 0 3.203 1.778 3.203 4.092v4.711zM5.005 6.575a1.548 1.548 0 11-.003-3.096 1.548 1.548 0 01.003 3.096zm-1.337 9.763H6.34v-8.59H3.667v8.59zM17.668 1H2.328C1.595 1 1 1.581 1 2.298v15.403C1 18.418 1.595 19 2.328 19h15.34c.734 0 1.332-.582 1.332-1.299V2.298C19 1.581 18.402 1 17.668 1z"
                                                            clipRule="evenodd"
                                                        />
                                                    </svg>
                                                </a>
                                            </li>
                                        )}
                                    </ul>
                                </div>
                            </div>
                        </li>
                    ))}
                </ul>
            </div>
        </div>
    )
}

export default Team
