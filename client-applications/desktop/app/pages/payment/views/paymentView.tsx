import React from "react"
import { connect } from "react-redux"
import styles from "pages/payment/payment.css"

import { openExternal } from "shared/utils/general/helpers"
import { FractalRoute } from "shared/types/navigation"
import { config } from "shared/constants/config"
import { history } from "store/history"

const PaymentView = () => {
    const handleRedirectPayment = () => {
        openExternal(`${config.url.FRONTEND_URL}/dashboard/settings/payment`)
    }

    const handleDone = () => {
        history.push(FractalRoute.DASHBOARD)
    }

    return (
        <div style={{ marginTop: 200 }}>
            <div style={{ margin: "auto", marginTop: 50, maxWidth: 450 }}>
                Thanks for trying out Fractal! If you&lsquo;d like to continue,
                click{" "}
                <span
                    role="button"
                    tabIndex={0}
                    onClick={handleRedirectPayment}
                    onKeyDown={handleRedirectPayment}
                    style={{ fontWeight: "bold", cursor: "pointer" }}
                >
                    here
                </span>{" "}
                to choose a payment plan.
            </div>

            <div style={{ marginTop: 30 }}>
                <button
                    type="button"
                    className={styles.dashboardButton}
                    id="login-button"
                    onClick={handleDone}
                >
                    Back to Dashboard
                </button>
            </div>
        </div>
    )
}

export const mapStateToProps = () => {
    return {}
}

export default connect(mapStateToProps)(PaymentView)
