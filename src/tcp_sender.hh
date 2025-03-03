#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"
#include <queue>

class TCPSender
{
  Wrap32 isn_;
  uint64_t initial_RTO_ms_;
  uint64_t now_RTO_ms_;
  /* use a queue to buffer segments. */
  std::queue<TCPSenderMessage> seg_to_send{}, seg_sent{};
  /* we record the ackno and win_size received last time, also the tick time. */
  Wrap32 last_ackno, now_ackno; // record now_ackno for each sending.
  uint16_t last_win_size;
  uint64_t last_tick_time;
  /* for 2 functions. */
  uint64_t flight_seqno, retx_num;
  /* we need to know whether SYN or FIN is sent. */
  bool SYN_sent, FIN_sent;
  /* a TCPSender should be corresponded to a stream. */
  // Reader *ob_stream;
  /* for tick() to inform maybe_send(): retx now! */
  bool retx_now;
  // treat a '0' window size as equal to '1' but don't back off RTO
  bool RTO_no_backoff;

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};
