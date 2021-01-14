import React from "react"
import { connect } from "react-redux"

import TitleBar from "shared/components/titleBar"
import Version from "shared/components/version"
import BackgroundView from "shared/views/backgroundView"
import PaymentView from "pages/payment/views/paymentView"

import styles from "pages/login/login.css"

const Payment = () => {
    return (
        <div className={styles.container} data-tid="container">
            <TitleBar />
            <Version />
            <div className={styles.removeDrag}>
                <div className={styles.landingHeader}>
                    <div className={styles.landingHeaderLeft}>
                        <span className={styles.logoTitle}>Fractal</span>
                    </div>
                </div>
                <div className={styles.loginContainer}>
                    <BackgroundView />
                    <PaymentView />
                </div>
            </div>
        </div>
    )
}

export const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(Payment)
