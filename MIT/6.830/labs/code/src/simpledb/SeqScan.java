package simpledb;
import java.util.*;

/**
 * SeqScan is an implementation of a sequential scan access method that reads
 * each tuple of a table in no particular order (e.g., as they are laid out on
 * disk).
 */
public class SeqScan implements DbIterator {
    private final TransactionId txId;
    private final int tableId;
    private final String tableAlias;

    private final DbFile dbFile;
    private DbFileIterator underlyingIter;

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
        this.tableId = tableid;
        this.tableAlias = tableAlias;

        this.dbFile = Database.getCatalog().getDbFile(tableid);
        this.underlyingIter = null;
    }

    public void open()
        throws DbException, TransactionAbortedException {
        rewind();
    }

    /**
     * Returns the TupleDesc with field names from the underlying HeapFile,
     * prefixed with the tableAlias string from the constructor.
     * @return the TupleDesc with field names from the underlying HeapFile,
     * prefixed with the tableAlias string from the constructor.
     */
    public TupleDesc getTupleDesc() {
        TupleDesc td = Database.getCatalog().getTupleDesc(tableId);
        Type[] typeAr = new Type[td.numFields()];
        String[] fieldAr = new String[td.numFields()];
        for (int i = 0; i < td.numFields(); ++i) {
            typeAr[i] = td.getType(i);
            fieldAr[i] = tableAlias + "-" + td.getFieldName(i);
        }
        return new TupleDesc(typeAr, fieldAr);
    }

    public boolean hasNext() throws TransactionAbortedException, DbException {
        if (underlyingIter == null) {
            return false;
        }
        return underlyingIter.hasNext();
    }

    public Tuple next()
        throws NoSuchElementException, TransactionAbortedException, DbException {
        if (!hasNext()) {
            throw new NoSuchElementException("End of SeqScan.");
        }
        // No need for fix here.
        return underlyingIter.next();
    }

    public void close() {
        this.underlyingIter = null;
    }

    public void rewind()
        throws DbException, NoSuchElementException, TransactionAbortedException {
        this.underlyingIter = dbFile.iterator(txId);
        this.underlyingIter.open();
    }
}
