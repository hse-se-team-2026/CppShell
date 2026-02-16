#pragma once

#include <condition_variable>
#include <cstring>
#include <iostream>
#include <mutex>
#include <streambuf>
#include <vector>

namespace cppshell {

// A thread-safe blocking queue of bytes
class Pipe {
public:
  void Write(const char *data, size_t size) {
    if (size == 0)
      return;
    std::unique_lock<std::mutex> lock(mutex_);
    buffer_.insert(buffer_.end(), data, data + size);
    cv_.notify_all();
  }

  size_t Read(char *buffer, size_t size) {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [this] { return !buffer_.empty() || closed_; });

    if (buffer_.empty() && closed_) {
      return 0; // EOF
    }

    const size_t available = buffer_.size();
    const size_t toRead = std::min(size, available);

    std::memcpy(buffer, buffer_.data(), toRead);
    buffer_.erase(buffer_.begin(), buffer_.begin() + toRead);

    return toRead;
  }

  void Close() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = true;
    cv_.notify_all();
  }

private:
  std::vector<char> buffer_;
  std::mutex mutex_;
  std::condition_variable cv_;
  bool closed_ = false;
};

// Stream buffer capable of reading from a Pipe
class PipeReadBuffer : public std::streambuf {
public:
  explicit PipeReadBuffer(Pipe &pipe) : pipe_(pipe) {}

protected:
  // Read from pipe into buffer
  int_type underflow() override {
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    const size_t n = pipe_.Read(buffer_, kBufferSize);
    if (n == 0) {
      return traits_type::eof();
    }

    setg(buffer_, buffer_, buffer_ + n);
    return traits_type::to_int_type(*gptr());
  }

private:
  static constexpr size_t kBufferSize = 1024;
  char buffer_[kBufferSize];
  Pipe &pipe_;
};

// Stream buffer capable of writing to a Pipe
class PipeWriteBuffer : public std::streambuf {
public:
  explicit PipeWriteBuffer(Pipe &pipe) : pipe_(pipe) {}

protected:
  // Write buffer to pipe
  int_type overflow(int_type ch) override {
    if (ch != traits_type::eof()) {
      char c = static_cast<char>(ch);
      pipe_.Write(&c, 1);
      return ch;
    }
    return traits_type::eof();
  }

  std::streamsize xsputn(const char *s, std::streamsize n) override {
    pipe_.Write(s, static_cast<size_t>(n));
    return n;
  }

private:
  Pipe &pipe_;
};

} // namespace cppshell
