import React from "react"

import team from "@app/assets/company/team.json"

export const Team = () => {
  return (
    <div className="mx-auto py-12 max-w-7xl lg:py-24">
      <div className="space-y-12">
        <div className="space-y-5 sm:space-y-4 md:max-w-xl lg:max-w-3xl xl:max-w-none m-auto text-center">
          <div className="text-5xl text-gray-300 tracking-tight">
            Meet our team
          </div>
          <p className="text-lg text-gray-500">
            We’re engineers passionate about the future of personal computing.
            To join our team, click{" "}
            <a
              href="/contact#careers"
              className="text-gray-300 hover:text-blue-light"
            >
              here
            </a>
            .
          </p>
        </div>
        <ul className="space-y-4 sm:grid sm:grid-cols-2 sm:gap-6 sm:space-y-0 lg:grid-cols-3 lg:gap-8">
          {team?.map(
            (person: { name: string; imagePath: string; title: string }) => (
              <li
                key={person.name}
                className="py-10 px-6 bg-gray-800 text-center rounded-lg xl:px-10 xl:text-left"
              >
                <div className="space-y-6 xl:space-y-10">
                  <div
                    className="mx-auto h-40 w-40 rounded-full xl:w-56 xl:h-56 bg-cover bg-center"
                    style={{
                      backgroundImage: `url(${person.imagePath})`,
                    }}
                  ></div>
                  <div className="space-y-2 xl:flex xl:items-center xl:justify-between mt-12">
                    <div className="font-medium leading-6 space-y-1 text-left">
                      <h3 className="text-gray-300">{person.name}</h3>
                      <p className="text-gray-500">{person.title}</p>
                    </div>
                  </div>
                </div>
              </li>
            )
          )}
          <li className="relative pt-64 pb-10 rounded overflow-hidden">
            <a href="/contact#careers">
              <img
                className="absolute inset-0 h-full w-full object-cover"
                src="https://images.unsplash.com/photo-1521510895919-46920266ddb3?ixid=MXwxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHw%3D&ixlib=rb-1.2.1&auto=format&fit=crop&fp-x=0.5&fp-y=0.6&fp-z=3&width=1440&height=1440&sat=-100"
                alt=""
              />
              <div className="absolute inset-0 bg-blue mix-blend-multiply" />
              <div className="absolute inset-0 bg-gradient-to-t from-indigo-600 via-indigo-600 opacity-90" />
              <div className="relative px-8">
                <div className="text-gray-100 text-3xl">Join our team</div>
                <blockquote className="mt-2">
                  <div className="relative text-md font-medium text-gray-300 md:flex-grow">
                    <p className="relative text-sm">
                      Want to build the future of the browser? Join our team
                      here!
                    </p>
                  </div>
                </blockquote>
              </div>
            </a>
          </li>
        </ul>
      </div>
    </div>
  )
}

export default Team
