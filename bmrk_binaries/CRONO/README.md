All Pairs Shortest Path: To execute with P number of threads, N vertices, and DEG edges per vertex ./apsp P N DEG (./apsp 2 16384 16)

Betweenness Centrality: To run with P number of threads, N vertices, and DEG edges per vertex ./bc P N DEG (./bc 2 16384 16)

Breadth First Search:
	Input Graph from File: To run with P number of threads, and an input file ./bfs 1 P <input_file>. The input file can be used as: sample.txt OR any other file such as road networks from the SNAP datasets (e.g. A FaceBook Graph) https://snap.stanford.edu/data/
	Generate and Input using the Synthetic Graph Generator: To run with P number of threads, N vertices, and DEG edges per vertex ./bfs 0 P N DEG work_per_thread

Community Detection:
	Input Graph from File: To run with P number of threads, I iterations, and an input file ./community_lock 1 P I <input_file>. To run a matrix format file (.mtx) ./community_lock 2 P I <input_file>. For the input file, use sample.txt OR any other file such as road networks from the SNAP datasets (e.g. roadNet-CA) https://snap.stanford.edu/data/#road.
	Generate and Input using the Synthetic Graph Generator: To run with P number of threads, I iterations, N vertices, and DEG edges per vertex ./community_lock 0 P I N DEG

Connected Components:
	Input Graph from File: To run with P number of threads, and an input file of .gr format, ./connected_components_lock 1 P <input_file>. To run a matrix format file (.mtx) ./connected_components_lock 2 P <input_file>. The input file can be used as: sample.txt OR any other file such as road networks from the SNAP datasets (e.g. A FaceBook Graph) https://snap.stanford.edu/data/.
	Generate and Input using the Synthetic Graph Generator: To run with P number of threads, N vertices, and DEG edges per vertex ./connected_components_lock 0 P N DEG

Depth First Search: To run with P number of threads, and an input file ./dfs P <input_file>. The input file can be used as: sample.txt OR any other file such as road networks from the SNAP datasets (e.g. A FaceBook Graph) https://snap.stanford.edu/data/.
./dfs_work_stealing 0 1 3000 2000
./dfs_work_stealing 0 <NUM_THREADS> <N> <DEG>

PageRank:
	Input Graph from File: To run with P number of threads, and an input file, ./pagerank 1 P <input_file>. It will then ask for the input file, enter: sample.txt OR any other file such as road networks from the SNAP datasets (e.g. A FaceBook Graph) https://snap.stanford.edu/data/.
	Generate and Input using the Synthetic Graph Generator: To run with P number of threads, N vertices, and DEG edges per vertex ./pagerank 0 P N DEG.

Single Source Shortest Path:
	Input Graph from a File: To run with P number of threads ./sssp 1 P <input_file>. It will then ask for the input file, enter: sample.txt OR any other file such as road networks from the SNAP datasets (e.g. roadNet-CA) https://snap.stanford.edu/data/#road.
	Generate and Input using the Synthetic Graph Generator: To run with P number of threads, N vertices, and DEG edges per vertex ./sssp 0 P N DEG

Triangle Counting:
	Input Graph from File: To run with P number of threads, and an .gr type (vertex edge) input file, ./triangle_counting_lock 1 P <input_file>. To run a matrix format file (.mtx) ./triangle_counting_lock 2 P <input_file>. The input file can be used as: sample.txt OR any other file such as road networks from the SNAP datasets (e.g. A FaceBook Graph) https://snap.stanford.edu/data/
	Generate and Input using the Synthetic Graph Generator: To run with P number of threads, N vertices, and DEG edges per vertex ./triangle_counting_lock 0 P N DEG

Travelling Salesman Problem: To run with P number of threads, C cities ./tsp P C (./tsp 2 16)


