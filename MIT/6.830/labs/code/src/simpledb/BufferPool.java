package simpledb;

import java.io.*;
import java.util.*;

/**
 * BufferPool manages the reading and writing of pages into memory from
 * disk. Access methods call into it to retrieve pages, and it fetches
 * pages from the appropriate location.
 * <p>
 * The BufferPool is also responsible for locking;  when a transaction fetches
 * a page, BufferPool which check that the transaction has the appropriate
 * locks to read/write the page.
 */
public class BufferPool {
    /** Bytes per page, including header. */
    public static final int PAGE_SIZE = 4096;

    /**
     * Default number of pages passed to the constructor. This is used by
     * other classes. BufferPool should use the numPages argument to the
     * constructor instead.
     */
    public static final int DEFAULT_PAGES = 50;

    private final int numPages;
    private final Map<PageId, Page> idToPages;
    private final DAG<TransactionId> lockDag;
    private final Map<PageId, PageLock> pageIdToLocks;
    private final Map<TransactionId, Set<PageId>> txnToLockedPages;
    /**
     * Creates a BufferPool that caches up to numPages pages.
     *
     * @param numPages maximum number of pages in this buffer pool.
     */
    public BufferPool(int numPages) {
        this.numPages = numPages;
        this.idToPages = new HashMap<>();

        this.lockDag = new DAG<>();
        this.pageIdToLocks = new HashMap<>();
        this.txnToLockedPages = new HashMap<>();
    }

    public void acquirePage(TransactionId tid, PageId pid, Permissions perm) throws TransactionAbortedException, DbException {
        // While we update {@code pageIdToLocks} and {@code txnToLockedPages} inside
        // the lock of {@code this}, we must acquire the page lock outside the lock,
        // otherwise no other thread can access the buffer pool.
        PageLock pl;
        synchronized (this) {
            pl = pageIdToLocks.computeIfAbsent(pid, p -> new PageLock(p, lockDag));
            txnToLockedPages.computeIfAbsent(tid, t -> new HashSet<>()).add(pid);
        }
        try {
            pl.acquire(tid, perm);
        } catch (PageDeadlockException e) {
            throw new TransactionAbortedException();
        }
    }

    public synchronized Page getLockedPage(TransactionId tid, PageId pid)
        throws TransactionAbortedException, DbException {
        assert(holdsLock(tid, pid));

        Page page = idToPages.get(pid);
        if (page == null) {
            while (idToPages.size() >= numPages) {
                evictPage();
            }
            page = Database.getCatalog().getDbFile(pid.getTableId()).readPage(pid);
            idToPages.put(pid, page);
        }
        return page;
    }
    /**
     * Retrieve the specified page with the associated permissions.
     * Will acquire a lock and may block if that lock is held by another
     * transaction.
     * <p>
     * The retrieved page should be looked up in the buffer pool.  If it
     * is present, it should be returned.  If it is not present, it should
     * be added to the buffer pool and returned.  If there is insufficient
     * space in the buffer pool, an page should be evicted and the new page
     * should be added in its place.
     *
     * @param tid the ID of the transaction requesting the page
     * @param pid the ID of the requested page
     * @param perm the requested permissions on the page
     */
    public Page getPage(TransactionId tid, PageId pid, Permissions perm)
        throws TransactionAbortedException, DbException {
        acquirePage(tid, pid, perm);
        return getLockedPage(tid, pid);
    }

    /**
     * Releases the lock on a page.
     * Calling this is very risky, and may result in wrong behavior. Think hard
     * about who needs to call this and why, and why they can run the risk of
     * calling it.
     *
     * @param tid the ID of the transaction requesting the unlock
     * @param pid the ID of the page to unlock
     */
    public synchronized void releasePage(TransactionId tid, PageId pid) {
        PageLock pl = pageIdToLocks.get(pid);
        if (pl == null) {
            return;
        }
        synchronized (pl) {
            if (pl.release(tid)) {
                if (pl.isClean()) {
                    pageIdToLocks.remove(pid);
                }
            }
        }
    }

    /**
     * Release all locks associated with a given transaction.
     *
     * @param tid the ID of the transaction requesting the unlock
     */
    public void transactionComplete(TransactionId tid) throws IOException {
        transactionComplete(tid, true);
    }

    /** Return true if the specified transaction has a lock on the specified page */
    public synchronized boolean holdsLock(TransactionId tid, PageId p) {
        PageLock pl = pageIdToLocks.get(p);
        if (pl == null) {
            return false;
        }
        return pl.holdsLock(tid);
    }

