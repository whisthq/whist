import React from "react"
import { Tooltip, OverlayTrigger } from "react-bootstrap"

import styles from "pages/dashboard/components/categoryIcon/categoryIcon.css"

const CategoryIcon = (props: {
    callback: (category: string) => void
    selectedCategory: string
    category: string
    icon: any
}) => {
    const { category, icon, callback, selectedCategory } = props

    const renderTooltip = (props: any) => (
        <Tooltip id="button-tooltip" {...props}>
            <div className={styles.tooltipText}>{category} Apps</div>
        </Tooltip>
    )

    return (
        <div>
            <OverlayTrigger placement="right" overlay={renderTooltip}>
                <div
                    className={
                        selectedCategory === category
                            ? styles.categoryIconSelected
                            : styles.categoryIcon
                    }
                    onClick={() => callback(category)}
                >
                    {icon}
                </div>
            </OverlayTrigger>
        </div>
    )
}

export default CategoryIcon
