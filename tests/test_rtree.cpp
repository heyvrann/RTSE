#include "../core/rtree.h"
#include <algorithm>
#include <gtest/gtest.h>
#include <random>
#include <set>

using namespace rtse;

static std::set<int> as_set(const std::vector<int> &vec)
{
    return {vec.begin(), vec.end()};
}

TEST(RTreeBasic, InsertAndQuery)
{
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

TEST(RTreeOverlap, TouchBoundary)
{
    RTree tree;
    tree.insert(Box2(Point2(0, 0), Point2(1, 1)), 10);
    auto ids = tree.query_range(Box2(Point2(1, 1), Point2(2, 2)));
    EXPECT_EQ(ids.size(), 1);
    EXPECT_EQ(ids[0], 10);
}

TEST(RTreeOverlap, JustOutsideBoundary)
{
    RTree tree;
    tree.insert(Box2(Point2(0, 0), Point2(1, 1)), 9);
    auto ids = tree.query_range(Box2(Point2(1 + 1e-9, 1 + 1e-9), Point2(2, 2)));
    EXPECT_TRUE(ids.empty());
}

TEST(RTreeSplit, OverflowCreatesBalancedTree)
{
    RTree tree;
    for (int i = 0; i < 20; i++)
        tree.insert(Box2(Point2(i, i), Point2(i + 0.5, i + 0.5)), i);
    auto result = tree.query_range(Box2(Point2(0, 0), Point2(19, 19)));
    EXPECT_EQ(result.size(), 20);
}

TEST(RTreeCorrectness, RandomAgainBruteForce)
{
    RTree tree;
    std::vector<std::pair<Box2, int>> data;
    for (int i = 0; i < 100; i++)
    {
        double x1 = i, y1 = i, x2 = i + 1, y2 = i + 1;
        Box2 b(Point2(x1, y1), Point2(x2, y2));
        data.push_back({b, i});
        tree.insert(b, i);
    }

    Box2 query(Point2(20, 20), Point2(50, 50));
    auto ids_tree = tree.query_range(query);

    std::vector<int> ids_brute;
    for (auto &[b, id] : data)
        if (query.overlap(b))
            ids_brute.push_back(id);

    std::sort(ids_tree.begin(), ids_tree.end());
    std::sort(ids_brute.begin(), ids_brute.end());
    EXPECT_EQ(ids_tree, ids_brute);
}

TEST(RTreeUpdate, MoveAcrossLevels)
{
    RTree tree;
    for (int i = 0; i < 64; ++i)
        tree.insert(Box2(Point2(i, i), Point2(i + 1, i + 1)), i);

    auto ids1 = tree.query_range(Box2(Point2(0, 0), Point2(20, 20)));
    EXPECT_TRUE(std::find(ids1.begin(), ids1.end(), 10) != ids1.end());

    tree.update(10, Box2(Point2(100, 100), Point2(101, 101)));

    auto ids2 = tree.query_range(Box2(Point2(0, 0), Point2(20, 20)));
    EXPECT_TRUE(std::find(ids2.begin(), ids2.end(), 10) == ids2.end());

    auto ids3 = tree.query_range(Box2(Point2(99, 99), Point2(102, 102)));
    EXPECT_TRUE(std::find(ids3.begin(), ids3.end(), 10) != ids3.end());
}

TEST(RTreeMixed, RandomVsOracle)
{
    std::mt19937 rng(314551132);
    std::uniform_real_distribution<double> U(0.0, 100.0);

    RTree tree;
    std::vector<std::pair<Box2, int>> oracle;

    auto rand_box = [&]()
    {
        double x1 = U(rng), y1 = U(rng), x2 = U(rng), y2 = U(rng);
        if (x1 == x2)
            x2 += 1e-3;
        if (y1 == y2)
            y2 += 1e-3;
        return Box2(Point2(x1, y1), Point2(x2, y2));
    };

    for (size_t i = 0; i < 100; i++)
    {
        auto box = rand_box();
        oracle.push_back({box, i});
        tree.insert(box, i);
    }

    size_t max_ids = 99;

    for (size_t step = 0; step < 300; step++)
    {
        int op = rng() % 3;
        if (op == 0)
        {
            int id = (int)(++max_ids);
            auto box = rand_box();
            oracle.push_back({box, id});
            tree.insert(box, id);
        }
        else if (op == 1 && !oracle.empty())
        {
            int idx = rng() % oracle.size();
            int id = oracle[idx].second;
            auto box = rand_box();
            oracle[idx].first = box;
            tree.update(id, box);
        }
        else if (op == 2 && !oracle.empty())
        {
            int idx = rng() % oracle.size();
            int id = oracle[idx].second;
            std::swap(oracle[idx], oracle.back());
            oracle.pop_back();
            tree.erase(id);
        }

        auto rand_range = rand_box();
        std::set<int> ids;
        for (auto &kv : oracle)
        {
            if (rand_range.overlap(kv.first))
                ids.insert(kv.second);
        }
        EXPECT_EQ(as_set(tree.query_range(rand_range)), ids);
    }
}

TEST(RTreeDuplicateBoxes, DifferentIdsBothReturned)
{
    RTree tree;
    Box2 box(Point2(1, 1), Point2(2, 2));
    tree.insert(box, 1);
    tree.insert(box, 2);
    auto s = as_set(tree.query_range(Box2(Point2(0, 0), Point2(3, 3))));
    EXPECT_EQ(s.size(), 2);
    EXPECT_TRUE(s.count(1));
    EXPECT_TRUE(s.count(2));
}

TEST(RTreeDegenerate, ZeroAreaPointOverlapBoundary)
{
    RTree tree;
    Box2 p = Box2::from_point(Point2(1, 1));
    tree.insert(p, 7);
    auto vec = tree.query_range(Box2(Point2(1, 1), Point2(2, 2)));
    EXPECT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], 7);
}

TEST(RTreeUpdate, NoOpKeepState)
{
    RTree tree;
    Box2 box(Point2(0, 0), Point2(1, 1));
    tree.insert(box, 10);
    tree.update(10, box);
    auto vec = tree.query_range(Box2(Point2(0, 0), Point2(2, 2)));
    EXPECT_EQ(vec[0], 10);
}