#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <iostream>
#include <tuple>
#include <chrono>
#include <cstring>
#include <queue>

namespace color {
  const int COUNT = 6;

  const int U = 0;
  const int R = 1;
  const int F = 2;
  const int D = 3;
  const int L = 4;
  const int B = 5;

  const int CORNERS[][3] = {
    {U, R, F}, {U, F, L}, {U, L, B}, {U, B, R},
    {D, F, R}, {D, L, F}, {D, B, L}, {D, R, B}
  };

  const int EDGES[][2] = {
    {U, R}, {U, F}, {U, L}, {U, B},
    {D, R}, {D, F}, {D, L}, {D, B},
    {F, R}, {F, L}, {B, L}, {B, R}
  };

  const char CHARS[] = {'U', 'R', 'F', 'D', 'L', 'B'};
}

namespace cubie {

  const int N_EDGES = 12;
  const int N_CORNERS = 8;

  const int URF = 0;
  const int UFL = 1;
  const int ULB = 2;
  const int UBR = 3;
  const int DFR = 4;
  const int DLF = 5;
  const int DBL = 6;
  const int DRB = 7;

  const int UR = 0;
  const int UF = 1;
  const int UL = 2;
  const int UB = 3;
  const int DR = 4;
  const int DF = 5;
  const int DL = 6;
  const int DB = 7;
  const int FR = 8;
  const int FL = 9;
  const int BL = 10;
  const int BR = 11;

  // Map a facelet to the cubie it's on
  const int FROM_FACELET[] = {
    ULB, UB, UBR, UL, -1, UR, UFL, UF, URF,
    URF, UR, UBR, FR, -1, BR, DFR, DR, DRB,
    UFL, UF, URF, FL, -1, FR, DLF, DF, DFR,
    DLF, DF, DFR, DL, -1, DR, DBL, DB, DRB,
    ULB, UL, UFL, BL, -1, FL, DBL, DL, DLF,
    UBR, UB, ULB, BR, -1, BL, DRB, DB, DBL
  };

}

const int N_FACELETS = 54;

const int FACELET_TO_POS[] = {
  0, 0, 0, 0, -1, 0, 0, 0, 0,
  1, 1, 2, 1, -1, 1, 2, 1, 1,
  1, 1, 2, 0, -1, 0, 2, 1, 1,
  0, 0, 0, 0, -1, 0, 0, 0, 0,
  1, 1, 2, 1, -1, 1, 2, 1, 1,
  1, 1, 2, 0, -1, 0, 2, 1, 1
};

using colset_t = uint8_t;


const int N_BGRS = 16777216;
uint16_t scantbl[N_BGRS][color::COUNT];

template <int n_cubies, int n_oris, const int cubiecols[n_cubies][n_oris]>
class Options {

  struct Opt { // tight memory to make copying faster
    uint8_t cols[n_oris];
    colset_t colset;
    uint8_t ori;
    uint8_t cubie;
  };

  Opt opts[n_cubies * n_oris];
  int rem;

  bool error;
  colset_t colset;
  int ori;
  int cubie;

  void update() {
    if (rem == 0) {
      error = true;
      return;
    }

    colset = opts[0].colset; // at this point there is at least one option left
    for (int i = 1; i < rem; i++)
      colset &= opts[i].colset;

    if (ori == -1) {
      int singleori = opts[0].ori;
      for (int i = 1; i < rem; i++) {
        if (opts[i].ori != singleori) {
          singleori = -1;
          break;
        }
      }
      if (singleori != -1)
        ori = singleori;
    }

    if (cubie == -1) {
      int singlecubie = opts[0].cubie;
      for (int i = 1; i < rem; i++) {
        if (opts[i].cubie != singlecubie) {
          singlecubie = -1;
          break;
        }
      }
      if (singlecubie != -1)
        cubie = singlecubie;
    }
  }

  public:

