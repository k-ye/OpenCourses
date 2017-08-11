package viewservice

import "net"
import "net/rpc"
import "log"
import "time"
import "sync"
import "fmt"
import "os"
import "sync/atomic"
import "mydebug"

type ViewServer struct {
	mu       sync.Mutex
	l        net.Listener
	dead     bool  // for testing
	rpccount int32 // for testing
	me       string


	// Your declarations here.
	View View
	ACKed bool
	TimeDelay map[string]uint
	AdditionalBackup string
	// whenever View.Viewnum changed, make ACKed false
}

func IsEmptyClerk(name string) bool {
	return name == ""
}

func IsSameClerk(name1 string, name2 string) bool {
	return name1 == name2
}

func IsEmptyView(View *View) bool {
	return IsEmptyClerk(View.Primary) && IsEmptyClerk(View.Backup)
}
//
// server Ping RPC handler.
//
func (vs *ViewServer) Ping(args *PingArgs, reply *PingReply) error {

	// Your code here.
	vs.mu.Lock()
	vs.TimeDelay[args.Me] = 0

	if IsEmptyView(&vs.View) {
		// initial
		mydebug.DPrintf("Initial state, primary %s coming in\n", args.Me)
		vs.View.Viewnum++
		vs.View.Primary = args.Me
		vs.View.Backup = ""
		vs.ACKed = false
	} else if !IsEmptyClerk(vs.View.Primary) && IsSameClerk(vs.View.Primary, args.Me) && 
	args.Viewnum == vs.View.Viewnum && !vs.ACKed {
		mydebug.DPrintf("Primary %s ACKed\n", args.Me)
		// primary acknowledging
		vs.ACKed = true
	}	else if !IsSameClerk(vs.View.Primary, args.Me) && IsEmptyClerk(vs.View.Backup) && vs.ACKed {
		// backup coming in, make it as backup (and thus change the view) only when ACKed
		mydebug.DPrintf("Backup %s coming in\n", args.Me)
		vs.View.Viewnum ++
		vs.View.Backup = args.Me
		vs.ACKed = false
	} else if IsSameClerk(vs.View.Primary, args.Me) && args.Viewnum == 0 {
		// primary restarted, no matter if it is ACKed
		// This scenario will only happen at the first few round, when primary hasn't ACKed itself.
		mydebug.DPrintf("primary restarted\n")
		vs.View.Viewnum ++
		vs.View.Primary = vs.View.Backup
		vs.View.Backup = args.Me
		vs.ACKed = false
	} else if !IsSameClerk(vs.View.Primary, args.Me) && !IsSameClerk(vs.View.Backup, args.Me) && 
		!IsEmptyClerk(vs.View.Primary) && !IsEmptyClerk(vs.View.Backup) && !IsSameClerk(vs.AdditionalBackup, args.Me) {
		// a third idle server, don't care if primary is ACKed
		mydebug.DPrintf("additonal backup %s coming in\n", args.Me)
		vs.AdditionalBackup = args.Me
	}

	reply.View = vs.View
	vs.mu.Unlock()
	return nil
}

//
// server Get() RPC handler.
//
func (vs *ViewServer) Get(args *GetArgs, reply *GetReply) error {

	// Your code here.
	reply.View = vs.View
	return nil
}

func RemoveClerk(timemap map[string]uint, name *string) {
	delete(timemap, *name)
	*name = ""
}


//
// tick() is called once per PingInterval; it should notice
// if servers have died or recovered, and change the view
// accordingly.
//
func (vs *ViewServer) tick() {

	// Your code here.
	vs.mu.Lock()
	// increase delay and check for each server to see if dead
	var primaryDead, backupDead, additionalDead = false, false, false
	for key, _ := range vs.TimeDelay {
		vs.TimeDelay[key]++
		if vs.TimeDelay[key] > DeadPings {
			switch key {
			case vs.View.Primary:
				primaryDead = true
			case vs.View.Backup:
				backupDead = true
			case vs.AdditionalBackup:
				additionalDead = true
			}
		}
	}

	if additionalDead {
		//fmt.Println("additional backup dead")
		RemoveClerk(vs.TimeDelay, &vs.AdditionalBackup)
	}

	if primaryDead && backupDead && vs.ACKed {
		// Not sure if vs.ACKed should be checked ?!?!
		mydebug.DPrintf("primary dead and backup dead\n")
		RemoveClerk(vs.TimeDelay, &vs.View.Primary)
		RemoveClerk(vs.TimeDelay, &vs.View.Backup)
		vs.View.Primary = vs.AdditionalBackup
		vs.AdditionalBackup = ""
		vs.View.Viewnum ++
		vs.ACKed = false		
	} else if primaryDead && vs.ACKed {
		// only when vs is ACKed can we move to the next view
		// This scenario will happen when primary has ACKed and then died
		mydebug.DPrintf("primary %s dead, backup %s prompted\n", vs.View.Primary, vs.View.Backup)
		RemoveClerk(vs.TimeDelay, &vs.View.Primary)
		vs.View.Primary = vs.View.Backup
		vs.View.Backup = vs.AdditionalBackup
		vs.AdditionalBackup = ""
		vs.View.Viewnum ++
		vs.ACKed = false
	} else if backupDead && vs.ACKed {
		// This scenario will happen when primary has ACKed and then backup died
		mydebug.DPrintf("backup %s dead, additional backup %s prompted\n", vs.View.Backup, vs.AdditionalBackup)
		RemoveClerk(vs.TimeDelay, &vs.View.Backup)
		vs.View.Backup = vs.AdditionalBackup
		vs.AdditionalBackup = ""
		vs.View.Viewnum ++
		vs.ACKed = false
	}

	vs.mu.Unlock()
}

//
// tell the server to shut itself down.
// for testing.
// please don't change this function.
//
func (vs *ViewServer) Kill() {
	vs.dead = true
	vs.l.Close()
}

// please don't change this function.
func (vs *ViewServer) GetRPCCount() int32 {
	return atomic.LoadInt32(&vs.rpccount)
}

func StartServer(me string) *ViewServer {
	vs := new(ViewServer)
	vs.me = me
	// Your vs.* initializations here.
	vs.View = View{Primary:"", Backup:""}
	vs.ACKed = false
	vs.TimeDelay = make(map[string]uint)
	vs.AdditionalBackup = ""

	// tell net/rpc about our RPC server and handlers.
	rpcs := rpc.NewServer()
	rpcs.Register(vs)

	// prepare to receive connections from clients.
	// change "unix" to "tcp" to use over a network.
	os.Remove(vs.me) // only needed for "unix"
	l, e := net.Listen("unix", vs.me)
	if e != nil {
		log.Fatal("listen error: ", e)
	}
	vs.l = l

	// please don't change any of the following code,
	// or do anything to subvert it.

	// create a thread to accept RPC connections from clients.
	go func() {
		for vs.dead == false {
			conn, err := vs.l.Accept()
			if err == nil && vs.dead == false {
				atomic.AddInt32(&vs.rpccount, 1)
				go rpcs.ServeConn(conn)
			} else if err == nil {
				conn.Close()
			}
			if err != nil && vs.dead == false {
				fmt.Printf("ViewServer(%v) accept: %v\n", me, err.Error())
				vs.Kill()
			}
		}
	}()

	// create a thread to call tick() periodically.
	go func() {
		for vs.dead == false {
			vs.tick()
			time.Sleep(PingInterval)
		}
	}()

	return vs
}
