import React from "react"
import { Col } from "react-bootstrap"

export const FeatureBox = (props: {
    icon: JSX.Element
    title: string
    text: string
}) => {
    /*
        Component for a feature box

        Arguments:
            icon (JSX.Element): Icon component
            title (string): Feature title
            text (string): Feature text
    */

    const { icon, title, text } = props

    return (
        <Col md={4} className="text-left mb-4">
            <div className="h-full px-12 py-12 bg-blue-lightest border border-transparent dark:border-gray-400 dark:bg-transparent rounded">
                {" "}
                {icon}
                <div className="text-gray dark:text-gray-300 text-lg mt-8">
                    {title}
                </div>
                <div className="mt-2 text-base text-gray-500">{text}</div>
            </div>
        </Col>
    )
}

export default FeatureBox
