# IAM roles

#IAM User groups

resource "aws_iam_group" "whist-2FA" {
  name = "Whist2FA"
}

resource "aws_iam_group" "whist-admins" {
  name = "WhistAdmins"
}

resource "aws_iam_group" "whist-ci" {
  name = "WhistCI"
}

resource "aws_iam_group" "whist-engineers" {
  name = "WhistEngineers"
}

# Custom group policies

resource "aws_iam_group_policy" "2FA-policy" {
  name = "Force_MFA"
  group = aws_iam_group.whist-2FA
  policy = jsonencode
}

# AWS managed group policies

resource "aws_iam_group_policy_attachment" "admin-policy" {
  group      = aws_iam_group.whist-admins.name
  for_each   = var.whist-admins-managed-policies
  policy_arn = each.key
}

resource "aws_iam_group_policy_attachment" "ci-policy" {
  group      = aws_iam_group.whist-engineers.name
  for_each   = var.whist-ci-managed-policies
  policy_arn = each.key
}

resource "aws_iam_group_policy_attachment" "engineering-policy" {
  group      = aws_iam_group.whist-engineers.name
  for_each   = var.whist-engineers-managed-policies
  policy_arn = each.key
}
