import React from 'react'
import { Link } from 'react-router-dom'

import PriceBox from './priceBox'
import { FractalPlan } from '@app/shared/types/payment'

// TODO: This should be stored in a database and pulled via GraphQL
// https://github.com/fractal/website/issues/337
export const PLANS: {
  [key: string]: {
    price: number
    name: FractalPlan
    subText: string
    details: string[]
  }
} = {
  Starter: {
    price: 50,
    name: FractalPlan.STARTER,
    subText: 'Additional 7-day free trial',
    details: ['Unlimited usage', 'Access to alpha features']
  },
  Pro: {
    price: 99,
    name: FractalPlan.PRO,
    subText: 'Coming Soon',
    details: ['Everything in Starter', 'Stream other browsers']
  }
}

export const Pricing = (props: { dark: boolean }) => {
  /*
        Pricing section of chrome product page

        Arguments:
            dark (boolean): True/false dark mode
    */

  const { dark } = props

  return (
        <div className="text-center mt-52 relative">
            <div className="flex justify-center text-5xl dark:text-gray-300">
                <div>
                    Try Fractal for{' '}
                    <span className="text-blue dark:text-mint">free</span>
                </div>
            </div>
            <div className="font-body dark:text-gray-400 tracking-wider mt-4">
                No credit card required, free for seven days.
            </div>
            <Link to="/auth">
                <button
                    type="button"
                    className="text-gray dark:text-gray-100 rounded bg-transparent dark:hover:bg-mint border border-black hover:border-mint px-12 py-3 my-10 duration-500 tracking-wide"
                >
                    TRY NOW
                </button>
            </Link>
            <div className="m-auto pt-8 flex flex-col md:flex-row md:justify-center space-y-8 md:space-y-0 md:space-x-8 max-w-screen-sm">
                <PriceBox
                    {...PLANS[FractalPlan.STARTER]}
                    background="#553de9"
                    textColor="white"
                />
                <PriceBox
                    {...PLANS[FractalPlan.PRO]}
                    background={dark ? '#00FFA2' : '#f6f9ff'}
                    textColor="black"
                />
            </div>
        </div>
  )
}

export default Pricing
