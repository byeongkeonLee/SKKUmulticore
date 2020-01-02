/* Each kernel handles the update of one pagerank score. In other
 * words, each kernel handles one row of the update:
 *
 *      pi(t+1) = (1/2) A pi(t) + (1 / (2N))
 *      
 * You may assume that num_nodes <= blockDim.x * 65535
 *
 */
#include<stdio.h>
__global__
void device_graph_propagate(const uint* graph_indices
		, const uint* graph_edges
		, const float* graph_nodes_in
		, float* graph_nodes_out
		, const float* inv_edges_per_node
		, int num_nodes) {
	// TODO: fill in the kernel code here
	int k= threadIdx.x + blockDim.x * blockIdx.x;
	if(k<num_nodes){
//	printf("--%d-/%d-\n",k,num_nodes);
		float sum=0.f;
		for(uint j=graph_indices[k];j<graph_indices[k+1] ; j++){
			sum+= graph_nodes_in[ graph_edges[j] ] * inv_edges_per_node[graph_edges[j] ];
		}
		graph_nodes_out[k] = 0.5f/(float)num_nodes + 0.5f*sum;
	}
}

/* This function executes a specified number of iterations of the
 * pagerank algorithm. The variables are:
 *
 * h_graph_indices, h_graph_edges:
 *     These arrays describe the indices of the neighbors of node i.
 *     Specifically, node i is adjacent to all nodes in the range
 *     h_graph_edges[h_graph_indices[i] ... h_graph_indices[i+1]].
 *
 * h_node_values_input:
 *     An initial guess of pi(0).
 *
 * h_gpu_node_values_output:
 *     Output array for the pagerank vector.
 *
 * h_inv_edges_per_node:
 *     The i'th element in this array is the reciprocal of the
 *     out degree of the i'th node.
 *
 * nr_iterations:
 *     The number of iterations to run the pagerank algorithm for.
 *
 * num_nodes:
 *     The number of nodes in the whole graph (ie N).
 *
 * avg_edges:
 *     The average number of edges in the graph. You are guaranteed
 *     that the whole graph has num_nodes * avg_edges edges.
 *
 */
double device_graph_iterate(const uint* h_graph_indices
		, const uint* h_graph_edges
		, const float* h_node_values_input
		, float* h_gpu_node_values_output
		, const float* h_inv_edges_per_node
		, int nr_iterations
		, int num_nodes
		, int avg_edges) {
	// TODO: allocate GPU memory
	float* buffer_1, *buffer_2;
	uint* graph_indices,* graph_edges;
	float * inv_edges_per_node;
	if(cudaMalloc((void**)&buffer_1,num_nodes*sizeof(float))==-1){
		check_launch("gpu allocate failure");
	}
	if(cudaMalloc((void**)&buffer_2,num_nodes*sizeof(float))==-1){
		check_launch("gpu allocate failure");
	}
	if(cudaMalloc((void**)&graph_indices,(num_nodes+1)*sizeof(uint))==-1){
		check_launch("gpu allocate failure");
	}
	if(cudaMalloc((void**)&graph_edges,(num_nodes*avg_edges)*sizeof(uint))==-1){
		check_launch("gpu allocate failure");
	}
	if(cudaMalloc((void**)&inv_edges_per_node,num_nodes*sizeof(float))==-1){
		check_launch("gpu allocate failure");
	}

	// TODO: check for allocation failure

	// TODO: copy data to the GPU
	cudaMemcpy(buffer_1,h_node_values_input,num_nodes*sizeof(float),cudaMemcpyHostToDevice);
	cudaMemcpy(graph_indices,h_graph_indices,(num_nodes+1)*sizeof(uint),cudaMemcpyHostToDevice);
	cudaMemcpy(graph_edges,h_graph_edges,(num_nodes*avg_edges)*sizeof(uint),cudaMemcpyHostToDevice);
	cudaMemcpy(inv_edges_per_node,h_inv_edges_per_node,num_nodes*sizeof(float),cudaMemcpyHostToDevice);
	start_timer(&timer);

	const int block_size = 1024;

	// TODO: launch your kernels the appropriate number of iterations
	for(int iter=0;iter<nr_iterations / 2 ; iter++){
		device_graph_propagate<<<num_nodes/block_size+1,block_size>>>(graph_indices,graph_edges,buffer_1,buffer_2,inv_edges_per_node,num_nodes);
		device_graph_propagate<<<num_nodes/block_size+1,block_size>>>(graph_indices,graph_edges,buffer_2,buffer_1,inv_edges_per_node,num_nodes);
	}

	check_launch("gpu graph propagate");
//		printf("\n------------------------gpu start---------------------------\n\n");
	double gpu_elapsed_time = stop_timer(&timer);

	// TODO: copy final data back to the host for correctness checking
	if(nr_iterations %2){
		device_graph_propagate<<<num_nodes/block_size+1,block_size>>>(graph_indices,graph_edges,buffer_1,buffer_2,inv_edges_per_node,num_nodes);
		cudaMemcpy(h_gpu_node_values_output, buffer_2, num_nodes*sizeof(float), cudaMemcpyDeviceToHost);
	}else{
		cudaMemcpy(h_gpu_node_values_output, buffer_1, num_nodes*sizeof(float), cudaMemcpyDeviceToHost);
	}
	// TODO: free the memory you allocated!
	cudaFree(buffer_1);
	cudaFree(buffer_2);
	cudaFree(graph_indices);
	cudaFree(graph_edges);
	cudaFree(inv_edges_per_node);
	return gpu_elapsed_time;
}
