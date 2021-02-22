// used ot handle gif imports with jest typescript

module.exports = {
    process() {
        return "module.exports = {};"
    },
    getCacheKey() {
        // The output is always the same.
        return "gifTransform"
    },
}
