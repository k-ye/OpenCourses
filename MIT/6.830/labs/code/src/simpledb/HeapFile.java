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
 * @author Sam Madden
 * @see simpledb.HeapPage#HeapPage
 */
public class HeapFile implements DbFile {
    private File file;
    private TupleDesc tupleDesc;
    private final int pageSize;
    private final int numPagesCached;
    private RandomAccessFile raf;

    /**
     * Constructs a heap file backed by the specified file.
     *
     * @param f the file that stores the on-disk backing store for this heap file.
     */
    public HeapFile(File f, TupleDesc td) {
        assert f != null && td != null;
        this.file = f;
        this.tupleDesc = td;
        this.pageSize = BufferPool.PAGE_SIZE;
        assert (file.length() % pageSize == 0);
        this.numPagesCached = (int) (file.length() / pageSize);
        try {
            raf = new RandomAccessFile(file, "r");
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
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
        // some code goes here
        return file.getAbsoluteFile().hashCode();
    }

    /**
     * Returns the TupleDesc of the table stored in this DbFile.
     *
     * @return TupleDesc of this DbFile.
     */
    public TupleDesc getTupleDesc() {
        // some code goes here
        return tupleDesc;
    }

    // see DbFile.java for javadocs
    public Page readPage(PageId pid) {
        assert pid instanceof HeapPageId;
        assert raf != null;

        try {
            long offset = pid.pageno() * pageSize;
            byte[] data = new byte[pageSize];
            synchronized (raf) {
                raf.seek(offset);
                raf.read(data, 0, pageSize);
            }
            return new HeapPage((HeapPageId) pid, data);
        } catch (IOException e) {
            e.printStackTrace();
        }
        return null;
    }

    // see DbFile.java for javadocs
    public void writePage(Page page) throws IOException {
        // some code goes here
        // not necessary for lab1
    }

    /**
     * Returns the number of pages in this HeapFile.
     */
    public int numPages() {
        return numPagesCached;
    }

    // see DbFile.java for javadocs
    public ArrayList<Page> addTuple(TransactionId tid, Tuple t)
            throws DbException, IOException, TransactionAbortedException {
        // some code goes here
        return null;
        // not necessary for lab1
    }

    // see DbFile.java for javadocs
    public Page deleteTuple(TransactionId tid, Tuple t)
            throws DbException, TransactionAbortedException {
        // some code goes here
        return null;
        // not necessary for lab1
    }

    // see DbFile.java for javadocs
    public DbFileIterator iterator(TransactionId tid) {
        // some code goes here
        class HfIterator implements DbFileIterator {
            private TransactionId tid;
            private int curPageNo;
            private HeapPage curPage;
            private Iterator<Tuple> pgTupleIter;
            private boolean closed;

            HfIterator(TransactionId tid) {
                this.tid = tid;
                this.curPageNo = 0;
                this.curPage = null;
                this.pgTupleIter = null;
                this.closed = true;
            }

            private HeapPageId makePageId() {
                return new HeapPageId(getId(), curPageNo);
            }

            private HeapPage makePage() throws DbException, TransactionAbortedException {
                return (HeapPage) Database.getBufferPool().getPage(tid, makePageId(), /* Permission */null);
            }

            private void fixIter() throws DbException, TransactionAbortedException {
                if (curPage == null || pgTupleIter == null) {
                    return;
                }
                while (!pgTupleIter.hasNext() && curPageNo + 1 < numPages()) {
                    ++curPageNo;
                    curPage = makePage();
                    pgTupleIter = curPage.iterator();
                }
                if (curPageNo >= numPages()) {
                    curPage = null;
                    pgTupleIter = null;
                }
            }

            private void reset() throws DbException, TransactionAbortedException {
                assert raf != null;
                curPageNo = 0;
                curPage = makePage();
                pgTupleIter = curPage.iterator();
                fixIter();
            }

            @Override
            public void open()
                    throws DbException, TransactionAbortedException {
                reset();
                closed = false;
            }

            @Override
            public boolean hasNext()
                    throws DbException, TransactionAbortedException {
                if (closed || curPageNo >= numPages() || curPage == null || pgTupleIter == null) {
                    return false;
                }
                return pgTupleIter.hasNext();
            }

            @Override
            public Tuple next()
                    throws DbException, TransactionAbortedException, NoSuchElementException {
                if (!hasNext()) {
                    throw new NoSuchElementException("End of HeapFile");
                }
                Tuple result = pgTupleIter.next();
                fixIter();
                return result;
            }

            @Override
            public void rewind() throws DbException, TransactionAbortedException {
                reset();
                closed = false;
            }

            @Override
            public void close() {
                closed = true;
            }
        }
        return new HfIterator(tid);
    }

}

