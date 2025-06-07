import os, osproc, strutils, std/tempfiles

proc prompt(msg: string, def = ""): string =
  stdout.write(msg & (if def != "": " [" & def & "]" else: "") & ": ")
  let line = stdin.readLine().strip()
  if line.len == 0: return def else: return line

# Overwrite then remove temp file (security-theater)
proc secureDelete(path: string) =
  if fileExists(path):
    let size = getFileSize(path)
    if size > 0:
      var f = open(path, fmWrite)
      for _ in 0..<size: f.write('\x00')
      f.close()
    removeFile(path)

# Usage/help
proc printHelp() =
  echo """
rezn-seedr: Step CA Bootstrapper
Usage: rezn-seedr [--help]
Interactively generate and initialize a step-ca config.
"""

if paramCount() > 0 and paramStr(1) == "--help":
  printHelp()
  quit(0)

let caName = prompt("CA name", "rezn-seedr")
let caDns = prompt("DNS name", "localhost")
let caAddr = prompt("Listen address", "127.0.0.1:9000")
let provName = prompt("Provisioner name", "admin")
let addAcme = prompt("Add ACME provisioner (y/N)", "N").toLowerAscii == "y"
let enableSsh = prompt("Enable SSH CA (y/N)", "N").toLowerAscii == "y"

let caPass = prompt("CA password (leave empty for random)", "")
if caPass.len == 0:
  echo "[INFO] Empty password: step-ca will generate a random one."
let provPass = prompt("Provisioner password (leave empty for random)", "")
if provPass.len == 0:
  echo "[INFO] Empty provisioner password: step-ca will generate a random one."
let noDb = prompt("Disable DB? (y/N)", "N").toLowerAscii == "y"

var pwFile = ""
var provPwFile = ""

if caPass.len > 0:
  let (pwF, pwPath) = createTempFile("step-pw", "")
  pwF.write(caPass)
  pwF.close()
  pwFile = pwPath

if provPass.len > 0:
  let (provF, provPath) = createTempFile("step-provpw", "")
  provF.write(provPass)
  provF.close()
  provPwFile = provPath


# Build args as a seq
var args: seq[string] = @["ca", "init",
  "--name", caName,
  "--dns", caDns,
  "--address", caAddr,
  "--provisioner", provName
]
if addAcme: args.add "--acme"
if enableSsh: args.add "--ssh"
if pwFile.len > 0: args.add ["--password-file", pwFile]
if provPwFile.len > 0: args.add ["--provisioner-password-file", provPwFile]
if noDb: args.add "--no-db"

echo "\nRunning: step ", args.join(" ")

let p = startProcess("step", "", args, options={poParentStreams})
let code = p.waitForExit()
if code == 0:
  echo "\n[OK] CA initialized successfully!\n"
else:
  stderr.writeLine("\n[ERR] step-ca init failed.")

# Cleanup temp files (securely)
if pwFile.len > 0: secureDelete(pwFile)
if provPwFile.len > 0: secureDelete(provPwFile)
