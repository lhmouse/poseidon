// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_udp_server.hpp"
#include "../socket/udp_socket.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../static/async_logger.hpp"
#include "../utils.hpp"

namespace poseidon {
namespace {

using thunk_function = void (void*, Socket_Address&&, linear_buffer&&);
class Final_Fiber;

struct Packet
  {
    Socket_Address addr;
    linear_buffer data;
  };

struct Packet_Queue
  {
    mutable plain_mutex m_mutex;
    deque<Packet> m_packets;
    bool m_fiber_active = false;
  };

struct Final_UDP_Socket final : UDP_Socket
  {
    thunk_function* m_cb_thunk;
    weak_ptr<void> m_cb_wobj;
    weak_ptr<void> m_wuniq;

    shared_ptr<Packet_Queue> m_queue;

    explicit
    Final_UDP_Socket(const Socket_Address& addr, thunk_function* cb_thunk, shared_ptrR<void> cb_obj, shared_ptrR<void> uniq)
      : UDP_Socket(addr),
        m_cb_thunk(cb_thunk), m_cb_wobj(cb_obj), m_wuniq(uniq)
      { }

    virtual
    void
    do_on_udp_packet(Socket_Address&& addr, linear_buffer&& data) override;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    thunk_function* m_cb_thunk;
    weak_ptr<void> m_cb_wobj;
    weak_ptr<void> m_wuniq;
    shared_ptr<Packet_Queue> m_queue;

    explicit
    Final_Fiber(const Final_UDP_Socket& socket)
      : m_cb_thunk(socket.m_cb_thunk), m_cb_wobj(socket.m_cb_wobj), m_wuniq(socket.m_wuniq), m_queue(socket.m_queue)
      { }

    virtual
    void
    do_abstract_fiber_on_work() override;
  };

void
Final_UDP_Socket::
do_on_udp_packet(Socket_Address&& addr, linear_buffer&& data)
  {
    if(this->m_wuniq.expired())
      return;

    if(!this->m_queue)
      this->m_queue = ::std::make_shared<Packet_Queue>();

    // We are in the network thread here.
    plain_mutex::unique_lock lock(this->m_queue->m_mutex);

    if(!this->m_queue->m_fiber_active) {
      // Create a new fiber, if none is active. The fiber shall only reset
      // `m_fiber_active` if no packet is pending.
      fiber_scheduler.insert(::std::make_unique<Final_Fiber>(*this));
      this->m_queue->m_fiber_active = true;
    }

    auto& packet = this->m_queue->m_packets.emplace_back();
    packet.addr.swap(addr);
    packet.data.swap(data);
  }

void
Final_Fiber::
do_abstract_fiber_on_work()
  {
    for(;;) {
      // The packet callback may stop this server, so we have to check for expiry
      // in every iteration.
      if(this->m_wuniq.expired())
        return;

      auto cb_obj = this->m_cb_wobj.lock();
      if(!cb_obj)
        return;

      // We are in the main thread here.
      plain_mutex::unique_lock lock(this->m_queue->m_mutex);

      if(this->m_queue->m_packets.empty()) {
        // Leave now.
        this->m_queue->m_fiber_active = false;
        return;
      }

      auto packet = ::std::move(this->m_queue->m_packets.front());
      this->m_queue->m_packets.pop_front();

      lock.unlock();

      // Invoke the user-defined callback.
      try {
        this->m_cb_thunk(cb_obj.get(), ::std::move(packet.addr), ::std::move(packet.data));
      }
      catch(exception& stdex) {
        POSEIDON_LOG_ERROR((
            "Unhandled exception thrown from easy UDP server: $1"),
            stdex);
      }
    }
  }

}  // namespace

Easy_UDP_Server::
~Easy_UDP_Server()
  {
  }

void
Easy_UDP_Server::
start(const Socket_Address& addr)
  {
    this->m_uniq = ::std::make_shared<int>();
    this->m_socket = ::std::make_shared<Final_UDP_Socket>(addr, this->m_cb_thunk, this->m_cb_obj, this->m_uniq);
    network_driver.insert(this->m_socket);
  }

void
Easy_UDP_Server::
start(stringR host_port)
  {
    Socket_Address addr(host_port);
    this->start(addr);
  }

void
Easy_UDP_Server::
start(uint16_t port)
  {
    Socket_Address addr((::in6_addr) IN6ADDR_ANY_INIT, port);
    this->start(addr);
  }

void
Easy_UDP_Server::
stop() noexcept
  {
    this->m_uniq = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_UDP_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_unspecified;

    return this->m_socket->local_address();
  }

void
Easy_UDP_Server::
join_multicast_group(const Socket_Address& maddr, uint8_t ttl, bool loopback, const char* ifname_opt)
  {
    if(!this->m_socket)
      POSEIDON_THROW((
          "Server not running",
          "[easy UDP server `$1` (class `$2`)]"),
          this, typeid(*this));

    this->m_socket->join_multicast_group(maddr, ttl, loopback, ifname_opt);
  }

void
Easy_UDP_Server::
leave_multicast_group(const Socket_Address& maddr, const char* ifname_opt)
  {
    if(!this->m_socket)
      POSEIDON_THROW((
          "Server not running",
          "[easy UDP server `$1` (class `$2`)]"),
          this, typeid(*this));

    this->m_socket->leave_multicast_group(maddr, ifname_opt);
  }

bool
Easy_UDP_Server::
send(const Socket_Address& addr, const char* data, size_t size)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->udp_send(addr, data, size);
  }

bool
Easy_UDP_Server::
send(const Socket_Address& addr, const linear_buffer& data)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->udp_send(addr, data.data(), data.size());
  }

bool
Easy_UDP_Server::
send(const Socket_Address& addr, const cow_string& data)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->udp_send(addr, data.data(), data.size());
  }

bool
Easy_UDP_Server::
send(const Socket_Address& addr, const string& data)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->udp_send(addr, data.data(), data.size());
  }

}  // namespace poseidon
