// SPDX-License-Identifier: Apache-2.0
// Inherited PARSAC code, not the submission optimization.
// Unchanged bodies; surrounding source omitted. See README.md.

void Swap(int node1, int node2)
{
    // swap parent
    //cout << "Committed swap operation " << "\n";
    int node1_parent = btree[node1].parent;
    int node2_parent = btree[node2].parent;

    if (node1_parent == node2_parent) {
        int left_child = btree[node1_parent].left_child;
        int right_child = btree[node1_parent].right_child;
        btree[node1_parent].left_child = right_child;
        btree[node1_parent].right_child =  left_child;
        //cout << "[DEBUG 0 for same parent] " << node1_parent << " - " << btree[node1_parent].left_child << " : " << btree[node1_parent].right_child << "\n";

    } else {
        //update node1's parent's left and right child to node2
        if (node1_parent != -1) {
            if (btree[node1_parent].left_child == node1)
                btree[node1_parent].left_child = node2;
            else if (btree[node1_parent].right_child == node1)
                btree[node1_parent].right_child = node2;
            else {
                cout << "[Error] node not parent's child\n";
                exit(1);
            }
        }

        //cout << "[DEBUG 0 for same parent] " << node1_parent << " - " << btree[node1_parent].left_child << " : " << btree[node1_parent].right_child << "\n";
        //update node2's parent's left and right child to node1
        if (node2_parent != -1) {
            if (btree[node2_parent].left_child == node2)
                btree[node2_parent].left_child = node1;
            else if (btree[node2_parent].right_child == node2)
                btree[node2_parent].right_child = node1;
            else {
                cout << "[Error] node not parent's child\n";
                exit(1);
            }
        }
    }
    //cout << "[DEBUG 1 for same parent] " << node1_parent << " - " << btree[node1_parent].left_child << " : " << btree[node1_parent].right_child << "\n";
    //update parent's if they are from diff parents
    if (node1_parent != node2_parent) {
        btree[node1].parent = node2_parent;
        btree[node2].parent = node1_parent;
    } else {
        //cout << "[DEBUG 2 for same parent] " << node1_parent << " - " << node2_parent << " *** " << btree[node1_parent].left_child << " : " << btree[node1_parent].right_child << "\n";
    }
    //else if (btree[node1].parent != btree[node2].parent) {//swap left and right
    //}
    // swap children
    int node1_left_child = btree[node1].left_child;
    int node1_right_child = btree[node1].right_child;
    int node2_left_child = btree[node2].left_child;
    int node2_right_child = btree[node2].right_child;
    btree[node1].left_child = node2_left_child;
    btree[node1].right_child = node2_right_child;
    btree[node2].left_child = node1_left_child;
    btree[node2].right_child = node1_right_child;

    //update child to parent relation 
    if (btree[node1].left_child != -1)
        btree[btree[node1].left_child].parent = node1;
    if (btree[node1].right_child != -1)
        btree[btree[node1].right_child].parent = node1;
    if (btree[node2].left_child != -1)
        btree[btree[node2].left_child].parent = node2;
    if (btree[node2].right_child != -1)
        btree[btree[node2].right_child].parent = node2;

    // node1, node2 are parent and child
    if (btree[node1].parent == node1)
        btree[node1].parent = node2;
    else if (btree[node1].left_child == node1)
        btree[node1].left_child = node2;
    else if (btree[node1].right_child == node1)
        btree[node1].right_child = node2;

    if (btree[node2].parent == node2)
        btree[node2].parent = node1;
    else if (btree[node2].left_child == node2)
        btree[node2].left_child = node1;
    else if (btree[node2].right_child == node2)
        btree[node2].right_child = node1;

    // root block may change
    if (node1 == root_block)
        root_block = node2;
    else if (node2 == root_block)
        root_block = node1;
}

void Move(int node, int to_node,bool favor_left = false,bool favor_right = false)
{
    // delete
    if (btree[node].left_child == -1 && btree[node].right_child == -1) {
        // no children
        int parent = btree[node].parent;
        if (btree[parent].left_child == node)
            btree[parent].left_child = -1;
        else if (btree[parent].right_child == node)
            btree[parent].right_child = -1;
        else {
            cout << "[Error] node not parent's child\n";
            exit(1);
        }
    }
    else if (btree[node].left_child != -1 && btree[node].right_child != -1) {
        // two children
        do {
            bool move_left;
            if (btree[node].left_child != -1 && btree[node].right_child != -1)
                move_left = rand() % 2 == 0;
            else if (btree[node].left_child != -1)
                move_left = true;
            else
                move_left = false;
            
            if (move_left)
                Swap(node, btree[node].left_child);
            else
                Swap(node, btree[node].right_child);
        } while (btree[node].left_child != -1 || btree[node].right_child != -1);

        int parent = btree[node].parent;
        if (btree[parent].left_child == node)
            btree[parent].left_child = -1;
        else if (btree[parent].right_child == node)
            btree[parent].right_child = -1;
        else {
            cout << "[Error] node not parent's child\n";
            exit(1);
        }
    }
    else {
        // one child
        int child;
        if (btree[node].left_child != -1)
            child = btree[node].left_child;
        else
            child = btree[node].right_child;

        int parent = btree[node].parent;
        if (parent != -1) {
            if (btree[parent].left_child == node)
                btree[parent].left_child = child;
            else if (btree[parent].right_child == node)
                btree[parent].right_child = child;
            else {
                cout << "[Error] [one child] node not parent's child\n";
                exit(1);
            }
        }

        btree[child].parent = parent;

        // root block may change
        if (node == root_block)
            root_block = child;
    }

    // insert (**corrected the flow for various rand values**)
    int random_left_right;
    if(favor_left)
      random_left_right = 0;
    else if(favor_right)
      random_left_right = 3;
    else
      random_left_right = rand() % 4;
      
    int child;
    if (random_left_right == 0) {
        child = btree[to_node].left_child;
        btree[node].left_child = child;
        btree[node].right_child = -1;
        btree[to_node].left_child = node;
    }
    else if (random_left_right == 1) {
        child = btree[to_node].right_child;
        btree[node].left_child = child;
        btree[node].right_child = -1;
        btree[to_node].right_child = node;
    }
    else if (random_left_right == 2) {
        child = btree[to_node].left_child;
        btree[node].left_child = -1;
        btree[node].right_child = child;
        btree[to_node].left_child = node;
    }
    else {
        child = btree[to_node].right_child;
        btree[node].left_child = -1;
        btree[node].right_child = child;
        btree[to_node].right_child = node;
    }
    btree[node].parent = to_node;
    if (child != -1)
        btree[child].parent = node;
}
