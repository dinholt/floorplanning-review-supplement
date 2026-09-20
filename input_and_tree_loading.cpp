// SPDX-License-Identifier: Apache-2.0
// Inherited PARSAC code, not the submission optimization.
// Unchanged bodies; surrounding source omitted. See README.md.

void ReadHardBlocks(std::vector<std::vector<int>> hardblocks_list,bool initialize)
{

    num_hardblocks = hardblocks_list.size();
    num_clusters = 0;
    num_clusters_rectilinear = 0;
    total_block_area = 0;
    hardblocks = vector<HardBlock>(num_hardblocks);
    initblocks = vector<HardBlock>(num_hardblocks);
    for (int i = 0; i < num_hardblocks; i++) {

        int width = hardblocks_list[i][0];
        int height = hardblocks_list[i][1];
        edge_loc required_loc = (edge_loc) hardblocks_list[i][2];
        int cluster_id = hardblocks_list[i][3];
        int rectilinear_id = hardblocks_list[i][4];        
        bool fixed_block = (bool)hardblocks_list[i][5];
        bool preplaced = (bool)hardblocks_list[i][6];
        int x_preplaced = hardblocks_list[i][7];
        int y_preplaced = hardblocks_list[i][8];                
        num_clusters = max(num_clusters,cluster_id);
        num_clusters_rectilinear = max(num_clusters_rectilinear,rectilinear_id);        
        
        hardblocks[i].id = i;
        hardblocks[i].x = hardblocks[i].required_x = (preplaced ? x_preplaced : -1);
        hardblocks[i].y = hardblocks[i].required_y = (preplaced ? y_preplaced : -1);
        hardblocks[i].width = width;
        hardblocks[i].height = height;
        hardblocks[i].orig_width = width;
        hardblocks[i].orig_height = height;
        hardblocks[i].required_loc = required_loc;
        hardblocks[i].cluster_id = cluster_id;
        hardblocks[i].rectilinear_id = rectilinear_id;        
        hardblocks[i].fixed_block = fixed_block;
        hardblocks[i].preplaced = preplaced;                                
        hardblocks[i].soft_preplaced = false;
        
        hardblocks[i].rotate = 0;
        if(initialize)
          {
            //will not be changed
            initblocks[i].id = i;
            initblocks[i].x = initblocks[i].required_x = (preplaced ? x_preplaced : -1);
            initblocks[i].y = initblocks[i].required_y = (preplaced ? y_preplaced : -1);
            initblocks[i].width = width;
            initblocks[i].height = height;
            initblocks[i].rotate = 0;
            initblocks[i].required_loc = required_loc;
            initblocks[i].cluster_id = cluster_id;
            initblocks[i].rectilinear_id = rectilinear_id;
            initblocks[i].fixed_block = fixed_block;
            initblocks[i].preplaced = preplaced;
            initblocks[i].soft_preplaced = false;            
          }

        
        total_block_area += width * height;
    }

    area_target = total_block_area * (1 + white_space_ratio);
    //FROM OUTLINE
    //W = sqrt(area_target);//for square chip
    //H = sqrt(area_target);// for square chip

    //non-square SOC chip_ar = Chip Width / Chip Height
    H = int(sqrt(area_target/chip_ar));
    W = int(area_target / H);

    cout << "Area:             " << total_block_area << '\n';
    cout << "Target Area:      " << area_target << '\n';
    cout << "W:                " << W << '\n';
    cout << "H:                " << H << '\n';
    cout << "Chip AR:          " << chip_ar << '\n';  
    cout << '\n';

}

void ReadNets(std::vector<std::vector<int>> nets_list)
{
    //nets = vector<vector<int>>(num_nets);
    nets = nets_list;

}

void ReadTerminals(std::vector<std::vector<int>> terminals_list)
{

    num_terminals = terminals_list.size();

    terminals = vector<Terminal>(num_terminals);
    for (int i = 0; i < num_terminals; i++) {
        terminals[i].id = i;
        terminals[i].x = terminals_list[i][0];
        terminals[i].y = terminals_list[i][1];
    }


}

