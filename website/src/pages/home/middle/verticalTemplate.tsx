import React from "react"
import { Row, Col } from "react-bootstrap"

import Geometric from "@app/pages/home/top/geometric"
import { withContext } from "@app/shared/utils/context"
import { ScreenSize } from "@app/shared/constants/screenSizes"

export const VerticalTemplate = (props: {
    visible: boolean
    title: JSX.Element
    text: JSX.Element
    image: JSX.Element
    background?: boolean
}) => {
    /*
        Template for arranging text and images vertically

        Arguments:
            visible (boolean): Should the image be visible
            title (Element): Title
            text (Element): Text under title
            image (Element): Image under text
            background (boolean): Should the animated background be visible
    */

    const { width } = withContext()
    const { title, text, image, visible } = props

    return (
        <div className="mt-24 md:mt-52">
            {props.background !== undefined && width > ScreenSize.MEDIUM && (
                <>
                    <div className="relative">
                        <div
                            style={{
                                position: "absolute",
                                top: 400,
                                left: 425,
                            }}
                        >
                            <Geometric scale={3} flip={false} />
                        </div>
                    </div>
                    <div className="relative">
                        <div
                            style={{
                                position: "absolute",
                                top: 400,
                                right: -840,
                            }}
                        >
                            <Geometric scale={3} flip={true} />
                        </div>
                    </div>
                </>
            )}
            <Row>
                <Col md={12} className="text-center">
                    <div className="text-gray dark:text-gray-300 text-4xl md:text-6xl mb-8 leading-relaxed">
                        {title}
                    </div>
                    <div className="max-w-screen-sm m-auto text-gray dark:text-gray-400 md:text-lg tracking-wider">
                        {text}
                    </div>
                </Col>
                <Col md={12} className="text-center mt-4">
                    {visible && image}
                </Col>
            </Row>
        </div>
    )
}

export default VerticalTemplate
