#include <stdexcept>

#include "byte_stream.hh"
#include <iostream>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ), stream(), close_sig(false), error_sig(false), 
  bytes_pushed_(0), bytes_popped_(0) {}

void Writer::push( string data )
{
  // Your code here.
  // (void)data;
  uint64_t size_can_push = std::min(this->available_capacity(), data.size());

  // for (uint64_t i = 0; i < size_can_push; i++) {
  //   // stream.push_back(data[i]);
  //   stream.emplace_back(data[i]);
  // }
  // stream.insert(stream.end(), data.begin(), data.begin() + size_can_push); // slower!
  stream.append(data.substr(0, size_can_push));
  bytes_pushed_ += size_can_push;
}

void Writer::close()
{
  // Your code here.
  close_sig = true;
}

void Writer::set_error()
{
  // Your code here.
  error_sig = true;
}

bool Writer::is_closed() const
{
  // Your code here.
  return close_sig;
}

uint64_t Writer::available_capacity() const
{
  // Your code here.
  return capacity_ - stream.size();
}

uint64_t Writer::bytes_pushed() const
{
  // Your code here.
  return bytes_pushed_;
}

string_view Reader::peek() const
{
  // Your code here.
  // string ret = "";

  // for (uint64_t i = 0; i < stream.size(); i++) {
  //   ret += stream[i];
  // }
  return stream;
}

bool Reader::is_finished() const
{
  // Your code here.
  return close_sig and stream.size() == 0;
}

bool Reader::has_error() const
{
  // Your code here.
  return error_sig;
}

void Reader::pop( uint64_t len )
{
  // Your code here.
  // (void)len;
  uint64_t size_can_pop = std::min(stream.size(), len);

  // for (uint64_t i = 0; i < size_can_pop; i++) {
  //   stream.pop_front();
  // }
  // stream.erase(stream.begin(), stream.begin() + size_can_pop);
  stream.erase(stream.begin(), stream.begin() + len);
  bytes_popped_ += size_can_pop;
}

uint64_t Reader::bytes_buffered() const
{
  // Your code here.
  return stream.size();
}

uint64_t Reader::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}

/* we want writer to access protected var in reassembler. */
uint64_t Writer::bytes_buffered() const
{
  // Your code here.
  return stream.size();
}

uint64_t Writer::bytes_popped() const
{
  // Your code here.
  return bytes_popped_;
}