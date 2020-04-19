#pragma once
#include "progressive_index.hpp"
#include "measurements/cost_model.hpp"

class ProgressiveQuicksort : public ProgressiveIndex {
  public:
    //! Progressive Avl Index Root
    std::unique_ptr<QSAVLTree> tree;

    ProgressiveQuicksort(double delta, bool isTest) : tree(nullptr), ProgressiveIndex(delta, isTest){
        constants = std::make_unique<CostModel>();
    };

    ProgressiveQuicksort(double interactivityThreshold, double fullScanTime, bool isTest)
        : ProgressiveIndex(interactivityThreshold, fullScanTime, isTest){
        constants = std::make_unique<CostModel>();
    };

    int64_t run_query(std::vector<int64_t>& originalColumn, int64_t low, int64_t high, std::vector<double>& time, size_t query_it) override;

  private:
    std::unique_ptr<CostModel> constants;
    ResultStruct create(std::vector<int64_t>& originalColumn, ssize_t& remaining_swaps, int64_t low, int64_t high) override;
    void refine(ssize_t& remaining_swaps, int64_t low, int64_t high) override;
    ResultStruct refinement_scan(int64_t low, int64_t high) override;
    double get_delta(std::vector<int64_t>& originalColumn, int64_t low, int64_t high) override;
    double get_cost_for_delta(std::vector<int64_t>& originalColumn, int64_t low, int64_t high, double estimatedDelta) override;

    QSAVLNode* node_refinement(QSAVLNode* node, ssize_t& remaining_swaps);

    std::unique_ptr<RefinementScan> find_offsets(int64_t low, int64_t high);

    void initializeRoot(int64_t pivot, size_t columnSize) {
        assert(!tree);
        tree = std::make_unique<QSAVLTree>(move(std::make_unique<QSAVLNode>(pivot, columnSize)), columnSize);
    }
};
