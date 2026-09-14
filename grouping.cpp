
bool BlocksTouching(int i,int j)
{
  bool x_overlap_left = (hardblocks[i].x  >= hardblocks[j].x) &&
    (hardblocks[i].x <= hardblocks[j].x + hardblocks[j].width);
  bool x_overlap_right = (hardblocks[i].x+hardblocks[i].width  >= hardblocks[j].x) &&
    (hardblocks[i].x + hardblocks[i].width <= hardblocks[j].x + hardblocks[j].width);

  bool x_overlap = x_overlap_left || x_overlap_right;


  bool y_overlap_bot = (hardblocks[i].y  >= hardblocks[j].y) &&
    (hardblocks[i].y <= hardblocks[j].y + hardblocks[j].height);
  bool y_overlap_top = (hardblocks[i].y+hardblocks[i].height  >= hardblocks[j].y) &&
    (hardblocks[i].y + hardblocks[i].height <= hardblocks[j].y + hardblocks[j].height);
  
  bool y_overlap = y_overlap_bot || y_overlap_top;
  
  return x_overlap && y_overlap;
}

bool BlockTouchingCluster(int i,const vector<int> & cluster)
{
  //  py::print("Checking if block is touching cluster ",i,"\n");  
  for(int b=0;b < cluster.size();++b)
    if(BlocksTouching(i,cluster[b]))
      return true;
  return false;
}

vector<vector<int>> MergeClusters(const vector<vector<int>> &clusters,
                                  const vector<int> & merge_locs)
{
  vector<vector<int> > merge_result;
  if(merge_locs.size() < 2)
    py::print("ERROR : cluster merge locations less than 2 ");
  
  if(merge_locs.back() >= clusters.size())
    py::print("ERROR : merge locs entry exceeds number of clusters  ",merge_locs.back()," ",clusters.size());

  vector<int> merged_cluster;
  for(int m=0;m < merge_locs.size();++m)
    merged_cluster.insert(merged_cluster.end(),clusters[merge_locs[m]].begin(),clusters[merge_locs[m]].end());

  merge_result.push_back(merged_cluster);

  int merge_idx = 0;
  for(int c=0;c < clusters.size(); ++ c)
    if(merge_idx >= merge_locs.size()  || c != merge_locs[merge_idx])
      merge_result.push_back(clusters[c]);
    else
      {
        merge_idx ++;
      }
  
  return merge_result;

}

vector<vector<vector<int> > > getClusterData(bool rectilinear_clusters)
{
  //py::print("getting cluster data for n_clusters ",num_clusters,"\n");
  vector<vector<vector<int> > > cluster_data(rectilinear_clusters ? num_clusters_rectilinear : num_clusters);
  
  for (int i = 0; i < num_hardblocks; i++) {
    int clustering_id = rectilinear_clusters ? hardblocks[i].rectilinear_id : hardblocks[i].cluster_id;
    if (clustering_id != 0)
      {
        int cluster_idx = clustering_id - 1;
        vector <int> touching_clusters;
        for(int c=0;c<cluster_data[cluster_idx].size();++c)
          if(BlockTouchingCluster(i,cluster_data[cluster_idx][c]))
            touching_clusters.push_back(c);
        
        if(touching_clusters.size() == 0) //create new de-factor cluster
          cluster_data[cluster_idx].push_back(vector<int>(1,i));
        else if (touching_clusters.size() == 1) // add to the cluster
          cluster_data[cluster_idx][touching_clusters[0]].push_back(i);
        else //i is a bridge between 2 previously separate clusters
          {
            auto merged_cluster = MergeClusters(cluster_data[cluster_idx],
                                                touching_clusters);
            merged_cluster[0].push_back(i); //0 is always the index of merged cluster
            cluster_data[cluster_idx] = merged_cluster;
          }
      }

  }
  return cluster_data;
}

void getClusterCost(bool rectilinear_clusters,double &clustering_cost,int & extra_fragments)
{
    vector<vector<vector<int>>> cluster_data = getClusterData(rectilinear_clusters);
    int extra_cluster_fragments = 0;
    int n_cluster_blocks = 0;
    for(int cluster_id = 0;cluster_id < cluster_data.size();++cluster_id)
      {
        extra_cluster_fragments += (cluster_data[cluster_id].size() - 1);
        for(int c= 0;c < cluster_data[cluster_id].size();++c)
          n_cluster_blocks += cluster_data[cluster_id][c].size();
      }
    if(n_cluster_blocks == 0)
      clustering_cost = 0;
    else
      clustering_cost = (extra_cluster_fragments * 1.0 / n_cluster_blocks);      
    extra_fragments = extra_cluster_fragments;

}
