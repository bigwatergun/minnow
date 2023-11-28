#include "reassembler.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
  // Your code here.
  // (void)first_index;
  // (void)data;
  // (void)is_last_substring;
  // (void)output;

  uint64_t desired_index = output.bytes_popped() + output.bytes_buffered();
  uint64_t cap_limit_index = desired_index + output.available_capacity();

  /* we mark the index of last_substring. */
  if (is_last_substring) {
    last_index = first_index + data.size();
    if (desired_index >= last_index) { // for corner case: "" and is_last_substring is true.
      output.close();
      return;
    }
  }

  /* first_index cannot exceed the upbound, so do end_index. */
  uint64_t end_index = first_index + data.size() - 1; // end_index is the exact place of last char.
  if (end_index < desired_index || first_index >= cap_limit_index || data.size() == 0 || output.available_capacity() == 0) {
    return;
  }
    
  /* we always make sequence at a fixed length(available_cap). */
  if (seq.size() < output.available_capacity()) {
    uint64_t left_size = output.available_capacity() - seq.size();
    seq.append(left_size, char(0)); // 0 is simply a token to preserve the position.
    seq_inf.insert(seq_inf.end(), left_size, false);
  }

  /* Step1. simply insert data to sequence. We can imagine there are 2 strings: data and sequence, 
     now we need to overlap them, merge data into sequence. */
  uint64_t seq_start_place, data_start_place, data_end_place;

  /* handle data's front. */
  if (first_index < desired_index) {
    seq_start_place = 0;
    data_start_place = desired_index - first_index;
  }
  else {
    seq_start_place = first_index - desired_index;
    data_start_place = 0;
  }

  /* handle data's rear. */
  data_end_place = std::min(cap_limit_index - first_index, data.size());

  /* replace part of sequence with data. */
  string replace_data = data.substr(data_start_place, data_end_place - data_start_place);

  /* count the number of pending char. */
  for (uint64_t i = 0; i < replace_data.size(); i++) {
    if (seq_inf[seq_start_place + i] == false) {
      pending_bytes++;
      seq_inf[seq_start_place + i] = true;
    }
  }
  seq.replace(seq_start_place, replace_data.size(), replace_data);

  /* Step2. move sequence to bytestream from head, until we see a false(missing byte). */
  uint64_t stop_index = seq_inf.size();
  for (uint64_t i = 0; i < seq_inf.size(); i++) {
    if (seq_inf[i] == false) {
      stop_index = i;
      break;
    }
  }
  if (stop_index > 0) {
    output.push(seq.substr(0, stop_index));
    seq.erase(seq.begin(), seq.begin() + stop_index);
    seq_inf.erase(seq_inf.begin(), seq_inf.begin() + stop_index);
    pending_bytes -= stop_index;
    /* after push, we arrive at the last_index. */
    if (desired_index + stop_index >= last_index)
      output.close(); 
  }
}

uint64_t Reassembler::bytes_pending() const
{
  // Your code here.
  return {pending_bytes};
}
