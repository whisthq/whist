import React from "react"
import { Col } from "react-bootstrap"
import { FaGlobeEurope, FaBook, FaPalette, FaLayerGroup } from "react-icons/fa"

import CategoryIcon from "pages/dashboard/components/categoryIcon/categoryIcon"
import { FractalAppCategory } from "shared/types/navigation"

import styles from "pages/dashboard/components/leftColumn/leftColumn.css"

const LeftColumn = (props: {
    callback: (category: string) => void
    selectedCategory: string
}) => {
    const { callback, selectedCategory } = props

    return (
        <>
            <Col xs={1}>
                <div style={{ position: "sticky", top: 25 }}>
                    <CategoryIcon
                        icon={
                            <FaLayerGroup
                                className={
                                    selectedCategory === FractalAppCategory.ALL
                                        ? styles.faIconSelected
                                        : styles.faIcon
                                }
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.ALL}
                    />
                    <CategoryIcon
                        icon={
                            <FaGlobeEurope
                                className={
                                    selectedCategory ===
                                    FractalAppCategory.BROWSER
                                        ? styles.faIconSelected
                                        : styles.faIcon
                                }
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.BROWSER}
                    />
                    <CategoryIcon
                        icon={
                            <FaPalette
                                className={
                                    selectedCategory ===
                                    FractalAppCategory.CREATIVE
                                        ? styles.faIconSelected
                                        : styles.faIcon
                                }
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.CREATIVE}
                    />
                    <CategoryIcon
                        icon={
                            <FaBook
                                className={
                                    selectedCategory ===
                                    FractalAppCategory.PRODUCTIVITY
                                        ? styles.faIconSelected
                                        : styles.faIcon
                                }
                            />
                        }
                        callback={callback}
                        selectedCategory={selectedCategory}
                        category={FractalAppCategory.PRODUCTIVITY}
                    />
                </div>
            </Col>
        </>
    )
}

export default LeftColumn
