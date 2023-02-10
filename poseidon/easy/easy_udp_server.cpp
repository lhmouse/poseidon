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

struct Packet_Queue
  {
    mutable plain_mutex mutex;
    weak_ptr<UDP_Socket> wsocket;  // read-only; no locking needed

    struct Packet
      {
        Socket_Address addr;
        linear_buffer data;
      };

    deque<Packet> packets;
    bool fiber_active = false;
  };

struct Shared_cb_args
  {
    weak_ptr<void> wobj;
    callback_thunk_ptr<shared_ptrR<UDP_Socket>, Socket_Address&&, linear_buffer&&> thunk;
    weak_ptr<Packet_Queue> wqueue;
  };

struct Final_Fiber final : Abstract_Fiber
  {
    Shared_cb_args m_cb;

    explicit
    Final_Fiber(const Shared_cb_args& cb)
      : m_cb(cb)
      { }

    virtual
    void
    do_abstract_fiber_on_work()
      {
        for(;;) {
          auto cb_obj = this->m_cb.wobj.lock();
          if(!cb_obj)
            return;

          // The packet callback may stop this server, so we have to check for
          // expiry in every iteration.
          auto queue = this->m_cb.wqueue.lock();
          if(!queue)
            return;

          // We are in the main thread here.
          plain_mutex::unique_lock lock(queue->mutex);

          auto socket = queue->wsocket.lock();
          if(!socket)
            return;

          if(queue->packets.empty()) {
            // Leave now.
            queue->fiber_active = false;
            return;
          }

          ROCKET_ASSERT(queue->fiber_active);
          auto packet = ::std::move(queue->packets.front());
          queue->packets.pop_front();

          lock.unlock();
          queue = nullptr;

          try {
            // Invoke the user-defined data callback.
            this->m_cb.thunk(cb_obj.get(), socket, ::std::move(packet.addr), ::std::move(packet.data));
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
    Shared_cb_args m_cb;

    explicit
    Final_UDP_Socket(const Socket_Address& addr, Shared_cb_args&& cb)
      : UDP_Socket(addr), m_cb(::std::move(cb))
      { }

    virtual
    void
    do_on_udp_packet(Socket_Address&& addr, linear_buffer&& data) override
      {
        auto queue = this->m_cb.wqueue.lock();
        if(!queue)
          return;

        // We are in the network thread here.
        plain_mutex::unique_lock lock(queue->mutex);

        if(!queue->fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_active` if no packet is pending.
          fiber_scheduler.insert(::std::make_unique<Final_Fiber>(this->m_cb));
          queue->fiber_active = true;
        }

        auto& packet = queue->packets.emplace_back();
        packet.addr.swap(addr);
        packet.data.swap(data);
      }
  };

}  // namespace

struct Easy_UDP_Server::X_Packet_Queue : Packet_Queue
  {
  };

Easy_UDP_Server::
~Easy_UDP_Server()
  {
  }

void
Easy_UDP_Server::
start(const Socket_Address& addr)
  {
    auto queue = ::std::make_shared<X_Packet_Queue>();
    Shared_cb_args cb = { this->m_cb_obj, this->m_cb_thunk, queue };
    auto socket = ::std::make_shared<Final_UDP_Socket>(addr, ::std::move(cb));
    queue->wsocket = socket;

    network_driver.insert(socket);
    this->m_queue = ::std::move(queue);
    this->m_socket = ::std::move(socket);
  }

void
Easy_UDP_Server::
stop() noexcept
  {
    this->m_queue = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_UDP_Server::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_invalid;

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
udp_send(const Socket_Address& addr, const char* data, size_t size)
  {
    if(!this->m_socket)
      return false;

    return this->m_socket->udp_send(addr, data, size);
  }

}  // namespace poseidon
