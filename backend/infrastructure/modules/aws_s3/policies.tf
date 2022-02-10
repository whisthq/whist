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
  bucket              = aws_s3_bucket.whist-browser-chromium-windows
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# Bucket for Chromium debug build

resource "aws_s3_bucket_public_access_block" "whist-browser-chromium-debug-build" {
  bucket              = aws_s3_bucket.whist-browser-chromium-debug-build.id
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
  bucket              = aws_s3_bucket.whist-brave-sscache.id
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
  bucket              = aws_s3_bucket.whist-brand-assets.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-website-assets" {
  bucket              = aws_s3_bucket.whist-website-assets.id
  block_public_acls   = false #tfsec:ignore:aws-s3-block-public-acls
  block_public_policy = false #tfsec:ignore:aws-s3-block-public-policy
  restrict_public_buckets = false #tfsec:ignore:aws-s3-no-public-buckets
  ignore_public_acls = false #tfsec:ignore:aws-s3-ignore-public-acls
}

resource "aws_s3_bucket_public_access_block" "whist-test-assets" {
  bucket              = aws_s3_bucket.whist-test-assets.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Policies for protocol ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-e2e-protocol-test-logs" {
  bucket              = aws_s3_bucket.whist-e2e-protocol-test-logs.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-protocol-shared-libs" {
  bucket = aws_s3_bucket.whist-protocol-shared-libs.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Policies for other data ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-technical-interview-data" {
  bucket              = aws_s3_bucket.whist-technical-interview-data.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# Policy for dev secrets

resource "aws_s3_bucket_public_access_block" "whist-dev-secrets" {
  bucket              = aws_s3_bucket.whist-dev-secrets.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}
