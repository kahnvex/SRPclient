SRPclient
=========

This is an implementation of the sliding window protocol for a client. The server implementation is at https://github.com/kahnjw/SRPserver This application transfers files over a wire using the C sockets API and other low level functionality.

##From Wikipedia
A sliding window protocol is a feature of packet-based data transmission protocols. Sliding window protocols are used where reliable in-order delivery of packets is required, such as in the Data Link Layer (OSI model) as well as in the Transmission Control Protocol (TCP).

Conceptually, each portion of the transmission (packets in most data link layers, but bytes in TCP) is assigned a unique consecutive sequence number, and the receiver uses the numbers to place received packets in the correct order, discarding duplicate packets and identifying missing ones. The problem with this is that there is no limit of the size of the sequence numbers that can be required.
