package simpledb;
import java.util.*;

/**
 * SeqScan is an implementation of a sequential scan access method that reads
 * each tuple of a table in no particular order (e.g., as they are laid out on
 * disk).
 */
public class SeqScan implements DbIterator {
    private TransactionId txId;
    private int tableid;
    private String tableAlias;
    private DbFile dbFile;
    private DbFileIterator fileIter;
    private TupleDesc td;
    /**
     * Creates a sequential scan over the specified table as a part of the
     * specified transaction.
     *
     * @param tid The transaction this scan is running as a part of.
     * @param tableid the table to scan.
     * @param tableAlias the alias of this table (needed by the parser);
     *         the returned tupleDesc should have fields with name tableAlias.fieldName
     *         (note: this class is not responsible for handling a case where tableAlias
     *         or fieldName are null.  It shouldn't crash if they are, but the resulting
     *         name can be null.fieldName, tableAlias.null, or null.null).
     */
    public SeqScan(TransactionId tid, int tableid, String tableAlias) {
        this.txId = tid;
        this.tableid = tableid;
        this.tableAlias = tableAlias;

        dbFile = Database.getCatalog().getDbFile(tableid);
        fileIter = dbFile.iterator(txId);
        // init td
        TupleDesc rawTd = dbFile.getTupleDesc();
        Type[] typeAr = new Type[rawTd.numFields()];
        String[] fieldAr = new String[rawTd.numFields()];
        for (int i = 0; i < rawTd.numFields(); ++i) {
            typeAr[i] = rawTd.getType(i);
            fieldAr[i] = tableAlias + rawTd.getFieldName(i);
        }
        td = new TupleDesc(typeAr, fieldAr);
    }

    @Override
    public void open()
        throws DbException, TransactionAbortedException {
        fileIter.open();
    }

    /**
     * Returns the TupleDesc with field names from the underlying HeapFile,
     * prefixed with the tableAlias string from the constructor.
     * @return the TupleDesc with field names from the underlying HeapFile,
     * prefixed with the tableAlias string from the constructor.
     */
    @Override
    public TupleDesc getTupleDesc() {
        return td;
    }

    @Override
    public boolean hasNext() throws TransactionAbortedException, DbException {
        return fileIter.hasNext();
    }

    @Override
    public Tuple next()
        throws NoSuchElementException, TransactionAbortedException, DbException {
        return fileIter.next();
    }

    @Override
    public void close() {
        fileIter.close();
    }

    @Override
    public void rewind()
        throws DbException, NoSuchElementException, TransactionAbortedException {
        fileIter.rewind();
    }
}
