#include "../include/progressive/constants.h"
#include "../include/progressive/incremental.h"
#include "../include/util/binary_search.h"
#include "../include/util/hybrid_radix_insert_sort.h"

void range_query_sorted_subsequent_value(int64_t *index, size_t index_size, int64_t low, int64_t high, int64_t min,
                                         int64_t max, ResultStruct &results) {
	if (low <= min) {
		if (high >= max) {
			//! just add all the elements
			for (size_t i = 0; i < index_size; i++) {
				results.push_back(index[i]);
			}
		} else {
			//! no need for binary search to obtain first entry
			//! first entry is 0
			for (size_t i = 0; i < index_size; i++) {
				if (index[i] <= high) {
					results.push_back(index[i]);
				} else {
					break;
				}
			}
		}
	} else {
		//! perform binary search to find first element
		auto entry = &index[binary_search_gte(index, low, 0, index_size)];
		if (high >= max) {
			//! no need for check after binary search
			for (; entry != index + index_size; entry++) {
				results.push_back(*entry);
			}
		} else {
			for (; entry != index + index_size; entry++) {
				if (*entry <= high) {
					results.push_back(*entry);
				} else {
					break;
				}
			}
		}
	}
}

void range_query_sorted_subsequent_value(int64_t *index, size_t index_size, int64_t low, int64_t high,
                                         ResultStruct &results) {

	int64_t lower_bound = binary_search_gte(index, low, 0, index_size);
	int64_t high_bound = binary_search_lte(index, high, 0, index_size);
	for (int64_t i = lower_bound; i <= high_bound; i++) {
		results.push_back(index[i]);
	}
}

void SortedCheck(Column &c, QuicksortNode &node) {
	if (c.qs_index.nodes[node.left].sorted && c.qs_index.nodes[node.right].sorted) {
		node.sorted = true;
		node.left = -1;
		node.right = -1;
		if (node.position >= 0) {
			if (node.parent >= 0) {
				SortedCheck(c, c.qs_index.nodes[node.parent]);
			} else {
				SortedCheck(c, c.qs_index.root);
			}
		}
	}
}

