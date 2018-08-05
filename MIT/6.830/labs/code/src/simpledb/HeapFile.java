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
    private final int precomputedNumPages;

    /**
     * Constructs a heap file backed by the specified file.
     *
     * @param f the file that stores the on-disk backing store for this heap file.
     */
    public HeapFile(File f, TupleDesc td) {
        this.file = f;
        this.tupleDesc = td;
        this.precomputedNumPages = (int)(f.length() / BufferPool.PAGE_SIZE);
        assert(this.precomputedNumPages > 0);
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
        return getFile().getAbsoluteFile().hashCode();
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
            throw new IllegalArgumentException("Invalid page ID.");
        }

        try (FileInputStream fis = new FileInputStream(file)) {
            final int pageNo = pid.pageno();
            final int pageSz = BufferPool.PAGE_SIZE;
            byte[] data = HeapPage.createEmptyPageData();
            fis.read(data, pageNo * pageSz, pageSz);
            return new HeapPage((HeapPageId) pid, data);
        } catch (Exception e) {
            throw new RuntimeException(e.getMessage());
        }
    }

    @Override
    public void writePage(Page page) throws IOException {
        // some code goes here
        // not necessary for lab1
    }

    /**
     * Returns the number of pages in this HeapFile.
     */
    public int numPages() {
        return precomputedNumPages;
    }

    @Override
    public ArrayList<Page> addTuple(TransactionId tid, Tuple t)
        throws DbException, IOException, TransactionAbortedException {
        // some code goes here
        // not necessary for lab1
        return null;
    }

    @Override
    public Page deleteTuple(TransactionId tid, Tuple t)
        throws DbException, TransactionAbortedException {
        // some code goes here
        // not necessary for lab1
        return null;
    }

    @Override
    public DbFileIterator iterator(TransactionId txId) {
        return new HeapFileIter(txId);
    }

    private class HeapFileIter implements DbFileIterator {
        private final TransactionId txId;
        private boolean closed;
        private int curPageNo;
        private Iterator<Tuple> curPageIter;

        public HeapFileIter(TransactionId txId) {
            this.txId = txId;
            this.closed = true;
        }

        private Iterator<Tuple> getCurPageIter() throws TransactionAbortedException, DbException {
            HeapPageId pageId = new HeapPageId(getId(), curPageNo);
            HeapPage page = (HeapPage) Database.getBufferPool().getPage(txId, pageId, /*perm=*/null);
            return page.iterator();
        }

        private void fix() throws TransactionAbortedException, DbException {
            if ((curPageIter != null) && curPageIter.hasNext()) {
                return;
            }
            curPageNo += 1;
            curPageIter = null;
            while (curPageNo < numPages()) {
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
            if (closed || (curPageNo >= numPages()) || (curPageIter == null)) {
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

