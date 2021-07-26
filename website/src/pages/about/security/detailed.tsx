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
                    <p className="mt-8 text-xl text-gray-400 leading-8 text-left">
                        Fractal works by streaming Google Chrome from our
                        datacenters to your computer. Because Google Chrome is
                        being streamed to your computer from a datacenter in the
                        form of video pixels, there are two types of security to
                        consider: security of your browsing stream and security
                        of your browsing data.
                    </p>
                    <p className="mt-8 text-xl text-mint leading-8 text-left">
                        How We Secure Your Browsing Stream
                    </p>
                    <p className="mt-8 text-xl text-gray-400 leading-8 text-left">
                        Each one of your packets is encrypted for every leg of
                        the journey between your PC and your cloud browser,
                        using ephemeral keys for each session. Feel free to lend
                        your shiny new account to your friends — them using
                        Fractal is just as secure as them using an on-device
                        browser.
                    </p>
                    <p className="mt-8 text-xl text-gray-400 leading-8 text-left">
                        These packets are only decrypted once they're actually
                        at the end of their journey — your keystrokes only
                        readable inside your browser, and your audio and visuals
                        only visible by you. Just like a browser should be.
                    </p>
                    <p className="mt-8 text-xl text-mint leading-8 text-left">
                        How We Secure Your Browsing Data
                    </p>
                    <p className="mt-8 text-xl text-gray-400 leading-8 text-left">
                        We know what you're thinking: 'if Fractal's saving
                        my cookies, history, and password, can't they see all of
                        it'? No — your session data is *never* stored
                        unencrypted — and not only that, but the encryption key
                        for that data (using AES 256, because we aren't savages)
                        only lives in 2 places: on your client machine, and
                        temporarily on your cloud browser, so it can decrypt
                        your session data at the start of your session and
                        reencrypt it at the end of the session. That key never
                        touches our permanent storage — and without it, your
                        session data is a blob of binary noise.
                    </p>
                    <p className="mt-8 text-xl text-gray-400 leading-8 text-left">
                        This is powerful security, but it does come with a
                        downside — if you lose access to your logged in Fractal
                        account, you'll have to restore your session on your
                        own. (For now. We're cooking up something similar to the
                        Lastpass and Bitwarden protocols to enable
                        security-question and passphrase-based generation of
                        those keys.) If you've ever wondered why certain
                        products have a 'master password' you can't just reset,
                        this is why! Security like this comes at a slight
                        convenience cost, but your browsing data should be
                        private. To us, to our engineers, and to everyone but
                        you.
                    </p>
                </div>
            </div>
        </div>
    )
}

export default Detailed