void range_query_incremental_quicksort_recursive(Column &c, QuicksortNode &node, ResultStruct &results, int64_t low,
                                                 int64_t high, ssize_t &remaining_swaps) {
	int64_t *index = c.qs_index.data;
	size_t *pointers = c.qs_index.index;
	if (node.sorted) {
		Profiler::Start(PROFILE_BASE_SCAN);
		if (low <= node.min && high >= node.max) {
			//! query contains entire node, just add all the entries to the result
			for (size_t i = node.start; i < node.end; i++) {
				results.push_back(index[i]);
			}
		} else {
			range_query_sorted_subsequent_value(c.qs_index.data + node.start, node.end - node.start, low, high,
			                                    node.min, node.max, results);
		}
		Profiler::End(PROFILE_BASE_SCAN);
		Profiler::AddTuples(PROFILE_BASE_SCAN, node.end - node.start);
		return;
	}

	if (node.left < 0) {
		if (node.min == node.max) {
			Profiler::Start(PROFILE_BASE_SCAN);
			node.sorted = true;
			SortedCheck(c, node.parent >= 0 ? c.qs_index.nodes[node.parent] : c.qs_index.root);
			range_query_sorted_subsequent_value(c.qs_index.data + node.start, node.end - node.start, low, high,
			                                    node.min, node.max, results);
			Profiler::End(PROFILE_BASE_SCAN);
			Profiler::AddTuples(PROFILE_BASE_SCAN, node.end - node.start);
			return;
		} else if ((node.end - node.start) <= 1024) {
			//! node is very small, just sort it normally
			if (remaining_swaps > (node.end - node.start) * 5) {
				Profiler::Start(PROFILE_INDEX_SORT);
				itqs(c.qs_index.data + node.start, c.qs_index.index + node.start, node.end - node.start);
				node.sorted = true;
				SortedCheck(c, node.parent >= 0 ? c.qs_index.nodes[node.parent] : c.qs_index.root);
				remaining_swaps -= (node.end - node.start) * 5; //! log2(8192)
				range_query_sorted_subsequent_value(c.qs_index.data + node.start, node.end - node.start, low, high,
				                                    node.min, node.max, results);
				Profiler::End(PROFILE_INDEX_SORT);
				Profiler::AddTuples(PROFILE_INDEX_SORT, node.end - node.start);
			} else {
				Profiler::Start(PROFILE_BASE_SCAN);
				//! node is small but we don't have enough swaps left to sort
				//! scan to add to result
				for (size_t i = node.start; i < node.end; i++) {
					int matching = index[i] >= low && index[i] <= high;
					results.maybe_push_back(index[i], matching);
				}
				Profiler::End(PROFILE_BASE_SCAN);
				Profiler::AddTuples(PROFILE_BASE_SCAN, node.end - node.start);
			}
			return;
		}
		//! we pivot again here
		//! first if we have already done some swaps
		//! look into the parts we have swapped already
		Profiler::Start(PROFILE_INDEX_SCAN);
		if (low < node.pivot) {
			for (size_t i = node.start; i < node.current_start; i++) {
				int matching = index[i] >= low && index[i] <= high;
				results.maybe_push_back(index[i], matching);
			}
			Profiler::AddTuples(PROFILE_INDEX_SCAN, node.current_start - node.start);
		}
		if (high >= node.pivot) {
			for (size_t i = node.current_end + 1; i < node.end; i++) {
				int matching = index[i] >= low && index[i] <= high;
				results.maybe_push_back(index[i], matching);
			}
			Profiler::AddTuples(PROFILE_INDEX_SCAN, node.end - node.current_end);
		}
		Profiler::End(PROFILE_INDEX_SCAN);

		auto old_start = node.current_start;
		auto old_end = node.current_end;

		//! now we crack some pieces baby
		Profiler::Start(PROFILE_INDEX_SWAP);
		while (node.current_start < node.current_end && remaining_swaps > 0) {
			int64_t start = index[node.current_start];
			int64_t end = index[node.current_end];
			size_t start_pointer = pointers[node.current_start];
			size_t end_pointer = pointers[node.current_end];

			int start_has_to_swap = start >= node.pivot;
			int end_has_to_swap = end < node.pivot;
			int has_to_swap = start_has_to_swap * end_has_to_swap;

			index[node.current_start] = !has_to_swap * start + has_to_swap * end;
			index[node.current_end] = !has_to_swap * end + has_to_swap * start;
			pointers[node.current_start] = !has_to_swap * start_pointer + has_to_swap * end_pointer;
			pointers[node.current_end] = !has_to_swap * end_pointer + has_to_swap * start_pointer;

			node.current_start += !start_has_to_swap + has_to_swap;
			node.current_end -= !end_has_to_swap + has_to_swap;
			remaining_swaps--;
		}
		Profiler::End(PROFILE_INDEX_SWAP);

		//! scan the remainder (if any)
		Profiler::Start(PROFILE_BASE_SCAN);
		for (size_t i = old_start; i <= old_end; i++) {
			int matching = index[i] >= low && index[i] <= high;
			results.maybe_push_back(index[i], matching);
		}
		Profiler::End(PROFILE_BASE_SCAN);
		Profiler::AddTuples(PROFILE_BASE_SCAN, old_end - old_start);
		if (node.current_start >= node.current_end) {
			if (node.current_start == node.start || node.current_end == node.end - 1) {
				//! either the left or right side would have zero entries
				//! this means either (1) the pivot was chosen poorly
				//!                or (2) the whole chunk only contains the same value
				int64_t old_pivot = node.pivot;
				//! otherwise in (1) we pivot again, but move the pivot closer to the min or max
				if (node.current_start == node.start) {
					assert(node.pivot != node.max);
					node.pivot = node.pivot / 2 + node.max / 2;
				} else {
					assert(node.pivot != node.min);
					node.pivot = node.pivot / 2 + node.min / 2;
				}
				if (node.pivot == old_pivot) {
					//! everything apparently has the same value
					node.sorted = true;
					SortedCheck(c, node.parent >= 0 ? c.qs_index.nodes[node.parent] : c.qs_index.root);
					return;
				}
				node.current_start = node.start;
				node.current_end = node.end - 1;
				return;
			}

			node.left = c.qs_index.nodes.size();
			QuicksortNode left(node.left, node.position);
			left.start = node.start;
			left.end = index[node.current_start] < node.pivot ? node.current_start + 1 : node.current_start;
			left.current_start = left.start;
			left.current_end = left.end - 1;
			left.min = node.min;
			left.max = node.pivot;
			left.pivot = (index[left.current_start] + index[left.current_end]) / 2;
			node.right = c.qs_index.nodes.size() + 1;
			QuicksortNode right(node.right, node.position);
			right.start = left.end;
			right.end = node.end;
			right.current_start = right.start;
			right.current_end = right.end - 1;
			right.min = node.pivot;
			right.max = node.max;
			right.pivot = (index[right.current_start] + index[right.current_end]) / 2;
			c.qs_index.nodes.push_back(left);
			c.qs_index.nodes.push_back(right);
		}
		return;
	} else {
		auto left = node.left;
		auto right = node.right;
		auto pivot = node.pivot;
		assert(left >= 0 && right >= 0);
		//! node has children, go into one of the children
		if (low < pivot) {
			range_query_incremental_quicksort_recursive(c, c.qs_index.nodes[left], results, low, high, remaining_swaps);
		}
		if (high >= pivot) {
			range_query_incremental_quicksort_recursive(c, c.qs_index.nodes[right], results, low, high,
			                                            remaining_swaps);
		}
	}
}

