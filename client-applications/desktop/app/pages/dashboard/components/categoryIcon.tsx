import React from "react"
import { Tooltip, OverlayTrigger } from "react-bootstrap"

import styles from "styles/dashboard.css"

const CategoryIcon = (props: {
    callback: (category: string) => void
    selectedCategory: string
    category: string
    icon: any
}) => {
    const { category, icon, callback, selectedCategory } = props

    const renderTooltip = (props: any) => (
        <Tooltip id="button-tooltip" {...props}>
            <div
                style={{
                    fontFamily: "Maven Pro",
                    paddingLeft: 8,
                    paddingRight: 8,
                }}
            >
                {category} Apps
            </div>
        </Tooltip>
    )

    return (
        <div>
            <OverlayTrigger placement="right" overlay={renderTooltip}>
                <div
                    className={styles.categoryIcon}
                    style={{
                        boxShadow:
                            selectedCategory === category
                                ? "0px 4px 10px rgba(0,0,0,0.25)"
                                : "0px 4px 10px rgba(0,0,0,0.1)",
                    }}
                    onClick={() => callback(category)}
                >
                    {icon}
                </div>
            </OverlayTrigger>
        </div>
    )
}

export default CategoryIcon