    void init() { // no constructor to make it a POD allowing super fast mem-copying
      int i = 0;
      for (int cubie = 0; cubie < n_cubies; cubie++) {
        for (int ori = 0; ori < n_oris; ori++) {
          opts[i].cubie = cubie;
          opts[i].ori = ori;
          for (int j = 0; j < n_oris; j++) {
            opts[i].cols[j] = cubiecols[cubie][(j + ori) % n_oris];
            opts[i].colset |= 1 << opts[i].cols[j];
          }
          i++;
        }
      }
      rem = n_cubies * n_oris;

      error = false;
      colset = 0;
      ori = - 1;
      cubie = -1;
    }

    bool get_error() {
      return error;
    }

    colset_t get_colset() {
      return colset;
    }

    int get_ori() {
      return ori;
    }

    int get_cubie() {
      return cubie;
    }

    void has_poscol(int pos, int col) {
      int rem1 = 0;
      for (int i = 0; i < rem; i++) {
        if (opts[i].cols[pos] == col)
          opts[rem1++] = opts[i];
      }
      if (rem1 != rem) {
        rem = rem1;
        update();
      } else
        rem = rem1;
    }

    void hasnot_col(int col) {
      int rem1 = 0;
      for (int i = 0; i < rem; i++) {
        if ((opts[i].colset & (1 << col)) == 0)
          opts[rem1++] = opts[i];
      }
      if (rem1 != rem) {
        rem = rem1;
        update();
      }
    }

    void has_ori(int ori) {
      int rem1 = 0;
      for (int i = 0; i < rem; i++) {
        if (opts[i].ori == ori)
          opts[rem1++] = opts[i];
      }
      if (rem1 != rem) {
        rem = rem1;
        update();
      }
    }

    void isnot_cubie(int cubie) {
      int rem1 = 0;
      for (int i = 0; i < rem; i++) {
        if (opts[i].cubie != cubie)
          opts[rem1++] = opts[i];
      }
      if (rem1 != rem) {
        rem = rem1;
        update();
      }
    }

};

template <int n_cubies, int n_oris, const int cubiecols[n_cubies][n_oris]>
class CubieBuilder {

  int colcounts[color::COUNT];
  colset_t colsets[n_cubies];
  int oris[n_cubies];
  int perm[n_cubies];
  int par;

  Options<n_cubies, n_oris, cubiecols> opts[n_cubies];
  int invcnt;
  int orisum;
  int aperm;
  int aoris;

  bool assign_cubie(int i) {
    int cubie = opts[i].get_cubie();
    if (cubie == -1 || perm[i] != -1)
      return false;

    perm[i] = cubie;
    for (int j = 0; j < i; j++) {
      if (perm[j] != - 1 && perm[j] > cubie)
        invcnt++;
    }
    for (int j = i; j < n_cubies; j++) {
      if (perm[j] != -1 && perm[j] < cubie)
        invcnt++;
    }
    aperm++;
    if (aperm == n_cubies)
      par = invcnt & 1; // permutation fully determined -> compute parity

    // Every cubie exists exactly once -> eliminate from all other options
    for (int j = 0; j < n_cubies; j++) {
      if (j != i)
        opts[j].isnot_cubie(cubie);
    }

    return true;
  }

  bool assign_ori(int i) {
    int ori = opts[i].get_ori();
    if (ori == -1 || oris[i] != -1)
      return false;

    oris[i] = ori;
    orisum += ori;
    aoris++;
    return true;
  }

  public:

    void init() { // again, we want to keep this a POD
      std::fill(colcounts, colcounts + color::COUNT, 4);
      std::fill(colsets, colsets + n_cubies, 0);
      std::fill(oris, oris + n_cubies, -1);
      std::fill(perm, perm + n_cubies, -1);
      par = -1;

      for (auto& o : opts)
        o.init();

      invcnt = 0;
      orisum = 0;
      aperm = 0;
      aoris = 0;
    }

    /*
    int* get_perm() {
      return perm;
    }

    int* get_oris() {
      return oris;
    }
    */

    int get_par() {
      return par;
    }

    void assign_col(int cubie, int pos, int col) {
      opts[cubie].has_poscol(pos, col);
    }

    void assign_par(int par) {
      this->par = par;
    }

