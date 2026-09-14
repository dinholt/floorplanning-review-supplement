
void BtreePreorderTraverseU(int cur_node, bool flag,bool place_root = false)
{
  if(!hardblocks[cur_node].preplaced)
    {    
      if(place_root) //we are placing the root
        hardblocks[cur_node].x = 0;
      else
        {
          int parent = btree[cur_node].parent;
          if (flag == false) {//LEFT child, right neighbor
            hardblocks[cur_node].x = hardblocks[parent].x + hardblocks[parent].width;
          } else {
            hardblocks[cur_node].x = hardblocks[parent].x;
          }
        }

      int x_start = hardblocks[cur_node].x;
      int x_end = x_start + hardblocks[cur_node].width;
      
      std::map<int,int> blocking_spans; //ordered by lower edge
      for (int element : placed_list) {
        int xleft = hardblocks[element].x;
        int xright = xleft + hardblocks[element].width;
        if (x_start < xright && x_end >xleft)
          {
            
            blocking_spans[hardblocks[element].y] = max(hardblocks[element].y + hardblocks[element].height,
                                                        blocking_spans[hardblocks[element].y]);
          }
      }
      
      int y_placed = 0;
      int cur_height = hardblocks[cur_node].height;
      
      for (auto vert_span : blocking_spans)
        {
          //vert_span.first is lower edge, vert_span.second is top edge
          //          py::print("blocking span for block ",cur_node," with y_placed ",y_placed, " and height " ,cur_height, " is ",vert_span.first, " to ",vert_span.second);

          if(vert_span.first >= y_placed + cur_height)
            break;
          if(vert_span.second > y_placed)
            y_placed = vert_span.second;
        }
      
      hardblocks[cur_node].y = y_placed;
  
      placed_list.push_back(cur_node);
    }
  
  if (btree[cur_node].left_child != -1)
    {
      current_btree.push_back({{cur_node, btree[cur_node].left_child, 0}});
      BtreePreorderTraverseU(btree[cur_node].left_child, false);//LEFT == 0 == FALSE
    }
  if (btree[cur_node].right_child != -1)
    {
      current_btree.push_back({{cur_node, btree[cur_node].right_child, 1}});
      BtreePreorderTraverseU(btree[cur_node].right_child, true);//RIGHT == 1 == TRUE
    }
}

std::vector<std::vector<int>> getBlockPos(){ 
  blockPos.clear();
  for (int i = 0; i < num_hardblocks; i++) {
    std::vector<int> current_pos;
    current_pos.push_back(hardblocks[i].x);
    current_pos.push_back(hardblocks[i].y);
    current_pos.push_back(hardblocks[i].width );
    current_pos.push_back(hardblocks[i].height);
    current_pos.push_back(hardblocks[i].actual_loc );
    blockPos.push_back(current_pos);
  }
  return blockPos;
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

void BtreeToFloorplan()
{
    //std::vector<std::vector<int>> current_btree;
    //cout << "Generating layout from btree \n";
    current_btree.clear();

    current_loc.clear();

    //clear the placed_list vector
    placed_list.clear();
    for (int i = 0; i < num_hardblocks; i++)
      if(hardblocks[i].preplaced)
        placed_list.push_back(i);
            
    
    BtreePreorderTraverseU(root_block, false,true);

    //get block positions
    current_loc = getBlockPos();

    AddEdgeLocs();
    
}
