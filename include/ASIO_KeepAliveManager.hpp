#pragma once
#ifndef WSOCKET__ASIO_KEEPALIVE_MANAGER_HPP
#define WSOCKET__ASIO_KEEPALIVE_MANAGER_HPP

#ifdef WITH_ASIO

#include <asio.hpp>

namespace wsocket {

class KeepAliveManager {
public:
    explicit KeepAliveManager(asio::any_io_executor io_executor) :
        expired_timer_(io_executor), timeout_timer_(io_executor), expired_time_ms_(EXPIRED_TIME_MS_DEFAULT),
        timeout_ms_(TIMEOUT_MS_DEFAULT) {}

    ~KeepAliveManager() { this->Stop(); }

    static constexpr int64_t EXPIRED_TIME_S_DEFAULT  = 2 * 60; // 2min
    static constexpr int64_t EXPIRED_TIME_MS_DEFAULT = EXPIRED_TIME_S_DEFAULT * 1000;
    static constexpr int64_t TIMEOUT_MS_DEFAULT      = 3 * EXPIRED_TIME_MS_DEFAULT;

    // Set expiration time (seconds)
    void SetExpiredTimeSec(int64_t sec) {
        expired_time_ms_ = std::chrono::milliseconds(sec * 1000);
        timeout_ms_      = 3 * expired_time_ms_;
    }

    // Set expiration time (milliseconds)
    void SetExpiredTimeMsec(int64_t msec) {
        expired_time_ms_ = std::chrono::milliseconds(msec);
        timeout_ms_      = 3 * expired_time_ms_;
    }

    // Start or refresh timer
    void Start() { Flush(); }

    // Refresh timer, restart counting
    void Flush();

    void Stop() {
        expired_timer_.cancel();
        timeout_timer_.cancel();
    }

    class Listener {
    public:
        virtual ~Listener()                                 = default;
        virtual void OnKeepAliveExpired(std::error_code ec) = 0;
        virtual void OnKeepAliveTimeout(std::error_code ec) = 0;
    };
    void ResetListener(Listener *listener) { listener_ = listener; }

private:
    // Expiration timer timeout handler
    void OnExpiredTimerTimeout(std::error_code ec);

    // Timeout timer timeout handler
    void OnTimeoutTimerTimeout(std::error_code ec);

private:
    asio::steady_timer        expired_timer_;   // Keep-alive expiration timer
    asio::steady_timer        timeout_timer_;   // Connection timeout timer
    std::chrono::milliseconds expired_time_ms_; // Keep-alive expiration time
    std::chrono::milliseconds timeout_ms_;      // Connection timeout time
    Listener                 *listener_{nullptr};
};


void KeepAliveManager::Flush() {
    // Cancel previous timers
    expired_timer_.cancel();
    timeout_timer_.cancel();

    // Set new expiration timer
    expired_timer_.expires_after(expired_time_ms_);
    expired_timer_.async_wait([this](std::error_code ec) { OnExpiredTimerTimeout(ec); });

    // Set new timeout timer
    timeout_timer_.expires_after(timeout_ms_);
    timeout_timer_.async_wait([this](std::error_code ec) { OnTimeoutTimerTimeout(ec); });
}

void KeepAliveManager::OnExpiredTimerTimeout(std::error_code ec) {
    if(ec == asio::error::operation_aborted) {
        return;
    }

    // Notify listener that keep-alive has expired
    if(listener_) {
        listener_->OnKeepAliveExpired(ec);
    }
}

void KeepAliveManager::OnTimeoutTimerTimeout(std::error_code ec) {
    if(ec == asio::error::operation_aborted) {
        return;
    }
    // Notify listener of timeout
    if(listener_) {
        listener_->OnKeepAliveTimeout(ec);
    }
}

} // namespace wsocket

#endif

#endif // WSOCKET__ASIO_KEEPALIVE_MANAGER_HPP
