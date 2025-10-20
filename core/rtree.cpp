#include <iostream>
#include <vector>
#include <memory>

namespace rtse {

struct Point2 {
    double x, y;
    Point2() = default;
    Point2(double x_, double y_): x(x_), y(y_) {};
};

struct Box2 {
    Point2 min;
    Point2 max;
};

struct Node {
    bool is_leaf;
    std::vector<Box2> boxes;
    std::vector<int> ids;
    std::vector<std::shared_ptr<Node>> children;
};

class RTree {
public:
    RTree() {
        root = std::make_shared<Node>();
        root->is_leaf = true;
    }
    void insert(const Box2& box, int id) {
        std::cout << "[RTree] 'insert' called." << std::endl;
    }

    void erase(int id) {
        std::cout << "[RTree] 'erase' called." << std::endl;
    }
    void update(int id, const Box2& new_box) {
        std::cout << "[RTree] 'update' called." << std::endl;
    }
    std::vector<int> query_range(const Box2& query_box) {
        std::cout << "[RTree] 'query_range' called." << std::endl;
        return root->ids;
    }
private:
    std::shared_ptr<Node> root;
};

}


void hello_core() {
    std::cout << "RTSE core initialized." << std::endl;
}