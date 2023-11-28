#pragma once

#include "reassembler.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <queue>

class TCPReceiver
{
private:
  bool SYN_arrives{false};
  bool FIN_arrives{false};
  Wrap32 begin_seqno{0};
  /* use a queue to buffer before SYN arrives. */
  // std::queue<TCPSenderMessage> message_buf{};
  /* this var is for unwrap, since the segment received last is probably close to next seg. */
  uint64_t last_seqno{0};
  /* FIN_seqno is recorded to correctly return ackno. */
  uint64_t FIN_seqno{0};

public:
  /*
   * The TCPReceiver receives TCPSenderMessages, inserting their payload into the Reassembler
   * at the correct stream index.
   */
  void receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream );

  /* The TCPReceiver sends TCPReceiverMessages back to the TCPSender. */
  TCPReceiverMessage send( const Writer& inbound_stream ) const;
};
