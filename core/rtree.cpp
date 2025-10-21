#include "rtree.h"
#include <iostream>
#include <vector>
#include <memory>
#include <cassert>
#include <deque>
#include <algorithm>
#include <unordered_map>

std::unordered_map<int, rtse::Box2> id_to_box;

rtse::Box2::Box2(): is_empty(true) {}

rtse::Box2::Box2(const rtse::Point2& p1, const rtse::Point2& p2):
min(Point2(std::min(p1.x, p2.x), std::min(p1.y, p2.y))),
max(Point2(std::max(p1.x, p2.x), std::max(p1.y, p2.y))),
is_empty(false) {}

rtse::Box2 rtse::Box2::from_point(const rtse::Point2& p) { return Box2(p, p); }

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
    return Box2(Point2(std::min(box1.min.x, box2.min.x), std::min(box1.min.y, box2.min.y)),
                Point2(std::max(box1.max.x, box2.max.x), std::max(box1.max.y, box2.max.y)));
}

double rtse::Box2::enlarge_area(const Box2& other) const {
    Box2 merged_box = merge(*this, other);
    return merged_box.area() - this->area();
}

bool rtse::Box2::operator==(const Box2& other) const noexcept {
    return eq(min.x, other.min.x) && eq(max.x, other.max.x)
        && eq(min.y, other.min.y) && eq(max.y, other.max.y);
}

bool rtse::Box2::operator!=(const Box2& other) const noexcept {
    return !(*this == other);
}

std::pair<const rtse::Box2&, int> rtse::Node::entry(size_t i) const {
    return {boxes[i], ids[i]};
}

size_t rtse::Node::size() const { return boxes.size(); }

void rtse::Node::push_back(const rtse::Box2& box, int id) {
    boxes.push_back(box);
    ids.push_back(id);
    mbr = Box2::merge(mbr, box);
}

