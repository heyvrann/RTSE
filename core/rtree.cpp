#include "rtree.h"
#include <algorithm>
#include <cassert>
#include <iostream>
#include <limits>
#include <vector>

double rtse::Point2::x() const { return m_x; }

double rtse::Point2::y() const { return m_y; }

rtse::Box2::Box2() : m_is_empty(true) {}

rtse::Box2::Box2(const rtse::Point2 &p1, const rtse::Point2 &p2)
    : m_is_empty(false),
      m_min(Point2(std::min(p1.x(), p2.x()), std::min(p1.y(), p2.y()))),
      m_max(Point2(std::max(p1.x(), p2.x()), std::max(p1.y(), p2.y())))
{
}

const rtse::Point2 &rtse::Box2::min() const { return m_min; }

const rtse::Point2 &rtse::Box2::max() const { return m_max; }

bool rtse::Box2::is_empty() const { return m_is_empty; }

rtse::Box2 rtse::Box2::from_point(const rtse::Point2 &p) { return Box2(p, p); }

double rtse::Box2::area() const
{
    if (is_empty())
        return 0;
    return std::max(0.0, (max().x() - min().x()) * (max().y() - min().y()));
}

bool rtse::Box2::overlap(const rtse::Box2 &other) const
{
    if (this->is_empty())
        return false;
    if (other.is_empty())
        return false;
    return (max().x() >= other.min().x() &&
            min().x() <= other.max().x()) // x-axis intersects
           && (max().y() >= other.min().y() &&
               min().y() <= other.max().y()); // y-axis intersects
}

rtse::Box2 rtse::Box2::merge(const Box2 &box1, const Box2 &box2)
{
    if (box1.is_empty())
        return box2;
    if (box2.is_empty())
        return box1;
    return Box2(Point2(std::min(box1.min().x(), box2.min().x()),
                       std::min(box1.min().y(), box2.min().y())),
                Point2(std::max(box1.max().x(), box2.max().x()),
                       std::max(box1.max().y(), box2.max().y())));
}

double rtse::Box2::enlarge_area(const Box2 &other) const
{
    Box2 merged_box = merge(*this, other);
    return merged_box.area() - this->area();
}

bool rtse::Box2::operator==(const Box2 &other) const noexcept
{
    return eq(min().x(), other.min().x()) && eq(max().x(), other.max().x()) &&
           eq(min().y(), other.min().y()) && eq(max().y(), other.max().y());
}

bool rtse::Box2::operator!=(const Box2 &other) const noexcept
{
    return !(*this == other);
}

std::pair<const rtse::Box2 &, int> rtse::Node::entry(size_t i) const
{
    return {boxes[i], ids[i]};
}

size_t rtse::Node::size() const { return boxes.size(); }

void rtse::Node::push_back(const rtse::Box2 &box, int id)
{
    boxes.push_back(box);
    ids.push_back(id);
    mbr = Box2::merge(mbr, box);
}

void rtse::Node::push_back(Node *ptr)
{
    boxes.push_back(ptr->mbr);
    children.push_back(ptr);
    mbr = Box2::merge(mbr, ptr->mbr);
}

void rtse::Node::update_mbr()
{
    mbr = Box2();
    for (size_t i = 0; i < this->size(); i++)
        mbr = Box2::merge(mbr, boxes[i]);
}

rtse::RTree::RTree()
{
    root = new Node;
    root->is_leaf = true;
    root->mbr = Box2();
    M = 8;
    m = 2;
}

rtse::RTree::~RTree() { deallocate(root); }

void rtse::RTree::deallocate(Node *root)
{
    if (!root)
        return;
    if (!root->is_leaf)
    {
        for (auto child : root->children)
            deallocate(child);
    }
    delete root;
}

void rtse::RTree::insert(const Box2 &box, int id)
{
    std::cout << "[RTree] insert " << id << "." << std::endl;

    assert(id_to_box.find(id) == id_to_box.end()); // id should be unique
    id_to_box[id] = box;

    auto vec = choose_leaf(root, box);
    insert_to_node(vec, vec.size() - 1, box, id);
}

void rtse::RTree::erase(int id)
{
    std::cout << "[RTree] erase " << id << "." << std::endl;

    assert(id_to_box.find(id) != id_to_box.end()); // erased id should exist

    auto removed_box = id_to_box[id];
    NodeVec vec(0);
    choose_leaf(vec, root, removed_box, id);
    assert(!vec.empty()); // DFS path should exist
    remove_node(vec, vec.size() - 1, id);

    id_to_box.erase(id);

    if (!root->is_leaf && root->size() == 1)
    {
        auto old_root = root;
        root = root->children[0];
        delete old_root;
    }
    if (!root->is_leaf && root->size() == 0)
        root->is_leaf = true;
}

