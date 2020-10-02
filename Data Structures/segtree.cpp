#include "bits/stdc++.h"

using namespace std;

#define _ ios_base::sync_with_stdio(0);cin.tie(0);
#define endl '\n'
#define debug(x) cerr << #x << " == " << (x) << '\n';
#define all(X) begin(X), end(X)
#define sz(X) (int)X.size()

using ll = long long;

const int INF = 0x3f3f3f3f;
const ll LINF = 0x3f3f3f3f3f3f3f3fLL;

template<typename T>
struct SegmentTree
{
private:
    const int n; const T Tid;
    vector<T> st;
    T combine(T l, T r) { return l + r; }
    void build()
    {
        for (int i = n - 1; i > 0; --i)
            st[i] = combine(st[i << 1], st[i << 1 | 1]);
    }
public:
    SegmentTree(const vector<T>& a, T Tid) : n(sz(a)), Tid(Tid)
    {
        st.assign(2 * n, Tid);
        for (int i = 0; i < n; ++i) st[n + i] = a[i];
        build();
    }
    void modify(int p, T value)
    {
        for (st[p += n] = value; p > 1; p >>= 1)
            st[p >> 1] = combine(st[p & ~1], st[p | 1]);
    }
    T query(int l, int r) // inclusive
    {
        T resl = Tid, resr = Tid;
        for (l += n, r += n + 1; l < r; l >>= 1, r >>= 1)
        {
            if (l & 1) resl = combine(resl, st[l++]);
            if (r & 1) resr = combine(st[--r], resr);
        }
        return combine(resl, resr);
    }
};

int main()
{ _
    exit(0);
}

