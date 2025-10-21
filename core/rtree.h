#pragma once
#include <vector>
#include <memory>
#include <deque>

namespace rtse {

constexpr double eps = 1e-9;
auto eq = [](double a, double b) { return std::abs(a-b) < eps; };

struct Point2 {
    double x, y;
    Point2() = default;
    Point2(double x_, double y_): x(x_), y(y_) {};
};

struct Box2 {
    bool is_empty;
    Point2 min;
    Point2 max;
    Box2();
    Box2(const Point2& p1, const Point2& p2);
    static Box2 from_point(const Point2& p);
    double area() const;
    bool overlap(const Box2& other) const;
    static Box2 merge(const Box2& box1, const Box2& box2);
    double enlarge_area(const Box2& other) const;
    bool operator==(const Box2& other) const noexcept;
    bool operator!=(const Box2& other) const noexcept;
};

struct Node {
    bool is_leaf;
    Box2 mbr;
    std::vector<Box2> boxes;
    std::vector<int> ids;
    std::vector<std::shared_ptr<Node>> children;
    std::pair<const Box2&, int> entry(size_t i) const;
    size_t size() const;
    void push_back(const Box2& box, int id);
    void push_back(const Box2& box, const NodePtr ptr);
};

using NodePtr = std::shared_ptr<Node>;
using NodeVec = std::vector<NodePtr>;
using NodeDeq = std::deque<NodePtr>;

class RTree {
public:
    RTree();
    void insert(const Box2& box, int id);

    void erase(int id);
    void update(int id, const Box2& new_box);
    std::vector<int> query_range(const Box2& query_box);
    NodeDeq choose_leaf(NodePtr cur_node, const Box2& box);
    void insert_to_node(NodeDeq deq, const Box2& box, int id);
    std::pair<NodePtr, NodePtr> split(const NodePtr& node);
    void adjust(NodePtr& node, const NodePtr& removed_node, const std::pair<NodePtr, NodePtr>& split_pair);
    std::pair<NodePtr, NodePtr> choose_boxes(const NodePtr& node);

private:
    NodePtr root;
    size_t M, m;
};

};