const postInstall = (_env, ..._args) => {}

module.exports = postInstall

if (require.main === module) {
  postInstall()
}
