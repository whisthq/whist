/*
    These are all commented out, as we cannot deploy these until I import them into the state file
    because S3 is a global namespace
*/

# provider "aws" {
#   region = var.region
# }

# resource "aws_s3_bucket" "fractal-user-app-configs" {
#   bucket = "fractal-user-app-configs-asdasdasd"

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

# resource "aws_s3_bucket" "fractal-chromium-macos-arm64-prod" {
#   bucket = "fractal-chromium-macos-arm64-prod"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "fractal-chromium-macos-arm64-staging" {
#   bucket = "fractal-chromium-macos-arm64-staging"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "fractal-chromium-macos-arm64-dev" {
#   bucket = "fractal-chromium-macos-arm64-dev"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "fractal-chromium-macos-dev" {
#   bucket = "fractal-chromium-macos-dev"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "fractal-chromium-macos-prod" {
#   bucket = "fractal-chromium-macos-prod"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "fractal-chromium-macos-staging" {
#   bucket = "fractal-chromium-macos-staging"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }

# resource "aws_s3_bucket" "fractal-test-assets" {
#   bucket = "fractal-test-assets"
# }

# resource "aws_s3_bucket" "fractal-shared-libs" {
#   bucket = "fractal-shared-libs"

#   server_side_encryption_configuration {
#     rule {
#       apply_server_side_encryption_by_default {
#         sse_algorithm = "AES256"
#       }
#       bucket_key_enabled = false
#     }
#   }

# }
