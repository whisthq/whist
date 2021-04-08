import crypto from "crypto"

export const generateHashedPassword = (password: string): string => {
    /*
        Hash the user's plain-text password (to later use as encryption/decryption password for the config token)
        Arguments:
            password (string): plaintext password to hash
    */
    const token = crypto.pbkdf2Sync(
        password,
        process.env.REACT_APP_SHA_SECRET_KEY || "",
        100000,
        16,
        "sha256"
    )
    return token.toString("hex")
}

export const generateRandomString = async (bytes: number) => {
    /*
        Generate a cryptographically secure random string
        Arguments:
            bytes (number): number of bytes in the string
    */
    const randomString: Buffer = await new Promise((resolve, reject) => {
        crypto.randomBytes(bytes, (err: Error | null, buf: Buffer) => {
            if (err) {
                reject(err)
            }
            resolve(buf)
        })
    })
    return randomString.toString("hex")
}

const algorithm = "aes-256-gcm"

export const encryptConfigToken = async (text: string, password: string) => {
    /*
        Encrypt the given config token with the given password
        Arguments:
            text (string): text to encrypt
            password (string): plaintext password to encrypt with
        Returns:
            (string): the encrypted text
    */
    console.log("ENCRYPTING", text, password)
    const configIV = await generateRandomString(48)
    const hashedPassword = generateHashedPassword(password)

    console.log("hashed passwrod", hashedPassword)
    console.log("config", configIV)

    var cipher = crypto.createCipheriv(algorithm, hashedPassword, configIV)

    console.log("cipher", cipher)

    var encrypted = cipher.update(text, "utf8", "hex")

    console.log("encrypted", encrypted)
    
    encrypted += cipher.final("hex")
    var tag = cipher.getAuthTag().toString("hex")

    return encrypted.concat(configIV, tag)
}

export const decryptConfigToken = (text: string, password: string): string => {
    /*
        Decrypt the given config token with the given password
        Arguments:
            text (string): text to decrypt
            password (string): plaintext password to decrypt with
        Returns:
            (string): the decrypted text
    */
    const hashedPassword = generateHashedPassword(password)
    const encrypted = text.substring(0, 128)
    const configIV = text.substring(128, 224)
    const tag = Buffer.from(text.substring(224), "hex")
    console.log("CONFIG IV", configIV)
    console.log(text.length)
    var decipher = crypto.createDecipheriv(algorithm, hashedPassword, configIV)
    decipher.setAuthTag(tag)
    var dec = decipher.update(encrypted, "hex", "utf8")
    dec += decipher.final("utf8")
    return dec
}

export const createConfigToken = async () => {
    return await generateRandomString(32)
}
