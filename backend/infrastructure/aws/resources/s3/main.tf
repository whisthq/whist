/*
    These are all commented out, as we cannot deploy these until I import them into the state file
    because S3 is a global namespace
*/

# provider "aws" {
#   region = var.region
# }

# resource "aws_s3_bucket" "whist-user-app-configs" {
#   bucket = "whist-user-app-configs-asdasdasd"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

#   versioning {
#     enabled = true
#   }
# }

# resource "aws_s3_bucket" "whist-chromium-macos-arm64-prod" {
#   bucket = "whist-chromium-macos-arm64-prod"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "whist-chromium-macos-arm64-staging" {
#   bucket = "whist-chromium-macos-arm64-staging"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "whist-chromium-macos-arm64-dev" {
#   bucket = "whist-chromium-macos-arm64-dev"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "whist-chromium-macos-dev" {
#   bucket = "whist-chromium-macos-dev"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "whist-chromium-macos-prod" {
#   bucket = "whist-chromium-macos-prod"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "whist-chromium-macos-staging" {
#   bucket = "whist-chromium-macos-staging"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "whist-test-assets" {
#   bucket = "whist-test-assets"
# }

# resource "aws_s3_bucket" "whist-shared-libs" {
#   bucket = "whist-shared-libs"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }
