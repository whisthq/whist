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
            <div
                style={{
                    width: 400,
                    margin: "auto",
                    marginTop: 120,
                }}
            >
                <h2
                    style={{
                        color: "#111111",
                        textAlign: "center",
                    }}
                >
                    {title}
                </h2>
            </div>
            {subtitle ? (
                <div
                    style={{
                        color: "#3930b8", // let's add some variety
                        textAlign: "center",
                    }}
                >
                    {subtitle}
                </div>
            ) : (
                <div />
            )}
        </div>
    )
}

export const DivSpace = (props: { height: number }) => (
    <div style={{ marginTop: props.height }} />
)