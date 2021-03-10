import React from "react"
import { Col } from "react-bootstrap"
import { FaCheck } from "react-icons/fa"
import classNames from "classnames"

export const PriceBox = (props: {
    name: string
    subText: string
    price: number
    details: string[]
    checked: boolean
    disabled?: boolean
}) => {
    /*
        Component that shows a plan tier as a checkable box
 
        Arguments:
            name (string): e.g. Starter, Pro
            subText (string): e.g. 7 day free trial
            price (number): Price in dollars
            details (string[]): e.g. ["Unlimited access", "24/7 support"]
            checked (boolean): e.g. Whether this plan is clicked/selected
            disabled (boolean): e.g. Whether this plan is clickable
    */

    const { name, subText, price, details, checked, disabled } = props

    return (
        <Col className="p-0 pt-4">
            <div
                className={
                    !checked
                        ? "px-10 py-8 mb-6 bg-blue-lightest w-full h-full rounded"
                        : "px-10 py-8 mb-6 bg-blue w-full h-full rounded text-white"
                }
                style={{
                    cursor: !disabled ? "pointer" : "normal",
                }}
            >
                <div className="font-medium text-2xl mb-1">{name}</div>
                <div
                    className={classNames(
                        checked ? "text-white text-sm" : "text-gray text-sm",
                        "font-body"
                    )}
                >
                    {subText}
                </div>
                <div
                    className="flex mt-3 mb-6"
                    style={{ color: checked ? "white" : "#111111" }}
                >
                    <div className="relative top-2 mr-2 text-sm">$</div>
                    <div className="text-6xl font-medium inline relative">
                        {price}
                    </div>
                    <div className="relative top-9 ml-2 text-sm">/ mo</div>
                </div>
                {details &&
                    details.map((detail: string) => (
                        <div className="flex mb-2" key={detail}>
                            <FaCheck
                                className={classNames(
                                    !checked
                                        ? "text-sm mr-3 text-gray"
                                        : "text-sm mr-3 text-white",
                                    "font-body"
                                )}
                            />
                            <div
                                className={classNames(
                                    !checked
                                        ? "text-sm text-gray relative bottom-1"
                                        : "text-sm text-white relative bottom-1",
                                    "font-body"
                                )}
                            >
                                {detail}
                            </div>
                        </div>
                    ))}
            </div>
        </Col>
    )
}

export default PriceBox
