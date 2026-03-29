#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <utility>
#include <vector>

// неориентированный граф
class PlainGraph {
public:
    using Vertex = uint64_t;

    struct Edge {
        using Vertex = uint64_t;
        Vertex src;
        Vertex dst;
    };

private:
    size_t                                   n_edges_ = 0; // число ребер
    mutable std::vector<std::vector<Vertex>> adj_; // списки смежности
    mutable bool                             sorted_ = true; // флаг отсортирован ли списки смежности


private:
    mutable bool                  comp_ready_ = false; // посчитаны ли компоненты связности уже
    mutable size_t                comp_cnt_ = 0; //cколько компонент связности
    mutable std::vector<uint64_t> comp_id_;  // номер компоненты вершины V

    mutable bool                  bc_ready_ = false; // флаг посчитаны ли мосты и точки сочленения
    mutable std::vector<Edge>     bridges_; //мосты
    mutable std::vector<Vertex>   arts_;    //точки сочленения

private:
    void invalidate() { // функция для сбрасывания флагов
        sorted_ = false;
        comp_ready_ = false;
        bc_ready_ = false;
    }

    void ensure_sorted() const { // сортировка списков смежности
        if (sorted_) return;
        for (auto& v : adj_) std::sort(v.begin(), v.end());
        sorted_ = true;
    }

    void build_components() const { // функция считает компоненты связности
        if (comp_ready_) return;
        const Vertex n = static_cast<Vertex>(adj_.size());
        comp_id_.assign(n, UINT64_MAX);
        uint64_t color = 0;
        std::vector<Vertex> st;
        st.reserve(n);

        for (Vertex s = 0; s < n; ++s) {
            if (comp_id_[s] != UINT64_MAX) continue;
            comp_id_[s] = color;
            st.clear();
            st.push_back(s);
            while (!st.empty()) { //dfs
                Vertex v = st.back();
                st.pop_back();
                for (Vertex u : adj_[v]) {
                    if (comp_id_[u] == UINT64_MAX) {
                        comp_id_[u] = color;
                        st.push_back(u);
                    }
                }
            }
            ++color;
        }
        comp_cnt_ = static_cast<size_t>(color);
        comp_ready_ = true;
    }

    void build_bridges_and_arts() const { // поиск вершин и точек сочленения
        if (bc_ready_) return;

        const Vertex n = static_cast<Vertex>(adj_.size());
        bridges_.clear();
        arts_.clear();
        if (n == 0) { bc_ready_ = true; return; }

        std::vector<Vertex>   tin(n, 0);
        std::vector<Vertex>   low(n, 0);
        std::vector<Vertex>   parent(n, UINT64_MAX);
        std::vector<uint32_t> children(n, 0);
        std::vector<uint8_t>  is_cut(n, 0);

        struct Frame { Vertex v; size_t it; bool skipped_parent; };
        std::vector<Frame> st;
        st.reserve(n);

        size_t timer = 0;

        for (Vertex root = 0; root < n; ++root) {
            if (tin[root] != 0) continue;

            parent[root] = UINT64_MAX;
            st.clear();
            st.push_back(Frame{root, 0, false});
            tin[root] = low[root] = static_cast<Vertex>(++timer);

            while (!st.empty()) {
                Frame& fr = st.back();
                Vertex v = fr.v;

                if (fr.it == adj_[v].size()) {
                    st.pop_back();
                    Vertex p = parent[v];
                    if (p != UINT64_MAX) {
                        if (low[v] > tin[p]) bridges_.push_back(Edge{p, v});
                        if (parent[p] != UINT64_MAX && low[v] >= tin[p]) is_cut[p] = 1;
                        low[p] = std::min(low[p], low[v]);
                    }
                    continue;
                }

                Vertex u = adj_[v][fr.it++];
                Vertex p = parent[v];

                if (u == p && !fr.skipped_parent) {
                    fr.skipped_parent = true;
                    continue;
                }

                if (tin[u] != 0) {
                    low[v] = std::min(low[v], tin[u]);
                    continue;
                }

                parent[u] = v;
                ++children[v];
                tin[u] = low[u] = static_cast<Vertex>(++timer);
                st.push_back(Frame{u, 0, false});
            }

            if (children[root] >= 2) is_cut[root] = 1;
        }

        for (auto& e : bridges_) if (e.src > e.dst) std::swap(e.src, e.dst);
        std::sort(bridges_.begin(), bridges_.end(),
                  [](const Edge& a, const Edge& b) {
                      return (a.src < b.src) || (a.src == b.src && a.dst < b.dst);
                  });

        for (Vertex v = 0; v < n; ++v) if (is_cut[v]) arts_.push_back(v);
        std::sort(arts_.begin(), arts_.end());

        bc_ready_ = true;
    }

public:
    PlainGraph() = default;
    friend bool operator==(const PlainGraph& x, const PlainGraph& y) {
        if (x.n_edges_ != y.n_edges_) return false;
        if (x.adj_.size() != y.adj_.size()) return false;
        x.ensure_sorted();
        y.ensure_sorted();
        return x.adj_ == y.adj_;
    }

