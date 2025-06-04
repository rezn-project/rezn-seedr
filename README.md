# rezn-seedr

**rezn-seedr** bootstraps Rezn + Orqos infrastructure nodes

---

## Features

- One-shot provisioning for Rezn and/or Orqos nodes
- Installs certs, trust configs, and SSH host/user certs (see below)
- Sets up systemd units for long-running services


---

## CA Integration (mTLS)

Rezn/Seedr relies on **mutual TLS** (mTLS) for all node and service communication.  
You must provide a CA to issue and rotate X.509 certs—*we don’t care which one*.

**Recommended:**
- [step-ca](https://smallstep.com/docs/step-ca/) — maintained, automatable, supports SSH CA
- [cfssl](https://github.com/cloudflare/cfssl) — works, but limited features
- Anything that spits out PEM certs

**Revocation & Rotation:**  
Short-lived certs (24-72h) recommended.  
Revocation (“kill switch”) is up to your CA—if you need instant revocation, pay for it or build it yourself.

**TL;DR:**  
You bring the certs, Rezn takes care of the mTLS.

---

## SSH Certificate Authority (SSH CA) Support

Rezn/Seedr expects your environment to support **SSH CA** (short-lived, signed SSH host and user certs).  
- [step-ssh](https://smallstep.com/docs/step-ssh/) is fully supported and recommended.
- Traditional static SSH keys are discouraged and may not be supported in future releases.
- If you’re not familiar with SSH CA, read: https://smallstep.com/docs/step-ssh/

---

## Directory Layout

```

/etc/rezn/
├── trust.toml
├── certs/
│   ├── rezn-client.pem
│   ├── rezn-client.key
│   └── orqos-ca.pem
│   ├── ssh\_host\_key-cert.pub     # SSH host cert (recommended)
│   └── ssh\_user-cert.pub         # SSH user cert (optional)
```

---

## Supported operating systems

Currently: Debian Bookworm, Ubuntu LTS.

**Docker is required.**

---

### What `rezn-seedr` Does

1. Ensures `/etc/rezn` and `/etc/rezn/certs/` exist
2. Copies certs (X.509, SSH) to the right places
3. Writes a valid `trust.toml`
4. Installs systemd units for rezn and orqos
5. Verifies services are running
6. Logs locally and optionally to Loki

---

## Trust Model

mTLS for all RPC and service calls.  
SSH CA for shell/automation access.  
Config via `/etc/rezn/trust.toml`:

```toml
[trust]
client_cert = "/etc/rezn/certs/rezn-client.pem"
client_key  = "/etc/rezn/certs/rezn-client.key"
ca_bundle   = "/etc/rezn/certs/orqos-ca.pem"
```

---

### WARNING

This tool **takes over the box.**
Run it only on clean, dedicated hosts, not shared or dev laptops.

---

## Status

**Under active development.**
Breaking changes happen weekly.
