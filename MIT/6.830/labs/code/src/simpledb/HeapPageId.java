package simpledb;

/** Unique identifier for HeapPage objects. */
public class HeapPageId implements PageId {
    private int tableid;
    private int pageNo;
    /**
     * Constructor. Create a page id structure for a specific page of a
     * specific table.
     *
     * @param tableid The table that is being referenced
     * @param pageNo The page number in that table.
     */
    public HeapPageId(int tableid, int pageNo) {
        this.tableid = tableid;
        this.pageNo = pageNo;
    }

    /** @return the table associated with this PageId */
    public int getTableId() {
        return tableid;
    }

    /**
     * @return the page number in the table getTableId() associated with
     *   this PageId
     */
    public int pageno() {
        return pageNo;
    }

    @Override
    public String toString() {
        return "(<HeapPageId> tableid: " + tableid + " pageno: " + pageNo + ")";
    }

    /**
     * @return a hash code for this page, represented by the concatenation of
     *   the table number and the page number (needed if a PageId is used as a
     *   key in a hash table in the BufferPool, for example.)
     * @see BufferPool
     */
    @Override
    public int hashCode() {
        int hash = Utility.hashFunc(0, tableid);
        hash = Utility.hashFunc(hash, pageNo);
        hash = Utility.hashFunc(hash, ((tableid << 16) | pageNo));
        return hash;
    }

    /**
     * Compares one PageId to another.
     *
     * @param o The object to compare against (must be a PageId)
     * @return true if the objects are equal (e.g., page numbers and table
     *   ids are the same)
     */
    @Override
    public boolean equals(Object o) {
        if (!(o instanceof HeapPageId) || o == null) {
            return false;
        }
        HeapPageId other = (HeapPageId)o;
        return (tableid == other.tableid && pageNo == other.pageNo);
    }

    /**
     *  Return a representation of this object as an array of
     *  integers, for writing to disk.  Size of returned array must contain
     *  number of integers that corresponds to number of args to one of the
     *  constructors.
     */
    public int[] serialize() {
        int data[] = new int[2];

        data[0] = getTableId();
        data[1] = pageno();

        return data;
    }

}