    bool propagate() {
      bool change = true;
      while (change) {
        change = false;

        for (int cubie = 0; cubie < n_cubies; cubie++) {
          if (opts[cubie].get_error())
            return false;

          colset_t diff = opts[cubie].get_colset() ^colsets[cubie]; // latter will always be included in former
          colsets[cubie] |= diff;
          for (int col = 0; col < color::COUNT; col++) {
            if ((diff & (1 << col)) == 0)
              continue;
            colcounts[col]--;
            if (colcounts[col] == 0) { // all cubies of some color known
              for (int i = 0; i < n_cubies; i++) {
                // Some `colset` update might not have been registered yet
                if ((opts[i].get_colset() & (1 << col)) == 0) {
                  opts[i].hasnot_col(col);
                  change = true;
                }
              }
            }
          }

          change |= assign_ori(cubie);
          change |= assign_cubie(cubie);
        }

        // Figure out last ori by parity
        if (aoris == n_cubies - 1) {
          int lastori = (n_oris - orisum % n_oris) % n_oris;
          for (int i = 0; i < n_cubies; i++) {
            if (oris[i] == -1) {
              opts[i].has_ori(lastori);
              assign_ori(i);
              break;
            }
          }
          change = true;
        }

        // Figure out position of last two cubies by parity
        if (par != -1 && aperm == n_cubies - 2) {
          int i1 = std::find(perm, perm + n_cubies, -1) - perm;
          int i2 = std::find(perm + i1 + 1, perm + n_cubies, -1) - perm;

          bool contained[n_cubies] = {};
          for (int c : perm) {
            if (c != -1)
              contained[c] = true;
          }
          int cubie1 = 0;
          while (contained[cubie1])
            cubie1++;
          int cubie2 = cubie1 + 1;
          while (contained[cubie2])
            cubie2++;

          int invcnt1 = 0;
          for (int i = 0; i < n_cubies; i++) {
            if (perm[i] == -1)
              continue;
            invcnt1 += i < i1 && perm[i] > cubie1;
            invcnt1 += i > i1 && perm[i] < cubie1;
            invcnt1 += i < i2 && perm[i] > cubie2;
            invcnt1 += i > i2 && perm[i] < cubie2;
          }
          if (((invcnt + invcnt1) & 1) != par)
            std::swap(i1, i2); // flip cubie positions to fix parity

          opts[i1].isnot_cubie(cubie2); // safe because only `cubie1` and `cubie2` may be left at this point
          opts[i2].isnot_cubie(cubie1);
          assign_cubie(i1);
          assign_cubie(i2);
          change = true;
        }
      }

      return true;
    }

};

using CornersBuilder = CubieBuilder<cubie::N_CORNERS, 3, color::CORNERS>;
using EdgesBuilder = CubieBuilder<cubie::N_EDGES, 2, color::EDGES>;

