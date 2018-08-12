package simpledb;

import java.util.HashSet;
import java.util.Set;

/**
 * Lock for a single page. This class is NOT thread safe!
 */
public class PageLock {
    private Set<TransactionId> readHolders;
    private TransactionId writeHolder;
    private long numWaiters;

    public PageLock() {
        this.readHolders = new HashSet<>();
        this.writeHolder = null;
        this.numWaiters = 0;
    }

    public synchronized boolean isReaderHeld() {
        return readHolders.isEmpty();
    }

    public synchronized boolean isWriterHeld() {
        return writeHolder != null;
    }

    public synchronized boolean holdsLock(TransactionId txId) {
        return (readHolders.contains(txId) || writeHolder.equals(txId));
    }

    public boolean isClean() {
        return ((writeHolder == null) && readHolders.isEmpty() && (numWaiters == 0));
    }

    public boolean tryAcquireReader(TransactionId txId) {
        invariants();
        if (writeHolder != null) {
            // A transaction that holds the writer lock also holds the reader lock.
            return writeHolder.equals(txId);
        }
        readHolders.add(txId);
        return true;
    }

    private boolean isSingleReaderLockHolder(TransactionId txId) {
        return ((writeHolder == null) && (readHolders.size() == 1) && readHolders.contains(txId));
    }

    public boolean tryAcquireWriter(TransactionId txId) {
        invariants();
        // Re-entrant is fine.
        if (writeHolder != null) {
            return writeHolder.equals(txId);
        }
        // Can upgrade from a reader to a writer lock if txId is the only holder.
        if (isSingleReaderLockHolder(txId)) {
            readHolders.clear();
        }
        if (readHolders.isEmpty()) {
            writeHolder = txId;
            return true;
        }
        return false;
    }

    public boolean tryAcquire(TransactionId txId, Permissions perm) {
        if (perm.equals(Permissions.READ_ONLY)) {
            return tryAcquireReader(txId);
        }
        return tryAcquireWriter(txId);
    }

    public void acquire(TransactionId txId, Permissions perm) {
        synchronized (this) {
            if (!tryAcquire(txId, perm)) {
                numWaiters += 1;
                while (!tryAcquire(txId, perm)) {
                    try {
                        wait();
                    } catch (InterruptedException e) {}
                }
                numWaiters -= 1;
            }
        }
    }

    public void acquireReader(TransactionId txId) {
        acquire(txId, Permissions.READ_ONLY);
    }

    public void acquireWriter(TransactionId txId) {
        acquire(txId, Permissions.READ_WRITE);
    }


    public void release(TransactionId txId) {
        invariants();
        if (writeHolder == null) {
            boolean removed = readHolders.remove(txId);
            assert(removed);
        } else {
            assert(writeHolder.equals(txId));
            writeHolder = null;
        }
    }

    private void invariants() {
        if (!readHolders.isEmpty()) {
            assert(writeHolder == null);
        }
        if (writeHolder != null) {
            assert(readHolders.isEmpty());
        }
    }
}

