#pragma once
#include <vector>
#include <memory>
#include <cmath>
#include <unordered_map>

namespace rtse {

constexpr double eps = 1e-9;
inline auto eq = [](double a, double b) { return std::abs(a-b) < eps; };

struct Point2 {
    Point2() = default;
    Point2(double x_, double y_): m_x(x_), m_y(y_) {};
    double x() const;
    double y() const;
private:
    double m_x, m_y;
};

struct Box2 {
    Box2();
    Box2(const Point2& p1, const Point2& p2);
    const Point2& min() const;
    const Point2& max() const;
    bool is_empty() const;
    static Box2 from_point(const Point2& p);
    double area() const;
    bool overlap(const Box2& other) const;
    static Box2 merge(const Box2& box1, const Box2& box2);
    double enlarge_area(const Box2& other) const;
    bool operator==(const Box2& other) const noexcept;
    bool operator!=(const Box2& other) const noexcept;
private:
    bool m_is_empty;
    Point2 m_min;
    Point2 m_max;
};

struct Node {
    bool is_leaf;
    Box2 mbr;
    std::vector<Box2> boxes;
    std::vector<int> ids;
    std::vector<Node*> children;
    std::vector<bool> allocated;
    std::pair<const Box2&, int> entry(size_t i) const;
    size_t size() const;
    void push_back(const Box2& box, int id);
    void push_back(Node* ptr);
    void update_mbr();
};

using NodeVec = std::vector<Node*>;

class RTree {
public:
    RTree();
    ~RTree();
    // RTree(const RTree&) = delete;
    // RTree& operator=(const RTree&) = delete;
    // RTree(RTree&&) = delete;
    // RTree& operator=(RTree&&) = delete;
    void insert(const Box2& box, int id);
    void erase(int id);
    void update(int id, const Box2& new_box);
    std::vector<int> query_range(const Box2& query_box) const;

private:
    Node* root;
    size_t M, m;
    std::unordered_map<int, Box2> id_to_box;
    // private function for destructor
    void deallocate(Node* root);
    // private function for insert()
    NodeVec choose_leaf(Node* cur_node, const Box2& box) const;
    void insert_to_node(const NodeVec& vec, size_t level, const Box2& box, int id);
    std::pair<Node*, Node*> split(Node* node) const;
    void adjust(const NodeVec& vec, size_t level, const std::pair<Node*, Node*>& split_pair);
    std::pair<Node*, Node*> choose_boxes(Node* node) const;
    void make_new_root(const std::pair<Node*, Node*>& split_pair);
    // private function for query_range()
    void find_queried_boxes(Node* node, const Box2& target, std::vector<int>& ids) const;
    // private function for erase()
    void choose_leaf(NodeVec& vec, Node* node, const Box2& box, int id) const;
    Box2 remove_node(const NodeVec& vec, size_t level, int id);
};

};