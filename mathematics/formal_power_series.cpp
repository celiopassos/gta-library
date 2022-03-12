template <typename T>
T root_of_unity(int N);

template <>
std::complex<double> root_of_unity<std::complex<double>>(int N) {
  static constexpr double PI = std::acos(-1);
  return std::polar<double>(1, 2 * PI / N);
}

constexpr int ntt_mod = 998244353;
template <>
Z<ntt_mod> root_of_unity(int N) {
  return Z<ntt_mod>(3).power((ntt_mod - 1) / N);
}

template <typename T>
struct fft_t {
  int N;
  std::vector<int> rev;
  std::vector<T> rs;
  fft_t(int N) : N(N), rev(N) {
    for (int i = 0; i < N; ++i) {
      int r = 0;
      for (int b = 1, j = i; b < N; b <<= 1, j >>= 1) {
        r = (r << 1) | j & 1;
      }
      rev[i] = r;
    }
    for (auto sgn : {-1, +1}) {
      for (int b = 1; b < N; b <<= 1) {
        T w = root_of_unity<T>(sgn * 2 * b);
        rs.push_back(1);
        for (int i = 0; i + 1 < b; ++i) {
          rs.push_back(rs.back() * w);
        }
      }
    }
  }
  std::vector<T> operator()(std::vector<T> p, bool inverse) {
    p.resize(N);
    for (int i = 0; i < N; ++i) {
      if (i < rev[i]) {
        std::swap(p[i], p[rev[i]]);
      }
    }
    T* r = rs.data();
    if (inverse) {
      r += rs.size() / 2;
    }
    for (int b = 1; b < N; b <<= 1) {
      for (int s = 0; s < N; s += 2 * b) {
        for (int i = 0; i < b; ++i) {
          int u = s | i, v = u | b;
          T x = p[u], y = r[i] * p[v];
          p[u] = x + y;
          p[v] = x - y;
        }
      }
      r += b;
    }
    if (inverse) {
      T inv = T(1) / T(N);
      for (int i = 0; i < N; ++i) p[i] *= inv;
    }
    return p;
  }
};

template <typename T>
struct FormalPowerSeries : public std::vector<T> {
  using F = FormalPowerSeries;
  using std::vector<T>::vector;
  FormalPowerSeries() : std::vector<T>(1) {}

  F operator+(const F& rhs) const {
    return F(*this) += rhs;
  }
  F& operator+=(const F& rhs) {
    if (this->size() < rhs.size()) {
      this->resize(rhs.size());
    }
    for (int i = 0; i < rhs.size(); ++i) {
      (*this)[i] += rhs[i];
    }
    return *this;
  }
  F operator-(const F& rhs) const {
    return F(*this) -= rhs;
  }
  F& operator-=(const F& rhs) {
    if (this->size() < rhs.size()) {
      this->resize(rhs.size());
    }
    for (int i = 0; i < rhs.size(); ++i) {
      (*this)[i] -= rhs[i];
    }
    return *this;
  }
  F& operator*=(T alpha) {
    for (auto& x : *this) {
      x *= alpha;
    }
    return *this;
  }
  F operator*(T alpha) {
    return F(*this) *= alpha;
  }
  friend F operator*(T alpha, F rhs) {
    return rhs *= alpha;
  }
  F operator-() const {
    return F(*this) *= -1;
  }

  F naive_multiply(const F& rhs) const {
    int N = this->size(), M = rhs.size();
    F r(N + M - 1);
    for (int i = 0; i < N; ++i) {
      for (int j = 0; j < M; ++j) {
        r[i + j] += (*this)[i] * rhs[j];
      }
    }
    return r;
  }
  F operator*(const F& rhs) {
    return F(*this) *= rhs;
  }
  F& operator*=(const F& rhs) {
    if (std::min(this->size(), rhs.size()) <= 64) {
      auto r = naive_multiply(rhs);
      this->swap(r);
    } else {
      int N = this->size() + rhs.size() - 1, M = 1;
      while (M < N) M <<= 1;
      fft_t<T> fft(M);
      auto phat = fft(std::move(*this), false), qhat = fft(rhs, false);
      for (int i = 0; i < M; ++i) {
        phat[i] *= qhat[i];
      }
      auto r = fft(phat, true);
      this->swap(r);
      this->resize(N);
    }
    return *this;
  }

