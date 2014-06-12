
#ifndef CPPA_IMAGE_TYPE_HPP
#define CPPA_IMAGE_TYPE_HPP

#include <vector>
#include "cppa/opencl/global.hpp"
#include "cppa/opencl/smart_ptr.hpp"

namespace cppa {
namespace opencl {

typedef std::vector<float> fvec;
typedef std::vector<size_t> svec;

class image_type {

 public:

    image_type() = default;

    image_type(const fvec& data,
               const int width,
               const int height,
               const int bytes_per_line)
        : m_data(data)
        , m_width(width)
        , m_height(height)
        , m_bytes_per_line(bytes_per_line)
        , m_region{width, height, 0}
        , m_origin{0, 0, 0}
        , m_format{CL_RGBA, CL_FLOAT} { }

    image_type(const image_type&) = default;
    image_type& operator=(const image_type&) = default;

    static void announce() {
        cppa::announce<fvec>();
        cppa::announce<svec>();
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

    const fvec& get_data() const { return m_data; }
    int get_width() const { return m_width; }
    int get_height() const { return m_height; }
    int get_bytes_per_line() const { return m_bytes_per_line; }
    svec get_region() const { return m_region; }
    svec get_origin() const { return m_origin; }
    cl_image_format get_format() const { return m_format; }
    void set_data(fvec vec) { m_data = std::move(vec); }
    void set_width(int width) { m_width = width; }
    void set_height(int height) { m_height = height; }
    void set_bytes_per_line(int bpl) { m_bytes_per_line = bpl; }

 private:
    fvec                  m_data;
    int                   m_width;
    int                   m_height;
    int                   m_bytes_per_line;
    const svec            m_region;
    const svec            m_origin;
    cl_image_format       m_format;
};

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