void rtse::RTree::update(int id, const rtse::Box2 &new_box)
{
    std::cout << "[RTree] update " << id << "." << std::endl;

    assert(id_to_box.find(id) != id_to_box.end()); // updated id should exist

    erase(id);
    insert(new_box, id);
}

std::vector<int> rtse::RTree::query_range(const rtse::Box2 &query_box) const
{
    std::vector<int> satisfied_ids;
    find_queried_boxes(root, query_box, satisfied_ids);
    return satisfied_ids;
}

// choose the leaf node for insertion
rtse::NodeVec rtse::RTree::choose_leaf(Node *cur_node,
                                       const rtse::Box2 &box) const
{
    if (cur_node->is_leaf)
        return NodeVec(1, cur_node);     // base case: leaf node
    assert(!cur_node->children.empty()); // empty node should not exist

    // recursive case: non-leaf node
    Node *min_ptr = cur_node->children.front();
    double min_enlargement = min_ptr->mbr.enlarge_area(box);
    for (auto &child : cur_node->children)
    {
        double enlarge_area = child->mbr.enlarge_area(box);
        if (enlarge_area < min_enlargement)
        {
            min_enlargement = enlarge_area;
            min_ptr = child;
        }
        else if (eq(enlarge_area, min_enlargement) &&
                 child->mbr.area() < min_ptr->mbr.area())
        {
            min_enlargement = enlarge_area;
            min_ptr = child;
        }
    }
    // returned vector will be: [leaf, parent, grandparent, ... ]
    auto vec = choose_leaf(min_ptr, box);
    vec.push_back(cur_node);
    return vec;
}

// insertion detail implementation
void rtse::RTree::insert_to_node(const rtse::NodeVec &vec, size_t level,
                                 const rtse::Box2 &box, int id)
{
    auto cur_node = vec[level];
    cur_node->mbr = Box2::merge(cur_node->mbr, box);
    if (!cur_node->is_leaf)
    {
        // enlarge the mbr of child node
        auto child = vec[level - 1];
        for (size_t i = 0; i < cur_node->children.size(); i++)
        {
            if (cur_node->children[i] == child)
                cur_node->boxes[i] = Box2::merge(cur_node->boxes[i], box);
        }
        insert_to_node(vec, level - 1, box, id); // recursive insertion
    }
    else
    {
        cur_node->boxes.push_back(box);
        cur_node->ids.push_back(id);
        // overflow occurrs
        if (cur_node->boxes.size() > M)
        {
            auto split_pair = split(cur_node);
            if (cur_node == root)
            {
                auto old_root = root;
                make_new_root(split_pair);
                delete old_root;
            }
            else
                adjust(vec, 1, split_pair);
        }
    }
}

