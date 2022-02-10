# Block all public ACL on the following buckets

resource "aws_s3_bucket_public_access_block" "whist-browser-chromium-debug-build-block-public" {
  count = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-browser-chromium-debug-build[0].id
  block_public_acls   = true
  block_public_policy = true
}

resource "aws_s3_bucket_public_access_block" "whist-brave-sscache-block-public" {
  count = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-brave-sscache[0].id
  block_public_acls   = true
  block_public_policy = true
}

resource "aws_s3_bucket_public_access_block" "whist-user-app-config-block-public" {
  bucket              = aws_s3_bucket.whist-user-app-config.id
  block_public_acls   = true
  block_public_policy = true
}

resource "aws_s3_bucket_public_access_block" "whist-e2e-protocol-test-logs-block-public" {
  count = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-e2e-protocol-test-logs[0].id
  block_public_acls   = true
  block_public_policy = true
}

resource "aws_s3_bucket_public_access_block" "whist-dev-secrets-block-public" {
  count               = var.env == "prod" ? 1 : 0
  bucket              = aws_s3_bucket.whist-dev-secrets[0].id
  block_public_acls   = true
  block_public_policy = true
}

# Allow all public access on the whist website assets bucket

data "aws_iam_policy_document" "allow-all-public-access" {
  statement {
    effect = "Allow"

    principals {
      type        = "AWS"
      identifiers = ["*"]
    }

    actions = [
      "s3:GetObject",
    ]

    resources = [
      "arn:aws:s3:::whist-website-assets/*"
    ]
  }
}

resource "aws_s3_bucket_policy" "allow-all-public-access-website-assets" {
  count  = var.env == "prod" ? 1 : 0
  bucket = aws_s3_bucket.whist-website-assets[0].id
  policy = data.aws_iam_policy_document.allow-all-public-access.json
}