  F operator/(const F& rhs) const {
    return F(*this) /= rhs;
  }
  F& operator/=(F rhs) {
    int N = this->size(), M = rhs.size();
    if (N < M) {
      *this = F();
    } else {
      int K = N - M + 1;
      std::reverse(rhs.begin(), rhs.end());
      rhs.resize(K);
      auto res = F(this->rbegin(), this->rbegin() + K) * inv(rhs);
      res.resize(K);
      std::reverse(res.begin(), res.end());
      this->swap(res);
    }
    return *this;
  }
  F operator%(const F& rhs) const {
    return divided_by(rhs).second;
  }
  F operator%=(const F& rhs) {
    return *this = divided_by(rhs)->second;
  }
  std::pair<F, F> divided_by(F rhs) const {
    auto q = *this / rhs;
    if (rhs.size() > 1) {
      rhs.pop_back();
    }
    auto q0 = F(q.begin(), q.begin() + std::min(q.size(), rhs.size()));
    auto r = *this - rhs * q0;
    r.resize(rhs.size());
    return {q, r};
  }

  T operator()(T x) const {
    T pow = 1, y = 0;
    for (auto& c : *this) {
      y += c * pow;
      pow *= x;
    }
    return y;
  }
};

template <typename T>
FormalPowerSeries<T> inv(const FormalPowerSeries<T>& P) {
  using F = FormalPowerSeries<T>;
  assert(P[0] != 0);
  F Q = {1 / P[0]};
  int N = P.size(), K = 1;
  while (K < N) {
    K *= 2;
    fft_t<T> fft(2 * K);
    auto Qhat = fft(Q, false);
    auto Phat = fft(F(P.begin(), P.begin() + std::min(K, N)), false);
    for (int i = 0; i < 2 * K; ++i) {
      Qhat[i] *= 2 - Phat[i] * Qhat[i];
    }
    auto nQ = fft(Qhat, true);
    Q.swap(nQ);
    Q.resize(K);
  }
  Q.resize(N);
  return Q;
}

template <typename T>
FormalPowerSeries<T> D(FormalPowerSeries<T> P) {
  if (P.size() == 1) {
    P[0] = 0;
  } else {
    for (int i = 0; i + 1 < P.size(); ++i) {
      P[i] = (i + 1) * P[i + 1];
    }
    P.pop_back();
  }
  return P;
}

template <typename T>
FormalPowerSeries<T> I(FormalPowerSeries<T> P) {
  int N = P.size();
  P.push_back(0);
  for (int i = N - 1; i >= 0; --i) {
    P[i + 1] = P[i] / (i + 1);
  }
  P[0] = 0;
  return P;
}

template <typename T>
FormalPowerSeries<T> log(const FormalPowerSeries<T>& P) {
  assert(P[0] == 1);
  int N = P.size();
  auto r = D(P) * inv(P);
  r.resize(N - 1);
  return I(std::move(r));
}

template <typename T>
FormalPowerSeries<T> exp(const FormalPowerSeries<T>& P) {
  assert(P[0] == 0);
  FormalPowerSeries<T> Q = {1};
  int N = P.size(), K = 1;
  while (K < N) {
    K *= 2;
    Q.resize(K);
    auto B = -log(Q);
    B[0] += 1;
    for (int i = 0; i < std::min(N, K); ++i) {
      B[i] += P[i];
    }
    Q *= B;
    Q.resize(K);
  }
  return Q;
}