// split overflow leaf node
std::pair<rtse::Node *, rtse::Node *> rtse::RTree::split(rtse::Node *node) const
{
    auto [node_A, node_B] = choose_boxes(node);
    // leaf case: boxes and ids
    if (node->is_leaf)
    {
        size_t cur_idx = 0, remained = node->size() - 2;
        while (cur_idx < node->size() && node_A->size() + remained > m &&
               node_B->size() + remained > m)
        {
            if (node->allocated[cur_idx])
            {
                ++cur_idx;
                continue;
            }
            int cur_id = node->ids[cur_idx];
            Box2 &cur_box = node->boxes[cur_idx];
            double enlarged_A = node_A->mbr.enlarge_area(cur_box),
                   enlarged_B = node_B->mbr.enlarge_area(cur_box);
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

            node->allocated[cur_idx++] = true;
            --remained;
        }
        if (node_A->size() + remained == m)
        {
            while (cur_idx < node->size())
            {
                if (node->allocated[cur_idx])
                {
                    ++cur_idx;
                    continue;
                }
                node_A->push_back(node->boxes[cur_idx], node->ids[cur_idx]);
                node->allocated[cur_idx++] = true;
                --remained;
            }
        }
        if (node_B->size() + remained == m)
        {
            while (cur_idx < node->size())
            {
                if (node->allocated[cur_idx])
                {
                    ++cur_idx;
                    continue;
                }
                node_B->push_back(node->boxes[cur_idx], node->ids[cur_idx]);
                node->allocated[cur_idx++] = true;
                --remained;
            }
        }
        // check if all geometries allocated
        assert(remained == 0);
        assert(node->allocated == std::vector<bool>(node->size(), true));
    }
    // non-leaf case: boxes and children
    else
    {
        size_t cur_idx = 0, remained = node->size() - 2;
        while (cur_idx < node->size() && node_A->size() + remained > m &&
               node_B->size() + remained > m)
        {
            if (node->allocated[cur_idx])
            {
                ++cur_idx;
                continue;
            }
            auto cur_ptr = node->children[cur_idx];
            double enlarged_A = node_A->mbr.enlarge_area(node->boxes[cur_idx]),
                   enlarged_B = node_B->mbr.enlarge_area(node->boxes[cur_idx]);
            // choose smaller enlarged area
            if (enlarged_A < enlarged_B)
                node_A->push_back(cur_ptr);
            else if (enlarged_A > enlarged_B)
                node_B->push_back(cur_ptr);
            // if tie, chooese smaller mbr
            else if (node_A->mbr.area() < node_B->mbr.area())
                node_A->push_back(cur_ptr);
            else if (node_A->mbr.area() > node_B->mbr.area())
                node_B->push_back(cur_ptr);
            // if tie again, choose smaller node
            else if (node_A->size() < node_B->size())
                node_A->push_back(cur_ptr);
            else if (node_A->size() > node_B->size())
                node_B->push_back(cur_ptr);
            // if still tie, add to node_A
            else
                node_A->push_back(cur_ptr);

            node->allocated[cur_idx++] = true;
            --remained;
        }
        if (node_A->size() + remained == m)
        {
            while (cur_idx < node->size())
            {
                if (node->allocated[cur_idx])
                {
                    ++cur_idx;
                    continue;
                }
                node_A->push_back(node->children[cur_idx]);
                node->allocated[cur_idx++] = true;
                --remained;
            }
        }
        if (node_B->size() + remained == m)
        {
            while (cur_idx < node->size())
            {
                if (node->allocated[cur_idx])
                {
                    ++cur_idx;
                    continue;
                }
                node_B->push_back(node->children[cur_idx]);
                node->allocated[cur_idx++] = true;
                --remained;
            }
        }
        // check if all geometries allocated
        assert(remained == 0);
        assert(node->allocated == std::vector<bool>(node->size(), true));
    }
    return {node_A, node_B};
}

// remove the overflow node and add the new nodes
void rtse::RTree::adjust(const rtse::NodeVec &vec, size_t level,
                         const std::pair<rtse::Node *, rtse::Node *> &new_nodes)
{
    assert(new_nodes.first !=
           new_nodes.second); // two nodes should not be the same
    assert(level < vec.size());

    auto node = vec[level], overflow_node = vec[level - 1];
    // remove the overflow node from its parent's childrean vector
    auto it_child =
        find(node->children.begin(), node->children.end(), vec[level - 1]);
    if (it_child != node->children.end())
        node->children.erase(it_child);
    auto it_box =
        find(node->boxes.begin(), node->boxes.end(), vec[level - 1]->mbr);
    if (it_box != node->boxes.end())
        node->boxes.erase(it_box);

    node->push_back(new_nodes.first);
    node->push_back(new_nodes.second);

    if (node->size() > M)
    {
        auto split_pair = split(node);
        if (level < vec.size() - 1)
        {
            adjust(vec, level + 1, split_pair);
        }
        else
        {
            auto old_root = root;
            make_new_root(split_pair);
            delete old_root;
        }
    }

    delete overflow_node;
}

