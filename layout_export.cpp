
void MaterializeBlockRows(std::vector<std::vector<int>> &rows) {
  ProfileClock::time_point export_start;
  if (aggregate_profile.enabled)
    export_start = ProfileClock::now();
  rows.resize(num_hardblocks);
  for (int i = 0; i < num_hardblocks; ++i) {
    rows[i].resize(5);
    rows[i][0] = hardblocks[i].x;
    rows[i][1] = hardblocks[i].y;
    rows[i][2] = hardblocks[i].width;
    rows[i][3] = hardblocks[i].height;
    rows[i][4] = hardblocks[i].actual_loc;
  }
  aggregate_profile.export_materialization_call_count++;
  aggregate_profile.exported_block_row_count += static_cast<uint64_t>(num_hardblocks);
  if (aggregate_profile.enabled) {
    const uint64_t ns = elapsed_ns(export_start);
    aggregate_profile.export_materialization_ns += ns;
    aggregate_profile.result_export_ns += ns;
  }
}

void MaterializeBtreePreorder(int cur_node, std::vector<std::vector<int>> &rows, size_t &row_index) {
  const int left = btree[cur_node].left_child;
  if (left != -1) {
    rows[row_index].resize(3);
    rows[row_index][0] = cur_node; rows[row_index][1] = left; rows[row_index][2] = 0;
    ++row_index;
    MaterializeBtreePreorder(left, rows, row_index);
  }
  const int right = btree[cur_node].right_child;
  if (right != -1) {
    rows[row_index].resize(3);
    rows[row_index][0] = cur_node; rows[row_index][1] = right; rows[row_index][2] = 1;
    ++row_index;
    MaterializeBtreePreorder(right, rows, row_index);
  }
}

void MaterializeBtreeRows(std::vector<std::vector<int>> &rows) {
  ProfileClock::time_point export_start;
  if (aggregate_profile.enabled)
    export_start = ProfileClock::now();
  const size_t expected_rows = num_hardblocks > 0 ? static_cast<size_t>(num_hardblocks - 1) : 0;
  rows.resize(expected_rows);
  size_t row_index = 0;
  if (root_block != -1)
    MaterializeBtreePreorder(root_block, rows, row_index);
  rows.resize(row_index);
  aggregate_profile.export_materialization_call_count++;
  aggregate_profile.exported_btree_row_count += static_cast<uint64_t>(row_index);
  if (aggregate_profile.enabled) {
    const uint64_t ns = elapsed_ns(export_start);
    aggregate_profile.export_materialization_ns += ns;
    aggregate_profile.result_export_ns += ns;
  }
}

void BtreeToFloorplan()
{
    PackForCost();
    MaterializeBlockRows(current_loc);
    MaterializeBtreeRows(current_btree);
}

std::vector<std::vector<int>> getBlockPos()
{
    aggregate_profile.result_getter_call_count++;
    // A rejected final candidate can leave coordinates stale after B*Tree
    // rollback, so always restore exact current-state geometry at this boundary.
    PackForCost();
    MaterializeBlockRows(blockPos);
    return blockPos;
}
