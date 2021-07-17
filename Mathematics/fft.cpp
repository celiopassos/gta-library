template<typename T>
complex<T> getroot(int N) {
    static const double PI = acos(-1);
    return polar<T>(1, 2 * PI / N);
}
//Mint<998244353> getroot(int N) {
//     return Mint<998244353>(3).power(7 * 17 * (1LL << 23) / N);
//}
// inplace fft (resizes if necessary)
template<typename T>
void fft(vector<T>& p, bool inverse) {
    int N = 1;
    while ((int)size(p) > N) N *= 2;
    p.resize(N);
    vector<T> q(N);
    for (int i = 0; i < N; ++i) {
        int rev = 0;
        for (int b = 1; b < N; b <<= 1) {
            rev = (rev << 1) | !!(i & b);
        }
        q[rev] = p[i];
    }
    swap(p, q);
    for (int b = 1; b < N; b <<= 1, swap(p, q)) {
        T rt = inverse ? getroot(b << 1) : 1 / getroot(b << 1);
        for (auto [i, x] = pair(0, T(1)); i < N; ++i, x *= rt) {
            q[i] = p[i & ~b] + x * p[i | b];
        }
    }
    if (inverse) {
        T inv = 1 / T(N);
        for (int i = 0; i < N; ++i) p[i] *= inv;
    }
}
template<typename T>
vector<T> operator*(vector<T> p, vector<T> q) {
    size_t N = size(p) + size(q) - 1;
    p.resize(N), q.resize(N);
    fft(p, false), fft(q, false);
    for (size_t i = 0; i < size(p); ++i) p[i] *= q[i];
    fft(p, true);
    p.resize(N);
    return p;
}
