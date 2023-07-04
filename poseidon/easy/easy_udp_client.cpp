// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_udp_client.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

struct Packet_Queue
  {
    // read-only fields; no locking needed
    wkptr<UDP_Socket> wsocket;
    cacheline_barrier xcb_1;

    // shared fields between threads
    struct Packet
      {
        Socket_Address addr;
        linear_buffer data;
      };

    mutable plain_mutex mutex;
    deque<Packet> packets;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_UDP_Client::thunk_type m_thunk;
    wkptr<Packet_Queue> m_wqueue;

    explicit
    Final_Fiber(const Easy_UDP_Client::thunk_type& thunk, const shptr<Packet_Queue>& queue)
      : m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_abstract_fiber_on_work()
      {
        for(;;) {
          // The packet callback may stop this server, so we have to check for
          // expiry in every iteration.
          auto queue = this->m_wqueue.lock();
          if(!queue)
            return;

          auto socket = queue->wsocket.lock();
          if(!socket)
            return;

          // Pop an event and invoke the user-defined callback here in the
          // main thread. Exceptions are ignored.
          plain_mutex::unique_lock lock(queue->mutex);

          if(queue->packets.empty()) {
            // Terminate now.
            queue->fiber_active = false;
            return;
          }

          ROCKET_ASSERT(queue->fiber_active);
          auto packet = ::std::move(queue->packets.front());
          queue->packets.pop_front();
          lock.unlock();

          try {
            this->m_thunk(socket, *this, ::std::move(packet.addr), ::std::move(packet.data));
          }
          catch(exception& stdex) {
            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from easy UDP server: $1"),
                stdex);
          }
        }
      }
  };

struct Final_UDP_Socket final : UDP_Socket
  {
    Easy_UDP_Client::thunk_type m_thunk;
    wkptr<Packet_Queue> m_wqueue;

    explicit
    Final_UDP_Socket(const Easy_UDP_Client::thunk_type& thunk, const shptr<Packet_Queue>& queue)
      : UDP_Socket(), m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_on_udp_packet(Socket_Address&& addr, linear_buffer&& data) override
      {
        auto queue = this->m_wqueue.lock();
        if(!queue)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(queue->mutex);

        if(!queue->fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_active` if no packet is pending.
          fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_thunk, queue));
          queue->fiber_active = true;
        }

        auto& packet = queue->packets.emplace_back();
        packet.addr.swap(addr);
        packet.data.swap(data);
      }
  };

}  // namespace

POSEIDON_HIDDEN_X_STRUCT(Easy_UDP_Client, Packet_Queue);

Easy_UDP_Client::
~Easy_UDP_Client()
  {
  }

void
Easy_UDP_Client::
open()
  {
    auto queue = new_sh<X_Packet_Queue>();
    auto socket = new_sh<Final_UDP_Socket>(this->m_thunk, queue);
    queue->wsocket = socket;

    network_driver.insert(socket);
    this->m_queue = ::std::move(queue);
    this->m_socket = ::std::move(socket);
  }

void
Easy_UDP_Client::
close() noexcept
  {
    this->m_queue = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_UDP_Client::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

    return this->m_socket->local_address();
  }

void
Easy_UDP_Client::
connect(const Socket_Address& addr)
  {
    if(!this->m_socket)
      POSEIDON_THROW(("Client not running"));

    this->m_socket->connect(addr);
  }

void
Easy_UDP_Client::
join_multicast_group(const Socket_Address& maddr, uint8_t ttl, bool loopback, const char* ifname_opt)
  {
    if(!this->m_socket)
      POSEIDON_THROW(("Client not running"));

    this->m_socket->join_multicast_group(maddr, ttl, loopback, ifname_opt);
  }

void
Easy_UDP_Client::
leave_multicast_group(const Socket_Address& maddr, const char* ifname_opt)
  {
    if(!this->m_socket)
      POSEIDON_THROW(("Client not running"));

    this->m_socket->leave_multicast_group(maddr, ifname_opt);
  }

bool
Easy_UDP_Client::
udp_send(const Socket_Address& addr, const char* data, size_t size)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->udp_send(addr, data, size);
  }

}  // namespace poseidon
