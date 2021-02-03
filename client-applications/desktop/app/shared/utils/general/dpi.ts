const binSearch = (
    fn: (x: number) => boolean,
    min: number,
    max: number
): number => {
    /*
        Description:
            Binary search function

        Arguments:
            fn ((x: number) =>): function to binary search with
            min: minimum value
            max: maximum value

        Returns:
            number: Output of binary search
    */

    if (max < min) return -1 // not found

    const mid = (min + max) / 2
    if (fn(mid)) {
        if (mid === min || !fn(mid - 1)) {
            return mid
        }
        return binSearch(fn, min, mid - 1)
    }
    return binSearch(fn, mid + 1, max)
}

const findFirstMatch = (fn: (x: number) => boolean) => {
    /*
        Description:
            Find lowest number matching DPI

        Arguments: none

        Returns:
            number: DPI
    */

    let start = 1
    while (!fn(start)) start *= 2
    return binSearch(fn, start / 2, start)
}

export const findDPI = () => {
    /*
        Description:
            Get the client DPI

        Arguments: none

        Returns:
            number: DPI of client
    */

    return findFirstMatch(
        (x: number) => matchMedia(`(max-resolution: ${x}dpi)`).matches
    )
}
