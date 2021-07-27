import React from "react"
import { HashLink } from "react-router-hash-link"

/* This example requires Tailwind CSS v2.0+ */
const faqs = [
    {
        id: 1,
        question: "How does Fractal work?",
        answer: (
            <div>
                Fractal is like remote desktop, but with a much greater emphasis
                on performance and user experience. You can read more about our
                technology{" "}
                <HashLink
                    to="/technology#top"
                    className="text-mint hover:text-mint"
                >
                    here
                </HashLink>
                .
            </div>
        ),
    },
    {
        id: 2,
        question: "Can I trust Fractal with my data?",
        answer: (
            <div>
                We take care to encrypt and protect every piece of data that you
                input through Fractal, from your browsing data to your
                keystrokes. You can read more about how we handle privacy and
                security{" "}
                <HashLink
                    to="/security#top"
                    className="text-mint hover:text-mint"
                >
                    here
                </HashLink>
                .
            </div>
        ),
    },
    {
        id: 3,
        question: "What's the story behind Fractal?",
        answer: "Fractal was founded in 2019 out of Harvard University. Today, Fractal is based in New York City and recruits the best systems engineers to push the frontiers of browser streaming.",
    },
    {
        id: 4,
        question: "How is Fractal different than `X cloud streaming company?`",
        answer: "Because we care only about streaming Chrome, we aim to create the user experience of running a browser on native hardware.",
    },
    {
        id: 5,
        question: "Will there be latency?",
        answer: "Running applications in the cloud means that there will always be some latency, but there is a certain point at which latency becomes unnoticeable. We aim to reach that point.",
    },
    {
        id: 6,
        question: "Is Fractal hiring?",
        answer: (
            <div>
                Yes! You can see our job postings{" "}
                <a
                    href="https://www.notion.so/tryfractal/Fractal-Job-Board-a39b64712f094c7785f588053fc283a9"
                    className="text-mint hover:text-mint"
                    target="_blank"
                    rel="noreferrer"
                >
                    here
                </a>
                .
            </div>
        ),
    },
]

const Questions = () => {
    return (
        <div className="bg-gray-800 my-16">
            <div className="max-w-7xl mx-auto py-16 px-4 sm:py-24 sm:px-6 lg:px-8">
                <div className="lg:max-w-2xl lg:mx-auto lg:text-center">
                    <h2 className="text-3xl text-gray-300 sm:text-4xl">
                        Frequently asked questions
                    </h2>
                    <p className="mt-4 text-gray-500">
                        Have a question about Fractal? You might find some
                        answers here.
                    </p>
                </div>
                <div className="mt-20">
                    <dl className="space-y-10 lg:space-y-0 lg:grid lg:grid-cols-2 lg:gap-x-8 lg:gap-y-10">
                        {faqs.map((faq) => (
                            <div key={faq.id}>
                                <dt className="font-semibold text-gray-300">
                                    {faq.question}
                                </dt>
                                <dd className="mt-3 text-gray-500">
                                    {faq.answer}
                                </dd>
                            </div>
                        ))}
                    </dl>
                </div>
            </div>
        </div>
    )
}

export default Questions
