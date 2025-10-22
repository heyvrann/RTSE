#include "../core/rtree.h"
#include <gtest/gtest.h>

using namespace rtse;

TEST(RTreeBasic, InsertAndQuery) {
    RTree tree;
    tree.insert(Box2(Point2(0, 0), Point2(1, 1)), 1);
    tree.insert(Box2(Point2(2, 2), Point2(3, 3)), 2);
    tree.insert(Box2(Point2(4, 4), Point2(5, 5)), 3);

    auto result = tree.query_range(Box2(Point2(0.5, 0.5), Point2(4.5, 4.5)));

    EXPECT_EQ(result.size(), 3);
    EXPECT_TRUE(std::find(result.begin(), result.end(), 1) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(), 2) != result.end());
    EXPECT_TRUE(std::find(result.begin(), result.end(), 3) != result.end());
}

TEST(RTreeOverlap, TouchBoundary) {
    RTree tree;
    tree.insert(Box2(Point2(0, 0), Point2(1, 1)), 10);
    auto ids = tree.query_range(Box2(Point2(1, 1), Point2(2, 2)));
    EXPECT_EQ(ids.size(), 1);
    EXPECT_EQ(ids[0], 10);
}

TEST(RTreeSplit, OverflowCreatesBalancedTree) {
    RTree tree;
    for (int i = 0 ; i < 20 ; i++)
        tree.insert(Box2(Point2(i, i), Point2(i+0.5, i+0.5)), i);
    auto result = tree.query_range(Box2(Point2(0, 0), Point2(19, 19)));
    EXPECT_EQ(result.size(), 20);
}

TEST(RTreeCorrectness, RandomAgainBruteForce) {
    RTree tree;
    std::vector<std::pair<Box2, int>> data;
    for (int i = 0 ; i < 100 ; i++) {
        double x1= i, y1 = i, x2 = i+1, y2 = i+1;
        Box2 b(Point2(x1, y1), Point2(x2, y2));
        data.push_back({b, i});
        tree.insert(b, i);
    }

    Box2 query(Point2(20, 20), Point2(50, 50));
    auto ids_tree = tree.query_range(query);

    std::vector<int> ids_brute;
    for (auto& [b, id] : data) 
        if (query.overlap(b))
            ids_brute.push_back(id);

    std::sort(ids_tree.begin(), ids_tree.end());
    std::sort(ids_brute.begin(), ids_brute.end());
    EXPECT_EQ(ids_tree, ids_brute);
}