package pbservice

import "net"
import "fmt"
import "net/rpc"
import "log"
import "time"
import "viewservice"
import "sync"
import "os"
import "syscall"
import "math/rand"
import "mydebug" // package to provide DPrintf

type PBServer struct {
	mu         sync.Mutex
	l          net.Listener
	dead       bool // for testing
	unreliable bool // for testing
	me         string
	vs         *viewservice.Clerk
	// Your declarations here.
	curview viewservice.View // current view
	database DatabaseType // saved data
	
	seenReqIDs map[string]bool // record all the seen ReqID, true if ReqID has been commited
	oldvaluesGet map[string]string // store that maps a Get reqID to its value
	lastUncommitPAReqID string // stores the last undone Put/Append ReqID
}

const (
	Get = "Get"
	Put = "Put"
	Append = "Append"
)

// Check if two servers s1 and s2 are the same
func IsSameServer(s1 string, s2 string) bool {
	return s1 == s2
}

// Check if a given server s1 is Empty
func IsEmptyServer(s1 string) bool {
	return s1 == ""
}

// check if pb is Primary
func (pb *PBServer) IsPrimary() bool {
	return IsSameServer(pb.me, pb.curview.Primary)
}

// Check if pb has seen a given ReqID
func (pb *PBServer) HasSeenReqId(ReqID string) bool {
	_, ok := pb.seenReqIDs[ReqID]
	return ok
}

// Check if pb has already commited a given ReqID
func (pb *PBServer) HasCommitedReqID(ReqID string) bool {
	if pb.HasSeenReqId(ReqID) {
		return pb.seenReqIDs[ReqID]
	}
	return false
}

// Commit ReqID and value into the server
func (pb *PBServer) CommitReqID(ReqID string, Value string, op string) {
	pb.seenReqIDs[ReqID] = true
	switch op {
	case Get:
		pb.oldvaluesGet[ReqID] = Value
	case Put, Append:
		if pb.lastUncommitPAReqID != ReqID {
			mydebug.DPrintf("%s Commit P/A, last uncommit: %s, to commit: %s, should be same\n", pb.me, pb.lastUncommitPAReqID, ReqID)
		}
		pb.lastUncommitPAReqID = ""
	}
}

// Get Handler in server side
func (pb *PBServer) Get(args *GetArgs, reply *GetReply) error {

	// Your code here.
	Key := args.Key
	ReqID := args.ReqID

	pb.mu.Lock()
	defer pb.mu.Unlock()
	mydebug.DPrintf("\n")
	if args.IsFromClient && !pb.IsPrimary() {
		// reject since pb is not Primary
		reply.Value = ""
		reply.Err = ErrWrongServer
		return nil
	} else if !args.IsFromClient && pb.IsPrimary() {
		// reject since pb is Primary and this is a Forwarded op
		reply.Value = ""
		reply.Err = ErrForwardReject
		return nil
	}

	// if primary got response from backup, P should not read local database
	useReplyFromBackup := false 
	// Forward the op to Backup to keep sync
	if pb.IsPrimary() && !IsEmptyServer(pb.curview.Backup) && 
		args.IsFromClient && !pb.HasCommitedReqID(ReqID) {
		mydebug.DPrintf("'Get' Backup Forward, key: %s, ReqID: %s\n", args.Key, ReqID)
		// remember to send the args.ReqID so that backup sync with the same ReqID
		var bArgs = GetArgs{Key:args.Key, IsFromClient:false, ReqID:ReqID}
		var bReply GetReply
		bOK := call(pb.curview.Backup, "PBServer.Get", &bArgs, &bReply)
		// Forward has been rejected due to network or disagreement on who's Primary
		if !bOK || bReply.Err == ErrForwardReject {
			mydebug.DPrintf("'Get' Forward Failed, ok: %v, Err: %s, ReqID: %s\n", bOK, bReply.Err, ReqID)
			reply.Value = ""
			reply.Err = ErrFailBackup
			return nil
		} else {
			// backup responded, just use the replied value
			reply.Value = bReply.Value
			reply.Err = bReply.Err
			useReplyFromBackup = true
		}
	}

	if !useReplyFromBackup {
		if pb.HasCommitedReqID(ReqID) {
			// skip duplicate ops, return the cached value
			mydebug.DPrintf("%s Duplicated Get op, Key: %s, ReqID: %s\n", pb.me, Key, ReqID)
			reply.Value = pb.oldvaluesGet[ReqID]
			reply.Err = OK
			return nil
		} else if val, ok := pb.database[Key]; ok {	
			// exec read	
			reply.Value = val
			reply.Err = OK
		} else {
			// Oops no key
			reply.Value = ""
			reply.Err = ErrNoKey
		}
	}

	mydebug.DPrintf("%s made a GET, Key: %s, Value: %s, ReqID: %s\n", pb.me, Key, reply.Value, ReqID)
	// there will be no error at this point, commit safely
	pb.CommitReqID(ReqID, reply.Value, Get)

	return nil
}