std::string match_colors(int bgrs[N_FACELETS][3]) {
  int facecube[N_FACELETS];

  int conf[N_FACELETS][color::COUNT];
  for (int f = 0; f < N_FACELETS; f++) {
    for (int col = 0; col < color::COUNT; col++)
      conf[f][col] = scantbl[256 * (256 * bgrs[f][0] + bgrs[f][1]) + bgrs[f][2]][col];
  }

  std::priority_queue<std::tuple<int, int, int>> heap;
  for (int f = 0; f < N_FACELETS; f++) {
    if (f % 9 == 4) // centers are fixed
      facecube[f] = f / 9;
    else {
      int imax = std::max_element(conf[f], conf[f] + color::COUNT) - conf[f];
      heap.emplace(conf[f][imax], f, imax);
      conf[f][imax] = -1; // makes it easy to find the next largest index
    }
  }

  // Pointers to simply swap backups back in instead of having to copy them again
  auto* corners = new CornersBuilder();
  auto* edges = new EdgesBuilder();
  corners->init();
  edges->init();
  auto* corners1 = new CornersBuilder();
  auto* edges1 = new EdgesBuilder();

  while (!heap.empty()) {
    auto ass = heap.top();
    heap.pop();

    int f = std::get<1>(ass);
    int cubie = cubie::FROM_FACELET[f];
    int pos = FACELET_TO_POS[f];
    int col = std::get<2>(ass);

    bool succ;
    if ((f % 9) % 2 != 1) { // is on an edge
      memcpy(edges1, edges, sizeof(*edges));
      edges->assign_col(cubie, pos, col);
      if (!(succ = edges->propagate()))
        std::swap(edges1, edges);
      else if (edges->get_par() != -1 && corners->get_par() == -1) {
        corners->assign_par(edges->get_par());
        if (!(succ = edges->propagate())) {
          std::swap(edges1, edges);
          std::swap(corners1, corners);
        }
      }
    } else {
      memcpy(corners1, corners, sizeof(*corners));
      corners->assign_col(cubie, pos, col);
      if (!(succ = corners->propagate()))
        std::swap(corners1, corners);
      else if (corners->get_par() != -1 && edges->get_par() == -1) {
        memcpy(edges1, edges, sizeof (*edges));
        edges->assign_par(corners->get_par());
        if (!(succ = edges->propagate())) {
          std::swap(corners1, corners);
          std::swap(edges1, edges);
        }
      }
    }

    if (!succ) {
      auto tmp = std::max_element(conf[f], conf[f] + color::COUNT);
      if (*tmp == -1)
        return ""; // scan error
      int imax = tmp - conf[f];
      heap.emplace(conf[f][imax], f, imax);
      conf[f][imax] = -1;
    } else
      facecube[f] = col;
  }

  char s[N_FACELETS];
  for (int i = 0; i < N_FACELETS; i++)
    s[i] = color::CHARS[facecube[i]];
  return std::string(s, N_FACELETS);
}

bool init() {

}

/*
void print_arr(int arr[], int len) {
  for (int i = 0; i < len; i++)
    std::cout << arr[i] << " ";
  std::cout << std::endl;
}

int main() {
  std::random_device device;
  std::mt19937 gen(device());

  int n_cubies = cubie::N_CORNERS;
  int n_oris = 3;
  auto cubiecols = color::CORNERS;

  int perm[n_cubies];
  for (int i = 0; i < n_cubies; i++)
    perm[i] = i;
  std::shuffle(perm, perm + n_cubies, gen);

  int invs = 0;
  for (int i = 0; i < n_cubies; i++) {
    for (int j = 0; j < i; j++) {
      if (perm[j] > perm[i])
        invs++;
    }
  }
  int par = invs & 1;

  int oris[n_cubies];
  do {
    for (int i = 0; i < n_cubies; i++)
      oris[i] = std::uniform_int_distribution<int>(0, n_oris - 1)(gen);
  } while ((std::accumulate(oris, oris + n_cubies, 0) % n_oris) != 0);

  std::cout << par << std::endl;
  print_arr(perm, n_cubies);
  print_arr(oris, n_cubies);

  std::vector<std::tuple<int, int, int>> assignments;
  for (int cubie = 0; cubie < n_cubies; cubie++) {
    for (int pos = 0; pos < n_oris; pos++)
      assignments.emplace_back(cubie, pos, cubiecols[perm[cubie]][(pos + oris[cubie]) % n_oris]);
  }
  std::shuffle(assignments.begin(), assignments.end(), gen);

  auto* builder = new CornersBuilder();
  builder->init();
  builder->assign_par(par);
  auto* backup = new CornersBuilder();

  auto tick = std::chrono::high_resolution_clock::now();

  for (auto& a : assignments) {
    // std::cout << std::get<0>(a) << " " << std::get<1>(a) << " " << std::get<2>(a) << std::endl;
    memcpy(backup, builder, sizeof (*builder));
    builder->assign_col(std::get<0>(a), std::get<1>(a), std::get<2>(a));
    if (!builder->propagate())
      std::swap(builder, backup);
  }

  std::cout << std::chrono::duration_cast<std::chrono::microseconds>(
    std::chrono::high_resolution_clock::now() - tick
  ).count() / 1000. << "ms" << std::endl;

  std::cout << builder->get_par() << std::endl;
  print_arr(builder->get_perm(), n_cubies);
  print_arr(builder->get_oris(), n_cubies);

  return 0;
}
*/