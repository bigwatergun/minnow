#include "wrapping_integers.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  // Your code here.
  // (void)n;
  // (void)zero_point;

  uint64_t n_with_offset = n + (uint64_t)zero_point.raw_value_;
  uint32_t cut_off = (uint32_t)n_with_offset;
  return Wrap32 { cut_off };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  // Your code here.
  // (void)zero_point;
  // (void)checkpoint;

  /* get the minimum offset value. */
  uint32_t min_distance = this->raw_value_ - zero_point.raw_value_;

  /* get 3 possible heads of final result. */
  uint64_t possible_offset = uint64_t(1) << 32;
  uint64_t head_1 = (checkpoint & (~uint64_t(0) << 32)) - possible_offset;
  uint64_t head_2 = checkpoint & (~uint64_t(0) << 32);
  uint64_t head_3 = (checkpoint & (~uint64_t(0) << 32)) + possible_offset;

  /* see which one is the closest. */
  uint64_t head_1_ans = head_1 | (uint64_t)min_distance;
  uint64_t head_2_ans = head_2 | (uint64_t)min_distance;
  uint64_t head_3_ans = head_3 | (uint64_t)min_distance;
  uint64_t head_1_dis, head_2_dis, head_3_dis;

  /* codes below can be simplified into 2 lines. */
  if (head_1_ans > checkpoint)  
    head_1_dis = head_1_ans - checkpoint;
  else
    head_1_dis = checkpoint - head_1_ans;
  if (head_2_ans > checkpoint)  
    head_2_dis = head_2_ans - checkpoint;
  else
    head_2_dis = checkpoint - head_2_ans;
  if (head_3_ans > checkpoint)  
    head_3_dis = head_3_ans - checkpoint;
  else
    head_3_dis = checkpoint - head_3_ans;

  if (head_1_dis < head_2_dis)
    if (head_1_dis < head_3_dis)
      return head_1_ans;
    else
      return head_3_ans;
  else
    if (head_2_dis < head_3_dis)
      return head_2_ans;
    else
      return head_3_ans;
}
