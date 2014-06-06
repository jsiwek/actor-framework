
#ifndef CPPA_IMAGE_TYPE_HPP
#define CPPA_IMAGE_TYPE_HPP

#include <vector>

#include "cppa/opencl/global.hpp"
#include "cppa/opencl/smart_ptr.hpp"

namespace cppa {
namespace opencl {

typedef unsigned char uchar;
typedef std::vector<uchar> uvec;

class image_type {

 public:

    image_type() = default;

    image_type(const uvec& data,
               const int width,
               const int height,
               const int bytes_per_line)
        : m_data(data)
        , m_width(width)
        , m_height(height)
        , m_bytes_per_line(bytes_per_line) { }

    image_type(const image_type&) = default;
    image_type& operator=(const image_type&) = default;

    const uvec&  get_data() const;
    int   get_width() const;
    int   get_height() const;
    int   get_bytes_per_line() const;

    void set_data(uvec vec);
    void set_width(int width);
    void set_height(int height);
    void set_bytes_per_line(int bpl);

    static void announce() {
        cppa::announce<uvec>();
        cppa::announce<image_type>(
                    std::make_pair(&image_type::get_data,
                                   &image_type::set_data),
                    std::make_pair(&image_type::get_width,
                                   &image_type::set_width),
                    std::make_pair(&image_type::get_height,
                                   &image_type::set_height),
                    std::make_pair(&image_type::get_bytes_per_line,
                                   &image_type::set_bytes_per_line));
    }

    uvec m_data;
    int  m_width;
    int  m_height;
    int  m_bytes_per_line;
};

/******************************************************************************\
 *                 implementation of inline member functions                  *
\******************************************************************************/

const uvec& image_type::get_data() const {
    return m_data;
}

int image_type::get_width() const {
    return m_width;
}

int image_type::get_height() const {
    return m_height;
}

int image_type::get_bytes_per_line() const {
    return m_bytes_per_line;
}

void image_type::set_data(uvec vec) { m_data = std::move(vec); }
void image_type::set_width(int width) { m_width = width; }
void image_type::set_height(int height) { m_height = height; }
void image_type::set_bytes_per_line(int bpl) { m_bytes_per_line = bpl; }

inline bool operator==(const image_type& lhs, const image_type& rhs) {
    return    lhs.get_data() == rhs.get_data()
           && lhs.get_width() == rhs.get_width()
           && lhs.get_height() == rhs.get_height()
           && lhs.get_bytes_per_line() == rhs.get_bytes_per_line();
}

inline bool operator!=(const image_type& lhs, const image_type& rhs) {
    return    lhs.get_data() != rhs.get_data()
           || lhs.get_width() != rhs.get_width()
           || lhs.get_height() != rhs.get_height()
           || lhs.get_bytes_per_line() != rhs.get_bytes_per_line();
}

} // namespace opencl
} // namespace cppa

#endif // CPPA_IMAGE_TYPE_HPP
