// learn to create class

import { any, string } from "prop-types";

import { platform } from 'process';
import fs from 'fs';
import { filter } from "lodash";

const keytar = require('keytar')
const pbkdf2 = require('pbkdf2')
const {homedir} = require('os');
const {dirname} = require('path');



// copy over all classes

// maybe include brave


// start with


class ChromiumBased {
    // Super class for all Chromium based browser
    private salt: Buffer = Buffer.from('saltysalt', 'base64')
    private iv: Buffer = Buffer.from('                ', 'base64')
    private length = 16
    private browserName: string = "Chromium"
    private cookieFile: string[] = []
    private domainName: string = ""
    private key: any = ""
    private keyFile: any
    private browserDetails: any = {}
    private tmpCookieFIle: string[] = []

    constructor(init?:Partial<ChromiumBased>) {
        Object.assign(this, init);
        this.addKeyAndCookieFile()
    }

    private addKeyAndCookieFile(): void {
        switch(platform) {
            case "darwin": {
                const myPass = keytar.getPassword(this.browserDetails["osx_key_service"], this.browserDetails["osx_key_user"])
                
                let iterations = 1003 // number of pbkdf2 iterations on mac
                this.key = pbkdf2.pbkdf2(myPass, this.salt, iterations, this.length)

                this.cookieFile = this.cookieFile ? this.cookieFile : expandPaths(this.browserDetails["osx_cookies"],"osx")
                break
            } case "linux": {
                const myPass = getLinuxPass(this.browserDetails["os_crypt_name"])
                
                let iterations = 1 
                this.key = pbkdf2.pbkdf2(myPass, this.salt, iterations, this.length)
                
                this.cookieFile = this.cookieFile ? this.cookieFile : expandPaths(this.browserDetails["linux_cookies"],"linux")
                break
            } default:
                throw("OS not recognized. Works on OSX or linux.")
        }

        if (this.cookieFile == null){
            throw(`Failed to find ${this.browserName} cookie`)
        }

        this.tmpCookieFIle = createLocalCopy(this.cookieFile)

    }

}

const createLocalCopy = (cookieFile: string[]): string[] => {

}


const expanduser = (text: string): string => {
    return text.replace(
        /^~([a-z]+|\/)/,
        (_, $1) => $1 === '/' ?
            homedir() : `${dirname(homedir())}/${$1}`
    );
}

const expandPaths = (paths: string[], os_name: string): string[] => {
    os_name = os_name.toLowerCase()

    // expand the path of file and remove invalid files
    paths = paths.map(expanduser)
    paths = paths.filter(fs.existsSync)

    return paths
}

const getLinuxPass = (os_crypt_name: string): any => {
    // need to figure equivalent to import secretservice

}