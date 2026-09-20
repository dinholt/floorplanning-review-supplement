// SPDX-License-Identifier: Apache-2.0
// Inherited PARSAC code, not the submission optimization.
// Unchanged bodies; surrounding source omitted. See README.md.

Cost CalculateCost()
{
    BtreeToFloorplan();//convert tree to layout

    int width = 0, height = 0;
    double block_area = 0.0;
    for (int i = 0; i < num_hardblocks; i++) {
        block_area += (int(hardblocks[i].width)*int(hardblocks[i].height));
        if (hardblocks[i].x + hardblocks[i].width > width)
            width = hardblocks[i].x + hardblocks[i].width;
        if (hardblocks[i].y + hardblocks[i].height > height)
            height = hardblocks[i].y + hardblocks[i].height;
    }

    // area of current floorplan
    double floorplan_area = width * height;
    // aspect ratio of current floorplan
    double R = (double)height / width;

    // half perimeter wire length
    double wirelength = 0;
    for (const vector<int> &net : nets) {
        int x_min = width + 1, x_max = 0;
        int y_min = height + 1, y_max = 0;
        for (const int pin : net) {
            if (pin < num_hardblocks) {
                int x_center = hardblocks[pin].x + hardblocks[pin].width / 2;
                int y_center = hardblocks[pin].y + hardblocks[pin].height / 2;
                if (x_center < x_min)
                    x_min = x_center;
                if (y_center < y_min)
                    y_min = y_center;
                if (x_center > x_max)
                    x_max = x_center;
                if (y_center > y_max)
                    y_max = y_center;
            }
            else {
                const Terminal &t = terminals[pin - num_hardblocks];
                if (t.x < x_min)
                    x_min = t.x;
                if (t.y < y_min)
                    y_min = t.y;
                if (t.x > x_max)
                    x_max = t.x;
                if (t.y > y_max)
                    y_max = t.y;
            }
        }

        wirelength += (x_max - x_min) + (y_max - y_min);
    }

    double preplaced_cost = 0;
    for (int i = 0; i < num_hardblocks; i++)
      if(hardblocks[i].preplaced || hardblocks[i].soft_preplaced)
        preplaced_cost += abs(hardblocks[i].x - hardblocks[i].required_x) + abs(hardblocks[i].y - hardblocks[i].required_y) ;
    

    Cost c;
    c.width = width;
    c.height = height;
    c.area = floorplan_area;
    c.wirelength = wirelength;
    c.R = R;
    c.ws = (floorplan_area - block_area)*100.0/(floorplan_area);
    c.preplaced_cost = preplaced_cost;
    
    // set normalization to initial floorplan area and wirelength (also added edge norm)
    if (area_norm == 0) {
        area_norm = floorplan_area;
    }
    if (wl_norm == 0) {
        wl_norm = wirelength;
        //cout << "INITIAL WIRELENGTH COST = " << wl_norm << "\n";
    }

    //    py::print("INITIAL AREA COST = ",area_norm," ",c.area,"\n");

    
  ///Calculate edge violations
    int n_violated_edge_constraints  = 0;
    int n_edge_constraints = 0;
    for (int i = 0; i < num_hardblocks; i++)
      {
        if(hardblocks[i].required_loc != NC)
          {
            n_edge_constraints += 1;
            if(hardblocks[i].actual_loc != hardblocks[i].required_loc)
              n_violated_edge_constraints += 1;
          }
      }
    c.n_violated_edge_constraints = n_violated_edge_constraints;
    double edge_violation_cost = 0.0;
    if(n_edge_constraints > 0)
      edge_violation_cost = (n_violated_edge_constraints * 1.0) / n_edge_constraints;
    ////

    

    


    getClusterCost(false,c.clustering_cost,c.extra_cluster_fragments);
    getClusterCost(true,c.rectilinear_cost,c.extra_rectilinear_fragments);    
    
            
          
      
    
    double area_cost = c.ws / 100.0;
    //double area_cost = c.area / area_norm;    
    double wl_cost = c.wirelength / wl_norm;
    double R_cost = 0;//(1 - R) * (1 - R);//made 0, to remove square AR for chip-level
    double width_penalty = 0;
    double height_penalty = 0;


    if (enable_floorplan_boundaries && width > floorplan_width) {
        width_penalty = ((double)width / floorplan_width);
    }
    if (enable_floorplan_boundaries &&height > floorplan_height) {
        height_penalty = ((double)height / floorplan_height);
    }
    c.width_penalty = width_penalty * floorplan_boundaries_weight;
    c.height_penalty = height_penalty * floorplan_boundaries_weight;
    
    c.cost = (area_cost)*(1 - alpha) + (width_penalty + height_penalty) * floorplan_boundaries_weight + wl_cost*(alpha) + _beta * edge_violation_cost
      + beta_cluster * c.clustering_cost + beta_rectilinear * c.rectilinear_cost +  (c.preplaced_cost * beta_preplaced / (width + height));
    if (inverse) {
        c.cost = 1 / c.cost ;
    }

#ifdef DEBUG
    cout << "Width:      " << c.width << '\n';
    cout << "Height:     " << c.height << '\n';
    cout << "Area:       " << c.area << '\n';
    cout << "Wirelength: " << c.wirelength << '\n';
    cout << "R:          " << c.R << '\n';
    cout << "Cost:       " << c.cost << '\n';
    cout << '\n';
#endif

    return c;
}

void Verify(vector<HardBlock> &hb)
{
    for (int i = 0; i < num_hardblocks; i++) {
        int x_bl1 = hb[i].x;
        int y_bl1 = hb[i].y;
        int x_tr1 = x_bl1 + hb[i].width;
        int y_tr1 = y_bl1 + hb[i].height;
        for (int j = i + 1; j < num_hardblocks; j++) {
            //if (i == j)
            //    continue;

            int x_bl2 = hb[j].x;
            int y_bl2 = hb[j].y;
            int x_tr2 = x_bl2 + hb[j].width;
            int y_tr2 = y_bl2 + hb[j].height;

            if (!(x_tr1 <= x_bl2 || x_bl1 >= x_tr2 || y_tr1 <= y_bl2 || y_bl1 >= y_tr2)) {
                //printf("[Error] hardblocks overlapped\n");
                exit(1);
            }
            else {
                //printf("No hardblocks overlapped: \n");
                //cout << 'Blk:' <<  i << j << '\n';
                //cout << 'Pos: ' << x_bl1 << y_bl1 << x_tr1 << y_tr1 << x_bl2 << y_bl2 << x_tr2 << y_tr2 << '\n'; 
            }
        }

    }
}
