#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  // (void)route_prefix;
  // (void)prefix_length;
  // (void)next_hop;
  // (void)interface_num;
  auto new_item = route_item(route_prefix, prefix_length, next_hop, interface_num);
  auto it = lower_bound(route_table.begin(), route_table.end(), new_item);

  /* keep the sorted order. */  
  route_table.insert(it, std::move(new_item));
}

void Router::route() {
  for (auto& it : interfaces_) {
    /* For each interface, use the maybe_receive() method to consume every incoming datagram */
    // for (auto dgram = it.maybe_receive(); dgram.has_value(); dgram = it.maybe_receive()) {
    while (optional<InternetDatagram> op_dgram = it.maybe_receive()) {
      InternetDatagram dgram = op_dgram.value();

      /* deal with the TTL of this dgram. */
      if (dgram.header.ttl == 1 || dgram.header.ttl == 0) {
        continue;
      }
      dgram.header.ttl--;
      dgram.header.compute_checksum(); // don't forget to recompute checksum, cause 10.1 bug!!!

      /* search the route for this dgram. */
      auto route_it = route_table.rbegin(); 
      for (; route_it != route_table.rend() - 1; route_it++) {
        uint8_t tar_offset = (uint8_t)32 - route_it->prefix_length;
        
        /* hit an item. */
        if (route_it->route_prefix >> tar_offset == dgram.header.dst >> tar_offset) {
          interface(route_it->interface_num).send_datagram(dgram, route_it->next_hop.value_or(Address::from_ipv4_numeric(dgram.header.dst)));
          return;
        }
      }

      /* if we didn't found an hit item for this dgram. Simply send it through 0.0.0.0 (suck werror, it makes runtime error: shift exponent 32 is too large for 32-bit type 'unsigned int) */
      interface(route_it->interface_num).send_datagram(dgram, route_it->next_hop.value_or(Address::from_ipv4_numeric(dgram.header.dst)));
    }
  }
}

/* debug 10.1 problem. */
std::string prettify_test( std::string_view str, size_t max_length )
{
  std::ostringstream ss;
  const std::string_view str_prefix = str.substr( 0, max_length );
  for ( const uint8_t ch : str_prefix ) {
    if ( isprint( ch ) ) {
      ss << ch;
    } else {
      ss << "\\x" << std::fixed << std::setw( 2 ) << std::setfill( '0' ) << std::hex << static_cast<size_t>( ch );
    }
  }
  if ( str.size() > str_prefix.size() ) {
    ss << "...";
  }
  return ss.str();
}

std::string concat( std::vector<Buffer>& buffers )
{
  return std::accumulate(
    buffers.begin(), buffers.end(), std::string {}, []( const std::string& x, const Buffer& y ) {
      return x + static_cast<std::string>( y );
    } );
}