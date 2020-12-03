import React, { FC } from "react"
import { Tooltip, OverlayTrigger } from "react-bootstrap"

import styles from "pages/dashboard/components/categoryIcon/categoryIcon.css"

/* eslint-disable react/jsx-props-no-spreading */

const renderTooltip = <T,>(props: T) => (
    <Tooltip id="button-tooltip" {...props}>
        <div className={styles.tooltipText}>{category} Apps</div>
    </Tooltip>
)

const CategoryIcon = (props: {
    selectedCategory: string
    category: string
    icon: FC
    callback: (category: string) => void
}) => {
    const { category, icon, selectedCategory, callback } = props

    return (
        <div>
            <OverlayTrigger placement="right" overlay={renderTooltip}>
                <button
                    type="button"
                    className={
                        selectedCategory === category
                            ? styles.categoryIconSelected
                            : styles.categoryIcon
                    }
                    onClick={() => callback(category)}
                >
                    {icon}
                </button>
            </OverlayTrigger>
        </div>
    )
}

export default CategoryIcon
