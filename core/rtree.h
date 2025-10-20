#pragma once
#include <vector>
#include <memory>
#include <deque>

namespace rtse {

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
};

struct Node {
    bool is_leaf;
    Box2 mbr;
    std::vector<Box2> boxes;
    std::vector<int> ids;
    std::vector<std::shared_ptr<Node>> children;
};

class RTree {
public:
    RTree();
    void insert(const Box2& box, int id);

    void erase(int id);
    void update(int id, const Box2& new_box);
    std::vector<int> query_range(const Box2& query_box);
    std::deque<std::shared_ptr<Node>> choose_leaf(std::shared_ptr<Node> cur_node, const Box2& box);
    void insert_to_node(std::deque<std::shared_ptr<Node>> deq, const Box2& box, int id);
    
private:
    std::shared_ptr<Node> root;
    size_t M, m;
};

};