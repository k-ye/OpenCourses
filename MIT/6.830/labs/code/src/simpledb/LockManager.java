package simpledb;

import java.util.*;
import java.util.concurrent.locks.Lock;
import java.util.function.IntToDoubleFunction;

/**
 *
 */
public class LockManager {
    private static class PageLock {
        private Set<TransactionId> readHolders;
        private TransactionId writeHolder;

        public PageLock() {
            this.readHolders = new HashSet<>();
            this.writeHolder = null;
        }

        public synchronized boolean holdsLock(TransactionId txId) {
            return (readHolders.contains(txId) || writeHolder.equals(txId));
        }

        public synchronized boolean tryAcquireReader(TransactionId txId) {
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

        public synchronized boolean tryAcquireWriter(TransactionId txId) {
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

        public synchronized void release(TransactionId txId) {
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

    public synchronized boolean holdsLock(TransactionId txId, PageId pageId) {
        PageLock l = pageIdToLocks.get(pageId);
        if (l == null) {
            return false;
        }
        return l.holdsLock(txId);
    }

    public void acquireReaderLock(TransactionId txId, PageId pageId) throws InterruptedException {
        PageLock l = getLock(pageId);
        synchronized (l) {
            while (!l.tryAcquireReader(txId)) {
                l.wait();
            }
        }
    }

    public void acquireWriterLock(TransactionId txId, PageId pageId) throws InterruptedException {
        PageLock l = getLock(pageId);
        synchronized (l) {
            while (!l.tryAcquireWriter(txId)) {
                l.wait();
            }
        }
    }

    public void release(TransactionId txId, PageId pageId) {
        PageLock l = getLock(pageId);
        synchronized (l) {
            l.release(txId);
            l.notifyAll();
        }
    }
}
