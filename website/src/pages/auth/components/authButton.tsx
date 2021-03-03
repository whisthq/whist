import React from "react"

const AuthButton = (props: {
    text: string
    onClick: () => void 
    dataTestId?: string
}) => {
    const {text, onClick, dataTestId} = props

    return(
        <>
            <button className="rounded bg-blue dark:bg-mint px-8 py-3 mt-4 text-white dark:text-black w-full hover:bg-mint hover:text-black duration-500 font-medium" onClick={onClick} data-testid={dataTestId}>
                {text}
            </button>
        </>
    )
}

export default AuthButton