package simpledb;
import java.util.*;

/**
 * TupleDesc describes the schema of a tuple.
 */
public class TupleDesc {
    private final List<Type> types;
    private final List<String> fields;
    private int sizeInBytes;

    public static TupleDesc fromTypes(Type... types) {
        return new TupleDesc(types, null);
    }

    /**
     * Merge two TupleDescs into one, with td1.numFields + td2.numFields
     * fields, with the first td1.numFields coming from td1 and the remaining
     * from td2.
     * @param td1 The TupleDesc with the first fields of the new TupleDesc
     * @param td2 The TupleDesc with the last fields of the TupleDesc
     * @return the new TupleDesc
     */
    public static TupleDesc combine(TupleDesc td1, TupleDesc td2) {
        int newSize = td1.types.size() + td2.types.size();
        Type newTypeAr[] = new Type[newSize];
        for (int i = 0; i < td1.types.size(); ++i) {
            newTypeAr[i] = td1.types.get(i);
        }
        for (int i = 0; i < td2.types.size(); ++i) {
            newTypeAr[i + td1.types.size()] = td2.types.get(i);
        }

        String newFieldAr[] = null;
        if (td1.fields != null && td2.fields != null) {
            newFieldAr = new String[newSize];
            for (int i = 0; i < td1.fields.size(); ++i) {
                newFieldAr[i] = td1.fields.get(i);
            }
            for (int i = 0; i < td2.fields.size(); ++i) {
                newFieldAr[i + td1.fields.size()] = td2.fields.get(i);
            }
        }

        return new TupleDesc(newTypeAr, newFieldAr);
    }

    /**
     * Create a new TupleDesc with typeAr.length fields with fields of the
     * specified types, with associated named fields.
     *
     * @param typeAr array specifying the number of and types of fields in
     *        this TupleDesc. It must contain at least one entry.
     * @param fieldAr array specifying the names of the fields. Note that names may be null.
     */
    public TupleDesc(Type[] typeAr, String[] fieldAr) {
        // some code goes here
        assert typeAr != null && typeAr.length > 0;
        assert fieldAr == null || fieldAr.length == typeAr.length;
        // https://stackoverflow.com/questions/16748030/difference-between-arrays-aslistarray-vs-new-arraylistintegerarrays-aslist
        types = new ArrayList<Type>(Arrays.asList(typeAr));
        fields = (fieldAr == null ? null : new ArrayList<String>(Arrays.asList(fieldAr)));

        sizeInBytes = 0;
        for (Type t : types) {
            sizeInBytes += t.getLen();
        }
    }

    /**
     * Constructor.
     * Create a new tuple desc with typeAr.length fields with fields of the
     * specified types, with anonymous (unnamed) fields.
     *
     * @param typeAr array specifying the number of and types of fields in
     *        this TupleDesc. It must contain at least one entry.
     */
    public TupleDesc(Type[] typeAr) {
        this(typeAr, null);
    }

    /**
     * @return the number of types in this TupleDesc
     */
    private int numTypes() {
        return types.size();
    }

    /**
     * @return the number of fields in this TupleDesc
     */
    public int numFields() {
        // |fields| could be null, hence we still need to use size of |types|.
        return numTypes();
    }

    private void validateIndex(int i) throws NoSuchElementException {
        if (i < 0 || i >= numTypes()) {
            throw new NoSuchElementException("Invalid index: " + i);
        }
    }
    /**
     * Gets the (possibly null) field name of the ith field of this TupleDesc.
     *
     * @param i index of the field name to return. It must be a valid index.
     * @return the name of the ith field
     * @throws NoSuchElementException if i is not a valid field reference.
     */
    public String getFieldName(int i) throws NoSuchElementException {
        // some code goes here
        validateIndex(i);
        return (fields == null ? null : fields.get(i));
    }

    /**
     * Find the index of the field with a given name.
     *
     * @param name name of the field.
     * @return the index of the field that is first to have the given name.
     * @throws NoSuchElementException if no field with a matching name is found.
     */
    public int nameToId(String name) throws NoSuchElementException {
        if (fields != null) {
            for (int i = 0; i < numFields(); ++i) {
                if (fields.get(i).equals(name)) {
                    return i;
                }
            }
        }
        throw new NoSuchElementException("No field name");
    }

    /**
     * Gets the type of the ith field of this TupleDesc.
     *
     * @param i The index of the field to get the type of. It must be a valid index.
     * @return the type of the ith field
     * @throws NoSuchElementException if i is not a valid field reference.
     */
    public Type getType(int i) throws NoSuchElementException {
        validateIndex(i);
        return types.get(i);
    }

    /**
     * @return The size (in bytes) of tuples corresponding to this TupleDesc.
     * Note that tuples from a given TupleDesc are of a fixed size.
     */
    public int getSize() {
        return sizeInBytes;
    }

    /**
     * Compares the specified object with this TupleDesc for equality.
     * Two TupleDescs are considered equal if they are the same size and if the
     * n-th type in this TupleDesc is equal to the n-th type in td.
     *
     * @param o the Object to be compared for equality with this TupleDesc.
     * @return true if the object is equal to this TupleDesc.
     */
    public boolean equals(Object o) {
        if (!(o instanceof TupleDesc) || o == null) {
            return false;
        }

        TupleDesc other = (TupleDesc)o;
        if (numTypes() != other.numTypes()) {
            return false;
        }
        for (int i = 0; i < numTypes(); ++i) {
            if (!getType(i).equals(other.getType(i))) {
                return false;
            }
        }
        return true;
    }

    public int hashCode() {
        // If you want to use TupleDesc as keys for HashMap, implement this so
        // that equal objects have equals hashCode() results
        int hash = 0;
        for (Type t : types) {
            hash = Utility.hashFunc(hash, t.getLen());
        }
        return hash;
    }

    /**
     * Returns a String describing this descriptor. It should be of the form
     * "fieldType[0](fieldName[0]), ..., fieldType[M](fieldName[M])", although
     * the exact format does not matter.
     * @return String describing this descriptor.
     */
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (int i = 0; i < numTypes(); ++i) {
            switch (getType(i)) {
            case INT_TYPE:
                sb.append("INT_TYPE");
                break;
            case STRING_TYPE:
                sb.append("STRING_TYPE");
                break;
            default:
                sb.append("UNKONW_TYPE");
                break;
            }
            if (fields != null) {
                sb.append("(" + getFieldName(i) + ")");
            }
            if (i < numTypes() - 1) {
                sb.append(", ");
            }
        }
        return sb.toString();
    }
}
