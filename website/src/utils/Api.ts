export async function apiPost(endpoint: any, body: any, token: any) {
    try {
        const response = await fetch(endpoint, {
            method: "POST",
            mode: "cors",
            headers: {
                "Content-Type": "application/json",
                Authorization: "Bearer " + token,
            },
            body: JSON.stringify(body),
        })
        const json = await response.json()
        return { json, response }
    } catch (err) {
        console.log(err)
        return err
    }
}
