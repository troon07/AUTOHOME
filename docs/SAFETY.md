# Safety boundary and limitations

Updated 20 October 2026. This is a low-voltage DC prototype. No mains switching is permitted. Current native builds inhibit physical outputs and register all virtual devices with pin -1. Both boards are currently away.

Node A's SafetyKernel is the only component that changes registered output state. It validates room/device/action, limits rapid cycling with a three-second dwell, limits continuous ON to four hours, and gives manual actions a five-minute hold over automatic policy. Maximum-run cutoff overrides a manual hold. The registry rejects duplicate identities/GPIOs and overlong names.

The firmware sets the inactive output latch before enabling each pin as an output. Hardware reset/power-up levels, relay polarity, brownouts and connected-load transients have not been measured. Software initialization does not prove absence of electrical pulses during power-up, and a crash does not guarantee an external relay de-energizes. Physical fail-safe behavior requires suitable pull resistors, drivers and independent electrical validation.

Network proposals require paired peer identity, a mandatory application authentication tag, the current persistent actuator boot identity, next request ID and valid command issuance/TTL. An identical retry returns the cached outcome. No network message directly writes a GPIO. The HTTP control endpoint requires its own token on the protected AP; the token is not a substitute for secure production provisioning.

Scenes call the same kernel for each output. If one action is denied, a scene may partially apply, and its ACK reports denial and actual states. All-off obeys dwell and is not a physical emergency stop. Auto-off timers retry a denied OFF; they do not bypass the kernel.

Local automation uses bedroom PIR evidence only. Its VACANT policy state means the grace timer expired, not verified human absence. The four virtual rooms have explicitly synthetic inputs, processed separately on A; these are not physical occupancy evidence. Fan automation is optional and motion-based; it is not thermal protection.

Boot self-tests use mock pins and run before real devices are registered. USB bench commands can trigger GPIO changes through the kernel and deliberately drop application frames/ACKs; use the HIL runner only with bare boards. Production deployment would remove or restrict those diagnostics and validate every reset/fault condition physically.

The offline interface preview performs no automation or inference: APIs explicitly return503 and device states stayunknown. A loaded speech model produces proposals only. Missing sensors/expired input leases cannot authorize unlimited automatic occupancy; manual holds and maximum-run cutoff retain their own documented semantics.
