package simpledb;

import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.function.IntToDoubleFunction;

/**
 *
 */
public class LockManager {
    /**
     * Lock for a single page. This class is NOT thread safe!
     */
    private static class PageLock {
        private Set<TransactionId> readHolders;
        private TransactionId writeHolder;
        private long numWaiters;

        public PageLock() {
            this.readHolders = new HashSet<>();
            this.writeHolder = null;
            this.numWaiters = 0;
        }

        public synchronized boolean holdsLock(TransactionId txId) {
            return (readHolders.contains(txId) || writeHolder.equals(txId));
        }

        public void incNumWaiters() {
            numWaiters += 1;
        }

        public void decNumWaiters() {
            numWaiters -= 1;
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

    private final Map<PageId, PageLock> pageIdToLocks;

    public LockManager() {
        this.pageIdToLocks = new HashMap<>();
    }

    private synchronized PageLock getLock(PageId pageId) {
        PageLock l = pageIdToLocks.computeIfAbsent(pageId, p -> new PageLock());
        return l;
    }

    private synchronized PageLock getLockIfExists(PageId pageId) {
        return pageIdToLocks.get(pageId);
    }

    public synchronized boolean holdsLock(TransactionId txId, PageId pageId) {
        PageLock l = getLockIfExists(pageId);
        if (l == null) {
            return false;
        }
        return l.holdsLock(txId);
    }

    public void acquireReaderLock(TransactionId txId, PageId pageId) throws InterruptedException {
        PageLock l = getLock(pageId);
        synchronized (l) {
            if (l.tryAcquireReader(txId)) {
                return;
            }
            l.incNumWaiters();
            while (!l.tryAcquireReader(txId)) {
                l.wait();
            }
            l.decNumWaiters();
        }
    }

    public void acquireWriterLock(TransactionId txId, PageId pageId) throws InterruptedException {
        PageLock l = getLock(pageId);
        synchronized (l) {
            if (l.tryAcquireWriter(txId)) {
                return;
            }
            l.incNumWaiters();
            while (!l.tryAcquireWriter(txId)) {
                l.wait();
            }
            l.decNumWaiters();
        }
    }

    public synchronized void release(TransactionId txId, PageId pageId) {
        PageLock l = getLockIfExists(pageId);
        assert(l != null);
        synchronized (l) {
            l.release(txId);
            l.notifyAll();
            if (l.isClean()) {
                // Recycle some memory.
                pageIdToLocks.remove(pageId);
            }
        }
    }
}
