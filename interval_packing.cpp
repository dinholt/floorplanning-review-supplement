
  size_t reference_sort_merge() {
    const size_t input_size = raw_size_;
    ProfileClock::time_point phase;
    if (detailed_pack_timing_active) phase = ProfileClock::now();
    std::sort(storage_.begin(), storage_.begin() + input_size,
              [](const BlockingSpan &a, const BlockingSpan &b) {
                return a.y_begin < b.y_begin;
              });
    if (aggregate_profile.enabled) aggregate_profile.y_reference_sort_calls++;
    if (detailed_pack_timing_active) {
      aggregate_profile.y_reference_sort_ns += elapsed_ns(phase);
      phase = ProfileClock::now();
    }
    use_unique_ = false;
    if (input_size == 0) { active_size_ = 0; return 0; }
    size_t write = 0, duplicates = 0;
    for (size_t read = 1; read < input_size; ++read) {
      if (storage_[read].y_begin == storage_[write].y_begin) {
        storage_[write].y_end = std::max(storage_[write].y_end, storage_[read].y_end);
        ++duplicates;
      } else {
        storage_[++write] = storage_[read];
      }
    }
    active_size_ = write + 1;
    if (aggregate_profile.enabled) aggregate_profile.y_reference_merge_visits += input_size - 1;
    if (detailed_pack_timing_active)
      aggregate_profile.y_reference_duplicate_merge_ns += elapsed_ns(phase);
    return duplicates;
  }

  int find_lowest_gap(int block_height) const {
    int y_placed = 0;
    for (size_t i = 0; i < active_size_; ++i) {
      const BlockingSpan &span = use_unique_ ? unique_storage_[i] : storage_[i];
      if (span.y_begin >= y_placed + block_height) break;
      if (span.y_end > y_placed) y_placed = span.y_end;
    }
    return y_placed;
  }

  void initialize(size_t maximum_spans) {
    storage_.resize(maximum_spans);
    unique_storage_.resize(maximum_spans);
    raw_histogram_.assign(maximum_spans + 1, 0);
    unique_histogram_.assign(maximum_spans + 1, 0);
    duplicate_histogram_.assign(maximum_spans + 1, 0);
    active_size_ = raw_size_ = 0;
    use_unique_ = false;
    raw_capacity_ = storage_.capacity();
    unique_capacity_ = unique_storage_.capacity();
  }

  void reset() { active_size_ = raw_size_ = 0; use_unique_ = false; }

  void append(int y_begin, int y_end) {
    if (raw_size_ >= storage_.size()) {
      aggregate_profile.flat_span_capacity_growth_count++;
      throw std::runtime_error("FlatBlockingSpans capacity exceeded");
    }
    storage_[raw_size_++] = BlockingSpan{y_begin, y_end};
    active_size_ = raw_size_;
  }

class FlatXStartIndex {
 public:
  void initialize(size_t maximum_blocks) {
    block_ids_.resize(maximum_blocks);
    active_size_ = 0;
  }

  void reset() { active_size_ = 0; }

  size_t insert(int block_id) {
    if (active_size_ >= block_ids_.size()) {
      aggregate_profile.x_index_capacity_growth_count++;
      throw std::runtime_error("FlatXStartIndex capacity exceeded");
    }
    const int x = hardblocks[block_id].x;
    size_t position = active_size_;
    while (position > 0 && hardblocks[block_ids_[position - 1]].x > x) {
      block_ids_[position] = block_ids_[position - 1];
      --position;
    }
    block_ids_[position] = block_id;
    const size_t shifted = active_size_ - position;
    ++active_size_;
    return shifted;
  }

  size_t collect_overlaps(int x_start, int x_end, std::vector<int> &output,
                          size_t &prefix_candidates) const {
    size_t hit_count = 0;
    prefix_candidates = 0;
    for (size_t i = 0; i < active_size_; ++i) {
      const HardBlock &placed = hardblocks[block_ids_[i]];
      if (placed.x >= x_end) break;
      ++prefix_candidates;
      if (placed.x + placed.width > x_start)
        output[hit_count++] = block_ids_[i];
    }
    return hit_count;
  }

 private:
  std::vector<int> block_ids_;
  size_t active_size_ = 0;
};

