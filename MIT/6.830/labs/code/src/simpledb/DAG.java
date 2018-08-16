package simpledb;

import java.util.HashMap;
import java.util.Map;

/**
 */
public class DAG<T> {
    private final Map<T, Map<T, Integer>> adjacency;

    public DAG() {
        this.adjacency = new HashMap<>();
    }

    public void add(T u, T v) {
        Map<T, Integer> tos = adjacency.computeIfAbsent(u, from -> new HashMap<>());
        Integer count = tos.computeIfAbsent(v, to -> 0);
        tos.put(v, count + 1);
    }

    public boolean remove(T u, T v) {
        Map<T, Integer> tos = adjacency.get(u);
        if (tos == null) {
            return false;
        }
        Integer count = tos.get(v);
        if (count == null) {
            return false;
        }
        assert(count > 0);
        count -= 1;
        if (count == 0) {
            tos.remove(v);
            if (tos.isEmpty()) {
                adjacency.remove(u);
            }
        } else {
            tos.put(v, count);
        }
        return true;
    }

    public boolean cyclic(T u, T v) {
        return false;
    }
}