void BuildInitBtree()
{
    btree = vector<Node>(num_hardblocks);
    queue<int> bfs;
    vector<int> inserted(num_hardblocks, 0);

    root_block = rand() % num_hardblocks;
    btree[root_block].parent = -1;
    bfs.push(root_block);
    inserted[root_block] = 1;

    int left = num_hardblocks - 1;
    while (!bfs.empty()) {
        int parent = bfs.front();
        bfs.pop();
        int left_child = -1, right_child = -1;
        if (left > 0) {
            do {
                left_child = rand() % num_hardblocks;
            } while (inserted[left_child]);
            btree[parent].left_child = left_child;
            bfs.push(left_child);
            inserted[left_child] = 1;
            left--;
            if (left > 0) {
                do {
                    right_child = rand() % num_hardblocks;
                } while (inserted[right_child]);
                btree[parent].right_child = right_child;
                bfs.push(right_child);
                inserted[right_child] = 1;
                left--;
            }
        }
        btree[parent].left_child = left_child;
        btree[parent].right_child = right_child;
        if (left_child != -1)
            btree[left_child].parent = parent;
        if (right_child != -1)
            btree[right_child].parent = parent;
    }
}

void LoadBTree(std::vector<std::vector<int>>  tree_list){
    cout << "Loading binary tree " << "\n";
    btree = vector<Node>(num_hardblocks);
    vector<int> inserted(num_hardblocks, 0);

    int n1, n2, edge_type;
    //cout << "TREE Initializating \n";
    //initialize childs for each node
    for (int i = 0; i < num_hardblocks; i++) {
        btree[i].left_child = -1;
        btree[i].right_child = -1;
        btree[i].parent = -1;
    }

    int num_edges = tree_list.size();
    for (int i = 0; i < num_edges; i++) {
        edge_type = tree_list[i][2] ;
        n1 = tree_list[i][0];
        n2 = tree_list[i][1];
        //cout << "Edge : " << i << " :: " << edge_type << " " << n1 << " " << n2 << "\n"; 

        if (edge_type == 0) {
            //cout << "LEFT EDGE: " << n1 << " : " << n2 << "\n";
            inserted[n1] = 1;
            inserted[n2] = 1;
            btree[n1].left_child = n2;
            btree[n2].parent = n1;
            if (i == 0) {
                root_block = n1;
            }
        } else if (edge_type == 1) {
            //cout << "RIGHT EDGE: " << n1 << " : " << n2 << "\n";
            inserted[n1] = 1;
            inserted[n2] = 1;
            btree[n1].right_child = n2;
            btree[n2].parent = n1;
            if (i == 0) {
                root_block = n1;
            }
        } else {
            cout << "[ERROR]: Invalid edge found in the tree file: \n";
        }
    }



    bool hasZero = false;

    for (int number : inserted) {
        //cout << "Inserted element " << number << "\n";
        if (number == 0) {
            hasZero = true;
            break;  // Exit the loop early if "0" is found
        }
    }

    if (hasZero) {
        std::cout << "TREE is INVALID..." << std::endl;
    } else {
        std::cout << "[SUCCESS] TREE IS VALID..." << std::endl;
    }



}

void BuildPreExistBTree(std::vector<std::vector<int>> tree_list){
    btree = vector<Node>(num_hardblocks);
    vector<int> inserted(num_hardblocks, 0);
    if(tree_list.size() != num_hardblocks - 1)
      {
        py::print("Error : Invalid tree with only ",tree_list.size()," entries. Expecting ",num_hardblocks - 1);
        return ;
      }

    int temp1, temp2, temp3;

    //initialize childs for each node
    for (int i = 0; i < num_hardblocks; i++) {
        btree[i].left_child = -1;
        btree[i].right_child = -1;
        btree[i].parent = -1;
    }

    for (int i = 0; i < num_hardblocks-1; i++) {
      temp1 = tree_list[i][0];
      temp2 = tree_list[i][1];
      temp3 = tree_list[i][2];
      
      if (temp3 == 0) {
        //cout << "LEFT EDGE: " << temp1 << " : " << temp2 << "\n";
        inserted[temp1] = 1;
        inserted[temp2] = 1;
        btree[temp1].left_child = temp2;
        btree[temp2].parent = temp1;
        if (i == 0) {
          root_block = temp1;
        }
      } else if (temp3 == 1) {
        //cout << "RIGHT EDGE: " << temp1 << " : " << temp2 << "\n";
        inserted[temp1] = 1;
        inserted[temp2] = 1;
        btree[temp1].right_child = temp2;
        btree[temp2].parent = temp1;
        if (i == 0) {
          root_block = temp1;
        }
      } else {
        cout << "[ERROR]: Invalid edge found in the tree file: \n";
      }
    }



    bool hasZero = false;

    for (int number : inserted) {
        //cout << "Inserted element " << number << "\n";
        if (number == 0) {
            hasZero = true;
            break;  // Exit the loop early if "0" is found
        }
    }

    if (hasZero) {
      py::print("Tree is invalid");
        std::cout << "TREE is INVALID..." << std::endl;
    } else {
        std::cout << "[SUCCESS] TREE IS VALID..." << std::endl;
    }



}
