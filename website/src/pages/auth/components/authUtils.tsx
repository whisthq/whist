import React from "react"
import "styles/auth.css"

/**
 * A title component that announces the state of the Verify page when it is not processing. It is used
 * to say (specifically) when verify is not being arrived at via email or when verification failed.
 * @param props.title the text announcing what state we are at.
 * @param props.subtitle a small subtitle than can add some clarification.
 */
export const Title = (props: { title: string; subtitle?: string }) => {
    const { title, subtitle } = props

    return (
        <div>
            <div className="auth-title" style={{ marginBottom: 15 }}>
                {title}
            </div>
            {subtitle && (
                <div
                    style={{
                        color: "#3930b8", // let's add some variety
                        textAlign: "center",
                    }}
                >
                    {subtitle}
                </div>
            )}
        </div>
    )
}
