package diskv

//
// Sharded key/value server.
// Lots of replica groups, each running op-at-a-time paxos.
// Shardmaster decides which group serves each shard.
// Shardmaster may change shard assignment from time to time.
//
// You will have to modify these definitions.
//

const (
	OK             = "OK"
	ErrNoKey       = "ErrNoKey"
	ErrWrongGroup  = "ErrWrongGroup"
	ErrStaleConfig = "ErrStaleConfig"
	ErrHasDone     = "ErrHasDone"

	Get      = "Get"
	Put      = "Put"
	Append   = "Append"
	Reconfig = "Reconfig"
	Nop      = "Nop"
)

type Err string

type PutAppendArgs struct {
	Key   string
	Value string
	Op    string // "Put" or "Append"
	// You'll have to add definitions here.
	// Field names must start with capital letters,
	// otherwise RPC will break.
	ReqID string
}

type PutAppendReply struct {
	Err string
}

type GetArgs struct {
	Key string
	// You'll have to add definitions here.
	ReqID string
}

type GetReply struct {
	Err   string
	Value string
}

type RequestShardArgs struct {
	Shard     int
	Confignum int
}

type RequestShardReply struct {
	Database  map[string]string
	MaxReqIDs map[string]int64
	Err       string
}