ResultStruct range_query_incremental_quicksort(Column &c, int64_t low, int64_t high, double delta) {
	if (!c.qs_index.index) {
		//! fill the initial sortindex
		//! choose an initial pivot point
		c.qs_index.index = (size_t *)malloc(sizeof(size_t) * c.data.size());
		c.qs_index.data = (int64_t *)malloc(sizeof(int64_t) * c.data.size());
		c.qs_index.root.pivot = (c.data.front() + c.data.back()) / 2;
		c.qs_index.root.start = 0;
		c.qs_index.root.end = c.data.size();
		c.qs_index.root.current_start = c.qs_index.root.start;
		c.qs_index.root.current_end = c.qs_index.root.end - 1;
		c.qs_index.root.min = INT_MIN;
		c.qs_index.root.max = INT_MAX;
		c.qs_index.current_position = 0;
	}

	ResultStruct results;
	results.reserve(c.data.size());

	if (c.qs_index.root.sorted) {
		Profiler::Start(PROFILE_BINARY_SEARCH);
		c.converged = true;
		c.final_data = c.qs_index.data;
		//! array is sorted entirely already
		range_query_sorted_subsequent_value(c.qs_index.data, c.data.size(), low, high, results);
		Profiler::End(PROFILE_BINARY_SEARCH);
		return results;
	}

	ssize_t remaining_swaps = (ssize_t)(c.data.size() * delta);

	//! start doing swaps
	bool initial_run = c.qs_index.root.left < 0;
	if (initial_run) {
		int64_t *index = c.qs_index.data;
		size_t *pointers = c.qs_index.index;

		//! for the initial run, we write the indices instead of swapping them
		//! because the current array has not been initialized yet
		QuicksortNode &node = c.qs_index.root;
		//! first look through the part we have already pivoted
		//! for data that matches the points
		Profiler::Start(PROFILE_INDEX_SCAN);
		if (low < node.pivot) {
			for (size_t i = 0; i < node.current_start; i++) {
				int matching = index[i] >= low && index[i] <= high;
				results.maybe_push_back(index[i], matching);
			}
		}
		if (high >= node.pivot) {
			for (size_t i = node.current_end + 1; i < c.data.size(); i++) {
				int matching = index[i] >= low && index[i] <= high;
				results.maybe_push_back(index[i], matching);
			}
		}
		Profiler::End(PROFILE_INDEX_SCAN);

		Profiler::Start(PROFILE_INDEX_SWAP);
		//! now we start filling the index with at most remaining_swap entries
		size_t next_index = std::min(c.qs_index.current_position + remaining_swaps, c.data.size());
		remaining_swaps -= next_index - c.qs_index.current_position;
		for (size_t i = c.qs_index.current_position; i < next_index; i++) {
			int matching = c.data[i] >= low && c.data[i] <= high;
			results.maybe_push_back(c.data[i], matching);

			int bigger_pivot = c.data[i] >= node.pivot;
			int smaller_pivot = 1 - bigger_pivot;
			;

			index[node.current_start] = c.data[i];
			index[node.current_end] = c.data[i];
			pointers[node.current_start] = i;
			pointers[node.current_end] = i;

			node.current_start += smaller_pivot;
			node.current_end -= bigger_pivot;
		}
		Profiler::End(PROFILE_INDEX_SWAP);
		c.qs_index.current_position = next_index;
		if (next_index == c.data.size()) {
			//! we are finished with the initial run
			//! construct the left and right side of the root node
			QuicksortNode left(0);
			left.start = node.start;
			left.end = index[node.current_start] < node.pivot ? node.current_start + 1 : node.current_start;
			left.current_start = left.start;
			left.current_end = left.end - 1;
			left.pivot = (index[left.current_start] + index[left.current_end]) / 2;
			left.min = node.min;
			left.max = node.pivot;
			QuicksortNode right(1);
			right.start = left.end;
			right.end = node.end;
			right.current_start = right.start;
			right.current_end = right.end - 1;
			right.pivot = (index[right.current_start] + index[right.current_end]) / 2;
			right.min = node.pivot;
			right.max = node.max;
			c.qs_index.root.left = c.qs_index.nodes.size();
			c.qs_index.nodes.push_back(left);
			c.qs_index.root.right = c.qs_index.nodes.size();
			c.qs_index.nodes.push_back(right);
			c.qs_index.current_pivot = 0;
		} else {
			//! we have done all the swapping for this run
			//! now we query the remainder of the data
			Profiler::Start(PROFILE_BASE_SCAN);

			for (size_t i = c.qs_index.current_position; i < c.data.size(); i++) {
				int matching = c.data[i] >= low && c.data[i] <= high;
				results.maybe_push_back(c.data[i], matching);
			}
			Profiler::End(PROFILE_BASE_SCAN);
		}
	} else {
		range_query_incremental_quicksort_recursive(c, c.qs_index.root, results, low, high, remaining_swaps);
	}

	while (remaining_swaps > 0 && c.qs_index.current_pivot < c.qs_index.nodes.size()) {
		QuicksortNode &node = c.qs_index.nodes[c.qs_index.current_pivot];
		if (node.sorted || node.left >= 0) {
			c.qs_index.current_pivot++;
			continue;
		}
		if (node.min == node.max) {
			node.sorted = true;
			SortedCheck(c, node.parent >= 0 ? c.qs_index.nodes[node.parent] : c.qs_index.root);
			c.qs_index.current_pivot++;
		} else if ((node.end - node.start) <= 1024) {
			Profiler::Start(PROFILE_INDEX_SORT);
			if (remaining_swaps < (node.end - node.start) * 5) {
				//! not enough swaps left to sort
				break;
			}
			//! node is very small, just sort it normally
			itqs(c.qs_index.data + node.start, c.qs_index.index + node.start, node.end - node.start);
			node.sorted = true;
			SortedCheck(c, node.parent >= 0 ? c.qs_index.nodes[node.parent] : c.qs_index.root);
			remaining_swaps -= (node.end - node.start) * 5; //! log2(8192)
			c.qs_index.current_pivot++;
			Profiler::End(PROFILE_INDEX_SORT);
			Profiler::AddTuples(PROFILE_INDEX_SORT, node.end - node.start);
		} else {
			int64_t *index = c.qs_index.data;
			size_t *pointers = c.qs_index.index;

			ssize_t elements_in_piece = node.current_end - node.current_start;
			ssize_t swaps_to_do = std::min((ssize_t)remaining_swaps, elements_in_piece);
			Profiler::Start(PROFILE_INDEX_SWAP);
			while (node.current_start < node.current_end && remaining_swaps > 0) {
				int64_t start = index[node.current_start];
				int64_t end = index[node.current_end];
				size_t start_pointer = pointers[node.current_start];
				size_t end_pointer = pointers[node.current_end];

				int start_has_to_swap = start >= node.pivot;
				int end_has_to_swap = end < node.pivot;
				int has_to_swap = start_has_to_swap * end_has_to_swap;

				index[node.current_start] = !has_to_swap * start + has_to_swap * end;
				index[node.current_end] = !has_to_swap * end + has_to_swap * start;
				pointers[node.current_start] = !has_to_swap * start_pointer + has_to_swap * end_pointer;
				pointers[node.current_end] = !has_to_swap * end_pointer + has_to_swap * start_pointer;

				node.current_start += !start_has_to_swap + has_to_swap;
				node.current_end -= !end_has_to_swap + has_to_swap;
				remaining_swaps--;
			}
			Profiler::End(PROFILE_INDEX_SWAP);
			if (node.current_start >= node.current_end) {
				if (node.current_start == node.start || node.current_end == node.end - 1) {
					//! either the left or right side would have zero entries
					//! this means either (1) the pivot was chosen poorly
					//!                or (2) the whole chunk only contains the same value
					//! first check for (2)
					//! otherwise in (1) we pivot again, but move the pivot closer to the min or max
					remaining_swaps -= node.end - node.start;
					auto entry = index[node.start];
					if (node.current_start == node.start) {
						assert(node.pivot != node.max);
						node.pivot = (node.pivot + node.max) / 2;
					} else {
						assert(node.pivot != node.min);
						node.pivot = (node.pivot + node.min) / 2;
					}
					node.current_start = node.start;
					node.current_end = node.end - 1;
					continue;
				}
				node.left = c.qs_index.nodes.size();
				QuicksortNode left(node.left, node.position);
				left.start = node.start;
				left.end = index[node.current_start] < node.pivot ? node.current_start + 1 : node.current_start;
				left.current_start = left.start;
				left.current_end = left.end - 1;
				left.min = node.min;
				left.max = node.pivot;
				left.pivot = (index[left.current_start] + index[left.current_end]) / 2;
				node.right = c.qs_index.nodes.size() + 1;
				QuicksortNode right(node.right, node.position);
				right.start = left.end;
				right.end = node.end;
				right.current_start = right.start;
				right.current_end = right.end - 1;
				right.pivot = (index[right.current_start] + index[right.current_end]) / 2;
				right.min = node.pivot;
				right.max = node.max;
				c.qs_index.nodes.push_back(left);
				c.qs_index.nodes.push_back(right);
				c.qs_index.current_pivot++;
			}
		}
	}
	return results;
}

