import React from "react"

const Detailed = () => {
  return (
    <div className="relative py-16 overflow-hidden mt-24">
      <div className="relative px-4 sm:px-6 lg:px-8">
        <div className="text-lg max-w-prose mx-auto">
          <h1>
            <span className="mt-3 block text-3xl text-center leading-8 text-gray-300 sm:text-4xl">
              How We Keep Your Data Private
            </span>
          </h1>
          <p className="mt-8 text-gray-400 leading-8 text-left">
            Fractal works by streaming Google Chrome from our datacenters to
            your computer. Because Google Chrome is being streamed to your
            computer from a datacenter in the form of video pixels, there are
            two types of security to consider: security of your browsing stream
            and security of your browsing data.
          </p>
          <p className="mt-8 text-xl text-mint leading-8 text-left">
            How We Secure Your Browsing Stream
          </p>
          <p className="mt-8 text-gray-400 leading-8 text-left">
            Each of your packets is encrypted at every leg of the journey
            between your PC and your cloud browser using ephemeral keys for each
            session.
          </p>
          <p className="mt-8 text-gray-400 leading-8 text-left">
            These packets are only decrypted once they&lsquo;re at the end of
            their journey. We discard your keystrokes, audio, and video
            immediately after they are replayed.
          </p>
          <p className="mt-8 text-xl text-mint leading-8 text-left">
            How We Secure Your Browsing Data
          </p>
          <p className="mt-8 text-gray-400 leading-8 text-left">
            We know what you&lsquo;re thinking: If Fractal saves my cookies and
            history, can&lsquo;t they see all of it? No — your session data is
            never stored unencrypted. Further, the encryption key for that data
            lives in 2 places: on your computer and temporarily on your cloud
            browser so it can decrypt your session data at the start of your
            session and re-encrypt it at the end of the session. That key never
            touches our permanent storage, so even if a malicious actor were to
            gain access to our system, they would just see blobs of binary
            noise.
          </p>
        </div>
      </div>
    </div>
  )
}

export default Detailed
