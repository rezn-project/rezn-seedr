# Package

version       = "0.1.0"
author        = "andreamancuso"
description   = "A new awesome nimble package"
license       = "MPL-2.0"
srcDir        = "src"
bin           = @["step_ca_init", "step_service"]


# Dependencies

requires "nim >= 2.2.4"
requires "passgen >= 0.2.0"
requires "nimcrypto >= 0.6.3"