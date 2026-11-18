# AmiHASS plugin 0.1

Enable AmiHASS in Beam68 Preferences after installing and configuring AmiHASS.
The plugin is disabled by default and requires its running AmiHASS.Service.
It does not start the service or store connection settings or tokens.

- `ha:kitchen`: search entity names and IDs. Next/Previous browse results.
- `ha:status`: inspect local service and Home Assistant connection status.
- Select an entity and press Enter to read its state and supported basic actions.
- Select Turn on, Turn off, Toggle or Activate, press Enter and confirm.

Sensors, locks and doors are read-only. Availability and supported actions are
checked again by the service. Action acceptance is not success: the plugin reads
fresh state for on/off/toggle, and reports an unconfirmed result if the expected
state is not received. Scene/script activation reports acceptance only.
Typing does not issue actions. No automatic typing preview is advertised.

The local native ABI 1 transport and headers are adapted from the MIT-licensed
AmiHASS SDK (https://github.com/retrocs-org/amihass); see LICENSE. Requests have
bounded waits and safely abandon late replies using the SDK ownership contract.
