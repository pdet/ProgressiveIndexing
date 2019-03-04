//
// Created by Pedro Holanda on 04/03/19.
//

#include "print_main.h"

if (print_mode){
Workload W(COLUMN_SIZE, QUERIES_PATTERN, COLUMN_SIZE / 100 * SELECTIVITY_PERCENTAGE);
int64_t a, b;
for (
size_t i = 0;
i<NUM_QUERIES;
i++) {
W.
query(a, b
);
cout << i+1 << ";" << a << ";"<< b <<"\n";
}
}

//    if(true){
//        for (size_t i = 0; i < NUM_QUERIES; i++) {
//            cout << i+1 << ";" << rangequeries.leftpredicate[i] << ";"<< rangequeries.rightpredicate[i] <<"\n";
//        }
//    }
//    if(true){
//        int64_t highest = 0;
//        for (size_t i = 0; i < COLUMN_SIZE; i++) {
//            if(c.data[i]> highest)
//                highest = c.data[i];
//        }
//        fprintf(stderr, " max:  %lld \n",highest);
//    }
// if(true){
//     int64_t highest = 0;
//     size_t random;
//     for (size_t i = 0; i < COLUMN_SIZE/10000; i++) {
//         random = rand()%((i+1)*10000-i*10000 + 1) + i*10000;
//         if (i == 0)
//             cout << i+1 << ";" << c.data[random] <<"\n";
//         else
//             cout << i*10000 << ";" << c.data[random] <<"\n";
//     }
// }