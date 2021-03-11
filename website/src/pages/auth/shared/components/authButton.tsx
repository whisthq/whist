import React from "react"
import { FontAwesomeIcon } from "@fortawesome/react-fontawesome"
import { faCircleNotch } from "@fortawesome/free-solid-svg-icons"

const AuthButton = (props: {
    text: string
    onClick: () => void
    dataTestId?: string
    processing?: boolean
}) => {
    const { text, onClick, dataTestId, processing } = props

    return (
        <>
            <button
                className="rounded bg-blue dark:bg-mint px-8 py-3 mt-4 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium"
                onClick={onClick}
                data-testid={dataTestId}
                disabled={processing}
            >
                {processing ? (
                    <>
                        <FontAwesomeIcon
                            icon={faCircleNotch}
                            spin
                            className="text-white"
                        />
                    </>
                ) : (
                    <>{text}</>
                )}
            </button>
        </>
    )
}

export default AuthButton
