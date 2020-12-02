import React from "react"
import { Col, Carousel } from "react-bootstrap"
import { connect } from "react-redux"

import { openExternal } from "shared/utils/helpers"
import { FractalBanner } from "shared/types/ui"

import styles from "pages/dashboard/components/news/news.css"

const News = (props: { mediaData: FractalBanner[] }) => {
    const { mediaData } = props

    if (mediaData && mediaData.length > 0) {
        return (
            <Col xs={5} className={styles.newsWrapper}>
                <div className={styles.carouselWrapper}>
                    <Carousel
                        className={styles.carousel}
                        prevIcon={<div></div>}
                        nextIcon={<div></div>}
                        indicators={false}
                    >
                        {mediaData.map((mediaItem: FractalBanner) => (
                            <Carousel.Item
                                key={mediaItem.background}
                                onClick={() => openExternal(mediaItem.url)}
                            >
                                <div className={styles.scrollWrapper}>
                                    <div
                                        style={{
                                            backgroundImage: `url(${mediaItem.background})`,
                                        }}
                                        className={styles.mediaImage}
                                    ></div>
                                    <div className={styles.textWrapper}>
                                        <div className={styles.heading}>
                                            Media
                                        </div>
                                        <div>
                                            {mediaItem.heading}
                                            <div className={styles.subheading}>
                                                {mediaItem.subheading}
                                            </div>
                                        </div>
                                    </div>
                                </div>
                            </Carousel.Item>
                        ))}
                    </Carousel>
                </div>
            </Col>
        )
    } else {
        return <Col xs={4} className={styles.emptyNews}></Col>
    }
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {}
}

export default connect(mapStateToProps)(News)
