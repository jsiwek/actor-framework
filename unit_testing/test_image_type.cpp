
#include <vector>

#include "test.hpp"
#include "cppa/opencl/image_type.hpp"


using namespace std;
using namespace cppa;
using namespace cppa::opencl;

namespace {

typedef unsigned char uchar;
typedef vector<uchar> uvec;

void receiver(event_based_actor* self) {
    self->become(
        on_arg_match >> [=] (image_type& t) {
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
    { // scoped actor
        scoped_actor self;
        uvec vec;
        for(int i = 0; i < 100; ++i) {
            vec.push_back(i);
        }
        image_type img(vec, 100, 1, 100);
        self->sync_send(self->spawn(receiver), img).await(
            on_arg_match >> [=](image_type& t){
                // this should pass...
                CPPA_CHECK(equal(begin(img.get_data()), end(img.get_data()), begin(t.get_data())));
            }
        );
    } // scoped actor
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

