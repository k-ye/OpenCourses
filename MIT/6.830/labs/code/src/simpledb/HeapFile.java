package simpledb;

import java.io.*;
import java.util.*;

/**
 * HeapFile is an implementation of a DbFile that stores a collection
 * of tuples in no particular order.  Tuples are stored on pages, each of
 * which is a fixed size, and the file is simply a collection of those
 * pages. HeapFile works closely with HeapPage.  The format of HeapPages
 * is described in the HeapPage constructor.
 *
 * @see simpledb.HeapPage#HeapPage
 * @author Sam Madden
 */
public class HeapFile implements DbFile {
    private final File file;
    private final TupleDesc tupleDesc;
    private final int tableId;

    /**
     * Constructs a heap file backed by the specified file.
     *
     * @param f the file that stores the on-disk backing store for this heap file.
     */
    public HeapFile(File f, TupleDesc td) {
        this.file = f;
        this.tupleDesc = td;
        this.tableId = getFile().getAbsoluteFile().hashCode();
    }

    /**
     * Returns the File backing this HeapFile on disk.
     *
     * @return the File backing this HeapFile on disk.
     */
    public File getFile() {
        return file;
    }

    /**
    * Returns an ID uniquely identifying this HeapFile. Implementation note:
    * you will need to generate this tableid somewhere ensure that each
    * HeapFile has a "unique id," and that you always return the same value
    * for a particular HeapFile. We suggest hashing the absolute file name of
    * the file underlying the heapfile, i.e. f.getAbsoluteFile().hashCode().
    *
    * @return an ID uniquely identifying this HeapFile.
    */
    public int getId() {
        return tableId;
    }
    
    /**
     * Returns the TupleDesc of the table stored in this DbFile.
     * @return TupleDesc of this DbFile.
     */
    public TupleDesc getTupleDesc() {
        return tupleDesc;
    }

    @Override
    public Page readPage(PageId pid) {
        if ((pid.getTableId() != getId()) || (pid.pageno() >= numPages())) {
            throw new IllegalArgumentException("Cannot read page.");
        }

        try (RandomAccessFile raf = new RandomAccessFile(file, "r")) {
            final int pageSz = BufferPool.PAGE_SIZE;
            final int pageNo = pid.pageno();
            byte[] data = HeapPage.createEmptyPageData();
            raf.seek(pageNo * pageSz);
            raf.read(data, 0, pageSz);
            return new HeapPage((HeapPageId) pid, data);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    @Override
    public void writePage(Page page) throws IOException {
        if ((page.getId().getTableId() != getId())) {
            throw new IllegalArgumentException("Cannot write page.");
        }

        try (RandomAccessFile raf = new RandomAccessFile(file, "rw")) {
            final int pageSz = BufferPool.PAGE_SIZE;
            final int pageNo = page.getId().pageno();
            raf.seek(pageNo * pageSz);
            raf.write(page.getPageData());
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Returns the number of pages in this HeapFile.
     */
    public int numPages() {
        return (int)(file.length() / BufferPool.PAGE_SIZE);
    }

    private ArrayList<Page> addTuple(TransactionId tid, Tuple t, int startPageNo)
            throws DbException, IOException, TransactionAbortedException {
        BufferPool bufferPool = Database.getBufferPool();
        ArrayList<Page> result = new ArrayList<>();

        int numPagesSnapshot = numPages();
        for (int i = startPageNo; i < numPagesSnapshot; ++i) {
            HeapPage page = (HeapPage) bufferPool.getPage(tid, new HeapPageId(getId(), i), Permissions.READ_WRITE);
            if (page.getNumEmptySlots() > 0) {
                page.addTuple(t);
                result.add(page);
                break;
            } else {
                bufferPool.releasePage(tid, page.getId());
            }
        }
        if (result.isEmpty()) {
            synchronized (this) {
                if (numPagesSnapshot == numPages()) {
                    // First we write a new page to physical device.
                    HeapPageId pageId = new HeapPageId(getId(), numPages());
                    writePage(new HeapPage(pageId, HeapPage.createEmptyPageData()));
                }
                // Otherwise someone else has inserted a new page for us.
            }
            // Then retrieve it via buffer pool. This sets up
            // lock manager for the page properly.
            return addTuple(tid, t, numPagesSnapshot);
        }
        return result;
    }

    @Override
    public ArrayList<Page> addTuple(TransactionId tid, Tuple t)
        throws DbException, IOException, TransactionAbortedException {
        return addTuple(tid, t, 0);
    }

    @Override
    public Page deleteTuple(TransactionId tid, Tuple t)
        throws DbException, TransactionAbortedException {
        HeapPage page = (HeapPage) Database.getBufferPool().getPage(tid, t.getRecordId().getPageId(), Permissions.READ_WRITE);
        page.deleteTuple(t);
        return page;
    }

    @Override
    public DbFileIterator iterator(TransactionId txId) {
        return new HeapFileIter(txId, getId(), numPages());
    }

    private static class HeapFileIter implements DbFileIterator {
        private final TransactionId txId;
        private final int tableId;
        // Note that we cannot use numPages() of the parent, as it could grow when this iterator
        // is still used. Possible scenario:
        //
        // 1. Creates an iterator, I1, from this heap file,
        // 2. A transaction inserts each element in I1 into the same heap file. Since we could
        //  create new empty pages on the physical device. This will grow the actual number of
        //  pages.
        private final int numPagesSnapshot;

        private boolean closed;
        private int curPageNo;
        private Iterator<Tuple> curPageIter;

        public HeapFileIter(TransactionId txId, int tableId, int numPagesSnapshot) {
            this.txId = txId;
            this.tableId = tableId;
            this.numPagesSnapshot = numPagesSnapshot;
            this.closed = true;
        }

        private Iterator<Tuple> getCurPageIter() throws TransactionAbortedException, DbException {
            HeapPageId pageId = new HeapPageId(tableId, curPageNo);
            HeapPage page = (HeapPage) Database.getBufferPool().getPage(txId, pageId, Permissions.READ_ONLY);
            return page.iterator();
        }

        private void fix() throws TransactionAbortedException, DbException {
            if ((curPageIter != null) && curPageIter.hasNext()) {
                return;
            }
            curPageNo += 1;
            curPageIter = null;
            while (curPageNo < numPagesSnapshot) {
                Iterator<Tuple> pageIter = getCurPageIter();
                if (pageIter.hasNext()) {
                    curPageIter = pageIter;
                    return;
                }
                curPageNo += 1;
            }
        }

        @Override
        public void open()
                throws DbException, TransactionAbortedException {
            rewind();
        }

        @Override
        public boolean hasNext()
                throws DbException, TransactionAbortedException {
            if (closed || (curPageNo >= numPagesSnapshot) || (curPageIter == null)) {
                return false;
            }
            // If {@code curPageIter} is not null, then it must has a next value.
            assert (curPageIter.hasNext());
            return true;
        }

        @Override
        public Tuple next()
                throws DbException, TransactionAbortedException, NoSuchElementException {
            if (!hasNext()) {
                throw new NoSuchElementException("End of HeapFile.");
            }
            Tuple result = curPageIter.next();
            fix();
            return result;
        }

        @Override
        public void rewind() throws DbException, TransactionAbortedException {
            this.curPageNo = 0;
            this.curPageIter = getCurPageIter();
            fix();
            this.closed = false;
        }

        @Override
        public void close() {
            this.closed = true;
        }
    }
}

