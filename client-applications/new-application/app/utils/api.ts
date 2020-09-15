export async function apiPost(endpoint, body, token) {
    try {
        const response = await fetch(endpoint, {
            method: "POST",
            mode: "cors",
            headers: {
                "Content-Type": "application/json",
                Authorization: "Bearer " + token,
            },
            body: JSON.stringify(body),
        });
        const json = await response.json();
        return { json, response };
    } catch (err) {
        console.log(err);
        return err;
    }
}

export async function apiGet(endpoint, token) {
    try {
        const response = await fetch(endpoint, {
            method: "GET",
            mode: "cors",
            headers: {
                "Content-Type": "application/json",
                Authorization: "Bearer " + token,
            },
        });
        const json = await response.json();
        return { json, response };
    } catch (err) {
        console.log(err);
        return err;
    }
}