    Vertex addVertex() { // добавляет вершины и возвращает ее создает новый пустой список смежности
        adj_.emplace_back();
        invalidate();
        return static_cast<Vertex>(adj_.size() - 1);
    }

    bool addEdge(Vertex v1, Vertex v2) { // добавления ребра
        const Vertex n = static_cast<Vertex>(adj_.size());
        if (v1 >= n || v2 >= n) return false;

        invalidate();

        if (v1 == v2) { // петли разрешены
            adj_[v1].push_back(v1);
            adj_[v1].push_back(v1);
            ++n_edges_;
            return true;
        }

        adj_[v1].push_back(v2);
        adj_[v2].push_back(v1);
        ++n_edges_;
        return true;
    }

    size_t nVertices() const { return adj_.size(); }
    size_t nEdges()    const { return n_edges_; }

    bool has(Edge e) const { // проверка существования ребра
        const Vertex n = static_cast<Vertex>(adj_.size());
        if (e.src >= n || e.dst >= n) return false;
        ensure_sorted();
        const auto& a = adj_[e.src];
        if (std::binary_search(a.begin(), a.end(), e.dst)) return true;
        if (e.src == e.dst) return true;
        const auto& b = adj_[e.dst];
        return std::binary_search(b.begin(), b.end(), e.src);
    }

    const std::vector<Vertex>& getAdjuscent(Vertex v) const {  // соседи вершины V
        static const std::vector<Vertex> empty;
        if (v >= adj_.size()) return empty;
        ensure_sorted();
        return adj_[v];
    }
    void validate() const { ensure_sorted(); }

    void dump(const char* path = nullptr) const { //gпечатает граф
        ensure_sorted();
        std::ostream* out = &std::cout;
        std::ofstream file;
        if (path) { file.open(path); out = &file; }
        *out << "PlainGraph V=" << nVertices() << " E=" << nEdges() << "\n";
        for (Vertex u = 0; u < static_cast<Vertex>(adj_.size()); ++u) {
            *out << u << ": ";
            for (Vertex v : adj_[u]) *out << v << " ";
            *out << "\n";
        }
    }

    // Проверка на дерево или лес
    bool isTree() const {
        const size_t V = adj_.size();
        if (V == 0) return true;
        if (nJointComponents() != 1) return false;
        return n_edges_ == V - 1;
    }

    bool isForest() const {
        const size_t V = adj_.size();
        const size_t C = nJointComponents();
        return n_edges_ == V - C;
    }

    // Компоненты связности
    size_t nJointComponents() const {
        build_components();
        return comp_cnt_;
    }

    std::vector<uint64_t> getJointComponents() const {
        build_components();
        return comp_id_;
    }


    // Мосты и точки сочленения
    std::vector<Edge> getBridges() const {
        build_bridges_and_arts();
        return bridges_;
    }

    std::vector<Vertex> getArticulationPoints() const {
        build_bridges_and_arts();
        return arts_;
    }
};


// Ориентированный граф
class DirectionalGraph {
public:
    using Vertex = uint64_t;

    struct Edge {
        using Vertex = uint64_t;
        Vertex src;
        Vertex dst;
    };

private:
    size_t                                   n_edges_ = 0;
    mutable std::vector<std::vector<Vertex>> adj_; // списки смежности
    mutable bool                             sorted_ = true; // флаг сортировки списков смежности

private:

    void invalidate() { sorted_ = false; } // сброс флага

