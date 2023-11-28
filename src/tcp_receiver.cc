#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  // Your code here.
  // (void)message;
  // (void)reassembler;
  // (void)inbound_stream;
  uint64_t unwrap_seqno;

  /* with buf version(not neeeded). */
  // if (message.SYN) {
  //   SYN_arrives = true;
  //   begin_seqno = message.seqno;

  //   /* clear the messages buffer. */
  //   while (message_buf.size() != 0) {
  //     TCPSenderMessage temp = message_buf.front();
  //     message_buf.pop();
  //     unwrap_seqno = temp.seqno.unwrap(begin_seqno, last_seqno);
  //     reassembler.insert(unwrap_seqno, temp.payload, temp.FIN, inbound_stream);
  //     last_seqno = unwrap_seqno;
  //   }
  // }

  // /* then we handle this message. */
  // if (SYN_arrives) {
  //   unwrap_seqno = message.seqno.unwrap(begin_seqno, last_seqno);
  //   reassembler.insert(unwrap_seqno, message.payload, message.FIN, inbound_stream);
  //   last_seqno = unwrap_seqno;
  // }
  // else {
  //   message_buf.push(message);
  // }

  if (message.SYN) {
    SYN_arrives = true;
    begin_seqno = message.seqno;
  }

  /* then we handle this message only if SYN arrives. */
  if (SYN_arrives) {
    unwrap_seqno = message.seqno.unwrap(begin_seqno, last_seqno);
    /* check FIN only after SYN arrives, cuz we abandoned previous segments. */
    if (message.FIN) {
      FIN_arrives = true;
      FIN_seqno = unwrap_seqno; 
    }
    last_seqno = unwrap_seqno;

    /* if this segment is SYN or FIN, its payload must be empty, and dosen't have stream index. */
    /* we need FIN, cuz it represents is_closed flag. */
    uint64_t stream_index = unwrap_seqno - 1;
    if (message.SYN) // when there is SYN, we make index 0 to insert "".
      stream_index = 0;
    reassembler.insert(stream_index, message.payload, message.FIN, inbound_stream);
  }
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{ 
  // Your code here.
  // (void)inbound_stream;

  TCPReceiverMessage ret;
  uint64_t desired_index = inbound_stream.bytes_popped() + inbound_stream.bytes_buffered();

  if (SYN_arrives)
    /* if desired_index + 1 exceeds FIN_seq, it means that an additional 1 needed. */
    ret.ackno = Wrap32::wrap(desired_index + 1 + (FIN_arrives && desired_index + 1 >= FIN_seqno? 1: 0) , begin_seqno); // this var(desired_index) appears in Reassembler.
  /* window_size has a strange constraint: uint16_t. */
  ret.window_size = min(static_cast<uint64_t>(UINT16_MAX)  - inbound_stream.bytes_buffered(), inbound_stream.available_capacity());
  return ret;
}
