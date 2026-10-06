#include <chrono>
#include <clocale>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const long long INF = 1LL << 60;

struct Node {
	vector<vector<long long>> matrix;
	vector<pair<int, int>> edges;
	long long bound;
};

vector<vector<long long>> readMatrix(const string &filename, int &n) {
	ifstream file(filename);
	if (!file) {
		cout << "Ошибка открытия файла!" << endl;
		exit(1);
	}
	file >> n;
	vector<vector<long long>> matrix(n, vector<long long>(n));
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			file >> matrix[i][j];
			if (i == j)
				matrix[i][j] = INF;
		}
	}
	file.close();

	cout << "Исходная матрица:\n";
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (matrix[i][j] == INF)
				cout << setw(5) << "INF";
			else
				cout << setw(5) << matrix[i][j];
		}
		cout << "\n";
	}
	cout << "\n";
	return matrix;
}

long long reduceMatrix(vector<vector<long long>> &matrix) {
	int n = matrix.size();
	long long reduction = 0;

	for (int i = 0; i < n; i++) {
		long long mn = INF;
		for (int j = 0; j < n; j++)
			if (matrix[i][j] < mn)
				mn = matrix[i][j];
		if (mn != INF && mn != 0) {
			for (int j = 0; j < n; j++)
				if (matrix[i][j] != INF)
					matrix[i][j] -= mn;
			reduction += mn;
		}
	}

	for (int j = 0; j < n; j++) {
		long long mn = INF;
		for (int i = 0; i < n; i++)
			if (matrix[i][j] < mn)
				mn = matrix[i][j];
		if (mn != INF && mn != 0) {
			for (int i = 0; i < n; i++)
				if (matrix[i][j] != INF)
					matrix[i][j] -= mn;
			reduction += mn;
		}
	}
	return reduction;
}

void printReducedMatrix(const vector<vector<long long>> &matrix) {
	int n = matrix.size();
	cout << "Приведенная матрица:\n";
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (matrix[i][j] == INF)
				cout << setw(5) << "INF";
			else
				cout << setw(5) << matrix[i][j];
		}
		cout << "\n";
	}
	cout << "\n";
}

pair<int, int> chooseEdge(const vector<vector<long long>> &matrix) {
	int n = matrix.size();
	long long maxPenalty = -1;
	pair<int, int> best = {-1, -1};
	for (int i = 0; i < n; i++) {
		for (int j = 0; j < n; j++) {
			if (matrix[i][j] != 0 || i == j)
				continue;
			long long rowMin = INF, colMin = INF;
			for (int k = 0; k < n; k++) {
				if (k != j && matrix[i][k] < rowMin)
					rowMin = matrix[i][k];
			}
			for (int k = 0; k < n; k++) {
				if (k != i && matrix[k][j] < colMin)
					colMin = matrix[k][j];
			}
			if (rowMin == INF)
				rowMin = 0;
			if (colMin == INF)
				colMin = 0;
			long long penalty = rowMin + colMin;
			if (penalty > maxPenalty) {
				maxPenalty = penalty;
				best = {i, j};
			}
		}
	}
	return best;
}

bool createsSubcycle(const vector<pair<int, int>> &edges, int from, int to,
					 int n) {
	vector<int> next(n, -1);
	for (auto e : edges)
		next[e.first] = e.second;
	next[from] = to;
	int cur = to;
	for (int step = 0; step <= n; step++) {
		if (cur == -1)
			return false;
		if (cur == from)
			return edges.size() + 1 < n;
		cur = next[cur];
	}
	return true;
}

vector<int> buildPath(const vector<pair<int, int>> &edges, int n) {
	if (edges.size() != n)
		return {};
	vector<int> next(n, -1);
	vector<bool> visited(n, false);
	for (auto e : edges)
		next[e.first] = e.second;
	vector<int> path;
	int cur = 0;
	for (int i = 0; i < n; i++) {
		if (cur < 0 || cur >= n || visited[cur] || next[cur] == -1)
			return {};
		visited[cur] = true;
		path.push_back(cur);
		cur = next[cur];
	}
	if (cur != 0)
		return {};
	path.push_back(0);
	return path;
}

long long calculateCost(const vector<int> &path,
						const vector<vector<long long>> &original) {
	if (path.size() < 2)
		return INF;
	long long cost = 0;
	for (int i = 0; i + 1 < path.size(); i++) {
		int f = path[i], t = path[i + 1];
		if (original[f][t] == INF)
			return INF;
		cost += original[f][t];
	}
	return cost;
}

struct Result {
	long long cost;
	vector<int> path;
	int nodesProcessed;
};

Result branchAndBound(vector<vector<long long>> matrix) {
	int n = matrix.size();
	auto original = matrix;

	long long startBound = reduceMatrix(matrix);

	auto cmp = [](const Node &a, const Node &b) { return a.bound > b.bound; };
	priority_queue<Node, vector<Node>, decltype(cmp)> pq(cmp);
	pq.push({matrix, {}, startBound});

	long long bestCost = INF;
	vector<int> bestPath;
	int nodesProcessed = 0;

	while (!pq.empty()) {
		Node cur = pq.top();
		pq.pop();
		nodesProcessed++;

		if (cur.bound >= bestCost)
			continue;

		if (cur.edges.size() == n) {
			vector<int> path = buildPath(cur.edges, n);
			if (!path.empty()) {
				long long c = calculateCost(path, original);
				if (c < bestCost) {
					bestCost = c;
					bestPath = path;
				}
			}
			continue;
		}

		auto edge = chooseEdge(cur.matrix);
		int from = edge.first, to = edge.second;
		if (from == -1)
			continue;

		if (!createsSubcycle(cur.edges, from, to, n)) {
			Node inc = cur;
			inc.edges.push_back({from, to});
			for (int k = 0; k < n; k++) {
				inc.matrix[from][k] = INF;
				inc.matrix[k][to] = INF;
			}
			inc.matrix[to][from] = INF;
			inc.bound += reduceMatrix(inc.matrix);
			if (inc.bound < bestCost)
				pq.push(inc);
		}

		Node exc = cur;
		exc.matrix[from][to] = INF;
		exc.bound += reduceMatrix(exc.matrix);
		if (exc.bound < bestCost)
			pq.push(exc);
	}

	return {bestCost, bestPath, nodesProcessed};
}

void printResult(const Result &res) {
	if (res.path.empty()) {
		cout << "Маршрут не найден.\n";
		return;
	}
	cout << "Маршрут: ";
	for (int i = 0; i < res.path.size(); i++) {
		cout << res.path[i];
		if (i + 1 < res.path.size())
			cout << " -> ";
	}
	cout << "\nСтоимость: " << res.cost << "\n";
}

int main() {
	setlocale(LC_ALL, "Russian");

	string filename = "10_points.txt";
	int n;
	vector<vector<long long>> matrix = readMatrix(filename, n);

	vector<vector<long long>> reduced = matrix;
	reduceMatrix(reduced);
	printReducedMatrix(reduced);

	auto start = chrono::steady_clock::now();
	Result res = branchAndBound(matrix);
	auto end = chrono::steady_clock::now();
	double elapsed = chrono::duration<double>(end - start).count();
	cout << "Обработано узлов: " << res.nodesProcessed << "\n";
	cout << "Время: " << fixed << setprecision(6) << elapsed << " с\n\n";
	printResult(res);
	return 0;
}
