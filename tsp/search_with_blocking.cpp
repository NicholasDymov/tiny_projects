#include <algorithm>
#include <chrono>
#include <clocale>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <vector>

using namespace std;

const long long INF = (1LL << 60);

struct Solution {
	vector<int> route;
	long long cost = INF;
};

struct Neighbor {
	vector<int> route;
	long long cost = INF;
	int i = -1, j = -1;
};

struct TabuEdge {
	int a, b, expires;
};

vector<vector<long long>> reader(const string &filename, int &n) {
	ifstream file(filename);
	if (!file.is_open()) {
		cerr << "Ошибка открытия " << filename << endl;
		exit(1);
	}
	file >> n;
	vector<vector<long long>> m(n, vector<long long>(n));
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			long long v;
			file >> v;
			m[i][j] = (i == j || v == -1) ? INF : v;
		}
	}
	return m;
}

long long cost(const vector<int> &route, const vector<vector<long long>> &m) {
	if (route.size() < 2)
		return INF;
	long long c = 0;
	for (size_t i = 0; i + 1 < route.size(); ++i) {
		int from = route[i], to = route[i + 1];
		if (m[from][to] == INF)
			return INF;
		c += m[from][to];
	}
	return c;
}

vector<int> randomRoute(int n) {
	vector<int> cities;
	for (int i = 1; i < n; ++i)
		cities.push_back(i);
	shuffle(cities.begin(), cities.end(), mt19937(random_device{}()));
	vector<int> route;
	route.reserve(n + 1);
	route.push_back(0);
	route.insert(route.end(), cities.begin(), cities.end());
	route.push_back(0);
	return route;
}

bool isTabu(int a, int b, const vector<TabuEdge> &tabuList, int iter) {
	for (const auto &e : tabuList) {
		if (iter <= e.expires && a == e.a && b == e.b)
			return true;
	}
	return false;
}

void cleanTabu(vector<TabuEdge> &tabuList, int iter) {
	for (auto it = tabuList.begin(); it != tabuList.end();) {
		if (it->expires < iter) {
			it = tabuList.erase(it);
		} else {
			++it;
		}
	}
}

void swapEdges(const vector<int> &r, int i, int j,
			   vector<pair<int, int>> &removed, vector<pair<int, int>> &added) {
	int prev_i = r[i - 1], cur_i = r[i], next_i = r[i + 1];
	int prev_j = r[j - 1], cur_j = r[j], next_j = r[j + 1];

	removed.clear();
	added.clear();

	if (j == i + 1) {
		removed = {{prev_i, cur_i}, {cur_i, cur_j}, {cur_j, next_j}};
		added = {{prev_i, cur_j}, {cur_j, cur_i}, {cur_i, next_j}};
	} else {
		removed = {
			{prev_i, cur_i}, {cur_i, next_i}, {prev_j, cur_j}, {cur_j, next_j}};
		added = {
			{prev_i, cur_j}, {cur_j, next_i}, {prev_j, cur_i}, {cur_i, next_j}};
	}
}

