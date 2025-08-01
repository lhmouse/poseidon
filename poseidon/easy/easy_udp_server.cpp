// This file is part of Poseidon.
// Copyright (C) 2022-2025, LH_Mouse. All wrongs reserved.

#include "../xprecompiled.hpp"
#include "easy_udp_server.hpp"
#include "../static/network_scheduler.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../utils.hpp"
#include <deque>
namespace poseidon {
namespace {

struct Packet
  {
    IPv6_Address addr;
    linear_buffer data;
  };

struct Packet_Queue
  {
    // read-only fields; no locking needed
    wkptr<UDP_Socket> wsocket;
    cacheline_barrier xcb_1;

    // shared fields between threads
    mutable plain_mutex mutex;
    ::std::deque<Packet> packets;
    bool fiber_active = false;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Easy_UDP_Server::callback_type m_callback;
    wkptr<Packet_Queue> m_wqueue;

    Final_Fiber(const Easy_UDP_Server::callback_type& callback,
                const shptr<Packet_Queue>& queue)
      :
        m_callback(callback), m_wqueue(queue)
      { }

    virtual
    void
    do_on_abstract_fiber_execute()
      override
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
            this->m_callback(socket, *this, move(packet.addr), move(packet.data));
          }
          catch(exception& stdex) {
            POSEIDON_LOG_ERROR(("Unhandled exception: $1"), stdex);
          }
        }
      }
  };

struct Final_Socket final : UDP_Socket
  {
    Easy_UDP_Server::callback_type m_callback;
    wkptr<Packet_Queue> m_wqueue;

    Final_Socket(const IPv6_Address& addr,
                 const Easy_UDP_Server::callback_type& callback,
                 const shptr<Packet_Queue>& queue)
      :
        UDP_Socket(addr),
        m_callback(callback), m_wqueue(queue)
      { }

    virtual
    void
    do_on_udp_packet(IPv6_Address&& addr, linear_buffer&& data)
      override
      {
        auto queue = this->m_wqueue.lock();
        if(!queue)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(queue->mutex);

        if(!queue->fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_active` if no packet is pending.
          fiber_scheduler.launch(new_sh<Final_Fiber>(this->m_callback, queue));
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

const IPv6_Address&
Easy_UDP_Server::
local_address()
  const noexcept
  {
    if(!this->m_socket)
      return ipv6_unspecified;

    return this->m_socket->local_address();
  }

shptr<UDP_Socket>
Easy_UDP_Server::
start(const IPv6_Address& addr, const callback_type& callback)
  {
    auto queue = new_sh<X_Packet_Queue>();
    auto socket = new_sh<Final_Socket>(addr, callback, queue);
    queue->wsocket = socket;

    network_scheduler.insert_weak(socket);
    this->m_queue = move(queue);
    this->m_socket = socket;
    return socket;
  }

shptr<UDP_Socket>
Easy_UDP_Server::
start(const cow_string& addr, const callback_type& callback)
  {
    return this->start(IPv6_Address(addr), callback);
  }

shptr<UDP_Socket>
Easy_UDP_Server::
start(uint16_t port, const callback_type& callback)
  {
    return this->start(IPv6_Address(ipv6_unspecified, port), callback);
  }

void
Easy_UDP_Server::
stop()
  noexcept
  {
    this->m_queue = nullptr;
    this->m_socket = nullptr;
  }

}  // namespace poseidon