// find two farest nodes to the axis with larger separation
std::pair<rtse::Node *, rtse::Node *>
rtse::RTree::choose_boxes(Node *node) const
{
    double overall_low, highest_low, lowest_high, overall_high;
    double separation_x, separation_y;
    double denom;
    size_t idxA_x = 0, idxB_x = 1, idxA_y = 0, idxB_y = 1;
    assert(node->size() >= 2);

    // calculate normalized separation of x-axis
    overall_low = lowest_high = std::numeric_limits<double>::infinity();
    highest_low = overall_high = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < node->boxes.size(); i++)
    {
        if (node->boxes[i].min().x() < overall_low)
            overall_low = node->boxes[i].min().x();
        if (node->boxes[i].min().x() > highest_low)
        {
            highest_low = node->boxes[i].min().x();
            idxA_x = i;
        }
        if (node->boxes[i].max().x() < lowest_high)
        {
            lowest_high = node->boxes[i].max().x();
            idxB_x = i;
        }
        if (node->boxes[i].max().x() > overall_high)
            overall_high = node->boxes[i].max().x();
    }
    denom = overall_high - overall_low;
    if (eq(denom, 0.0))
        separation_x = 0;
    else
        separation_x = std::max(0.0, (highest_low - lowest_high) / denom);

    // calculate normalized seperation of y-axis
    overall_low = lowest_high = std::numeric_limits<double>::infinity();
    highest_low = overall_high = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < node->boxes.size(); i++)
    {
        if (node->boxes[i].min().y() < overall_low)
            overall_low = node->boxes[i].min().y();
        if (node->boxes[i].min().y() > highest_low)
        {
            highest_low = node->boxes[i].min().y();
            idxA_y = i;
        }
        if (node->boxes[i].max().y() < lowest_high)
        {
            lowest_high = node->boxes[i].max().y();
            idxB_y = i;
        }
        if (node->boxes[i].max().y() > overall_high)
            overall_high = node->boxes[i].max().y();
    }
    denom = overall_high - overall_low;
    if (eq(denom, 0.0))
        separation_y = 0;
    else
        separation_y = std::max(0.0, (highest_low - lowest_high) / denom);

    // choose the axis with larger separation
    size_t idxA, idxB;
    if (eq(separation_x, separation_y) && eq(separation_x, 0))
    {
        idxA = 0;
        idxB = 1;
    }
    else if (separation_x > separation_y)
    {
        idxA = idxA_x;
        idxB = idxB_x;
    }
    else
    {
        idxA = idxA_y;
        idxB = idxB_y;
    }

    assert(idxA != idxB); // ensure we reference two nodes

    node->allocated = std::vector<bool>(node->size(), false);
    node->allocated[idxB] = node->allocated[idxA] = true;
    auto ptrA = new Node, ptrB = new Node;
    ptrB->is_leaf = ptrA->is_leaf = node->is_leaf;
    if (node->is_leaf)
    {
        ptrA->push_back(node->boxes[idxA], node->ids[idxA]);
        ptrB->push_back(node->boxes[idxB], node->ids[idxB]);
    }
    else
    {
        ptrA->push_back(node->children[idxA]);
        ptrB->push_back(node->children[idxB]);
    }
    return {ptrA, ptrB};
}

// resursively find the overlaped node
void rtse::RTree::find_queried_boxes(Node *node, const rtse::Box2 &target,
                                     std::vector<int> &ids) const
{
    if (node->is_leaf)
    {
        for (size_t i = 0; i < node->size(); i++)
        {
            if (target.overlap(node->boxes[i]))
                ids.push_back(node->ids[i]);
        }
    }
    else
    {
        for (size_t i = 0; i < node->size(); i++)
        {
            if (target.overlap(node->boxes[i]))
            {
                find_queried_boxes(node->children[i], target, ids);
            }
        }
    }
}

void rtse::RTree::make_new_root(
    const std::pair<rtse::Node *, rtse::Node *> &split_pair)
{
    auto new_root = new Node;
    new_root->is_leaf = false;
    new_root->push_back(split_pair.first);
    new_root->push_back(split_pair.second);
    root = new_root;
}

// search for leaf node
void rtse::RTree::choose_leaf(NodeVec &vec, Node *node, const Box2 &box,
                              int id) const
{
    if (node->is_leaf)
    {
        bool found = false;
        for (size_t i = 0; i < node->size(); i++)
            if (id == node->ids[i])
                found = true;
        if (found)
            vec = NodeVec(1, node);
    }
    else
    {
        if (vec.size() > 0)
            return; // the path is unique
        for (size_t i = 0; i < node->size(); i++)
        {
            if (node->boxes[i].overlap(box))
            {
                size_t origin_size = vec.size();
                choose_leaf(vec, node->children[i], box, id);
                if (vec.size() > origin_size)
                {
                    vec.push_back(node);
                    break;
                }
            }
        }
    }
}

rtse::Box2 rtse::RTree::remove_node(const NodeVec &vec, size_t level, int id)
{
    auto node = vec[level];
    if (level == 0)
    {
        size_t idx = 0;
        // bool found = false;
        for (size_t i = 0; i < node->size(); i++)
        {
            if (id == node->ids[i])
            {
                idx = i;
                // found = true;
                break;
            }
        }
        // assert(found == true); // matched id should be found
        node->ids.erase(node->ids.begin() + idx);
        node->boxes.erase(node->boxes.begin() + idx);
    }
    else
    {
        auto child_mbr = remove_node(vec, level - 1, id);
        size_t child_idx;
        // bool found = false;
        for (size_t i = 0; i < node->size(); i++)
        {
            if (vec[level - 1] == node->children[i])
            {
                child_idx = i;
                // found = true;
                break;
            }
        }
        // assert(found == true); // child_idx should be found
        if (vec[level - 1]->boxes.empty())
        {
            node->children.erase(node->children.begin() + child_idx);
            node->boxes.erase(node->boxes.begin() + child_idx);
            delete vec[level - 1];
        }
        else
        {
            node->boxes[child_idx] = child_mbr;
        }
    }
    node->update_mbr();
    return node->mbr;
}

void hello_core() { std::cout << "RTSE core initialized." << std::endl; }