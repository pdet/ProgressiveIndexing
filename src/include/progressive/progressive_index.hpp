#pragma once
#include "../index/index.hpp"
#include "index/bptree.hpp"
#include "qs_avl_tree.hpp"
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <vector>

class ResultStruct {
  public:
    ResultStruct() : sum(0){};
    int64_t sum = 0;

    void reserve(size_t capacity) { (void)capacity; }

    inline void push_back(int64_t value) { sum += value; }
    inline void push_back(IdxColEntry value) { sum += value.m_key; }

    inline void maybe_push_back(IdxColEntry value, int maybe) { sum += maybe * value.m_key; }
    inline void maybe_push_back(int64_t value, int maybe) { sum += maybe * value; }
    inline void merge(ResultStruct other) { sum += other.sum; }
};

class RefinementScan {
  public:
    RefinementScan() : offsetLeft(-1), offsetLeftMiddle(-1), offsetRightMiddle(-1), offsetRight(-1), checkLeft(true), checkRight(true){};
    int64_t offsetLeft;
    int64_t offsetLeftMiddle;
    int64_t offsetRightMiddle;
    int64_t offsetRight;
    bool checkLeft;
    bool checkRight;
};

class ProgressiveIndex {
  public:
    ProgressiveIndex()
        : column(nullptr),current_position(0), fullIndex(nullptr), full_scan_time(0),
          interactivity_threshold(0), delta(0){};

    ProgressiveIndex(double delta, bool isTest)
        : column(nullptr), current_position(0), fullIndex(nullptr), full_scan_time(0),
          interactivity_threshold(0), delta(delta), isTest(isTest){};
    ProgressiveIndex(double interactivityThreshold, double fullScanTime, bool isTest)
        : column(nullptr), current_position(0), fullIndex(nullptr), full_scan_time(fullScanTime),
          interactivity_threshold(interactivityThreshold), delta(0), isTest(isTest),time_threshold(full_scan_time * interactivity_threshold){};

    //! If this set for true we run a bunch more checks
    bool isTest = false;
    virtual int64_t run_query(std::vector<int64_t>& originalColumn, int64_t low, int64_t high, std::vector<double>& time, size_t query_it) = 0;

  protected:
    //! Index Column
    std::unique_ptr<IdxCol> column;

    //! Converged BP-Tree
    std::unique_ptr<BPTree> fullIndex;
    size_t current_position;

    //! Adaptive Delta
    double full_scan_time;
    double interactivity_threshold;
    double delta;
    double time_threshold;

    //! Get Delta
    virtual double get_delta(std::vector<int64_t>& originalColumn, int64_t low, int64_t high) = 0;

    //! Get Delta
    virtual double get_cost_for_delta(std::vector<int64_t>& originalColumn, int64_t low, int64_t high, double delta) = 0;

    //! Create Phase
    //! In the creation phase we also perform scan simultaneously
    virtual ResultStruct create(std::vector<int64_t>& originalColumn, ssize_t& remaining_swaps, int64_t low, int64_t high) = 0;
    //! Refine Phase
    virtual void refine(ssize_t& remaining_swaps, int64_t low, int64_t high) = 0;
    //! Refinement Scan
    virtual ResultStruct refinement_scan(int64_t low, int64_t high) = 0;
    //! Consolidate Phase
    void consolidate(ssize_t& remaining_swaps);
    //! Consolidate Scan
    ResultStruct consolidate_scan(int64_t low, int64_t high);
};
