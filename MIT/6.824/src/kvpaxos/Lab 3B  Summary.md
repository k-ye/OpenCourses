Lab 3B Piazza Related

[@185](https://piazza.com/class/i56yu16lib05ii?cid=185)
Q:
What is a good strategy to fill holes on a peer? I know its a very open question. Any recommended text? Or Is it ok to pass decided sequences in Propose message. Such that server can check what it has against what others have. But this strategy will greatly slow down things. I dont want to propose a no op for a missing instance because that no op may get majority for its propose while other servers are are still struggling to get a majority on propose by the client.

A:
My kvpaxos server proposes a no-op for an instance after a while if it seems to be a hole. It's true that it would be a problem if servers were too eager about this, so it's good to 1) try not to leave holes in the common case and 2) don't be too eager about filling them.

[@213](https://piazza.com/class/i56yu16lib05ii?cid=213)
Q:
tldr: Can someone give some advice regarding preventing duplicates client requests from getting executed?
Since the servers can only communicate with each other using the Paxos protocol, I'm assuming that the best way to prevent duplicates is to create some way for the server that receives a request to tell its client that it will execute that request.
Can we always be certain that something a server writes in "reply" will be accessible to the client in a "timely" manner?

A:
No; the client may never get the reply and the server may die before it gets to execute the request. Basically, the server can't reply until it has completed the request. Regarding deduplication, you can safely use paxos to agree on a single operation multiple times as long as you don't actually execute it multiple times (you can deduplicate after running operations through paxos).


Thoughts:

1. Client does not change server unless RPC call is failed. If it succeeded, then server must make the request happen.

2. Notice that in **Paxos** layer, once we call `Start(seq, op)`, it will never stop until an instance is indeed created in `seq` position (although the instance might not be equal to `op`). So we need only to call `Start()` in **kvpaxos** layer once and we're ensured an instance will be created there (without concerning whether there's a network partitioning). 

The thing we need to handle here is the case where the instance we created in `seq` is not what we want to create. This will happen if our Paxos peer lagged for a while (because of partition and then restarts), then we can keep calling `Start()` with monotonically increasing `seq` until at one point, the instance does match the value we want to put in. Meanwhile, each time we receive the instance by calling `Start()`, we execute this instance to our local database and tell the Paxos peer that this `seq` is `Done()`.

3. Pseudo code:

KVPaxos:
```
PutAppend(args, reply) {
	If has seen args.request:
		just ignore it

	newOp := Op{Key, Value, PutAppendOp, RequestID}
	kv.AddOpToLog(newOp)
}

Get(args, reply) {
	If has seen args.request:
		reply.Value = kv.database[args.Key]

	newOp := Op{Key, EmptyValue, GetOP, RequestID}
	kv.AddOpToLog(newOp)

	reply.Value = kv.databse[args.Key]
}

AddOPToLog(newOp) {
	done := false
	while not done {
		seq := kv.lastExecSeq + 1
		result := kv.TryAddOp(newOP, seq)
		done = result.ReqID == newOp.ReqID
		kv.ExecuteLog(result, seq)
	}
}

TryAddOp(newOP, seq) {
	kv.px.Start(newOp, seq)
	done := false
	while not done {
		decided, _ := kv.px.Status(seq)
		switch decided {
			case Decided:
				done = true
			case Pending:
				WaitForPending()
		}
	}
	_, instance := kv.px.Status(seq) // instance might not be newOp!
	return instance
}

ExecuteLog(op, seq) {
	// precondition: index(op) == seq
	switch op.op {
		case Get, Nop:
			// we don't have to do anything
		case Put:
			kv.database[op.Key] = op.Value
		case Append:
			kv.databse[op.Key] += op.Value
	}
	
	kv.RecordRequest(op.ReqID)
	kv.Done(seq)
	kv.lastExecSeq = seq
}
```

Originally, when I'm recording the RequestID, I also stored the return value for that Request if it is Get. This turned out to be consuming too much memory which failed to pass TestDone(). The scenario is that, if we put a large amount of Value into some Key and then read the Key multiple times with different request, then the Value will be stored multiple times for each request. After I found this, I just changed to use a map to store the RequestID, only.