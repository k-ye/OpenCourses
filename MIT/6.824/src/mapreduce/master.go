package mapreduce

import "container/list"
import "fmt"


type WorkerInfo struct {
	address string
	// You can add definitions here.
}


// Clean up all workers by sending a Shutdown RPC to each one of them Collect
// the number of jobs each work has performed.
func (mr *MapReduce) KillWorkers() *list.List {
	l := list.New()
	for _, w := range mr.Workers {
		DPrintf("DoWork: shutdown %s\n", w.address)
		args := &ShutdownArgs{}
		var reply ShutdownReply
		ok := call(w.address, "Worker.Shutdown", args, &reply)
		if ok == false {
			fmt.Printf("DoWork: RPC %s shutdown error\n", w.address)
		} else {
			l.PushBack(reply.Njobs)
		}
	}
	return l
}

func (mr *MapReduce) RunMaster() *list.List {
	// Your code here
	var mapDoneChannel, mapCount = make(chan bool), 0
	var reduceDoneChannel, reduceCount = make(chan bool), 0
	/* At first, I tried to read out all the workers into a pool. It turned out 
		that this method would fail when there were failures in workers.
	*/
	/*
	var workerPool []string
	readWorkers := true
	
	fmt.Println("Initialize worker pool")
	for readWorkers {
		select {
			case workerAddress := <-mr.registerChannel:
				workerPool = append(workerPool, workerAddress)
			default:
				readWorkers = false
		}
	}
	var idleWorkerAddress = make(chan string, len(workerPool))
	for _, worker := range workerPool {
		idleWorkerAddress <- worker
	}
	*/
	// make this a buffered channel so that it won't block @mapDoneChannel or @reduceDoneChannel
	var idleWorkerAddress = make(chan string, 2)
	// get the available worker from channel
	var GetAvailableWorker = func() string {
		var workerAddress string
		select {
			case workerAddress = <- idleWorkerAddress:
			case workerAddress = <- mr.registerChannel:
		}
		return workerAddress
	}
	// get the available worker from channel
	var DispatchJob = func(workerAddress string, job JobType, piece int) bool {
		var args DoJobArgs
		var reply DoJobReply
		args.File = mr.file
		args.JobNumber = piece
		args.Operation = job
		switch job {
			case Map:
				args.NumOtherPhase = mr.nReduce
			case Reduce:
				args.NumOtherPhase = mr.nMap
		}
		
		done := call(workerAddress, "Worker.DoJob", args, &reply)
		return done
	}
	
	for i := 0; i < mr.nMap; i++ {
		go func(piece int) {
			for {
				// Read out registered workers, mr.registereChannel will get filled besides the initialization 
				// due to the simulated failures in the process.
				workerAddress := GetAvailableWorker()
				done := DispatchJob(workerAddress, Map, piece)
				if done {
					// this block might never be reached if we don't keep reading mr.registerChannel after failure
					mapCount++
					idleWorkerAddress <- workerAddress

					if mapCount == mr.nMap {
						close(mapDoneChannel)
					}
					return
				}
			}
		}(i)
	}

	<- mapDoneChannel
	//fmt.Println("Map done!")

	for i := 0; i < mr.nReduce; i++ {
		go func(piece int) {
			for {
				workerAddress := GetAvailableWorker()
				done := DispatchJob(workerAddress, Reduce, piece)
				if done {
					// this block might never be reached if we don't keep reading mr.registerChannel after failure
					reduceCount++
					idleWorkerAddress <- workerAddress

					if reduceCount == mr.nReduce {
						close(reduceDoneChannel)
					}
					return
				}
			}
		}(i)
	}

	<- reduceDoneChannel
	//fmt.Println("Reduce done!")
	return mr.KillWorkers()
}