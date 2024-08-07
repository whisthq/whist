# This secret will only be added on prod to avoid duplication of resources,
# or potential terraform failures.
resource "aws_secretsmanager_secret" "ghcrDockerAuthentication" {
  count       = var.env == "dev" ? 1 : 0
  name        = "ghcrDockerAuthentication"
  description = "Phil's GitHub Personal Access Token and Username, used for authenticating \"Read\" permissions with GitHub Packages for pulling container images from ghcr.io"
  tags = {
    Name      = "ghcrDockerAuthentication"
    Env       = var.env
    Terraform = true
  }
}
