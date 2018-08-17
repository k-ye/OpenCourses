package simpledb;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Lock for a single page. This class is NOT thread safe!
 */
public class PageLock {
    private final PageId pageId;
    private final DAG<TransactionId> lockDag;
    private final Set<TransactionId> readHolders;
    private TransactionId writeHolder;
    private long numWaiters;

    public PageLock(PageId pageId, DAG<TransactionId> lockDag) {
        this.pageId = pageId;
        this.lockDag = lockDag;
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

    public synchronized boolean isClean() {
        return ((writeHolder == null) && readHolders.isEmpty() && (numWaiters == 0));
    }

    private static class TryAcquireResult {
        private final boolean acquired;
        private final Set<TransactionId> holders;

        private TryAcquireResult(boolean acquired) {
            this.acquired = acquired;
            this.holders = new HashSet<>();
        }

        public boolean isAcquired() { return acquired; }

        public Iterable<TransactionId> getHolders() { return holders; }

        private void addHolder(TransactionId txId) {
            this.holders.add(txId);
        }

        static TryAcquireResult make(boolean acquired, TransactionId holder) {
            TryAcquireResult result = new TryAcquireResult(acquired);
            result.addHolder(holder);
            return result;
        }

        static TryAcquireResult make(boolean acquired, Iterable<TransactionId> holders) {
            TryAcquireResult result = new TryAcquireResult(acquired);
            for (TransactionId t : holders) {
                result.addHolder(t);
            }
            return result;
        }
    }

    /** Precondition: this lock is held. */
    private TryAcquireResult tryAcquireReaderImpl(TransactionId txId) {
        invariants();
        if (writeHolder != null) {
            // A transaction that holds the writer lock also holds the reader lock.
            boolean acquired = writeHolder.equals(txId);
            return TryAcquireResult.make(acquired, writeHolder);
        }
        readHolders.add(txId);
        return TryAcquireResult.make(true, readHolders);
    }

    /** Precondition: this lock is held. */
    private boolean isSingleReaderLockHolder(TransactionId txId) {
        return ((writeHolder == null) && (readHolders.size() == 1) && readHolders.contains(txId));
    }

    /** Precondition: this lock is held. */
    private TryAcquireResult tryAcquireWriterImpl(TransactionId txId) {
        invariants();
        // Re-entrant is fine.
        if (writeHolder != null) {
            boolean acquired = writeHolder.equals(txId);
            return TryAcquireResult.make(acquired, writeHolder);
        }
        // Can upgrade from a reader to a writer lock if txId is the only holder.
        if (isSingleReaderLockHolder(txId)) {
            readHolders.clear();
        }
        if (readHolders.isEmpty()) {
            writeHolder = txId;
            return TryAcquireResult.make(true, writeHolder);
        }
        return TryAcquireResult.make(false, readHolders);
    }

    /** Precondition: this lock is held. */
    private TryAcquireResult tryAcquireImpl(TransactionId txId, Permissions perm) {
        if (perm.equals(Permissions.READ_ONLY)) {
            return tryAcquireReaderImpl(txId);
        }
        return tryAcquireWriterImpl(txId);
    }

    public synchronized void acquire(TransactionId txId, Permissions perm) throws PageDeadlockException {
        boolean hasWaited = false;
        while (true) {
            TryAcquireResult acquireResult = tryAcquireImpl(txId, perm);
            if (acquireResult.isAcquired()) {
                if (hasWaited) {
                    numWaiters -= 1;
                }
                // System.out.printf("%s has acquired page %s\n", txId.toString(), pageId.toString());
                return;
            }
            // Did not get the lock.
            if (!hasWaited) {
                numWaiters += 1;
                hasWaited = true;
            }
            Set<TransactionId> holdersForRollback = new HashSet<>();
            synchronized (lockDag) {
                for (TransactionId holder : acquireResult.getHolders()) {
                    assert(holder != null);
                    if (txId.equals(holder)) {
                        // This happens when |txId| is trying to acquire writer lock,
                        // but it *and some other transactions* are already
                        // holding the reader lock.
                        continue;
                    }
                    if (lockDag.cyclic(txId, holder)) {
                        // System.out.printf("WARNING: %s will deadlock with %s\n", txId.toString(), holder.toString());
                        // Rollback the graph
                        for (TransactionId holder2 : holdersForRollback) {
                            lockDag.remove(txId, holder2);
                        }
                        throw new PageDeadlockException();
                    }
                    holdersForRollback.add(holder);
                    lockDag.add(txId, holder);
                    // System.out.printf("%s is waiting for %s on page %s\n", txId.toString(), holder.toString(), pageId.toString());
                }
            }
            try {
                wait();
            } catch (InterruptedException e) {
                // No op
            } finally {
                synchronized (lockDag) {
                    // Rollback the graph
                    for (TransactionId holder : holdersForRollback) {
                        lockDag.remove(txId, holder);
                    }
                }
            }
        }
    }

    public synchronized boolean release(TransactionId txId) {
        invariants();
        boolean result = false;
        if (writeHolder == null) {
            result = readHolders.remove(txId);
        } else if (writeHolder.equals(txId)) {
            writeHolder = null;
            result = true;
        }
        if (result) {
            notifyAll();
            // System.out.printf("%s has released page %s\n", txId.toString(), pageId.toString());
        }
        return result;
    }

    /** Precondition: this lock is held. */
    private void invariants() {
        if (!readHolders.isEmpty()) {
            assert(writeHolder == null);
        }
        if (writeHolder != null) {
            assert(readHolders.isEmpty());
        }
    }
}

