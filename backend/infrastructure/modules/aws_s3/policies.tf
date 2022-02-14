# ------------------------------ Policies for Chromium builds ------------------------------ #

resource "aws_s3_bucket_public_access_block" "whist-chromium-macos-arm64" {
  bucket              = aws_s3_bucket.whist-chromium-macos-arm64.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-chromium-macos-x64" {
  bucket              = aws_s3_bucket.whist-chromium-macos-x64.id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-chromium-windows" {
  bucket              = aws_s3_bucket.whist-chromium-windows.id
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

# Policy for dev secrets

resource "aws_s3_bucket_public_access_block" "whist-dev-secrets" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-dev-secrets[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

resource "aws_s3_bucket_public_access_block" "whist-terraform-state" {
  count  = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-terraform-state[0].id
  block_public_acls   = true
  block_public_policy = true
  restrict_public_buckets = true
  ignore_public_acls = true
}

# ------------------------------ Lifecycle policies for bucket versioning ------------------------------ #

resource "aws_s3_bucket_lifecycle_configuration" "whst-user-app-config-lifecycle" {
  depends_on = [aws_s3_bucket_versioning.versioning]
  bucket = aws_s3_bucket.whist-user-app-config.id

  # This rule keeps only the 3 most recent nonexpired objects
  # on the bucket. This applies to objects 5 days after becoming
  # noncurrent. It is also necessary to provide a filter or AWS will
  # return an error.
  rule {
    # To apply the rule to all user configs, add a filter using a range 
    # of size up to 1Gb.
    filter {
      # Represented in bytes
      and {
        object_size_greater_than = 0
        object_size_less_than = 1000000000
      }
    }

    noncurrent_version_expiration {
      newer_noncurrent_versions = 3
      noncurrent_days = 5
    }
  }
}