pair<Solution, int> oneRun(const vector<vector<long long>> &m,
						   const vector<int> &start, int tenure, int maxIt,
						   int limit) {
	int n = m.size();
	Solution cur{start, cost(start, m)};
	Solution best = cur;
	vector<TabuEdge> tabuList;

	int noImprovement = 0;
	int iterations = 0;

	for (int it = 0; it < maxIt; ++it) {
		Neighbor bestNeighbor;
		bool found = false;
		vector<pair<int, int>> removedEdges, addedEdges;

		for (int i = 1; i < n - 1; ++i) {
			for (int j = i + 1; j < n; ++j) {
				long long delta = 0;
				bool valid = true;

				int prev_i = cur.route[i - 1];
				int cur_i = cur.route[i];
				int next_i = cur.route[i + 1];
				int prev_j = cur.route[j - 1];
				int cur_j = cur.route[j];
				int next_j = cur.route[j + 1];

				if (j == i + 1) {
					long long old_cost = 0, new_cost = 0;
					if (m[prev_i][cur_i] == INF || m[cur_i][cur_j] == INF ||
						m[cur_j][next_j] == INF)
						valid = false;
					else
						old_cost = m[prev_i][cur_i] + m[cur_i][cur_j] +
								   m[cur_j][next_j];
					if (valid) {
						if (m[prev_i][cur_j] == INF || m[cur_j][cur_i] == INF ||
							m[cur_i][next_j] == INF)
							valid = false;
						else
							new_cost = m[prev_i][cur_j] + m[cur_j][cur_i] +
									   m[cur_i][next_j];
					}
					if (valid)
						delta = new_cost - old_cost;
				} else {
					long long old_cost = 0, new_cost = 0;
					if (m[prev_i][cur_i] == INF || m[cur_i][next_i] == INF ||
						m[prev_j][cur_j] == INF || m[cur_j][next_j] == INF)
						valid = false;
					else
						old_cost = m[prev_i][cur_i] + m[cur_i][next_i] +
								   m[prev_j][cur_j] + m[cur_j][next_j];
					if (valid) {
						if (m[prev_i][cur_j] == INF ||
							m[cur_j][next_i] == INF ||
							m[prev_j][cur_i] == INF || m[cur_i][next_j] == INF)
							valid = false;
						else
							new_cost = m[prev_i][cur_j] + m[cur_j][next_i] +
									   m[prev_j][cur_i] + m[cur_i][next_j];
					}
					if (valid)
						delta = new_cost - old_cost;
				}

				if (!valid)
					continue;

				swapEdges(cur.route, i, j, removedEdges, addedEdges);
				bool tabuMove = false;
				for (const auto &e : addedEdges) {
					if (isTabu(e.first, e.second, tabuList, it)) {
						tabuMove = true;
						break;
					}
				}

				long long c = cur.cost + delta;
				if (tabuMove && c >= best.cost)
					continue;

				vector<int> candidate = cur.route;
				swap(candidate[i], candidate[j]);

				if (!found || c < bestNeighbor.cost) {
					bestNeighbor = {candidate, c, i, j};
					found = true;
				}
			}
		}

		if (!found)
			break;

		vector<int> oldRoute = cur.route;
		cur = {bestNeighbor.route, bestNeighbor.cost};

		swapEdges(oldRoute, bestNeighbor.i, bestNeighbor.j, removedEdges,
				  addedEdges);
		for (const auto &e : removedEdges)
			tabuList.push_back({e.first, e.second, it + tenure});

		cleanTabu(tabuList, it);
		iterations++;

		if (cur.cost < best.cost) {
			best = cur;
			noImprovement = 0;
		} else {
			++noImprovement;
		}

		if (noImprovement >= limit)
			break;
	}

	return {best, iterations};
}

Solution tabuSearch(const vector<vector<long long>> &m, int n,
					int &totalIterations, int &totalRuns) {
	int tenure = max(5, min(30, (int)(0.5 * n)));
	int maxIt = 100 * n;
	int limit = 10 * n;
	int restarts = max(1, n / 5);

	cout << "Параметры: tenure=" << tenure << ", итераций=" << maxIt
		 << ", лимит=" << limit << ", рестартов=" << restarts << "\n\n";

	Solution best;
	totalIterations = 0;
	totalRuns = 0;

	auto startTime = chrono::steady_clock::now();

	for (int r = 0; r < restarts; ++r) {
		vector<int> start = randomRoute(n);
		auto result = oneRun(m, start, tenure, maxIt, limit);
		Solution sol = result.first;
		int iterCount = result.second;

		totalRuns++;
		totalIterations += iterCount;

		cout << "Запуск " << r + 1 << "/" << restarts << ": "
			 << "стоимость=" << sol.cost;
		if (sol.cost < best.cost) {
			cout << " (НОВЫЙ ЛУЧШИЙ!)";
			best = sol;
		}
		cout << "\n";
	}

	auto endTime = chrono::steady_clock::now();
	double elapsed = chrono::duration<double>(endTime - startTime).count();

	cout << "\nЗапусков: " << totalRuns << "\n";
	cout << "Суммарно итераций: " << totalIterations << "\n";
	cout << "Время: " << fixed << setprecision(6) << elapsed << " сек.\n";

	return best;
}

int main() {
	setlocale(LC_ALL, "Russian");

	string filename = "30_points_sim.txt";
	int n;
	vector<vector<long long>> m = reader(filename, n);

	cout << "Исходная матрица " << n << "x" << n << ":\n";
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (m[i][j] == INF)
				cout << setw(5) << "INF";
			else
				cout << setw(5) << m[i][j];
		}
		cout << "\n";
	}
	cout << "\n";

	int totalIter, totalRuns;
	Solution res = tabuSearch(m, n, totalIter, totalRuns);

	cout << "\n=== ОТВЕТ ===\n";
	cout << "Маршрут: ";
	for (size_t k = 0; k < res.route.size(); ++k) {
		cout << res.route[k];
		if (k + 1 < res.route.size())
			cout << " -> ";
	}
	cout << "\n";
	cout << "Стоимость: " << res.cost << "\n";

	return 0;
}
