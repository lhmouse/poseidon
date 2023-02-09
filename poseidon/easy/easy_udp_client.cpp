// This file is part of Poseidon.
// Copyleft 2022 - 2023, LH_Mouse. All wrongs reserved.

#include "../precompiled.ipp"
#include "easy_udp_client.hpp"
#include "../socket/udp_socket.hpp"
#include "../static/network_driver.hpp"
#include "../fiber/abstract_fiber.hpp"
#include "../static/fiber_scheduler.hpp"
#include "../static/async_logger.hpp"
#include "../utils.hpp"
namespace poseidon {
namespace {

using thunk_function = void (void*, Socket_Address&&, linear_buffer&&);

struct Packet
  {
    Socket_Address addr;
    linear_buffer data;
  };

struct Packet_Queue
  {
    mutable plain_mutex mutex;
    deque<Packet> packets;
    bool fiber_active = false;
  };

struct Shared_cb_args
  {
    thunk_function* thunk;
    weak_ptr<void> wobj;
    weak_ptr<void> wuniq;
    shared_ptr<Packet_Queue> q;
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
          // The packet callback may stop this server, so we have to check for expiry
          // in every iteration.
          if(this->m_cb.wuniq.expired())
            return;

          auto cb_obj = this->m_cb.wobj.lock();
          if(!cb_obj)
            return;

          // We are in the main thread here.
          plain_mutex::unique_lock lock(this->m_cb.q->mutex);

          if(this->m_cb.q->packets.empty()) {
            // Leave now.
            this->m_cb.q->fiber_active = false;
            return;
          }

          auto packet = ::std::move(this->m_cb.q->packets.front());
          this->m_cb.q->packets.pop_front();
          lock.unlock();

          try {
            // Invoke the user-defined data callback.
            this->m_cb.thunk(cb_obj.get(), ::std::move(packet.addr), ::std::move(packet.data));
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
    Final_UDP_Socket(Shared_cb_args&& cb)
      : UDP_Socket(), m_cb(::std::move(cb))
      { }

    virtual
    void
    do_on_udp_packet(Socket_Address&& addr, linear_buffer&& data) override
      {
        if(this->m_cb.wuniq.expired())
          return;

        if(!this->m_cb.q)
          this->m_cb.q = ::std::make_shared<Packet_Queue>();

        // We are in the network thread here.
        plain_mutex::unique_lock lock(this->m_cb.q->mutex);

        if(!this->m_cb.q->fiber_active) {
          // Create a new fiber, if none is active. The fiber shall only reset
          // `m_fiber_active` if no packet is pending.
          fiber_scheduler.insert(::std::make_unique<Final_Fiber>(this->m_cb));
          this->m_cb.q->fiber_active = true;
        }

        auto& packet = this->m_cb.q->packets.emplace_back();
        packet.addr.swap(addr);
        packet.data.swap(data);
      }
  };

}  // namespace

Easy_UDP_Client::
~Easy_UDP_Client()
  {
  }

void
Easy_UDP_Client::
start()
  {
    this->m_uniq = ::std::make_shared<int>();
    Shared_cb_args cb = { this->m_cb_thunk, this->m_cb_obj, this->m_uniq, nullptr };
    this->m_socket = ::std::make_shared<Final_UDP_Socket>(::std::move(cb));
    network_driver.insert(this->m_socket);
  }

void
Easy_UDP_Client::
stop() noexcept
  {
    this->m_uniq = nullptr;
    this->m_socket = nullptr;
  }

const Socket_Address&
Easy_UDP_Client::
local_address() const noexcept
  {
    if(!this->m_socket)
      return ipv6_unspecified;

    return this->m_socket->local_address();
  }

void
Easy_UDP_Client::
join_multicast_group(const Socket_Address& maddr, uint8_t ttl, bool loopback, const char* ifname_opt)
  {
    if(!this->m_socket)
      POSEIDON_THROW((
          "Client not running",
          "[easy UDP client `$1` (class `$2`)]"),
          this, typeid(*this));

    this->m_socket->join_multicast_group(maddr, ttl, loopback, ifname_opt);
  }

void
Easy_UDP_Client::
leave_multicast_group(const Socket_Address& maddr, const char* ifname_opt)
  {
    if(!this->m_socket)
      POSEIDON_THROW((
          "Client not running",
          "[easy UDP client `$1` (class `$2`)]"),
          this, typeid(*this));

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
