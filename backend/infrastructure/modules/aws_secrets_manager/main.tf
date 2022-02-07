resource "aws_secretsmanager_secret" "ghcrDockerAuthentication" {
  count               = var.env == "prod" ? 1 : 0
  name                = "ghcrDockerAuthentication"
  description = "Phil's GitHub Personal Access Token and Username, used for authenticating \"Read\" permissions with GitHub Packages for pulling container images from ghcr.io"
  tags = {
    Name      = "ghcrDockerAuthentication"
    Env       = var.env
    Terraform = true
  }
}