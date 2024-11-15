# ps4-devkit-payloads
Useful payloads for PS4 devkit, It is mostly modified existing retail payloads. Thanks original author.
Thanks z80 for his devkit ported kernel exploit code.


for 5.05


d = devkit

t = testkit

# Payload list
- Activate: RTC revert / reactPSPlus method, use this when possible. Persistent.
- Activate (noActCode): for kit without activation token such as brand new, must be used together with HEN to launch apps. Not persistent.
- FTP: Bugs are now fixed such as SELF dumping and stat.
- HEN: for fpkgs, also fixed problem where official sdk tools can't find process for debugging (need to change some bytes inside fake self header to work)
- HEN Spoof: HEN with firmware version spoofed, for backports fpkg (may broke something else, not sure), more debug settings
- Loader: aka Bin loader. port 9020.
- Linux loader
- kdump: dump kernel to specified ip inside userland.js
- Original: kernel exploit only
- Enable browser: Enable browser for lower firmware
- UART Enabler: Enable UART port. You can use the "Mini" USB port marked "DEV" at the back to get full UART access without soldering. Install correct driver, Run payload, turn it off and on again (Not reboot!). See: https://www.psdevwiki.com/ps4/Devkit_USB_Uart

Recommended payload host: https://github.com/Al-Azif/ps4-exploit-host/releases/tag/v0.4.5
extract exploits directory to payload host directory.


# Notes

Payload sdk = https://github.com/aomsin2526/ps4-payload-sdk/tree/86276f8a8946af34170744832fc8e7afd1d58f7a

export PS4SDK=sdk root dir

1.76 need older ones, not sure which (maybe CTurt)
