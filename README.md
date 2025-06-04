# rezn-seedr

**rezn-seedr** is a bootstrapper for Rezn + Orqos infrastructure nodes.

---

## Features

* One-shot provisioning of Rezn and/or Orqos nodes
* Installs certs and trust config
* Sets up systemd units for long-running services
* Designed to work without any user interaction

---

## CA Integration (mTLS)

Rezn/Seedr uses mTLS for node-to-node and service communication.  
You need an internal CA to issue and rotate X.509 certs.  
We don’t care what you use—just give us certs.

### Options That Actually Work

* [step-ca](https://smallstep.com/docs/step-ca/)
* [cfssl](https://github.com/cloudflare/cfssl)
* Anything else that produces certs in PEM format

### Revocation & Rotation

We recommend you keep certs short-lived (a few days, tops).  

#### TL;DR

You bring the certs, rezn takes care of the mTLS.

---

## Directory Layout

```
/etc/rezn/
├── trust.toml
├── certs/
│   ├── rezn-client.pem
│   ├── rezn-client.key
│   └── orqos-ca.pem

/etc/systemd/system/
├── rezn.service
├── orqos.service (optional)
```

---

## Supported operating systems

Initially only Debian Bookworm and latest Ubuntu LTS.

### Requirements

Docker must be installed on the target host.

---

### What `rezn-seedr` Does

1. Ensures `/etc/rezn` and `/etc/rezn/certs/` exist
2. Copies certs to the appropriate location
3. Writes a valid `trust.toml`
4. Sets up `rezn` either locally or on a remote host
5. Sets up `orqos` nodes
6. Verifies that services are running
7. Logs everything locally, optionally to Loki

---

### Trust Model

Based on mTLS Orqos.
The trust config is defined in `trust.toml`:

```toml
[trust]
client_cert = "/etc/rezn/certs/rezn-client.pem"
client_key  = "/etc/rezn/certs/rezn-client.key"
ca_bundle   = "/etc/rezn/certs/orqos-ca.pem"
```

---

### WARNING

This tool **takes over the box.** It is not meant to be used on shared systems or developer laptops.
You run `rezn-seedr` on clean hosts you intend to dedicate to infrastructure.

---

### Status

This project is **under active development**. Currently supported:

---

