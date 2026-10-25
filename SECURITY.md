# Security

Do not publish credentials, private keys, or exploitable device-specific details in
public issues. Until a private reporting channel is configured, use the repository
owner's established private contact. Do not assume this prototype is production-ready.

Provision unique device credentials, verify TLS peers and firmware authenticity,
restrict MQTT topics per device, and define credential rotation before deployment.
Secure boot and flash encryption require board-specific provisioning and recovery
planning; they are not enabled by this template.
