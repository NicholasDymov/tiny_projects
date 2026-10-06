ClearAll["Global`*"];


prepareMatrix[m_] := 
  Module[{a = m, n = Length[m]}, Do[a[[i, i]] = Infinity, {i, n}];
   a];



reduceMatrix[m_] := Module[{a = m, n = Length[m], h = 0, mn},
   Do[mn = Min[a[[i]]];
    If[mn =!= Infinity, a[[i]] = a[[i]] - mn; h += mn], {i, n}];
   Do[mn = Min[a[[All, j]]];
    If[mn =!= Infinity, a[[All, j]] = a[[All, j]] - mn; h += mn], {j, 
     n}];
   {a, h}];




feasibleQ[m_, edges_, n_] := 
  Module[{usedFrom, usedTo}, 
   usedFrom = If[edges === {}, {}, edges[[All, 1]]];
   usedTo = If[edges === {}, {}, edges[[All, 2]]];
   AllTrue[Complement[Range[n], usedFrom], 
     Min[m[[#]]] =!= Infinity &] && 
    AllTrue[Complement[Range[n], usedTo], 
     Min[m[[All, #]]] =!= Infinity &]];




chooseEdge[m_] := 
  Module[{n = Length[m], best = None, bestPen = -1, rMin, cMin, pen}, 
   Do[If[m[[i, j]] === 0, rMin = Min[Delete[m[[i]], j]];
     cMin = Min[Delete[m[[All, j]], i]];
     pen = rMin + cMin;
     If[pen > bestPen, bestPen = pen; best = {i, j}]], {i, n}, {j, n}];
   best];



createsSubcycle[edges_, from_, to_, n_] := 
  Catch[Module[{next, cur = to, steps = 0},
    next = Association[Rule @@@ edges];
    next[from] = to;
    While[steps <= n,
     If[cur === from, Throw[Length[edges] + 1 < n]];
     If[! KeyExistsQ[next, cur], Throw[False]];
     cur = next[cur];
     steps++];
    ]];



buildPath[edges_, n_] := 
  Catch[Module[{next, path = {}, cur = 1, seen = <||>}, 
    If[Length[edges] != n, Throw[{}]];
    next = Association[Rule @@@ edges];
    Do[If[! KeyExistsQ[next, cur] || KeyExistsQ[seen, cur], Throw[{}]];
     seen[cur] = True;
     AppendTo[path, cur];
     cur = next[cur],
     {n}];
    If[cur =!= 1, Throw[{}]];
    Append[path, 1]]];




tourCost[path_, m_] := 
  If[Length[path] < 2, Infinity, 
   Total[m[[#1, #2]] & @@@ Partition[path, 2, 1]]];




Options[branchAndBound] = {"MaxNodes" -> 500000, "Verbose" -> False};




branchAndBound[m0_, OptionsPattern[]] := 
  Module[{n = Length[m0], orig, m, h, pq, node, bestCost = Infinity, 
    bestPath = {}, nodes = 0, peak = 0, edge, from, to, mm, hh, kids, 
    path, c, maxNodes = OptionValue["MaxNodes"], 
    verbose = OptionValue["Verbose"], aborted = False, t0}, 
   t0 = AbsoluteTime[];
   orig = prepareMatrix[m0];
   {m, h} = reduceMatrix[orig];
   pq = {<|"matrix" -> m, "edges" -> {}, "bound" -> h|>};
   If[verbose, Print["Константа приведения корня h = ", h]];
   While[pq =!= {}, node = First[pq]; pq = Rest[pq];
    nodes++;
    If[nodes > maxNodes, aborted = True; Break[]];
    If[node["bound"] >= bestCost, Continue[]];
    If[Length[node["edges"]] === n, path = buildPath[node["edges"], n];
     If[path =!= {}, c = tourCost[path, orig];
      If[c < bestCost, bestCost = c; bestPath = path;
       If[verbose, Print["Новый рекорд: ", c, "  ", path]]]];
     Continue[]];
    edge = chooseEdge[node["matrix"]];
    If[edge === None, Continue[]];
    {from, to} = edge;
    kids = {};
    If[! createsSubcycle[node["edges"], from, to, n], 
     mm = node["matrix"];
     mm[[from, All]] = ConstantArray[Infinity, n];
     mm[[All, to]] = ConstantArray[Infinity, n];
     mm[[to, from]] = Infinity;
     {mm, hh} = reduceMatrix[mm];
     If[node["bound"] + hh < bestCost && 
       feasibleQ[mm, Append[node["edges"], {from, to}], n], 
      AppendTo[
       kids, <|"matrix" -> mm, 
        "edges" -> Append[node["edges"], {from, to}], 
        "bound" -> node["bound"] + hh|>]]];
    
    mm = node["matrix"];
    mm[[from, to]] = Infinity;
    {mm, hh} = reduceMatrix[mm];
    If[node["bound"] + hh < bestCost && 
      feasibleQ[mm, node["edges"], n], 
     AppendTo[
      kids, <|"matrix" -> mm, "edges" -> node["edges"], 
       "bound" -> node["bound"] + hh|>]];
    If[kids =!= {}, pq = SortBy[Join[pq, kids], #["bound"] &];
     peak = Max[peak, Length[pq]]];];
   <|"cost" -> bestCost, "path" -> bestPath, "nodes" -> nodes, 
    "queuePeak" -> peak, "time" -> AbsoluteTime[] - t0, 
    "aborted" -> aborted|>];





Reader[filename_String] := 
  Module[{data, n, matrix}, data = Import[filename, "Table"];
   If[Length[data] < 1, Return[$Failed]];
   n = data[[1, 1]];
   matrix = data[[2 ;; n + 1]];
   If[Length[matrix] =!= n, Return[$Failed]];
   matrix = matrix /. {-1 -> Infinity};
   
   prepareMatrix[matrix]  ];


(*функц вывода*)


printMatrix[m_List] := 
  Module[{n = Length[m]}, 
   Do[Row[Table[
       If[m[[i, j]] === Infinity, "  INF", 
        StringPadLeft[ToString[m[[i, j]]], 5]], {j, n}]] // Print,
    {i, n}]];

printResult[res_Association, orig_List] := 
  Module[{}, Print["=== ОТВЕТ ==="];
   Print["Маршрут: ", Row[res["path"], " -> "]];
   Print["Стоимость: ", res["cost"]];
   Print["Обработано узлов: ", res["nodes"], 
    If[res["aborted"], " (ПРЕРВАНО по лимиту)", ""]];
   Print["Время: ", NumberForm[res["time"], {8, 6}], " с"];];



filename = "20_points.txt";


matrix = Reader[filename];
If[matrix === $Failed, Print["Ошибка чтения файла"]; Abort[]];
n = Length[matrix];

Print["Исходная матрица ", n, " на ", n, ":"];
printMatrix[matrix];


reduced = matrix;
{reduced, h0} = reduceMatrix[reduced];
Print["Приведенная матрица:"];
printMatrix[reduced];
Print["Константа приведения: ", h0];


res = branchAndBound[matrix];


printResult[res, matrix];