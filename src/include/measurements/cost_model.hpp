#pragma once

class CostModel{
public:
    CostModel();
    size_t PAGESIZE = 4096;
    size_t ELEMENTS_PER_PAGE = PAGESIZE/sizeof(IdxColEntry);
    size_t PAGES_TO_WRITE = 100000;
    size_t ELEMENT_COUNT = PAGES_TO_WRITE * ELEMENTS_PER_PAGE;
    //! Cost Model Constants (All in Milliseconds)
    //! Cost to write one page sequentially
    double WRITE_ONE_PAGE_SEQ_MS;
    //! Cost to read one page without checks
    double READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS;
    //! Cost to read one page sequentially with checks
    double READ_ONE_PAGE_SEQ_MS;
    //! Cost of randomly accessing one page
    double RANDOM_ACCESS_PAGE_MS;
    //! Cost of swapping one page
    double SWAP_COST_PAGE_MS;
    //! Main method to calculate all the cost model constants
    void calculate_constants();
private:
    double write_sequential_page_cost();

    double read_sequential_with_matches_page_cost();

    double read_sequential_without_matches_page_cost();

    double read_random_access();

    double swap_cost();
    //! Original Column
    std::unique_ptr<IdxCol> base_column;

    //! Our index
    std::unique_ptr<IdxCol> index;

    //! Initial Pivot
    std::unique_ptr<QSAVLNode> node;

    //! Query
    int64_t low = 1000;
    int64_t high = 20000;
    int64_t sum = 0;
};