func (pb *PBServer) PutAppend(args *PutAppendArgs, reply *PutAppendReply) error {

	// Your code here.
	Key := args.Key
	ReqID := args.ReqID

	// for debug log
	var srv = ""
	switch pb.me {
	case pb.curview.Primary:
		srv = "Primary"
	case pb.curview.Backup:
		srv = "Backup"
	default:
		srv = "Nonsense"
	}
	
	pb.mu.Lock()
	defer pb.mu.Unlock()
	mydebug.DPrintf("\n")
	
	// Pre-checking
	if args.IsFromClient && !pb.IsPrimary() {
		// reject since pb is not Primary
		reply.Err = ErrWrongServer
		return nil
	} else if !args.IsFromClient && pb.IsPrimary() {
		// reject since pb is Primary and this is a Forwarded op
		reply.Err = ErrForwardReject
		return nil
	} else if pb.lastUncommitPAReqID != "" && !pb.HasCommitedReqID(pb.lastUncommitPAReqID) &&
	 pb.lastUncommitPAReqID != ReqID {
	 	// if we still got uncommited P/A request, and the coming req is same with the uncommited one,
	 	// then we should not execute this req
	 	mydebug.DPrintf("%s still got uncommited Put/Append, last uncommited: %s, current: %s\n", pb.me, pb.lastUncommitPAReqID, ReqID)
		reply.Err = ErrServerBusy
		return nil
	}
	// if this operation is previously unseen, now we have at least seen it
	if !pb.HasSeenReqId(ReqID) {
		pb.seenReqIDs[ReqID] = false
		pb.lastUncommitPAReqID = ReqID
	}

	// Forward the op to Backup to keep sync
	if pb.IsPrimary() && !IsEmptyServer(pb.curview.Backup) &&
		args.IsFromClient && !pb.HasCommitedReqID(ReqID) {
		mydebug.DPrintf("'PutAppend' Backup Forward, key: %s, val: %s, op: %s, ReqID: %s\n", args.Key, args.Value, args.Operation, ReqID)
		// remember to send the args.ReqID so that backup sync with the same ReqID
		var bArgs = PutAppendArgs{Key:args.Key, Value:args.Value, Operation:args.Operation, 
								IsFromClient:false, ReqID:args.ReqID}
		var bReply PutAppendReply
		bOK := call(pb.curview.Backup, "PBServer.PutAppend", &bArgs, &bReply)
		// Forward has been rejected due to 1) disagreement on who's Primary, or 2) Backup failed suddenly
		if !bOK || bReply.Err == ErrForwardReject {
			mydebug.DPrintf("'PutAppend' Forward Failed, ok: %v, Err: %s, ReqID: %s\n", bOK, bReply.Err, ReqID)
			reply.Err = ErrFailBackup
			return nil
		}
	}

	reply.Err = OK
	if pb.HasCommitedReqID(ReqID) {
		// skip duplicate ops
		mydebug.DPrintf("%s Duplicated operation. Key: %s, Value: %s, ReqID: %s\n", pb.me, Key, args.Value, ReqID)
		return nil
	} else if val, ok := pb.database[Key]; ok {
		// excute operation
		switch args.Operation {
		case Put:
			pb.database[Key] = args.Value
		case Append:
			pb.database[Key] = fmt.Sprintf("%s%s", val, args.Value)
		}
		mydebug.DPrintf("%s %s made a %s locally, Key: %s, Value: %s, ReqID: %s\n", srv, pb.me, args.Operation, Key, args.Value, ReqID)
	} else {
		// excute operation
		pb.database[Key] = args.Value
		mydebug.DPrintf("%s %s made a Put locally, Key: %s, Value: %s, ReqID: %s\n", srv, pb.me, Key, args.Value, ReqID)
	}
	
	// there will be no error at this point, commit safely
	pb.CommitReqID(ReqID, "", args.Operation)

	return nil
}

