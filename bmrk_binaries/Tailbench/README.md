img_dnn (Image Recognition): To execute with P number of threads, /path/model model, and REQS max reqs:

./img-dnn_integrated -r P -f /path/model -n REQS (./img-dnn_integrated -r 4 -f ${DATA_ROOT}/img-dnn/models/model.xml -n 60)

To run the benchmark with a small input data set for both the mnist data and the model (Both the binary and model.xml file have been modified):

./img-dnn_integrated_small -r 4 -f /scratch/dflyer/Tailbench/tailbench.inputs/img-dnn/models/model_small.xml -n 60

This small benchmark will cause a segfault in the run, but after the point in which the program deadlocks when trying to spawn pthreads, so the small benchmark is useful for debugging this specific issue and not much else at the moment.


** Environment Variables **

DATA_ROOT: Root directory for tailbench input data

TBENCH_MNIST_DIR: Directory of mnist data to be used for img_dnn

TBENCH_WARMUPREQS (application): Length of the warmup period in # requests. No
latency measurements are performed during this period.

TBENCH_MAXREQS (application): The total number of requests to be executed during
the measurement period (the region of interest). This count *does not* include
warmup requests.

TBENCH_MINSLEEPNS (client): The mininum length of time, in ns, for which the
client sleeps in the kernel upon encountering an idle period (i.e., when no
requests are submitted).

TBENCH_QPS (client): The average request rate (queries per second) during the
measurement period. The harness generates interarrival times using an
exponential distribution.

TBENCH_RANDSEED (client): Seed for the random number generator that generates
interarrival times.

To run the benchmarks with gem5, create a file to set environment variables and use the --env flag to specify the file

Example file (/scratch/dflyer/gem5-GPU_killer/env_vars):

	DATA_ROOT=/scratch/dflyer/Tailbench/tailbench.inputs
	THREADS=4
	REQS=100000000
	TBENCH_WARMUPREQS=5000
	TBENCH_MAXREQS=12500
	TBENCH_QPS=500
	TBENCH_MINSLEEPNS=10000
	TBENCH_MNIST_DIR=/scratch/dflyer/Tailbench/tailbench.inputs/img-dnn/mnist
	TBENCH_RANDSEED=1

