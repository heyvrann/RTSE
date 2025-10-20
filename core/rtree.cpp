#include "rtree.h"
#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <deque>

rtse::Box2::Box2(): is_empty(true) {}

rtse::Box2::Box2(rtse::Point2 p1, rtse::Point2 p2):
min(rtse::Point2(std::min(p1.x, p2.x), std::min(p1.y, p2.y))),
max(rtse::Point2(std::max(p1.x, p2.x), std::max(p1.y, p2.y))),
is_empty(false) {}

rtse::Box2 rtse::Box2::from_point(rtse::Point2 p) { return rtse::Box2(p, p); }

double rtse::Box2::area() const {
    if (is_empty) return 0;
    return std::max(0.0, (max.x - min.x) * (max.y - min.y));
}

bool rtse::Box2::overlap(const rtse::Box2& other) const {
    if (this->is_empty) return false;
    if (other.is_empty) return false;
    return (max.x >= other.min.x && min.x <= other.max.x)   // x-axis intersects
        && (max.y >= other.min.y && min.y <= other.max.y);  // y-axis intersects
}

rtse::Box2 rtse::Box2::merge(const Box2& box1, const Box2& box2) {
    if (box1.is_empty) return box2;
    if (box2.is_empty) return box1;
    return rtse::Box2(rtse::Point2(std::min(box1.min.x, box2.min.x), std::min(box1.min.y, box2.min.y)),
                      rtse::Point2(std::max(box1.max.x, box2.max.x), std::max(box1.max.y, box2.max.y)));
}

double rtse::Box2::enlarge_area(const Box2& other) const {
    Box2 merged_box = merge(*this, other);
    return merged_box.area() - this->area();
}

rtse::RTree::RTree() {
    root = std::make_shared<Node>();
    root->is_leaf = true;
    root->mbr = Box2();
    M = 8;
    m = 2;
}

void rtse::RTree::insert(const Box2& box, int id) {
    std::cout << "[RTree] 'insert' called." << std::endl;

    auto deq = choose_leaf(root, box);
    insert_to_node(deq, box, id);
}

void rtse::RTree::erase(int id) {
    std::cout << "[RTree] 'erase' called." << std::endl;
}

void rtse::RTree::update(int id, const rtse::Box2& new_box) {
    std::cout << "[RTree] 'update' called." << std::endl;
}

std::vector<int> rtse::RTree::query_range(const rtse::Box2& query_box) {
    std::cout << "[RTree] 'query_range' called." << std::endl;
    return root->ids;
}

std::deque<std::shared_ptr<rtse::Node>> rtse::RTree::choose_leaf(std::shared_ptr<rtse::Node> cur_node, const rtse::Box2& box) {
    if (cur_node->is_leaf) return std::deque<std::shared_ptr<rtse::Node>>(1, cur_node);
    assert(!cur_node->children.empty()); // empty node should not exist

    std::shared_ptr<Node> min_ptr = cur_node->children.front();
    double min_enlargement = min_ptr->mbr.enlarge_area(box); 
    for (auto& child : cur_node->children) {
        double enlarge_area = child->mbr.enlarge_area(box);
        if (enlarge_area < min_enlargement) {
            min_enlargement = enlarge_area;
            min_ptr = child;
        }
        else if (enlarge_area == min_enlargement && child->mbr.area() < min_ptr->mbr.area()) {
            min_enlargement = enlarge_area;
            min_ptr = child;
        }
    }
    auto deq = choose_leaf(min_ptr, box);
    deq.push_front(cur_node);
    return deq;
}

void rtse::RTree::insert_to_node(std::deque<std::shared_ptr<rtse::Node>>& deq, const rtse::Box2& box, int id) {
    auto cur_node = deq.front();
    deq.pop_front();

    cur_node->mbr = rtse::Box2::merge(cur_node->mbr, box);
    if (!cur_node->is_leaf) {
        insert_to_node(deq, box, id);
    }
    else {
        if (cur_node->boxes.size() < M) {
            cur_node->boxes.push_back(box);
            cur_node->ids.push_back(id);
        }
        else {
            // split and adjust
        }
    }
}


void hello_core() {
    std::cout << "RTSE core initialized." << std::endl;
}