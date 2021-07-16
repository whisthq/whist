import React from 'react'
import FadeIn from 'react-fade-in'
import { TypeWriter } from '@app/shared/components/typewriter'
import { Link } from 'react-scroll'

import { withContext } from '@app/shared/utils/context'
import { ScreenSize } from '@app/shared/constants/screenSizes'
import Geometric from './geometric'
import YoutubeLogo from '@app/assets/icons/youtubeLogo.svg'
import NotionLogo from '@app/assets/icons/notionLogo.svg'
import ShopifyLogo from '@app/assets/icons/shopifyLogo.svg'
import AirtableLogo from '@app/assets/icons/airtableLogo.svg'
import FigmaLogo from '@app/assets/icons/figmaLogo.svg'
import GoogleDriveLogo from '@app/assets/icons/googleDriveLogo.svg'

import {
  FractalButton,
  FractalButtonState
} from '@app/pages/home/components/button'

const SymmetricGeometric = (props: any) => (
  <FadeIn>
    <div
      className={props.className}
      style={{
        top: 400,
        left: 425
      }}
    >
      <Geometric
        className="relative pointer-events-none"
        scale={3}
        flip={false}
      />
    </div>
    <div
      className={props.className}
      style={{
        top: 400,
        right: -840
      }}
    >
      <Geometric
        className="relative pointer-events-none"
        scale={3}
        flip={true}
      />
    </div>
  </FadeIn>
)

export const Top = () => {
  /*
        Top section of Chrome product page

        Arguments: none
    */
  const { width } = withContext()
  const hovering = false

  const adjectives = ['faster', 'lighter', 'private']

  return (
    <div>
      <div className="text-white text-lg m-auto text-center py-8"></div>
      {width > ScreenSize.MEDIUM && <SymmetricGeometric className="absolute" />}
      <div className="mt-16 text-center">
        <FadeIn delay={width > ScreenSize.MEDIUM ? 1500 : 0}>
          <div className="text-5xl md:text-8xl dark:text-gray-300">
            <div>
              Chrome,
              <div className="flex justify-center relative">
                <div className="mr-3 py-2">just</div>
                <TypeWriter
                  words={adjectives}
                  startAt={5}
                  classNameCursor="bg-blue-800"
                  classNameText="py-2 text-blue dark:text-mint relative"
                />
              </div>
            </div>
          </div>
        </FadeIn>
        <FadeIn delay={width > ScreenSize.MEDIUM ? 1700 : 100}>
          <div className="mt-10 md:mt-12 relative">
            <p className="font-body m-auto max-w-screen-sm dark:text-gray-400 tracking-wider">
              Load pages instantly. Use 10x less memory. Enjoy complete privacy.
              Fractal is a supercharged version of Chrome that runs in the
              cloud.
            </p>
            <Link to="download" spy={true} smooth={true}>
              <FractalButton
                className="mt-12"
                contents="DOWNLOAD NOW"
                state={FractalButtonState.DEFAULT}
              />
            </Link>
          </div>
        </FadeIn>
        <FadeIn delay={width > ScreenSize.MEDIUM ? 1900 : 200}>
          <div className="relative max-w-screen-sm h-80 m-auto text-center">
            <img
              src={YoutubeLogo}
              style={{
                position: 'absolute',
                left: width > ScreenSize.MEDIUM ? 45 : 25,
                top: width > ScreenSize.MEDIUM ? 170 : 125,
                width: width > ScreenSize.MEDIUM ? 70 : 50,
                opacity: hovering ? 0.15 : 0.25
              }}
              className="animate-bounce"
              alt="youtube"
            />
            <img
              src={FigmaLogo}
              style={{
                position: 'absolute',
                left: width > ScreenSize.MEDIUM ? 250 : 150,
                top: width > ScreenSize.MEDIUM ? 80 : 50,
                width: width > ScreenSize.MEDIUM ? 40 : 30,
                opacity: hovering ? 0.15 : 0.25,
                animationDelay: '0.9s'
              }}
              className="animate-bounce"
              alt="figma"
            />
            <img
              src={NotionLogo}
              style={{
                position: 'absolute',
                left: width > ScreenSize.MEDIUM ? 170 : 150,
                top: width > ScreenSize.MEDIUM ? 280 : 175,
                width: width > ScreenSize.MEDIUM ? 45 : 35,
                opacity: hovering ? 0.15 : 0.25,
                animationDelay: '0.5s'
              }}
              className="animate-bounce"
              alt="instagram"
            />
            <img
              src={ShopifyLogo}
              style={{
                position: 'absolute',
                left: width > ScreenSize.MEDIUM ? 370 : 220,
                width: width > ScreenSize.MEDIUM ? 50 : 35,
                top: width > ScreenSize.MEDIUM ? 210 : 110,
                opacity: hovering ? 0.15 : 0.25,
                animationDelay: '0.2s'
              }}
              className="animate-bounce"
              alt="facebook"
            />
            <img
              src={GoogleDriveLogo}
              style={{
                position: 'absolute',
                left: width > ScreenSize.MEDIUM ? 450 : 30,
                top: width > ScreenSize.MEDIUM ? 60 : 210,
                width: width > ScreenSize.MEDIUM ? 50 : 35,
                opacity: hovering ? 0.15 : 0.25,
                animationDelay: '0.7s'
              }}
              className="animate-bounce"
              alt="gmail"
            />
            <img
              src={AirtableLogo}
              style={{
                position: 'absolute',
                left: width > ScreenSize.MEDIUM ? 500 : 240,
                top: width > ScreenSize.MEDIUM ? 350 : 240,
                width: width > ScreenSize.MEDIUM ? 50 : 35,
                opacity: hovering ? 0.15 : 0.2,
                animationDelay: '0.1s'
              }}
              className="animate-bounce"
              alt="reddit"
            />
          </div>
        </FadeIn>
      </div>
    </div>
  )
}

export default Top
