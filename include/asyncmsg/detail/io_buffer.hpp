#pragma once

#include <cstdint>
#include <memory>
#include <functional>

namespace bev {

// # IO Buffer
//
// A buffer whose main purpose it is to store binary data streams
// that must be buffered while being moved from a source to a sink,
// e.g. audio data waiting to go to the sound card or network traffic
// being pushed from one socket to another.
//
// The design is heavily inspired by `boost::beast::flat_static_buffer`,
// but the API has been adjusted for consistency with the `linear_ringbufer`
// in this repository and stripped of all boost dependencies.
//
//
// # Usage
//
// Writing data into the buffer:
//
//     bev::io_buffer iob;
//     bev::io_buffer::slab slab = iob.prepare(512);
//     ssize_t n = ::read(socket, slab.data, slab.size);
//     iob.commit(n);
//
// Reading data from the buffer:
//
//     bev::io_buffer iob;
//     ssize_t n = ::write(socket, iob.read_head(), iob.size());
//     iob.consume(n);
//
//
// # Multi-threading
//
// No concurrent operations are allowed.
//
//
// # Exceptions
//
// Both constructors of `io_buffer` may throw `std::bad_alloc` on allocation
// failure. (Although the constructor accepting a `unique_ptr` will in practice
// not allocate, because the type-erased deleter should be small enough for the
// small function optimization. However, this is not guaranteed.)
//
// All other operations on the buffer are noexcept. In particular, class `io_buffer_view`
// provides a fully noexcept interface.
//
//
// # Memory Management
//
// The constructor `io_buffer::io_buffer(std::unique_ptr<char> buffer, size_t size)` can be
// used to pass ownership of an existing memory region to an `io_buffer`. Custom deleters
// are supported with that constructor.
//
// The class `io_buffer_view` can be used to treat an existing memory region as an
// `io_buffer` without assuming ownership of the underlying memory.
//

using std::size_t;


// This class accepts an arbitrary region of memory and treats it as an `io_buffer`.
class io_buffer_view
{
public:
    struct slab {
        uint8_t* data;
        size_t size;
    };

    // NOTE: If the default constructor is used, the view is in undefined state
    // until `assign()` is called.
    io_buffer_view() noexcept = default;


    inline io_buffer_view(uint8_t* data, size_t size) noexcept
      : buffer_(data)
      , length_(size)
      , head_(0)
      , tail_(0)
    {
    }


    inline void assign(uint8_t* data, size_t size) noexcept
    {
        buffer_ = data;
        length_ = size;
        head_ = 0;
        tail_ = 0;
    }


    inline uint8_t* read_head() noexcept
    {
        return buffer_ + head_;
    }


    inline uint8_t* write_head() noexcept
    {
        return buffer_ + tail_;
    }


    inline size_t size() const noexcept
    {
        return tail_ - head_;
    }


    inline size_t capacity() const noexcept
    {
        return length_ - this->size();
    }


    inline size_t free_size() const noexcept
    {
        return length_ - tail_;
    }


    inline slab prepare(size_t n) noexcept
    {
        // Make as much room as we can
        if (n > this->free_size()) {
            std::size_t size = tail_ - head_;
            ::memmove(buffer_, buffer_ + head_, size);
            tail_ = size;
            head_ = 0;
        }

        // If we still don't have enough, adjust request.
        if (n > this->capacity()) {
            n = this->capacity();
        }

        return slab {buffer_ + tail_, n};
    }


    inline void commit(std::size_t n) noexcept
    {
        // assert: tail_ + n < size
        tail_ += n;
    }


    inline void consume(std::size_t n) noexcept
    {
        // assert: size() <= n
        head_ += n;
        if (head_ >= tail_) {
            head_ = tail_ = 0;
        }
    }


    inline void clear() noexcept
    {
        head_ = tail_ = 0;
    }

private:
    uint8_t* buffer_;
    size_t length_;
    size_t head_;
    size_t tail_;
};


namespace detail {

// Class `io_buffer_storage` holds a pointer to the allocated memory region along
// with a type-erased deleter.
class io_buffer_storage
{
public:
    io_buffer_storage(std::unique_ptr<uint8_t[]> storage)
    {
        buffer_ = std::move(storage);
    }

protected:
    std::unique_ptr<uint8_t[]> buffer_;
};

} // namespace detail


// The actual `io_buffer` is an `io_buffer_view` that allocates its own storage.
class io_buffer
  : private detail::io_buffer_storage
  , public io_buffer_view
{
public:
    io_buffer(size_t size)
      : detail::io_buffer_storage(std::make_unique<uint8_t[]>(size))
      , io_buffer_view(this->detail::io_buffer_storage::buffer_.get(), size)
    {
    }
};


} // namespace bev