void rtse::Node::push_back(const rtse::Box2& box, const rtse::NodePtr ptr) {
    boxes.push_back(box);
    children.push_back(ptr);
    mbr = Box2::merge(mbr, box);
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

    assert(id_to_box.find(id) == id_to_box.end());  // id should be unique
    id_to_box[id] = box;

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

rtse::NodeDeq rtse::RTree::choose_leaf(rtse::NodePtr cur_node, const rtse::Box2& box) {
    if (cur_node->is_leaf) return NodeDeq(1, cur_node); // base case: leaf node
    assert(!cur_node->children.empty());    // empty node should not exist

    // recursive case: non-leaf node
    NodePtr min_ptr = cur_node->children.front();
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

void rtse::RTree::insert_to_node(rtse::NodeDeq deq, const rtse::Box2& box, int id) {
    // keep the parent node to handle overflow
    NodePtr parent_node = nullptr;
    if (deq.front() != root) parent_node = deq.front();
    deq.pop_front();
    auto cur_node = deq.front();

    cur_node->mbr = Box2::merge(cur_node->mbr, box);
    if (!cur_node->is_leaf) {
        // enlarge the mbr of child node
        auto tmp = deq.front();
        deq.pop_front();
        auto child = deq.front();
        for (size_t i = 0 ; i < cur_node->children.size() ; i++) {
            if (cur_node->children[i] == child)
                cur_node->boxes[i] = Box2::merge(cur_node->boxes[i], box);
        }
        deq.push_front(tmp);
        
        insert_to_node(deq, box, id);   // recursive insertion
    }
    else {
        cur_node->boxes.push_back(box);
        cur_node->ids.push_back(id);
        // overflow occurrs
        if (cur_node->boxes.size() > M) {
            auto split_pair = split(cur_node);
            adjust(parent_node, cur_node, split_pair);
        }
    }
}

std::pair<rtse::NodePtr, rtse::NodePtr> rtse::RTree::split(const rtse::NodePtr& node) {
    auto [node_A, node_B] = choose_boxes(node);
    // leaf case: boxes and ids
    if (node->is_leaf) {
        int id_A = node_A->ids[0], id_B = node_B->ids[0];

        size_t cur_idx = 0, remained = node->size() - 2;
        while (cur_idx < node->size() && node_A->size() > m - remained && node_B->size() > m - remained) {
            if (node->ids[cur_idx] == id_A || node->ids[cur_idx] == id_B) { ++cur_idx; continue; }
            int cur_id = node->ids[cur_idx];
            Box2& cur_box = node->boxes[cur_idx];
            double enlarged_A = node_A->mbr.enlarge_area(cur_box), enlarged_B = node_B->mbr.enlarge_area(cur_box);
            // choose smaller enlarged area
            if (enlarged_A < enlarged_B) 
                node_A->push_back(cur_box, cur_id);
            else if (enlarged_A > enlarged_B) 
                node_B->push_back(cur_box, cur_id);
            // if tie, chooese smaller mbr 
            else if (node_A->mbr.area() < node_B->mbr.area())
                node_A->push_back(cur_box, cur_id);
            else if (node_A->mbr.area() > node_B->mbr.area())
                node_B->push_back(cur_box, cur_id);
            // if tie again, choose smaller node
            else if (node_A->size() < node_B->size())
                node_A->push_back(cur_box, cur_id);
            else if (node_A->size() > node_B->size())
                node_B->push_back(cur_box, cur_id);
            // if still tie, add to node_A 
            else 
                node_A->push_back(cur_box, cur_id);
            
            ++cur_idx;
            --remained;
        }   
        if (node_A->size() == m - remained) {
            while (cur_idx < node->size()) {
                if (node->ids[cur_idx] == id_A || node->ids[cur_idx] == id_B) { ++cur_idx; continue; }
                node_A->push_back(node->boxes[cur_idx], node->ids[cur_idx]);
                ++cur_idx;
            }
        }
        if (node_B->size() == m - remained) {
            while (cur_idx < node->size()) {
                if (node->ids[cur_idx] == id_A || node->ids[cur_idx] == id_B) { ++cur_idx; continue; }
                node_B->push_back(node->boxes[cur_idx], node->ids[cur_idx]);
                ++cur_idx;
            }
        }
    }
    // non-leaf case: boxes and children
    else {
        auto ptr_A = node_A->children[0], ptr_B = node_B->children[0];

        size_t cur_idx = 0, remained = node->size() - 2;
        while (cur_idx < node->size() && node_A->size() > m - remained && node_B->size() > m - remained) {
            if (node->children[cur_idx] == ptr_A || node->children[cur_idx] == ptr_B) { ++cur_idx; continue; }
            auto cur_ptr = node->children[cur_idx];
            Box2& cur_box = node->boxes[cur_idx];
            double enlarged_A = node_A->mbr.enlarge_area(cur_box), enlarged_B = node_B->mbr.enlarge_area(cur_box);
            // choose smaller enlarged area
            if (enlarged_A < enlarged_B) 
                node_A->push_back(cur_box, cur_ptr);
            else if (enlarged_A > enlarged_B) 
                node_B->push_back(cur_box, cur_ptr);
            // if tie, chooese smaller mbr 
            else if (node_A->mbr.area() < node_B->mbr.area())
                node_A->push_back(cur_box, cur_ptr);
            else if (node_A->mbr.area() > node_B->mbr.area())
                node_B->push_back(cur_box, cur_ptr);
            // if tie again, choose smaller node
            else if (node_A->size() < node_B->size())
                node_A->push_back(cur_box, cur_ptr);
            else if (node_A->size() > node_B->size())
                node_B->push_back(cur_box, cur_ptr);
            // if still tie, add to node_A 
            else 
                node_A->push_back(cur_box, cur_ptr);
            
            ++cur_idx;
            --remained;
        }   
        if (node_A->size() == m - remained) {
            while (cur_idx < node->size()) {
                if (node->children[cur_idx] == ptr_A || node->children[cur_idx] == ptr_B) { ++cur_idx; continue; }
                node_A->push_back(node->boxes[cur_idx], node->children[cur_idx]);
                ++cur_idx;
            }
        }
        if (node_B->size() == m - remained) {
            while (cur_idx < node->size()) {
                if (node->children[cur_idx] == ptr_A || node->children[cur_idx] == ptr_B) { ++cur_idx; continue; }
                node_B->push_back(node->boxes[cur_idx], node->children[cur_idx]);
                ++cur_idx;
            }
        }
    }
    return {node_A, node_B};
}

void rtse::RTree::adjust(rtse::NodePtr& node, const rtse::NodePtr& removed_node, const std::pair<rtse::NodePtr, rtse::NodePtr>& split_pair) {
    auto it = find(node->children.begin(), node->children.end(), removed_node);
    if (it != node->children.end()) node->children.erase(it);


}

std::pair<rtse::NodePtr, rtse::NodePtr> rtse::RTree::choose_boxes(const rtse::NodePtr& node) {
    double overall_low, highest_low, lowest_high, overall_high;
    double separation_x, separation_y;
    double denom;
    size_t idxA_x, idxB_x, idxA_y, idxB_y;

    // calculate normalized separation of x-axis
    overall_low = lowest_high = std::numeric_limits<double>::infinity();
    highest_low = overall_high = -std::numeric_limits<double>::infinity();
    for (size_t i = 0 ; i < node->boxes.size() ; i++) {
        if (node->boxes[i].min.x < overall_low) overall_low = node->boxes[i].min.x;
        if (node->boxes[i].min.x > highest_low) {
            highest_low = node->boxes[i].min.x;
            idxA_x = i;
        }
        if (node->boxes[i].max.x < lowest_high) {
            lowest_high = node->boxes[i].max.x;
            idxB_x = i;
        } 
        if (node->boxes[i].max.x > overall_high)
            overall_high = node->boxes[i].max.x;
    }
    denom = overall_high - overall_low;
    if (denom == 0) separation_x = 0;
    else separation_x = std::max(0.0, (highest_low - lowest_high) / denom);

    // calculate normalized seperation of y-axis
    overall_low = lowest_high = std::numeric_limits<double>::infinity();
    highest_low = overall_high = -std::numeric_limits<double>::infinity();
    for (size_t i = 0 ; i < node->boxes.size() ; i++) {
        if (node->boxes[i].min.y < overall_low) overall_low = node->boxes[i].min.y;
        if (node->boxes[i].min.y > highest_low) {
            highest_low = node->boxes[i].min.y;
            idxA_y = i;
        }
        if (node->boxes[i].max.y < lowest_high) {
            lowest_high = node->boxes[i].max.y;
            idxB_y = i;
        } 
        if (node->boxes[i].max.y > overall_high)
            overall_high = node->boxes[i].max.y;
    }
    denom = overall_high - overall_low;
    if (denom == 0) separation_y = 0;
    else separation_y = std::max(0.0, (highest_low - lowest_high) / denom);

    // choose the axis with larger separation
    size_t idxA, idxB;
    if (separation_x > separation_y) { idxA = idxA_x; idxB = idxB_x; }
    else { idxA = idxA_y; idxB = idxB_y; }
    auto ptrA = std::make_shared<Node>(), ptrB = std::make_shared<Node>();
    ptrB->is_leaf = ptrA->is_leaf = node->is_leaf;
    ptrA->boxes.push_back(node->boxes[idxA]); ptrA->mbr = node->boxes[idxA];
    ptrB->boxes.push_back(node->boxes[idxB]); ptrB->mbr = node->boxes[idxB];
    if (node->is_leaf) { 
        ptrA->ids.push_back(node->ids[idxA]); 
        ptrB->ids.push_back(node->ids[idxB]); 
    }
    else { 
        ptrA->children.push_back(node->children[idxA]);
        ptrB->children.push_back(node->children[idxB]);
    }
    return {ptrA, ptrB};
}

void hello_core() {
    std::cout << "RTSE core initialized." << std::endl;
}