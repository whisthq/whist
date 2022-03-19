import React from "react"

import URLs from "@app/shared/constants/urls"

const Careers = () => (
  <>
    <div
      className="max-w-7xl mx-auto bg-blue lg:bg-transparent lg:px-8 mt-36 mb-36 pt-12 relative bottom-12"
      id="careers"
    >
      <div className="lg:grid lg:grid-cols-12">
        <div className="relative z-10 lg:col-start-1 lg:row-start-1 lg:col-span-4 lg:py-8 lg:bg-transparent">
          <div className="max-w-md mx-auto px-4 sm:max-w-3xl sm:px-6 lg:max-w-none lg:p-0">
            <div className="aspect-w-10 aspect-h-6 sm:aspect-w-2 sm:aspect-h-1 lg:aspect-w-1">
              <img
                className="object-cover object-center rounded-3xl"
                src="https://images.unsplash.com/photo-1507207611509-ec012433ff52?ixlib=rb-1.2.1&ixid=MXwxMjA3fDB8MHxwaG90by1wYWdlfHx8fGVufDB8fHw%3D&auto=format&fit=crop&w=934&q=80"
                alt=""
              />
            </div>
          </div>
        </div>

        <div className="relative bg-blue lg:col-start-3 lg:row-start-1 lg:col-span-10 lg:rounded-3xl lg:grid lg:grid-cols-10 lg:items-center">
          <div className="relative max-w-md mx-auto py-12 px-4 space-y-6 sm:max-w-3xl sm:py-16 sm:px-6 lg:max-w-none lg:p-0 lg:col-start-4 lg:col-span-6">
            <h2
              className="text-3xl text-gray-900 font-semibold"
              id="join-heading"
            >
              Join our team
            </h2>
            <p className="text-lg text-gray-800">
              If working on a product that pushes the boundaries of networking,
              virtualization, or browser OSes excites you, you can apply to work
              with us!
            </p>
            <a
              href=URLs.NOTION_CAREERS
              target="_blank"
              rel="noreferrer"
            >
              <button className="mt-8 block w-full py-3 px-5 text-center rounded text-gray-900 sm:inline-block sm:w-auto bg-blue-light">
                Explore open positions
              </button>
            </a>
          </div>
        </div>
      </div>
    </div>
  </>
)

export default Careers
