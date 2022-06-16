# ps4-devkit-payloads
Useful payloads for PS4 devkit, It is mostly modified existing retail payloads.


# Payload list
- Activate: RTC revert / reactPSPlus method, use this when possible. Persistent.
- Activate (noActCode): for kit without activation token such as brand new, must be used together with HEN to launch apps. Not persistent.
- FTP
- HEN: for fpkgs, also fixed problem where official sdk tools can't find process for debugging (need to change some bytes inside fake self header to work)
- Loader: aka Bin loader. port 9020.
- kdump: dump kernel to specified ip inside userland.js
- Original: kernel exploit only

Recommended payload host: https://github.com/Al-Azif/ps4-exploit-host/releases/tag/v0.4.0RC1
extract exploits directory to payload host directory.
