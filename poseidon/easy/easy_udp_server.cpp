// This file is part of Poseidon.
// Copyleft 2022 - 2024, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_udp_server.hpp"
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
        IPv6_Address addr;
        linear_buffer data;
      };

    mutable plain_mutex mutex;
    deque<Packet> packets;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_UDP_Server::thunk_type m_thunk;
    wkptr<Packet_Queue> m_wqueue;

    Final_Fiber(const Easy_UDP_Server::thunk_type& thunk, shptrR<Packet_Queue> queue)
      :
        m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_on_abstract_fiber_execute() override
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
          auto packet = move(queue->packets.front());
          queue->packets.pop_front();
          lock.unlock();

          try {
            this->m_thunk(socket, *this, move(packet.addr), move(packet.data));
          }
          catch(exception& stdex) {
            POSEIDON_LOG_ERROR((
                "Unhandled exception thrown from UDP server: $1"),
                stdex);
          }
        }
      }
  };

struct Final_Socket final : UDP_Socket
  {
    Easy_UDP_Server::thunk_type m_thunk;
    wkptr<Packet_Queue> m_wqueue;

    Final_Socket(const Easy_UDP_Server::thunk_type& thunk, const IPv6_Address& addr,
                 shptrR<Packet_Queue> queue)
      :
        UDP_Socket(addr), m_thunk(thunk), m_wqueue(queue)
      { }

    virtual
    void
    do_on_udp_packet(IPv6_Address&& addr, linear_buffer&& data) override
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

POSEIDON_HIDDEN_X_STRUCT(Easy_UDP_Server,
  Packet_Queue);

Easy_UDP_Server::
~Easy_UDP_Server()
  {
  }

shptr<UDP_Socket>
Easy_UDP_Server::
start(chars_view addr)
  {
    // Parse the listen address string.
    IPv6_Address saddr;
    if(saddr.parse(addr) != addr.n)
      POSEIDON_THROW(("Invalid local IP address `$1`"), addr);

    // Initiate the server.
    auto queue = new_sh<X_Packet_Queue>();
    auto socket = new_sh<Final_Socket>(this->m_thunk, saddr, queue);
    queue->wsocket = socket;

    network_driver.insert(socket);
    this->m_queue = move(queue);
    this->m_socket = socket;
    return socket;
  }

void
Easy_UDP_Server::
stop() noexcept
  {
    this->m_queue = nullptr;
    this->m_socket = nullptr;
  }

}  // namespace poseidon
