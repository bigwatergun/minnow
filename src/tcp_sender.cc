#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms ), now_RTO_ms_(initial_RTO_ms),
    last_ackno(isn_), now_ackno(isn_), last_win_size(1), last_tick_time(0), flight_seqno(0), retx_num(0), 
    SYN_sent(false), FIN_sent(false), retx_now(false), RTO_no_backoff(false)
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  // Your code here.
  return flight_seqno;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  // Your code here.
  return retx_num;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  // Your code here.
  /* first we see whether retx is needed. */
  if (retx_now) {
    retx_now = false;
    /* corner case: an empty message needs retransmission. */
    if (seg_sent.size() == 0) {
      return {};
    }

    auto retx_message = seg_sent.front();
    if (!RTO_no_backoff) {
      now_RTO_ms_ *= 2;
    }
    return retx_message;
  }

  if (seg_to_send.size() == 0) {
    return {};
  }

  /* send message. */
  auto message = seg_to_send.front();
  seg_to_send.pop();
  seg_sent.push(message);
  return message;
}

void TCPSender::push( Reader& outbound_stream )
{
  // Your code here.
  // (void)outbound_stream;
  /* we keep cutting segments until last_win_size = 0. */
  while (last_win_size > 0) { 
    /* under 3 circumstances, we need to send sth. */
    if (!SYN_sent || (outbound_stream.is_finished() && !FIN_sent) || outbound_stream.bytes_buffered() > 0) {
      auto payload = outbound_stream.peek();
      auto payload_size_can_send = min(uint16_t(TCPConfig::MAX_PAYLOAD_SIZE), min(uint16_t(last_win_size - !SYN_sent), uint16_t(payload.length())));
      auto new_message = TCPSenderMessage();

      /* send SYN first. */
      if (!SYN_sent) {
        SYN_sent = true;
        new_message.SYN = true;
      }
      /* cut the target payload. */
      payload = payload.substr(0, payload_size_can_send);
      /* fill the Message. */
      new_message.payload = Buffer(string{payload}); // payload is a string_view.
      new_message.seqno = now_ackno;
      outbound_stream.pop(payload_size_can_send); // when we modify ob_stream, payload is also changed cuz it's string_view.
      // under 3 circumstances, we need to send FIN in this segment. */
      if (outbound_stream.bytes_buffered() == 0 && outbound_stream.is_finished() && new_message.sequence_length() < last_win_size) {
        FIN_sent = true;
        new_message.FIN = true;
      }

      /* update private variables. */
      now_ackno = now_ackno + new_message.sequence_length();
      seg_to_send.push(new_message);
      flight_seqno += new_message.sequence_length();
      last_win_size -= new_message.sequence_length();
    }
    /* we have nothing to send. */
    else break;
  }
}

TCPSenderMessage TCPSender::send_empty_message() const
{
  // Your code here.
  auto new_message = TCPSenderMessage();

  new_message.seqno = now_ackno;
  return new_message;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  // Your code here.
  // (void)msg;
  /* perhaps a ReceiverMessage has size but no ackno. */
  if (!msg.ackno.has_value()) {
    last_win_size = msg.window_size;
    return;
  }

  // Impossible ackno (beyond next seqno) is ignored.
  if (!(msg.ackno <= now_ackno))
    return;

  /* get the real win_size, considering ackno and win_size together. */  
  last_win_size = uint16_t(msg.ackno.value().subtract(now_ackno)) + msg.window_size;

  // treat a '0' window size as equal to '1' but don't back off RTO
  /* when msg.window_size=0, we can assume that seg_sent is empty. So retransmission only
  happens on this 1 byte payload segment. */
  if (msg.window_size == 0) {
    last_win_size = 1;
    RTO_no_backoff = true;
  }
  else {
    RTO_no_backoff = false;
  }

  /* go through seg_sent to ack segs if possible. */
  while (seg_sent.size() != 0) {
    auto temp_message = seg_sent.front();

    /* this message has been acknowledged. copied from check3.pdf 2.1.6. */
    if (temp_message.seqno + temp_message.sequence_length() <= msg.ackno) {
      seg_sent.pop();
      last_tick_time = 0;
      now_RTO_ms_ = initial_RTO_ms_; 
      retx_num = 0;
      flight_seqno -= temp_message.sequence_length();
    }
    else break;
  }
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  // Your code here.
  // (void)ms_since_last_tick;
  last_tick_time += ms_since_last_tick;

  /* completely flow the logic in check3.pdf 2.1.6 */
  if (last_tick_time >= now_RTO_ms_) {
    retx_now = true; // next time we call maybe_send(), it will retx.
    // if (last_win_size > 0) {
      retx_num++;
    // }
    last_tick_time = 0;
  } 
}