// RPC handler for transfer database from primary server to backup server
func (pb *PBServer) ReceiveReplica(args *TransferStateArgs, reply *TransferStateReply) error {
	pb.mu.Lock()
	defer pb.mu.Unlock()

	pb.database = args.Database
	pb.seenReqIDs = args.SeenReqIDs
	pb.lastUncommitPAReqID = args.LastUncommitPAReqID
	pb.oldvaluesGet = args.OldvaluesGet
	
	reply.Err = OK
	// mydebug.DPrintf("Recieved database as a backup: %s, %s, %s \n", pb.database, pb.oldvaluesGet, pb.seenReqIDs)
	return nil
}

// determine whether to transfer state or not according to current status of pb and newview
func (pb *PBServer) ShouldTransferState(newview *viewservice.View) bool {
	// pb is still Primary, new backup comes in
	case1 := pb.IsPrimary() && IsSameServer(pb.me, newview.Primary) &&
					!IsSameServer(pb.curview.Backup, newview.Backup) && !IsEmptyServer(newview.Backup)
	// pb is about to be prompted from backup to primary, and additional backup is none
	case2 := IsSameServer(pb.me, pb.curview.Backup) && IsSameServer(pb.me, newview.Primary) &&
					!IsEmptyServer(newview.Backup)
	return case1 || case2
}
//
// ping the viewserver periodically.
// if view changed:
//   transition to new view.
//   manage transfer of state from primary to new backup.
//
func (pb *PBServer) tick() {

	// Your code here.
	view, err := pb.vs.Ping(pb.curview.Viewnum)

	if err == nil {
		// fmt.Printf("View P: %s, Me: %s\n", view.Primary, pb.me)
		if pb.curview.Viewnum != view.Viewnum {
			mydebug.DPrintf("known view: %v, new view: %v\n", pb.curview, view)
			if pb.ShouldTransferState(&view) {
			mydebug.DPrintf("Transfer database from %s to backup %s\n", pb.me, view.Backup)
		 	args := TransferStateArgs{Database:pb.database, SeenReqIDs:pb.seenReqIDs,
		 					LastUncommitPAReqID:pb.lastUncommitPAReqID, OldvaluesGet:pb.oldvaluesGet }
		 	var reply TransferStateReply
		 	call(view.Backup, "PBServer.ReceiveReplica", &args, &reply)
		}
		}
	}
	// at last, change view
	pb.curview = view
}

// tell the server to shut itself down.
// please do not change this function.
func (pb *PBServer) kill() {
	pb.dead = true
	pb.l.Close()
}


func StartServer(vshost string, me string) *PBServer {
	pb := new(PBServer)
	pb.me = me
	pb.vs = viewservice.MakeClerk(me, vshost)
	// Your pb.* initializations here.
	pb.curview = viewservice.View{}
	pb.database = make(map[string]string)
	pb.seenReqIDs = make(map[string]bool)
	pb.oldvaluesGet = make(map[string]string)
	pb.lastUncommitPAReqID = ""
	
	rpcs := rpc.NewServer()
	rpcs.Register(pb)

	os.Remove(pb.me)
	l, e := net.Listen("unix", pb.me)
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	pb.l = l

	// please do not change any of the following code,
	// or do anything to subvert it.

	go func() {
		for pb.dead == false {
			conn, err := pb.l.Accept()
			if err == nil && pb.dead == false {
				if pb.unreliable && (rand.Int63()%1000) < 100 {
					// discard the request.
					conn.Close()
				} else if pb.unreliable && (rand.Int63()%1000) < 200 {
					// process the request but force discard of reply.
					c1 := conn.(*net.UnixConn)
					f, _ := c1.File()
					err := syscall.Shutdown(int(f.Fd()), syscall.SHUT_WR)
					if err != nil {
						fmt.Printf("shutdown: %v\n", err)
					}
					go rpcs.ServeConn(conn)
				} else {
					go rpcs.ServeConn(conn)
				}
			} else if err == nil {
				conn.Close()
			}
			if err != nil && pb.dead == false {
				fmt.Printf("PBServer(%v) accept: %v\n", me, err.Error())
				pb.kill()
			}
		}
	}()

	go func() {
		for pb.dead == false {
			pb.tick()
			time.Sleep(viewservice.PingInterval)
		}
	}()

	return pb
}