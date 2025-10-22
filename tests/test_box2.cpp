#include <cassert>
#include "../core/rtree.h"

int main() {
    rtse::Box2 box1(rtse::Point2(0, 0), rtse::Point2(1, 1));
    rtse::Box2 box2(rtse::Point2(0.5, 0.5), rtse::Point2(2, 2));
    assert(box1.overlap(box2));
    assert(box1.area() == 1.0);
    auto box3 = rtse::Box2::merge(box1, box2);
    assert(box3.max.x == 2 && box3.max.y == 2);
    assert(box1.enlarge_area(box2) > 0);
}