template <typename T>
FormalPowerSeries<T> pow(const FormalPowerSeries<T>& P, int64_t k) {
  int N = P.size();
  int t = 0;
  while (t < N && P[t] == 0) ++t;
  if (t == N || (t > 0 && k >= (N + t - 1) / t)) {
    return FormalPowerSeries<T>(N, 0);
  }
  int max = N - k * t;
  FormalPowerSeries<T> Q(P.begin() + t, P.begin() + t + max);
  T alpha = Q[0];
  for (int i = 0; i < Q.size(); ++i) {
    Q[i] *= alpha.power(-1);
  }
  Q = log(Q);
  for (auto& x : Q) {
    x *= k;
  }
  Q = exp(Q);
  for (auto& x : Q) {
    x *= alpha.power(k);
  }
  Q.insert(Q.begin(), k * t, 0);
  return Q;
}

template <typename T>
struct Interpolator {
  using F = FormalPowerSeries<T>;
  struct Node {
    F P;
    T y;
    Node* left = nullptr;
    Node* right = nullptr;
  };
  std::deque<Node> deq;
  template <typename Iterator>
  Interpolator(Iterator first, Iterator last) {
    Node* root = &deq.emplace_back();
    build(root, first, last);
    auto y = evaluate(D(root->P));
    auto iter = y.begin();
    for (auto& node : deq) {
      if (node.left) continue;
      node.y = *iter;
      ++iter;
    }
  }
  template <typename Iterator>
  void build(Node* node, Iterator first, Iterator last) {
    int len = last - first;
    if (len == 1) {
      node->P = {-first[0], 1};
    } else {
      node->left = &deq.emplace_back();
      node->right = &deq.emplace_back();
      Iterator middle = first + len / 2;
      build(node->left, first, middle);
      build(node->right, middle, last);
      node->P = node->left->P * node->right->P;
    }
  }
  std::vector<T> res;
  std::vector<T> evaluate(const F& Q) {
    res.clear();
    evaluate(&deq[0], Q % deq[0].P);
    return std::move(res);
  }
  void evaluate(Node* node, F Q) {
    if (node->left) {
      for (auto next : {node->left, node->right}) {
        evaluate(next, Q % next->P);
      }
    } else {
      assert(Q.size() == 1);
      res.push_back(Q[0]);
    }
  }
  template <typename Iterator>
  F interpolate(Iterator first, Iterator last) {
    return interpolate(&deq[0], first, last);
  }
  template <typename Iterator>
  F interpolate(Node* node, Iterator first, Iterator last) {
    int len = last - first;
    if (len == 1) {
      return {first[0] / node->y};
    } else {
      Iterator middle = first + len / 2;
      return node->right->P * interpolate(node->left, first, middle) +
        node->left->P * interpolate(node->right, middle, last);
    }
  }
};

// computes P(D)A
template <typename T>
FormalPowerSeries<T> apply_polynomial_of_derivative(FormalPowerSeries<T> P,
    FormalPowerSeries<T> A) {
  int N = A.size();
  if (P.size() > N) {
    P.resize(N);
  }
  std::vector<T> f(N);
  f[0] = 1;
  for (int k = 0; k + 1 < N; ++k) {
    f[k + 1] = (k + 1) * f[k];
  }
  for (int k = 0; k < N; ++k) {
    A[k] *= f[k];
  }
  std::reverse(P.begin(), P.end());
  auto res = P * A;
  res.erase(res.begin(), res.begin() + P.size() - 1);
  for (int k = 0; k < N; ++k) {
    res[k] /= f[k];
  }
  return res;
}

// finds coefficients of polynomial x -> P(x + c)
template <typename T>
FormalPowerSeries<T> taylor_shift(FormalPowerSeries<T> P, T c) {
  int N = P.size();
  FormalPowerSeries<T> expc(N);
  T f = 1, pow = 1;
  for (int k = 0; k < N; ++k) {
    expc[k] = pow / f;
    f *= k + 1;
    pow *= c;
  }
  return apply_polynomial_of_derivative(expc, P);
}
