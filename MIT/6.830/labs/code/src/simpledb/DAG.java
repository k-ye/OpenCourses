package simpledb;

import java.util.HashMap;
import java.util.Map;

/**
 */
public class DAG<T> {
    private final Map<T, Integer> nodesCount;
    private final Map<T, Map<T, Integer>> adjacency;

    private class DfsCycleDetector {
        private static final int WHITE = 1;
        private static final int GRAY = 2;
        private static final int BLACK = 3;
        private final Map<T, Integer> colors;

        public DfsCycleDetector() {
            this.colors = new HashMap<>();
        }

        public boolean cyclic(T u, T v) {
            add(u, v);
            for (T vertex : nodesCount.keySet()) {
                colors.put(vertex, WHITE);
            }
            boolean result = false;
            for (T vertex : nodesCount.keySet()) {
                if ((colors.get(vertex) == WHITE) && foundCycle(vertex)) {
                    result = true;
                    break;
                }
            }
            remove(u, v);
            return result;
        }

        private boolean foundCycle(T u) {
            assert(colors.get(u) == WHITE);
            colors.put(u, GRAY);
            Map<T, Integer> tos = adjacency.get(u);
            if (tos != null) {
                for (T v : adjacency.get(u).keySet()) {
                    assert(colors.containsKey(v));
                    int vColor = colors.get(v);
                    if (vColor == GRAY) {
                        // A back edge
                        return true;
                    }
                    if ((vColor == WHITE) && foundCycle(v)) {
                        return true;
                    }
                }
            }
            colors.put(u, BLACK);
            return false;
        }
    }

    public DAG() {
        this.nodesCount = new HashMap<>();
        this.adjacency = new HashMap<>();
    }

    private void incNodeCount(T u) {
        int nodeCount = nodesCount.computeIfAbsent(u, n -> 0);
        nodesCount.put(u, nodeCount + 1);
    }

    private void decNodeCount(T u) {
        if (!nodesCount.containsKey(u)) {
            return;
        }
        int nodeCount = nodesCount.get(u) - 1;
        if (nodeCount == 0) {
            nodesCount.remove(u);
        } else {
            nodesCount.put(u, nodeCount);
        }
    }

    public void add(T u, T v) {
        incNodeCount(u);
        incNodeCount(v);

        Map<T, Integer> tos = adjacency.computeIfAbsent(u, from -> new HashMap<>());
        Integer edgeCount = tos.computeIfAbsent(v, to -> 0);
        tos.put(v, edgeCount + 1);
    }

    public boolean remove(T u, T v) {
        decNodeCount(u);
        decNodeCount(v);

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
        DfsCycleDetector detector = new DfsCycleDetector();
        return detector.cyclic(u, v);
    }
}
