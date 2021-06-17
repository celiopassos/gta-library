// To do:
//
// * Range query
// * Order of key on ranges
// * Compatibility with lazy context
template<typename T, int K = 400>
struct SQRT
{
    const int n, blocks;
 
    vector<T> arr, offset;
    vector<vector<int>> bucket;
 
    void sort_bucket(int b) {
        sort(begin(bucket[b]), end(bucket[b]), [&](int i, int j){ return arr[i] < arr[j]; });
    }
 
    SQRT(const vector<T>& arr) : n((int)size(arr)), blocks((n + K - 1) / K), arr(arr), offset(blocks, 0), bucket(blocks) {
        for (int b = 0; b < blocks; ++b) {
            int len = min(K, n - b * K);
            bucket[b].resize(len), iota(begin(bucket[b]), end(bucket[b]), b * K);
            sort_bucket(b);
        }
    }
    // O(n/K + K)
    void update(int l, int r, T add) {
        int s = l / K, e = r / K;
        for (int b = s + 1; b <= e - 1; ++b) offset[b] += add;
        for (int i = l; i <= min(r, s * K + K - 1); ++i) arr[i] += add;
        auto cmp = [&](int i, int j){ return arr[i] < arr[j]; };
        auto pred = [&](int i) { return l <= i && i <= r; };
        auto mids = stable_partition(begin(bucket[s]), end(bucket[s]), pred);
        inplace_merge(begin(bucket[s]), mids, end(bucket[s]), cmp);
        if (s != e) {
            for (int i = e * K; i <= r; ++i) arr[i] += add;
            auto mide = stable_partition(begin(bucket[e]), end(bucket[e]), pred);
            inplace_merge(begin(bucket[e]), mide, end(bucket[e]), cmp);
        }
    }
    // O((n/K)log(K))
    int order_of_key(T key) {
        int res = 0;
        for (int b = 0; b < blocks; ++b) {
            auto iter = lower_bound(begin(bucket[b]), end(bucket[b]), key - offset[b], [&](int i, T x){ return arr[i] < x; });
            res += (int)distance(begin(bucket[b]), iter);
        }
        return res;
    }
};
