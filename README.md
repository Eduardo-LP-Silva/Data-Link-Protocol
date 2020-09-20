# Data-Link-Protocol
Application based on the data link protocol, it allows for the transfer of files between two computers connected through a serial port. It features error detection and correction mechanisms (a mix of Stop-&-Wait and Go-Back-N) in case the connection is temporarily lost and/or corrupted.

## Usage
The receiving computer must first open a connection. After that the transmitting computer can commence the transfer. During this process, the transmitting computer sees how many bytes are left to transfer while in the receiving computer a progress bar alongside a percentage indicator is present.
