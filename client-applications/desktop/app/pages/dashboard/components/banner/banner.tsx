import React from "react"
import { Carousel } from "react-bootstrap"
import { connect } from "react-redux"

import { openExternal } from "shared/utils/helpers"
import { FractalBanner } from "shared/types/ui"

import styles from "pages/dashboard/components/banner/banner.css"

const Banner = (props: { bannerData: FractalBanner[] }) => {
    const { bannerData } = props

    if (bannerData && bannerData.length > 0) {
        return (
            <Carousel
                className={styles.carousel}
                prevIcon={<div></div>}
                nextIcon={<div></div>}
                indicators={false}
            >
                {bannerData.map((bannerItem: FractalBanner) => (
                    <Carousel.Item
                        key={bannerItem.background}
                        onClick={() => openExternal(bannerItem.url)}
                    >
                        <div
                            style={{
                                backgroundImage: `url(${bannerItem.background})`,
                            }}
                            className={styles.bannerImage}
                        ></div>
                    </Carousel.Item>
                ))}
            </Carousel>
        )
    } else {
        return <div className={styles.emptyBanner}></div>
    }
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        os: state.MainReducer.client.os,
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Banner)