inline bool ExactXOverlap(int x_start, int x_end, const HardBlock &placed) {
  const int xleft = placed.x;
  const int xright = xleft + placed.width;
  return x_start < xright && x_end > xleft;
}

int FindLowestYReferenceMap(int x_start, int x_end, int block_height) {
  std::map<int, int> blocking_spans;
  size_t hit_count = 0;
  if (detailed_pack_timing_active) {
    const auto scan_start = ProfileClock::now();
    for (int element : placed_list)
      if (ExactXOverlap(x_start, x_end, hardblocks[element]))
        sampled_overlap_hits[hit_count++] = element;
    aggregate_profile.packing_overlap_scan_ns += elapsed_ns(scan_start);

    const auto insert_start = ProfileClock::now();
    for (size_t i = 0; i < hit_count; ++i) {
      const HardBlock &placed = hardblocks[sampled_overlap_hits[i]];
      blocking_spans[placed.y] = std::max(placed.y + placed.height, blocking_spans[placed.y]);
    }
    aggregate_profile.packing_span_insert_or_append_ns += elapsed_ns(insert_start);
  } else {
    for (int element : placed_list) {
      const HardBlock &placed = hardblocks[element];
      if (ExactXOverlap(x_start, x_end, placed)) {
        ++hit_count;
        blocking_spans[placed.y] = std::max(placed.y + placed.height, blocking_spans[placed.y]);
      }
    }
  }
  if (aggregate_profile.enabled) {
    aggregate_profile.total_placed_block_overlap_checks += placed_list.size();
    aggregate_profile.total_x_overlap_hits += hit_count;
    aggregate_profile.reference_map_insert_count += hit_count;
  }

  ProfileClock::time_point gap_start;
  if (detailed_pack_timing_active) gap_start = ProfileClock::now();
  int y_placed = 0;
  for (const auto &span : blocking_spans) {
    if (span.first >= y_placed + block_height) break;
    if (span.second > y_placed) y_placed = span.second;
  }
  if (detailed_pack_timing_active)
    aggregate_profile.packing_gap_search_ns += elapsed_ns(gap_start);
  return y_placed;
}

int FindLowestYFlatSpans(int x_start, int x_end, int block_height) {
  flat_blocking_spans.reset();
  size_t hit_count = 0;
  if (detailed_pack_timing_active) {
    const auto scan_start = ProfileClock::now();
    for (int element : placed_list)
      if (ExactXOverlap(x_start, x_end, hardblocks[element]))
        sampled_overlap_hits[hit_count++] = element;
    aggregate_profile.packing_overlap_scan_ns += elapsed_ns(scan_start);

    const auto append_start = ProfileClock::now();
    for (size_t i = 0; i < hit_count; ++i) {
      const HardBlock &placed = hardblocks[sampled_overlap_hits[i]];
      flat_blocking_spans.append(placed.y, placed.y + placed.height);
    }
    const uint64_t append_elapsed = elapsed_ns(append_start);
    aggregate_profile.packing_span_insert_or_append_ns += append_elapsed;
    aggregate_profile.y_span_collection_ns += append_elapsed;
  } else {
    for (int element : placed_list) {
      const HardBlock &placed = hardblocks[element];
      if (ExactXOverlap(x_start, x_end, placed)) {
        ++hit_count;
        flat_blocking_spans.append(placed.y, placed.y + placed.height);
      }
    }
  }
  if (aggregate_profile.enabled) {
    aggregate_profile.flat_span_reset_count++;
    aggregate_profile.flat_span_append_count += hit_count;
    aggregate_profile.total_placed_block_overlap_checks += placed_list.size();
    aggregate_profile.total_x_overlap_hits += hit_count;
  }

  ProfileClock::time_point order_start;
  if (detailed_pack_timing_active) order_start = ProfileClock::now();
  const size_t duplicates = flat_blocking_spans.order_and_merge_duplicates(block_height);
  if (aggregate_profile.enabled) {
    aggregate_profile.flat_span_sort_count++;
    aggregate_profile.flat_span_duplicate_key_count += duplicates;
    aggregate_profile.total_active_spans += flat_blocking_spans.size();
    aggregate_profile.maximum_active_spans = std::max<uint64_t>(
        aggregate_profile.maximum_active_spans, flat_blocking_spans.size());
  }
  if (detailed_pack_timing_active)
    aggregate_profile.packing_span_order_ns += elapsed_ns(order_start);

  ProfileClock::time_point gap_start;
  if (detailed_pack_timing_active) gap_start = ProfileClock::now();
  const int y_placed = flat_blocking_spans.find_lowest_gap(block_height);
  if (aggregate_profile.enabled) aggregate_profile.flat_span_gap_query_count++;
  if (detailed_pack_timing_active)
    aggregate_profile.packing_gap_search_ns += elapsed_ns(gap_start);
  return y_placed;
}