    /**
     * Commit or abort a given transaction; release all locks associated to
     * the transaction.
     *
     * @param tid the ID of the transaction requesting the unlock
     * @param commit a flag indicating whether we should commit or abort
     */
    public synchronized void transactionComplete(TransactionId tid, boolean commit)
        throws IOException {
        if (!txnToLockedPages.containsKey(tid)) {
            return;
        }
        for (PageId pageId : txnToLockedPages.get(tid)) {
            Page page = idToPages.get(pageId);
            if (page == null) {
                continue;
            }
            if (commit) {
                flushPage(pageId);
                // next abort should only roll back to here
                page.setBeforeImage();
            } else {
                idToPages.put(pageId, page.getBeforeImage());
            }
        }
        for (PageId pageId : txnToLockedPages.get(tid)) {
            releasePage(tid, pageId);
        }
        txnToLockedPages.remove(tid);
    }

    /**
     * Add a tuple to the specified table behalf of transaction tid.  Will
     * acquire a write lock on the page the tuple is added to(Lock 
     * acquisition is not needed for lab2). May block if the lock cannot 
     * be acquired.
     * 
     * Marks any pages that were dirtied by the operation as dirty by calling
     * their markDirty bit, and updates cached versions of any pages that have 
     * been dirtied so that future requests see up-to-date pages. 
     *
     * @param tid the transaction adding the tuple
     * @param tableId the table to add the tuple to
     * @param t the tuple to add
     */
    public void insertTuple(TransactionId tid, int tableId, Tuple t)
        throws DbException, IOException, TransactionAbortedException {
        DbFile file = Database.getCatalog().getDbFile(tableId);
        file.addTuple(tid, t);
    }

    /**
     * Remove the specified tuple from the buffer pool.
     * Will acquire a write lock on the page the tuple is removed from. May block if
     * the lock cannot be acquired.
     *
     * Marks any pages that were dirtied by the operation as dirty by calling
     * their markDirty bit.  Does not need to update cached versions of any pages that have 
     * been dirtied, as it is not possible that a new page was created during the deletion
     * (note difference from addTuple).
     *
     * @param tid the transaction adding the tuple.
     * @param t the tuple to add
     */
    public void deleteTuple(TransactionId tid, Tuple t)
        throws DbException, TransactionAbortedException {
        DbFile file = Database.getCatalog().getDbFile(t.getRecordId().getPageId().getTableId());
        file.deleteTuple(tid, t);
    }

    /**
     * Flush all dirty pages to disk.
     * NB: Be careful using this routine -- it writes dirty data to disk so will
     *     break simpledb if running in NO STEAL mode.
     */
    public synchronized void flushAllPages() throws IOException {
        for (PageId pageId : idToPages.keySet()) {
            flushPage(pageId);
        }
    }

    /** Remove the specific page id from the buffer pool.
        Needed by the recovery manager to ensure that the
        buffer pool doesn't keep a rolled back page in its
        cache.
    */
    public synchronized void discardPage(PageId pid) {
        idToPages.remove(pid);
    }

    /**
     * Flushes a certain page to disk
     * @param pid an ID indicating the page to flush
     */
    private synchronized void flushPage(PageId pid) throws IOException {
        Page page = idToPages.get(pid);
        if (page == null) {
            return;
        }
        // RECOVERY
        TransactionId dirtier = page.isDirty();

        if (dirtier == null)
            return;

        Database.getLogFile().logWrite(dirtier, page.getBeforeImage(), page);
        Database.getLogFile().force();

        Database.getCatalog().getDbFile(pid.getTableId()).writePage(page);
        page.markDirty(false, null);
    }

    /** Write all pages of the specified transaction to disk.
     */
    public synchronized  void flushPages(TransactionId tid) throws IOException {
        // some code goes here
        // not necessary for lab1|lab2|lab3
        for (PageId pageId : txnToLockedPages.get(tid)) {
            Page page = idToPages.get(pageId);
            if (page == null) {
                continue;
            }

            flushPage(pageId);
            // next abort should only roll back to here
            page.setBeforeImage();
        }
    }

    /**
     * Discards a page from the buffer pool.
     * Flushes the page to disk to ensure dirty pages are updated on disk.
     */
    private synchronized void evictPage() throws DbException {
        if (idToPages.isEmpty()) {
            return;
        }
        try {
            for (PageId pageId : idToPages.keySet()) {
                Page page = idToPages.get(pageId);
                if (page.isDirty() == null) {
                    // We can only evict a non-dirty page.
                    PageLock pl = pageIdToLocks.get(pageId);
                    assert((pl == null) || !pl.isWriterHeld());
                    flushPage(pageId);
                    idToPages.remove(pageId);
                    return;
                }
            }
            throw new DbException("Cannot evict a page: all pages are dirty.");
        } catch (IOException e) {
            throw new DbException(e.getMessage());
        }
    }

}
