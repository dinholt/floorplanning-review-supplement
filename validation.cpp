
class Graph {
public:
    Graph(int numVertices) : V(numVertices), adjList(numVertices) {}

    void addEdge(int u, int v) {
        adjList[u].push_back(v);
        adjList[v].push_back(u); // Undirected graph, so add edge in both directions
    }

    int countConnectedComponents() {
        std::vector<bool> visited(V, false);
        int count = 0;

        for (int v = 0; v < V; ++v) {
            if (!visited[v]) {
                DFS(v, visited);
                count++;
            }
        }

        return count;
    }

private:
    void DFS(int vertex, std::vector<bool>& visited) {
        visited[vertex] = true;

        for (int neighbor : adjList[vertex]) {
            if (!visited[neighbor]) {
                DFS(neighbor, visited);
            }
        }
    }

    int V; // Number of vertices
    std::vector<std::vector<int>> adjList; // Adjacency list
};

int CountBlockOverlaps(const std::vector<HardBlock> &blocks) {
  int overlaps = 0;
  for (int i = 0; i < num_hardblocks; ++i) {
    for (int j = i + 1; j < num_hardblocks; ++j) {
      const bool separated = blocks[i].x + blocks[i].width <= blocks[j].x ||
          blocks[j].x + blocks[j].width <= blocks[i].x ||
          blocks[i].y + blocks[i].height <= blocks[j].y ||
          blocks[j].y + blocks[j].height <= blocks[i].y;
      if (!separated) ++overlaps;
    }
  }
  return overlaps;
}

void ValidateSchedulerTree(const std::vector<std::vector<int>> &nodes, int root) {
    if (static_cast<int>(nodes.size()) != num_hardblocks || root < 0 || root >= num_hardblocks)
        throw std::invalid_argument("invalid scheduler B*Tree size or root");
    std::vector<int> seen(num_hardblocks, 0);
    std::vector<int> stack{root};
    while (!stack.empty()) {
        const int id = stack.back(); stack.pop_back();
        if (id < 0 || id >= num_hardblocks || seen[id]++)
            throw std::invalid_argument("scheduler B*Tree contains an invalid node or cycle");
        if (nodes[id].size() != 3) throw std::invalid_argument("invalid scheduler B*Tree row");
        for (int k = 1; k <= 2; ++k) {
            const int child = nodes[id][k];
            if (child == -1) continue;
            if (child < 0 || child >= num_hardblocks || nodes[child].size() != 3 || nodes[child][0] != id)
                throw std::invalid_argument("scheduler B*Tree parent-child mismatch");
            stack.push_back(child);
        }
    }
    if (nodes[root].size() != 3 || nodes[root][0] != -1)
        throw std::invalid_argument("scheduler B*Tree root has a parent");
    for (int v : seen) if (v != 1)
        throw std::invalid_argument("scheduler B*Tree does not contain every block exactly once");
}
