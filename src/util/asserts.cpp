#include <iostream>
void printTree(IndexEntry *&column, AvlTree T, size_t buggy_element) {
    auto allNodes = GetNodesInOrder(T);
    std::cout << "Cracker Index\n";
    std::cout << "..." << "\n";
    bool printed_element = false;
    for (size_t i = 0; i< allNodes.size(); i ++) {
        std::cout << column[allNodes[i]->offset].m_key << "\n";
        std::cout << "[ " << allNodes[i]->Element << " - " << allNodes[i]->offset << "]\n";
        std::cout << column[allNodes[i]->offset + 1].m_key << "\n";
        std::cout << "..." << "\n";
        if (!printed_element && buggy_element > allNodes[i]->offset) {
            std::cout << buggy_element << " >>>> " << column[buggy_element].m_key << "\n";
            printed_element = true;
        }
        std::cout << "..." << "\n";
    }

}

void verify_tree(IndexEntry *&column, AvlTree T) {

    auto allNodes = GetNodesInOrder(T);
    for (size_t i = 0; i< allNodes.size(); i ++){
        if (i == 0) {
            for (size_t j = 0; j <= allNodes[i]->offset; j ++)
                if (!(allNodes[i]->Element > column[j].m_key)) {
                    printTree(column, T, j);
                    assert(0);
                }
        } else if (i == allNodes.size()-1) {
            for (size_t j = allNodes[i]->offset+1; j < COLUMN_SIZE-1; j ++)
                if (!(allNodes[i]->Element <= column[j].m_key)) {
                    printTree(column, T, j);
                    assert(0);
                }
        } else {
            for (size_t j = allNodes[i-1]->offset+1; j <= allNodes[i]->offset; j ++){
                if (!(allNodes[i]->Element > column[j].m_key)) {
                    printTree(column, T, j);
                    assert(0);
                }
                if (!(allNodes[i-1]->Element <= column[j].m_key)) {
                    printTree(column, T, j);
                    assert(0);
                }
            }
        }
    }

}

void check_column(IndexEntry *&column){
    for (size_t i = 0; i < COLUMN_SIZE; i ++){
        assert (column[i].m_key >= 1 || column[i].m_rowId >= 1);
    }
}

void check_updates(IndexEntry *&column){
    int count = -1;
    for (size_t i = 0; i < COLUMN_SIZE; i ++){
        if (column[i].m_rowId < 1)
            count++;
    }
}
