# ------------------------------ Policies for Chromium builds ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-browser-chromium-macos-arm64" {
  bucket              = aws_s3_bucket.whist-browser-chromium-macos-arm64.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-browser-chromium-macos" {
  bucket              = aws_s3_bucket.whist-browser-chromium-macos.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-browser-chromium-windows" {
  bucket              = aws_s3_bucket.whist-browser-chromium-windows.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# Bucket for Chromium debug build

resource "aws_s3_bucket_public_access_block" "whist-browser-chromium-debug-build" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-browser-chromium-debug-build[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Policies for Brave builds ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-browser-brave-linux-x64" {
  bucket              = aws_s3_bucket.whist-browser-brave-linux-x64.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-browser-brave-macos-arm64" {
  bucket              = aws_s3_bucket.whist-browser-brave-macos-arm64.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-browser-brave-macos-x64" {
  bucket              = aws_s3_bucket.whist-browser-brave-macos-x64.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# Policy for Brave cache

resource "aws_s3_bucket_public_access_block" "whist-brave-sscache" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-brave-sscache[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Policies for user app configs ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-user-app-config" {
  bucket = aws_s3_bucket.whist-user-app-config.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Policies for assets ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-brand-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-brand-assets[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-website-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-website-assets[0].id
  block_public_acls   = false
  block_public_policy = false
  restrict_public_buckets = false
  ignore_public_acls = false
}

resource "aws_s3_bucket_public_access_block" "whist-test-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-test-assets[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Policies for protocol ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-e2e-protocol-test-logs" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-protocol-shared-libs" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-protocol-shared-libs[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Policies for other data ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-technical-interview-data" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-technical-interview-data[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# Policy for dev secrets

resource "aws_s3_bucket_public_access_block" "whist-dev-secrets" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-dev-secrets[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}
