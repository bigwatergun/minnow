#include "network_interface.hh"

#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address ), ARP_table({}), 
  ARP_msg(), frame_to_send(), ARP_sent_time()
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  // (void)dgram;
  // (void)next_hop;

  /* create a prepared frame of datagram. */
  optional<uint32_t> has_ARP_item = std::nullopt;
  EthernetFrame frame;
  frame.header.src = ethernet_address_; // use self's address.
  frame.header.type = EthernetHeader::TYPE_IPv4;
  frame.payload = serialize(dgram);

  /* search the ip of next_hop in ARP_table. */
  if (ARP_table.find(next_hop.ipv4_numeric()) == ARP_table.end()) {
    /* construct an ARPMessage(not a frame). */
    // EthernetFrame ARP_frame_;
    // ARP_frame_.header.src = ethernet_address_; // use self's address.
    // ARP_frame_.header.dst = ETHERNET_BROADCAST;
    // ARP_frame_.header.type = EthernetHeader::TYPE_ARP;
    ARPMessage arp;
    arp.opcode = ARPMessage::OPCODE_REQUEST;
    arp.sender_ethernet_address = ethernet_address_;
    arp.sender_ip_address = ip_address_.ipv4_numeric();
    // arp.target_ethernet_address = target_ethernet_address;
    arp.target_ip_address = next_hop.ipv4_numeric();
    has_ARP_item = next_hop.ipv4_numeric();

    /* buffer an ARPMessage. */ 
    // ARP_frame_.payload = std::move(serialize(arp));
    ARP_msg.push(make_pair(arp, ETHERNET_BROADCAST));
  }
  else {
    frame.header.dst = ARP_table.at(next_hop.ipv4_numeric()).addr;
  }

  /* no matter how, we buffer this frame for the incoming sending. */
  frame_to_send.push_back(std::make_pair(frame, has_ARP_item));
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  // (void)frame;
  /* 100% accords to the check4.pdf */
  if (frame.header.dst == ETHERNET_BROADCAST || frame.header.dst == ethernet_address_) {
    if (frame.header.type == EthernetHeader::TYPE_IPv4) {
      InternetDatagram dgram;

      if (parse(dgram, frame.payload)) {
        return dgram;
      }
    }
    else if (frame.header.type == EthernetHeader::TYPE_ARP) {
      ARPMessage arp_recv;

      if (parse(arp_recv, frame.payload)) {
        ARP_item new_item;

        /* insert this new_item to ARP_table, rewriting the old if exists. */
        new_item.addr = arp_recv.sender_ethernet_address;
        new_item.last_tick_time = 0;
        ARP_table[arp_recv.sender_ip_address] = new_item;

        /* if apr is a request and target ip is us, we need to construct a reply. */
        if (arp_recv.opcode == ARPMessage::OPCODE_REQUEST && arp_recv.target_ip_address == ip_address_.ipv4_numeric( )) {
          ARPMessage arp_reply;
          arp_reply.opcode = ARPMessage::OPCODE_REPLY;
          arp_reply.sender_ethernet_address = ethernet_address_;
          arp_reply.sender_ip_address = ip_address_.ipv4_numeric();
          arp_reply.target_ethernet_address = arp_recv.sender_ethernet_address;
          arp_reply.target_ip_address = arp_recv.sender_ip_address;
          ARP_msg.push(make_pair(arp_reply, frame.header.src));

          /* buffer an ARP_frame. */
          // EthernetFrame ARP_frame_;
          // ARP_frame_.header.src = ethernet_address_; // use self's address.
          // ARP_frame_.header.dst = arp_recv.sender_ethernet_address;
          // ARP_frame_.header.type = EthernetHeader::TYPE_ARP;
          // ARP_frame_.payload = std::move(serialize(arp_reply));
        }
      }
    }
  }

  return std::nullopt;
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  // (void)ms_since_last_tick;
  /* add time to ARP_table. */
  for (auto it = ARP_table.begin(); it != ARP_table.end(); ) {
    it->second.last_tick_time += ms_since_last_tick;

    /* erase is tricky!!! */
    if (it->second.last_tick_time >= 30000) {
      it = ARP_table.erase(it);
    }
    else {
      it++;
    }
  }

  /* add time to ARP_sent_time. */
  for (auto it = ARP_sent_time.begin(); it != ARP_sent_time.end(); ) {
    it->second += ms_since_last_tick;

    /* erase is tricky!!! */
    if (it->second >= 5000) {
      it = ARP_sent_time.erase(it);
    }
    else {
      it++;
    }
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  /* if ARP_frame is empty, it means there is no ARP queries, we can send frames of datagram. */
  if (ARP_msg.size() != 0) {
    auto msg = ARP_msg.front();
    ARP_msg.pop();

    /* ARP_sent_time doesn't have an item, means that this ARP_req can be sent. */
    if (ARP_sent_time.find(msg.first.target_ip_address) == ARP_sent_time.end()) {
      ARP_sent_time[msg.first.target_ip_address] = 0;

      /* buffer an ARP_frame. */
      EthernetFrame ARP_frame_;
      ARP_frame_.header.src = ethernet_address_; // use self's address.
      ARP_frame_.header.dst = msg.second;
      ARP_frame_.header.type = EthernetHeader::TYPE_ARP;
      ARP_frame_.payload = std::move(serialize(msg.first));
      return ARP_frame_;
    }
    else { // item hasn't expired, abandon this ARP_msg.
      return std::nullopt;
    }
  }
  else if (frame_to_send.size() != 0) {
    /* go through vector till we find a frame which can be sent. */
    for (auto it = frame_to_send.begin(); it != frame_to_send.end(); it++) {
      // we have an ARP item for this frame when send_dgram is called. Just return it.
      if (!(it->second.has_value())) {
        auto ready_frame = it->first;
        // frame_to_send.erase((++it).base()); // from reversed_it to it. 
        frame_to_send.erase(it);
        return ready_frame;
      }

      /* search ip of this frame in ARP_table. */
      if (ARP_table.find(it->second.value()) == ARP_table.end()) {
        continue;
      }
      auto filled_frame = it->first;
      filled_frame.header.dst = ARP_table.at(it->second.value()).addr;
      // frame_to_send.erase((++it).base()); // from reversed_it to it. 
      frame_to_send.erase(it);
      return filled_frame;
    }
  }
  return std::nullopt;
}
