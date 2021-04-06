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
    const configIV = await generateRandomString(48)
    const hashedPassword = generateHashedPassword(password)

    var cipher = crypto.createCipheriv(algorithm, hashedPassword, configIV)
    var encrypted = cipher.update(text, "utf8", "hex")
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
    console.log("1")
    const hashedPassword = generateHashedPassword(password)
    console.log("2")
    const encrypted = text.substring(0, 128)
    console.log("3")
    const configIV = text.substring(128, 224)
    console.log("4")
    const tag = Buffer.from(text.substring(224), "hex")
    console.log("5")
    var decipher = crypto.createDecipheriv(algorithm, hashedPassword, configIV)
    console.log("6")
    decipher.setAuthTag(tag)
    console.log("7")
    var dec = decipher.update(encrypted, "hex", "utf8")
    console.log("8")
    dec += decipher.final("utf8")
    return dec
}

export const createConfigToken = async () => {
    return await generateRandomString(32)
}
