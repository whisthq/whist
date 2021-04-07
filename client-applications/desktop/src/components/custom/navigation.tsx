import React, { FC, useState, useEffect } from 'react'
import classNames from 'classnames'

import { goTo } from '@app/utils/history'

/*
    Prop declarations
*/

interface BaseNavigationProps {
  url: string
  text: string
  linkText?: string
  className?: string
  onClick?: (e: any) => any
}

interface FractalNavigationProps extends BaseNavigationProps {}

/*
    Components
*/

const BaseNavigation: FC<BaseNavigationProps> = (
  props: BaseNavigationProps
) => {
  /*
        Description:
            Returns a hyperlink component with custom text and URL

        Arguments:
            url(string): Path to navigate to (e.g. "/auth/login")
            text(string): Text to display in link
            linkText(string): Subtext that should be highlighted and clickable
            className(string): Optional additional Tailwind styling
    */

  const [textList, setTextList] = useState([props.text])

  const onClick = (text: string) => {
    if (text === props.linkText) {
      goTo(props.url)
    }
  }

  useEffect(() => {
    if (props.linkText) {
      if (!props.text.includes(props.linkText)) {
        throw new Error(
          'prop linkText must be a substring of prop text'
        )
      } else {
        const textListTemp = props.text.split(
          new RegExp(`(${props.linkText})`)
        )
        setTextList(textListTemp)
      }
    }
  }, [props.linkText, props.text])

  return (
        <>
            {textList.map((text: string, index: number) => (
                <div key={`${index.toString()}`} className="inline">
                    <span
                        className={classNames(
                          text === props.linkText
                            ? 'font-body text-blue font-medium cursor-pointer'
                            : 'font-body font-medium',
                          props.className
                        )}
                        onClick={() => {
                          (props.onClick != null) && props.onClick(text)
                          onClick(text)
                        }}
                    >
                        {text}
                    </span>
                </div>
            ))}
        </>
  )
}

export const FractalNavigation: FC<FractalNavigationProps> = (
  props: FractalNavigationProps
) => {
  /*
        Description:
            Returns a hyperlink component with custom text and URL

        Arguments:
            url(string): Path to navigate to (e.g. "/auth/login")
            text(string): Text to display in link
            linkText(string): Subtext that should be highlighted and clickable
            className(string): Optional additional Tailwind styling
    */

  return <BaseNavigation {...props} />
}
