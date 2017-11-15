# RDT-3.0-Implementation
SENDER :

  1. The given file is read and each line is made into a packet, with the line string as the payload.
  2. Connection is setup with the reciever, using the reciever address and port number, by
     creating a socket, using socket() and connect().
  3. The packets are send to reciever using recv() at fixed time interval of 0.99 seconds.
  4. If the sequence number of the generated packet is within the pre-specified sender window,
     the packet is sent. If not, the packet is not sent.
  5. When a packet is sent, a thread is created which will wait to recieve the Ack for this packet,
     thus implementing Multithreading. This thread is implement with a select() with a pre-
     defined timeout value.
  6. If the user choses to simulate drop of this packet, a lower timeout value is used for this
     packet's Ack recieving thread.
  7. If a packet is lost, its thread retransmits the packet and waits for an Ack again, in which case
     a duplicate packet may be recived by reciever.
  8. When an Ack is recieved, the Send Base variable is updated to reflect cumulative
     acknowledgement, which is checked by the Sender while sending packets.
  9. If the user choses to simulate out of order packet, the packets are sent out of order.

RECIEVER:

  1. The reciever sets up a a socket using socket() and binds to the user provided port number.
  2. The reciever then waits listening for connections.
  3. When a connection comes, it is accepted and a new socket is created.
  4. The reciver then waits to recieve packets from the sender.
  5. If a packet is recieved, and the sequence number is within the recieve window, then the
     packet is stored and an Ack is sent to sender with the next expected sequence numbered
     packet, and if all packets before it are also recieved, all of the packets are written to file, and
     also the recieve base is updated.
  6. If not, the packet is buffered, and recieve base remains the same, until it is updated.
  7. If a packet with sequence number that was earlier recieved is recieved again, it is
     acknowledged as a duplicate packet, and an Ack is sent as the sender could have lost the
     previous Ack.

