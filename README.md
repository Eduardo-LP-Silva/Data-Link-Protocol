# Data-Link-Protocol
An application based on the data link protocol developed in the Computer Networks (RCOM) class. It allows for the transfer of files between two computers connected through a serial port. It features error detection and correction mechanisms (a mix of Stop-&-Wait and Go-Back-N) in case the connection is temporarily lost and/or corrupted.

## Usage
The receiving computer must first open a connection. After that the transmitting computer can commence the transfer. During this process, the transmitting computer sees how many bytes are left to transfer while in the receiving computer a progress bar alongside a percentage indicator is present.

![image](https://user-images.githubusercontent.com/32617691/52183960-260f5f80-2805-11e9-8c39-4ea6683af975.png)