std::unordered_map<int, double> Profiler::times;
std::chrono::time_point<std::chrono::system_clock> Profiler::start;
std::unordered_map<int, size_t> Profiler::tuples;
std::unordered_map<int, size_t> Profiler::scan_counts;

QuicksortNode &FindLowestNode(Column &c, QuicksortNode &node, int64_t value) {
	if (node.left < 0) {
		return node;
	}
	return FindLowestNode(c, c.qs_index.nodes[value < node.pivot ? node.left : node.right], value);
}

size_t GetHeight(Column &c, QuicksortNode &node) {
	if (node.left < 0) {
		return 1;
	}
	return 1 + std::max(GetHeight(c, c.qs_index.nodes[node.left]), GetHeight(c, c.qs_index.nodes[node.right]));
}

double get_estimated_time_quicksort(Column &c, int64_t low, int64_t high, double delta) {
	if (c.qs_index.root.sorted || c.converged) {
		auto lower_bound = binary_search_gte(c.qs_index.data, low, 0, c.data.size());
		auto high_bound = binary_search_lte(c.qs_index.data, high, 0, c.data.size());
		double page_count = (high_bound - lower_bound) / ELEMENTS_PER_PAGE;
		double scan_cost = READ_ONE_PAGE_WITHOUT_CHECKS_SEQ_MS * page_count;
		return (scan_cost + RANDOM_ACCESS_PAGE_MS * log2(c.data.size())) / 1000.0;
	}
	bool initial_run = c.qs_index.root.left < 0;
	QuicksortNode &node = c.qs_index.root;
	double page_count = (c.data.size() / ELEMENTS_PER_PAGE) + (c.data.size() % ((int)ELEMENTS_PER_PAGE) != 0 ? 1 : 0);
	double scan_speed = READ_ONE_PAGE_SEQ_MS * page_count;
	if (initial_run) {
		//! figure out unindexed fraction
		double rho = (double)c.qs_index.current_position / (double)c.data.size();
		//! figure out fraction of index to scan, either low or high or both
		double alpha = 0;
		if (c.qs_index.current_position > 0) {
			if (low < node.pivot) {
				alpha += (double)node.current_start / (double)c.data.size();
			}
			if (high >= node.pivot) {
				alpha += (double)(c.data.size() - node.current_end) / (double)c.data.size();
			}
		}
		double pivot_speed = WRITE_ONE_PAGE_SEQ_MS * page_count;
		return ((1 - rho + alpha - delta) * scan_speed + delta * pivot_speed) / 1000.0;
	} else {
		size_t height = GetHeight(c, node);
		double lookup_speed = height * RANDOM_ACCESS_PAGE_MS;
		double refine_speed = WRITE_ONE_PAGE_SEQ_MS * page_count;

		//! figure out alpha
		auto left_node = FindLowestNode(c, node, low);
		auto right_node = FindLowestNode(c, node, high);

		auto left_position = left_node.start;
		auto right_position = right_node.end;
		if (left_node.sorted) {
			left_position = left_node.start + binary_search_gte(c.qs_index.data + left_node.start, low, 0,
			                                                    left_node.end - left_node.start);
		}
		if (right_node.sorted) {
			right_position = right_node.start + binary_search_lte(c.qs_index.data + right_node.start, high, 0,
			                                                      right_node.end - right_node.start);
		}

		double alpha = (double)(right_position - left_position) / (double)c.data.size();
		assert(alpha <= 1);
		return (lookup_speed + alpha * scan_speed + delta * refine_speed) / 1000.0;
	}
}

void IncrementalQuicksortIndex::clear() {
	nodes.clear();
	root.left = -1;
	root.right = -1;
	root.sorted = false;
	current_pivot = 0;
	//	if (index)
	//		free (index);
	//	if (data)
	//		free (data);
	index = nullptr;
	data = nullptr;
}