int FindLowestYFlatSpansIndexed(int x_start, int x_end, int block_height) {
  flat_blocking_spans.reset();
  size_t prefix_candidates = 0;
  ProfileClock::time_point scan_start;
  if (detailed_pack_timing_active) scan_start = ProfileClock::now();
  const size_t hit_count = flat_x_start_index.collect_overlaps(
      x_start, x_end, sampled_overlap_hits, prefix_candidates);
  if (detailed_pack_timing_active)
    aggregate_profile.packing_overlap_scan_ns += elapsed_ns(scan_start);

  ProfileClock::time_point append_start;
  if (detailed_pack_timing_active) append_start = ProfileClock::now();
  for (size_t i = 0; i < hit_count; ++i) {
    const HardBlock &placed = hardblocks[sampled_overlap_hits[i]];
    flat_blocking_spans.append(placed.y, placed.y + placed.height);
  }
  if (detailed_pack_timing_active) {
    const uint64_t append_elapsed = elapsed_ns(append_start);
    aggregate_profile.packing_span_insert_or_append_ns += append_elapsed;
    aggregate_profile.y_span_collection_ns += append_elapsed;
  }

  if (aggregate_profile.enabled) {
    aggregate_profile.flat_span_reset_count++;
    aggregate_profile.flat_span_append_count += hit_count;
    aggregate_profile.total_placed_block_overlap_checks += prefix_candidates;
    aggregate_profile.total_x_overlap_hits += hit_count;
    aggregate_profile.x_index_query_count++;
    aggregate_profile.x_index_prefix_candidate_count += prefix_candidates;
  }

  ProfileClock::time_point order_start;
  if (detailed_pack_timing_active) order_start = ProfileClock::now();
  const size_t duplicates = flat_blocking_spans.order_and_merge_duplicates(block_height);
  if (aggregate_profile.enabled) {
    aggregate_profile.flat_span_sort_count++;
    aggregate_profile.flat_span_duplicate_key_count += duplicates;
    aggregate_profile.total_active_spans += flat_blocking_spans.size();
    aggregate_profile.maximum_active_spans = std::max<uint64_t>(
        aggregate_profile.maximum_active_spans, flat_blocking_spans.size());
  }
  if (detailed_pack_timing_active)
    aggregate_profile.packing_span_order_ns += elapsed_ns(order_start);

  ProfileClock::time_point gap_start;
  if (detailed_pack_timing_active) gap_start = ProfileClock::now();
  const int y_placed = flat_blocking_spans.find_lowest_gap(block_height);
  if (aggregate_profile.enabled) aggregate_profile.flat_span_gap_query_count++;
  if (detailed_pack_timing_active)
    aggregate_profile.packing_gap_search_ns += elapsed_ns(gap_start);
  return y_placed;
}

