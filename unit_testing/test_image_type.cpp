#include <vector>
#include <numeric>

#include "test.hpp"
#include "cppa/opencl/image_type.hpp"


using namespace std;
using namespace cppa;
using namespace cppa::opencl;

namespace {

typedef vector<float> fvec;

void receiver(event_based_actor* self) {
    self->become(
        on_arg_match >> [=] (image_type& t) {
            CPPA_CHECKPOINT();
            self->quit();
            return t;
        }
    );
}

void test_image_type() {
    // [scoped] --send-img--> [actorB]-+
    //    ^                            |
    //    |                            |
    //    + ------send-img-back--------+
    // after receiving the img back check for equality
    scoped_actor self;
    const size_t vec_size = 100;
    fvec vec(vec_size);
    iota(vec.begin(), vec.end(), vec_size);
    image_type img(vec, vec_size, 1, vec_size);
    self->sync_send(self->spawn(receiver), img).await(
        on_arg_match >> [&](const image_type& t){
            // this should pass...
            CPPA_CHECK(equal(begin(img.get_data()), end(img.get_data()), begin(t.get_data())));
        }
    );
}

}

int main() {
    CPPA_TEST(test_image_type);
    image_type::announce();
    test_image_type();
    await_all_actors_done();
    shutdown();
    return CPPA_TEST_RESULT();
}