    void ensure_sorted() const { // сортировка списков смежности
        if (sorted_) return;
        for (auto& v : adj_) std::sort(v.begin(), v.end());
        sorted_ = true;
    }

private:
    // проверка на DAG и построение топологической сортировки
    bool kahn(std::vector<Vertex>* order_out) const {
        const Vertex           n = static_cast<Vertex>(adj_.size());
        std::vector<uint64_t>  indeg(n, 0);
        for (Vertex v = 0; v < n; ++v)
            for (Vertex u : adj_[v]) ++indeg[u];

        std::vector<Vertex> q;
        q.reserve(n);

        for (Vertex v = 0; v < n; ++v) if (indeg[v] == 0) q.push_back(v);
        std::vector<Vertex> order;
        order.reserve(n);
        size_t head = 0;

        while (head < q.size()) {
            Vertex v = q[head++];
            order.push_back(v);
            for (Vertex u : adj_[v]) {
                auto& d = indeg[u];
                if (--d == 0) q.push_back(u);
            }
        }

        const bool ok = (order.size() == n);
        if (order_out) {
            if (ok) *order_out = std::move(order);
            else order_out->clear();
        }
        return ok;
    }


public:
    DirectionalGraph() = default;
    explicit DirectionalGraph(Vertex n) : n_edges_(0), adj_(n) {}

    friend bool operator==(const DirectionalGraph& x, const DirectionalGraph& y) {
        if (x.n_edges_ != y.n_edges_) return false;
        if (x.adj_.size() != y.adj_.size()) return false;
        x.ensure_sorted();
        y.ensure_sorted();
        return x.adj_ == y.adj_;
    }

    Vertex addVertex() { // добавляет вершины и возвращает ее создает новый пустой список смежности
        adj_.emplace_back();
        invalidate();
        return static_cast<Vertex>(adj_.size() - 1);
    }

    bool addEdge(Vertex v1, Vertex v2) { // добавления ребра
        const Vertex n = static_cast<Vertex>(adj_.size());
        if (v1 >= n || v2 >= n) return false;
        invalidate();
        adj_[v1].push_back(v2);
        ++n_edges_;
        return true;
    }

    size_t nVertices() const { return adj_.size(); }
    size_t nEdges()    const { return n_edges_; }

    bool has(Edge e) const { // проверка ребра
        const Vertex n = static_cast<Vertex>(adj_.size());
        if (e.src >= n || e.dst >= n) return false;
        ensure_sorted();
        const auto& a = adj_[e.src];
        return std::binary_search(a.begin(), a.end(), e.dst);
    }

    const std::vector<Vertex>& getAdjuscent(Vertex v) const {
        static const std::vector<Vertex> empty;
        if (v >= adj_.size()) return empty;
        ensure_sorted();
        return adj_[v];
    }

    void validate() const { ensure_sorted(); }

    void dump(const char* path = nullptr) const { //печать
        ensure_sorted();
        std::ostream* out = &std::cout;
        std::ofstream file;
        if (path) { file.open(path); out = &file; }
        *out << "DirectionalGraph V=" << nVertices() << " E=" << nEdges() << "\n";
        for (Vertex u = 0; u < static_cast<Vertex>(adj_.size()); ++u) {
            *out << u << ": ";
            for (Vertex v : adj_[u]) *out << v << " ";
            *out << "\n";
        }
    }

    bool isDAG() const { return kahn(nullptr); } // проверка на DAG

    std::vector<Vertex> getSources() const { //Истоки(вершины, у которых входная степень 0)
        const Vertex n = static_cast<Vertex>(adj_.size());
        std::vector<uint64_t> indeg(n, 0);
        for (Vertex v = 0; v < n; ++v)
            for (Vertex u : adj_[v]) ++indeg[u];

        std::vector<Vertex> ans;
        ans.reserve(n);
        for (Vertex v = 0; v < n; ++v) if (indeg[v] == 0) ans.push_back(v);
        std::sort(ans.begin(), ans.end());
        return ans;
    }

    std::vector<Vertex> getSinks() const { //Стоки(вершины, у которых нет исходящих ребер)
        const Vertex n = static_cast<Vertex>(adj_.size());
        std::vector<Vertex> ans;
        ans.reserve(n);
        for (Vertex v = 0; v < n; ++v) if (adj_[v].empty()) ans.push_back(v);
        std::sort(ans.begin(), ans.end());
        return ans;
    }

