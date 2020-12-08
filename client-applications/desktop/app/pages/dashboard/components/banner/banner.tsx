import React from "react"
import { Carousel } from "react-bootstrap"
import { connect } from "react-redux"

import { openExternal } from "shared/utils/general/helpers"
import { FractalBanner } from "shared/types/ui"

import styles from "pages/dashboard/components/banner/banner.css"

const Banner = (props: { bannerData: FractalBanner[] }) => {
    const { bannerData } = props

    if (bannerData && bannerData.length > 0) {
        return (
            <Carousel
                className={styles.carousel}
                prevIcon={<div />}
                nextIcon={<div />}
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
                        />
                    </Carousel.Item>
                ))}
            </Carousel>
        )
    }
    return <div className={styles.emptyBanner} />
}

const mapStateToProps = <T extends {}>(state: T): T => {
    return {
        clientOS: state.MainReducer.client.clientOS,
        accessToken: state.MainReducer.auth.accessToken,
    }
}

export default connect(mapStateToProps)(Banner)
