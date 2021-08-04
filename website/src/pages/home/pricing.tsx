import React from "react"

const Pricing = () => {
  return (
    <div className="bg-blue mt-24 rounded" id="pricing">
      <div className="max-w-7xl mx-auto py-16 px-4 sm:py-24 sm:px-6 lg:px-8">
        <div className="xl:flex xl:items-center xl:justify-between">
          <div>
            <h1 className="text-4xl font-semibold sm:text-5xl">
              <span className="text-gray-300">Unlimited access for </span>
              <span className="text-gray-100">$9 a month</span>
            </h1>
            <p className="mt-4 text-xl text-gray-300">
              Discounted for a limited time only. Pay now and lock in this price
              forever.
            </p>
          </div>
          <a
            href="/download"
            className="mt-8 w-full bg-indigo-900 px-5 py-3 inline-flex items-center justify-center text-base font-medium rounded-md text-gray-100 sm:mt-10 sm:w-auto xl:mt-0 hover:text-gray-100"
          >
            Download Now
          </a>
        </div>
      </div>
    </div>
  )
}

export default Pricing