    DirectionalGraph reverse() const { // построение обратного(реверснутого) графа
        const Vertex n = static_cast<Vertex>(adj_.size());
        std::vector<uint32_t> indeg(n, 0);
        for (Vertex v = 0; v < n; ++v)
            for (Vertex u : adj_[v]) ++indeg[u];

        DirectionalGraph r(n);
        r.n_edges_ = n_edges_;
        r.sorted_ = false;
        r.adj_.assign(n, {});
        for (Vertex v = 0; v < n; ++v) r.adj_[v].reserve(indeg[v]);

        for (Vertex v = 0; v < n; ++v)
            for (Vertex u : adj_[v]) r.adj_[u].push_back(v);

        return r;
    }

    std::vector<Vertex> topological() const { // топологическая сортировка
        std::vector<Vertex> order;
        kahn(&order);
        return order;
    }

    // конденсация или же поиск сильных компонент связности используем алгоритм Косараджу
    std::pair<DirectionalGraph, std::vector<Vertex>> condense() const {
        const Vertex n = static_cast<Vertex>(adj_.size());

        std::vector<uint32_t> indeg(n, 0);
        for (Vertex v = 0; v < n; ++v)
            for (Vertex u : adj_[v]) ++indeg[u];

        std::vector<std::vector<Vertex>> rev_adj(n);
        for (Vertex v = 0; v < n; ++v) rev_adj[v].reserve(indeg[v]);
        for (Vertex v = 0; v < n; ++v)
            for (Vertex u : adj_[v]) rev_adj[u].push_back(v);

        std::vector<uint8_t> vis(n, 0);
        std::vector<Vertex> order;
        order.reserve(n);

        struct Frame { Vertex v; size_t it; };
        std::vector<Frame> st;

        for (Vertex s = 0; s < n; ++s) {
            if (vis[s]) continue;
            vis[s] = 1;
            st.clear();
            st.push_back(Frame{s, 0});
            while (!st.empty()) {
                Frame& fr = st.back();
                Vertex v = fr.v;
                if (fr.it == adj_[v].size()) {
                    order.push_back(v);
                    st.pop_back();
                    continue;
                }
                Vertex u = adj_[v][fr.it++];
                if (!vis[u]) {
                    vis[u] = 1;
                    st.push_back(Frame{u, 0});
                }
            }
        }

        std::vector<Vertex> comp(n, UINT64_MAX);
        Vertex color = 0;
        std::vector<Vertex> stack_nodes;

        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            Vertex s = *it;
            if (comp[s] != UINT64_MAX) continue;
            comp[s] = color;
            stack_nodes.clear();
            stack_nodes.push_back(s);
            while (!stack_nodes.empty()) {
                Vertex v = stack_nodes.back();
                stack_nodes.pop_back();
                for (Vertex u : rev_adj[v]) {
                    if (comp[u] == UINT64_MAX) {
                        comp[u] = color;
                        stack_nodes.push_back(u);
                    }
                }
            }
            ++color;
        }

        std::vector<uint32_t> cnt(color, 0);
        for (Vertex v = 0; v < n; ++v) ++cnt[comp[v]];
        std::vector<std::vector<Vertex>> verts(color);
        for (Vertex c = 0; c < color; ++c) verts[c].reserve(cnt[c]);
        for (Vertex v = 0; v < n; ++v) verts[comp[v]].push_back(v);

        std::vector<uint32_t> outdeg(color, 0);
        {
            std::vector<uint32_t> seen(color, 0);
            uint32_t stamp = 1;
            for (Vertex a = 0; a < color; ++a) {
                ++stamp;
                for (Vertex v : verts[a]) {
                    for (Vertex u : adj_[v]) {
                        Vertex b = comp[u];
                        if (a == b) continue;
                        if (seen[b] == stamp) continue;
                        seen[b] = stamp;
                        ++outdeg[a];
                    }
                }
            }
        }
        DirectionalGraph condensed(color);
        condensed.sorted_ = false;
        condensed.adj_.assign(color, {});
        for (Vertex a = 0; a < color; ++a) condensed.adj_[a].reserve(outdeg[a]);

        std::vector<uint32_t> seen(color, 0);
        uint32_t stamp = 1;

        for (Vertex a = 0; a < color; ++a) {
            ++stamp;
            for (Vertex v : verts[a]) {
                for (Vertex u : adj_[v]) {
                    Vertex b = comp[u];
                    if (a == b) continue;
                    if (seen[b] == stamp) continue;
                    seen[b] = stamp;
                    condensed.adj_[a].push_back(b);
                    ++condensed.n_edges_;
                }
            }
        }

        return {condensed, comp};
    }
};