void PackBtreePreorder(int cur_node, bool flag, bool place_root, PackingMode mode) {
  if (!hardblocks[cur_node].preplaced) {
    ProfileClock::time_point x_start_time;
    if (detailed_pack_timing_active) x_start_time = ProfileClock::now();
    if (place_root) {
      hardblocks[cur_node].x = 0;
    } else {
      const int parent = btree[cur_node].parent;
      if (!flag)
        hardblocks[cur_node].x = hardblocks[parent].x + hardblocks[parent].width;
      else
        hardblocks[cur_node].x = hardblocks[parent].x;
    }
    if (detailed_pack_timing_active)
      aggregate_profile.packing_x_assignment_ns += elapsed_ns(x_start_time);

    const int x_begin = hardblocks[cur_node].x;
    const int x_end = x_begin + hardblocks[cur_node].width;
    int y_placed;
    if (mode == PackingMode::REFERENCE_MAP)
      y_placed = FindLowestYReferenceMap(x_begin, x_end, hardblocks[cur_node].height);
    else if (mode == PackingMode::FLAT_SPANS_INDEXED)
      y_placed = FindLowestYFlatSpansIndexed(x_begin, x_end, hardblocks[cur_node].height);
    else
      y_placed = FindLowestYFlatSpans(x_begin, x_end, hardblocks[cur_node].height);

    ProfileClock::time_point commit_start;
    if (detailed_pack_timing_active) commit_start = ProfileClock::now();
    hardblocks[cur_node].y = y_placed;
    placed_list.push_back(cur_node);
    if (mode == PackingMode::FLAT_SPANS_INDEXED) {
      const size_t shifted = flat_x_start_index.insert(cur_node);
      if (aggregate_profile.enabled) {
        aggregate_profile.x_index_insert_count++;
        aggregate_profile.x_index_shifted_element_count += shifted;
        aggregate_profile.x_start_insertion_shift_count += shifted;
      }
    }
    if (detailed_pack_timing_active)
      aggregate_profile.packing_block_commit_ns += elapsed_ns(commit_start);
  }

  if (btree[cur_node].left_child != -1)
    PackBtreePreorder(btree[cur_node].left_child, false, false, mode);
  if (btree[cur_node].right_child != -1)
    PackBtreePreorder(btree[cur_node].right_child, true, false, mode);
}

void AddEdgeLocs()
{
  int left_x,bottom_y,right_x,top_y;
  for (int i = 0; i < num_hardblocks; i++)
    {
      if(i==0)
        {
          left_x = hardblocks[i].x + hardblocks[i].width;
          right_x = hardblocks[i].x;
          bottom_y = hardblocks[i].y + hardblocks[i].height;
          top_y = hardblocks[i].y;
        }
      else
        {
          left_x = min(left_x,hardblocks[i].x + hardblocks[i].width);
          right_x = max(right_x,hardblocks[i].x);
          bottom_y = min(bottom_y,hardblocks[i].y + hardblocks[i].height);
          top_y = max(top_y,hardblocks[i].y);
        }
    }
    for (int i = 0; i < num_hardblocks; i++)
      {
        edge_loc horiz = NC;
        edge_loc vert = NC;

        if(hardblocks[i].x == 0)
          horiz = LEFT;

        if(hardblocks[i].x +  hardblocks[i].width  > right_x)
          horiz = RIGHT;

        if(hardblocks[i].y   == 0)
          vert = BOTTOM;


        if(hardblocks[i].y + hardblocks[i].height  > top_y)
          vert = TOP;

        hardblocks[i].actual_loc = (edge_loc)(horiz + vert);
        
      }
    
    
}

void ResetPlacedBlocksForPacking(PackingMode mode) {
  ProfileClock::time_point bookkeeping_start;
  if (detailed_pack_timing_active) bookkeeping_start = ProfileClock::now();
  placed_list.clear();
  if (mode == PackingMode::FLAT_SPANS_INDEXED) {
    flat_x_start_index.reset();
    if (aggregate_profile.enabled) aggregate_profile.x_index_reset_count++;
  }
  for (int i = 0; i < num_hardblocks; ++i) {
    if (!hardblocks[i].preplaced) continue;
    placed_list.push_back(i);
    if (mode == PackingMode::FLAT_SPANS_INDEXED) {
      const size_t shifted = flat_x_start_index.insert(i);
      if (aggregate_profile.enabled) {
        aggregate_profile.x_index_insert_count++;
        aggregate_profile.x_index_shifted_element_count += shifted;
        aggregate_profile.x_start_insertion_shift_count += shifted;
      }
    }
  }
  if (detailed_pack_timing_active)
    aggregate_profile.packing_bookkeeping_ns += elapsed_ns(bookkeeping_start);
}

void RunGeometryPacker(PackingMode mode) {
  ResetPlacedBlocksForPacking(mode);
  PackBtreePreorder(root_block, false, true, mode);
  AddEdgeLocs();
}
